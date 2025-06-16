// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libfile.h>
#include <memory.h>
#include <unistd.h>
#include <zero_page.h>
#include <linux/fs.h>
#include <linux/list_sort.h>

struct elf_section {
	struct list_head list;
	struct resource *r;
	void *phdr;
	bool is_iomem_region;
};

static int elf_request_region(struct elf_image *elf, resource_size_t start,
			      resource_size_t size, void *phdr)
{
	struct list_head *list = &elf->list;
	struct resource *r_new;
	struct elf_section *r;

	r = calloc(1, sizeof(*r));
	if (!r)
		return -ENOMEM;

	r_new = request_sdram_region("elf_section", start, size,
				     MEMTYPE_LOADER_CODE, MEMATTRS_RWX);
	if (!r_new) {
		r_new = request_iomem_region("elf_section", start, size);
		if (!r_new) {
			pr_err("Failed to request region: %pa %pa\n", &start, &size);
			return -EINVAL;
		}
		r->is_iomem_region = true;
	}

	r->r = r_new;
	r->phdr = phdr;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct elf_image *elf)
{
	struct list_head *list = &elf->list;
	struct elf_section *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		if (r->is_iomem_region)
			release_region(r->r);
		else
			release_sdram_region(r->r);
		list_del(&r->list);
		free(r);
	}
}

static int request_elf_segment(struct elf_image *elf, void *phdr)
{
	void *dst = (void *) (phys_addr_t) elf_phdr_p_paddr(elf, phdr);
	int ret;
	u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

	/* we care only about PT_LOAD segments */
	if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
		return 0;

	if (!p_memsz)
		return 0;

	if (dst < elf->low_addr)
		elf->low_addr = dst;
	if (dst + p_memsz > elf->high_addr)
		elf->high_addr = dst + p_memsz;

	pr_debug("Requesting segment 0x%p (%llu bytes)\n", dst, p_memsz);

	ret = elf_request_region(elf, (resource_size_t)dst, p_memsz, phdr);
	if (ret)
		return ret;

	return 0;
}

static int elf_section_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct elf_image *elf = priv;
	struct elf_section *elf_a, *elf_b;

	if (a == b)
		return 0;

	elf_a = list_entry(a, struct elf_section, list);
	elf_b = list_entry(b, struct elf_section, list);

	return elf_phdr_p_offset(elf, elf_a->phdr) >
	       elf_phdr_p_offset(elf, elf_b->phdr);
}

static int load_elf_to_memory(struct elf_image *elf)
{
	void *dst;
	int ret = 0, fd = -1;
	u64 p_filesz, p_memsz, p_offset;
	struct elf_section *r;
	struct list_head *list = &elf->list;

	if (elf->filename) {
		fd = open(elf->filename, O_RDONLY);
		if (fd < 0) {
			pr_err("could not open: %m\n");
			return -errno;
		}
	}

	zero_page_access();

	list_for_each_entry(r, list, list) {
		p_offset = elf_phdr_p_offset(elf, r->phdr);
		p_filesz = elf_phdr_p_filesz(elf, r->phdr);
		p_memsz = elf_phdr_p_memsz(elf, r->phdr);
		dst = (void *) (phys_addr_t) elf_phdr_p_paddr(elf, r->phdr);

		pr_debug("Loading phdr offset 0x%llx to 0x%p (%llu bytes)\n",
			 p_offset, dst, p_filesz);

		if (fd >= 0) {
			ret = lseek(fd, p_offset, SEEK_SET);
			if (ret == -1) {
				pr_err("lseek at offset 0x%llx failed\n",
				       p_offset);
				goto out;
			}

			if (read_full(fd, dst, p_filesz) < 0) {
				pr_err("could not read elf segment: %m\n");
				ret = -errno;
				goto out;
			}
		} else {
			memcpy(dst, elf->hdr_buf + p_offset, p_filesz);
		}

		if (p_filesz < p_memsz)
			memset(dst + p_filesz, 0x00, p_memsz - p_filesz);
	}

out:
	zero_page_faulting();

	close(fd);

	return ret >= 0 ? 0 : ret;
}

