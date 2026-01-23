// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libfile.h>
#include <memory.h>
#include <unistd.h>
#include <zero_page.h>
#include <linux/fs.h>
#include <linux/list_sort.h>

struct elf_segment {
	struct list_head list;
	struct resource *r;
	void *phdr;
};

static int elf_request_region(struct elf_image *elf, resource_size_t start,
			      resource_size_t size, void *phdr)
{
	struct list_head *list = &elf->list;
	struct resource *r_new;
	struct elf_segment *r;

	r = calloc(1, sizeof(*r));
	if (!r)
		return -ENOMEM;

	r_new = request_sdram_region_silent("elf_segment", start, size,
					    MEMTYPE_LOADER_CODE, MEMATTRS_RWX);
	if (!r_new) {
		r_new = request_iomem_region("elf_segment", start, size);
		if (!r_new) {
			pr_err("Failed to request region: %pa %pa\n", &start, &size);
			return -EINVAL;
		}
	}

	r->r = r_new;
	r->phdr = phdr;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct elf_image *elf)
{
	struct list_head *list = &elf->list;
	struct elf_segment *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		release_region(r->r);
		list_del(&r->list);
		free(r);
	}
}

static int elf_compute_load_offset(struct elf_image *elf)
{
	void *buf = elf->hdr_buf;
	void *phdr = buf + elf_hdr_e_phoff(elf, buf);
	u64 min_vaddr = (u64)-1;
	u64 min_paddr = (u64)-1;
	unsigned long base_load_addr;

	/* Find lowest p_vaddr and p_paddr in PT_LOAD segments */
	elf_for_each_segment(phdr, elf, buf) {
		if (elf_phdr_p_type(elf, phdr) == PT_LOAD) {
			u64 vaddr = elf_phdr_p_vaddr(elf, phdr);
			u64 paddr = elf_phdr_p_paddr(elf, phdr);

			if (vaddr < min_vaddr)
				min_vaddr = vaddr;
			if (paddr < min_paddr)
				min_paddr = paddr;
		}
	}

	/*
	 * Determine base load address:
	 * 1. If user specified load_address, use it
	 * 2. Otherwise for ET_EXEC, use NULL (segments use p_paddr directly)
	 * 3. For ET_DYN, use lowest p_paddr
	 */
	if (elf->load_address)
		base_load_addr = (unsigned long)elf->load_address;
	else if (elf->type == ET_EXEC)
		base_load_addr = 0;
	else
		base_load_addr = min_paddr;

	/*
	 * Calculate relocation offset:
	 * - For ET_EXEC with no custom load address: no offset needed
	 * - Otherwise: offset = base_load_addr - lowest_vaddr
	 */
	if (elf->type == ET_EXEC && !elf->load_address)
		elf->reloc_offset = 0;
	else
		elf->reloc_offset = base_load_addr - min_vaddr;

	pr_debug("ELF load: type=%s, base=%08lx, offset=%08lx\n",
		 elf->type == ET_EXEC ? "ET_EXEC" : "ET_DYN",
		 base_load_addr, elf->reloc_offset);

	return 0;
}

static int request_elf_segment(struct elf_image *elf, void *phdr)
{
	void *dst;
	int ret;
	u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

	/* we care only about PT_LOAD segments */
	if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
		return 0;

	if (!p_memsz)
		return 0;

	/*
	 * Calculate destination address:
	 * - If reloc_offset is set (custom load address or ET_DYN):
	 *   dst = reloc_offset + p_vaddr
	 * - Otherwise (ET_EXEC, no custom address):
	 *   dst = p_paddr (original behavior)
	 */
	dst = elf_phdr_relocated_paddr(elf, phdr);

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

static int elf_segment_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct elf_image *elf = priv;
	struct elf_segment *elf_a, *elf_b;

	if (a == b)
		return 0;

	elf_a = list_entry(a, struct elf_segment, list);
	elf_b = list_entry(b, struct elf_segment, list);

	return elf_phdr_p_offset(elf, elf_a->phdr) >
	       elf_phdr_p_offset(elf, elf_b->phdr);
}

static int load_elf_to_memory(struct elf_image *elf)
{
	void *dst;
	int ret = 0, fd = -1;
	u64 p_filesz, p_memsz, p_offset;
	struct elf_segment *r;
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

		dst = elf_phdr_relocated_paddr(elf, r->phdr);

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
	void *phdr;
	int ret;

	/* File as already been loaded */
	if (!list_empty(&elf->list))
		return -EINVAL;

	/* Calculate load offset for ET_DYN */
	ret = elf_compute_load_offset(elf);
	if (ret)
		return ret;

	elf_for_each_segment(phdr, elf, buf) {
		ret = request_elf_segment(elf, phdr);
		if (ret)
			goto elf_release_regions;
	}

	/*
	 * Sort the list to avoid doing backward lseek while loading the elf
	 * segments from file to memory(some filesystems don't support it)
	 */
	list_sort(elf, &elf->list, elf_segment_cmp);

	ret = load_elf_to_memory(elf);
	if (ret)
		goto elf_release_regions;

	return 0;

elf_release_regions:
	elf_release_regions(elf);

	return ret;
}

struct elf_image *elf_open_binary(void *buf)
{
	int ret;
	struct elf_image *elf;

	elf = calloc(1, sizeof(*elf));
	if (!elf)
		return ERR_PTR(-ENOMEM);

	ret = elf_open_binary_into(elf, buf);
	if (ret) {
		free(elf);
		return ERR_PTR(ret);
	}

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

	elf->filename = strdup_const(filename);
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

static int elf_relocate(struct elf_image *elf)
{
	void *dyn_seg;

	/*
	 * Relocations needed if:
	 * - ET_DYN (position-independent), OR
	 * - ET_EXEC with custom load address
	 */
	if (elf->type == ET_EXEC && !elf->load_address)
		return 0;

	/* Find PT_DYNAMIC segment */
	dyn_seg = elf_find_dynamic_segment(elf);
	if (!dyn_seg) {
		/*
		 * No PT_DYNAMIC segment found.
		 * For ET_DYN this is unusual but legal.
		 * For ET_EXEC with custom load address, this means no relocations
		 * can be applied - warn the user.
		 */
		if (elf->type == ET_EXEC && elf->load_address) {
			pr_warn("ET_EXEC loaded at custom address but no PT_DYNAMIC segment - "
				"relocations cannot be applied\n");
		} else {
			pr_debug("No PT_DYNAMIC segment found\n");
		}
		return 0;
	}

	/* Call architecture-specific relocation handler */
	return elf_apply_relocations(elf, dyn_seg);
}

int elf_load(struct elf_image *elf)
{
	int ret;

	ret = load_elf_image_segments(elf);
	if (ret)
		return ret;

	/* Apply relocations if needed */
	ret = elf_relocate(elf);
	if (ret) {
		pr_err("Relocation failed: %d\n", ret);
		return ret;
	}

	return 0;
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