static int load_elf_image_segments(struct elf_image *elf)
{
	void *buf = elf->hdr_buf;
	void *phdr = (void *) (buf + elf_hdr_e_phoff(elf, buf));
	int i, ret;

	/* File as already been loaded */
	if (!list_empty(&elf->list))
		return -EINVAL;

	for (i = 0; i < elf_hdr_e_phnum(elf, buf) ; ++i) {
		ret = request_elf_segment(elf, phdr);
		if (ret)
			goto elf_release_regions;

		phdr += elf_size_of_phdr(elf);
	}

	/*
	 * Sort the list to avoid doing backward lseek while loading the elf
	 * segments from file to memory(some filesystems don't support it)
	 */
	list_sort(elf, &elf->list, elf_section_cmp);

	ret = load_elf_to_memory(elf);
	if (ret)
		goto elf_release_regions;

	return 0;

elf_release_regions:
	elf_release_regions(elf);

	return ret;
}

static int elf_check_image(struct elf_image *elf, void *buf)
{
	if (strncmp(buf, ELFMAG, SELFMAG)) {
		pr_err("ELF magic not found.\n");
		return -EINVAL;
	}

	elf->class = ((char *) buf)[EI_CLASS];

	if (elf_hdr_e_type(elf, buf) != ET_EXEC) {
		pr_err("Non EXEC ELF image.\n");
		return -ENOEXEC;
	}

	if (!elf_hdr_e_phnum(elf, buf)) {
		pr_err("No phdr found.\n");
		return -ENOEXEC;
	}

	return 0;
}

static void elf_init_struct(struct elf_image *elf)
{
	INIT_LIST_HEAD(&elf->list);
	elf->low_addr = (void *) (unsigned long) -1;
	elf->high_addr = 0;
	elf->filename = NULL;
}

struct elf_image *elf_open_binary(void *buf)
{
	int ret;
	struct elf_image *elf;

	elf = calloc(1, sizeof(*elf));
	if (!elf)
		return ERR_PTR(-ENOMEM);

	elf_init_struct(elf);

	elf->hdr_buf = buf;
	ret = elf_check_image(elf, buf);
	if (ret) {
		free(elf);
		return ERR_PTR(-EINVAL);
	}

	elf->entry = elf_hdr_e_entry(elf, elf->hdr_buf);

	return elf;
}

static struct elf_image *elf_check_init(const char *filename)
{
	int ret, fd;
	int hdr_size;
	struct elf64_hdr hdr;
	struct elf_image *elf;

	elf = calloc(1, sizeof(*elf));
	if (!elf)
		return ERR_PTR(-ENOMEM);

	elf_init_struct(elf);

	/* First pass is to read elf header only */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		pr_err("could not open: %m\n");
		ret = -errno;
		goto err_free_elf;
	}

	if (read_full(fd, &hdr, sizeof(hdr)) < 0) {
		pr_err("could not read elf header: %m\n");
		close(fd);
		ret = -errno;
		goto err_free_elf;
	}
	close(fd);

	ret = elf_check_image(elf, &hdr);
	if (ret)
		goto err_free_elf;

	hdr_size = elf_hdr_e_phoff(elf, &hdr) +
		   elf_hdr_e_phnum(elf, &hdr) *
		   elf_hdr_e_phentsize(elf, &hdr);

	/* Second pass is to read the whole elf header and program headers */
	elf->hdr_buf = malloc(hdr_size);
	if (!elf->hdr_buf) {
		ret = -ENOMEM;
		goto err_free_elf;
	}

	/*
	 * We must open the file again since some fs (tftp) do not support
	 * backward lseek operations
	 */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		pr_err("could not open: %m\n");
		ret = -errno;
		goto err_free_hdr_buf;
	}

	if (read_full(fd, elf->hdr_buf, hdr_size) < 0) {
		pr_err("could not read elf program headers: %m\n");
		ret = -errno;
		close(fd);
		goto err_free_hdr_buf;
	}
	close(fd);

	elf->filename = strdup(filename);
	if (!elf->filename) {
		ret = -ENOMEM;
		goto err_free_hdr_buf;
	}

	elf->entry = elf_hdr_e_entry(elf, elf->hdr_buf);

	return elf;

err_free_hdr_buf:
	free(elf->hdr_buf);
err_free_elf:
	free(elf);

	return ERR_PTR(ret);
}

struct elf_image *elf_open(const char *filename)
{
	return elf_check_init(filename);
}

int elf_load(struct elf_image *elf)
{
	return load_elf_image_segments(elf);
}

void elf_close(struct elf_image *elf)
{
	elf_release_regions(elf);

	if (elf->filename) {
		free(elf->hdr_buf);
		free_const(elf->filename);
	}

	free(elf);
}
