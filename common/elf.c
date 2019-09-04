// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <elf.h>
#include <memory.h>

struct elf_section {
	struct list_head list;
	struct resource *r;
};

static int elf_request_region(struct elf_image *elf, resource_size_t start,
			      resource_size_t size)
{
	struct list_head *list = &elf->list;
	struct resource *r_new;
	struct elf_section *r;

	r = xzalloc(sizeof(*r));
	r_new = request_sdram_region("elf_section", start, size);
	if (!r_new) {
		pr_err("Failed to request region: %pa %pa\n", &start, &size);
		return -EINVAL;
	}

	r->r = r_new;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct elf_image *elf)
{
	struct list_head *list = &elf->list;
	struct elf_section *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		release_sdram_region(r->r);
		free(r);
	}
}


static int load_elf_phdr_segment(struct elf_image *elf, void *src,
				 void *phdr)
{
	void *dst = (void *) elf_phdr_p_paddr(elf, phdr);
	int ret;
	u64 p_filesz = elf_phdr_p_filesz(elf, phdr);
	u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

	/* we care only about PT_LOAD segments */
	if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
		return 0;

	if (!p_filesz)
		return 0;

	pr_debug("Loading phdr to 0x%p (%llu bytes)\n", dst, p_filesz);

	ret = elf_request_region(elf, (resource_size_t)dst, p_filesz);
	if (ret)
		return ret;

	memcpy(dst, src, p_filesz);

	if (p_filesz < p_memsz)
		memset(dst + p_filesz, 0x00,
		       p_memsz - p_filesz);

	return 0;
}

static int load_elf_image_phdr(struct elf_image *elf)
{
	void *buf = elf->buf;
	void *phdr = (void *) (buf + elf_hdr_e_phoff(elf, buf));
	int i, ret;

	elf->entry = elf_hdr_e_entry(elf, buf);

	for (i = 0; i < elf_hdr_e_phnum(elf, buf) ; ++i) {
		void *src = buf + elf_phdr_p_offset(elf, phdr);

		ret = load_elf_phdr_segment(elf, src, phdr);
		/* in case of error elf_load_image() caller should clean up and
		 * call elf_release_image() */
		if (ret)
			return ret;

		phdr += elf_size_of_phdr(elf);
	}

	return 0;
}

static int elf_check_image(struct elf_image *elf)
{
	if (strncmp(elf->buf, ELFMAG, SELFMAG)) {
		pr_err("ELF magic not found.\n");
		return -EINVAL;
	}

	elf->class = ((char *) elf->buf)[EI_CLASS];

	if (elf_hdr_e_type(elf, elf->buf) != ET_EXEC) {
		pr_err("Non EXEC ELF image.\n");
		return -ENOEXEC;
	}

	return 0;
}

struct elf_image *elf_load_image(void *buf)
{
	struct elf_image *elf;
	int ret;

	elf = xzalloc(sizeof(*elf));

	INIT_LIST_HEAD(&elf->list);

	elf->buf = buf;

	ret = elf_check_image(elf);
	if (ret)
		return ERR_PTR(ret);

	ret = load_elf_image_phdr(elf);
	if (ret) {
		elf_release_image(elf);
		return ERR_PTR(ret);
	}

	return elf;
}

void elf_release_image(struct elf_image *elf)
{
	elf_release_regions(elf);

	free(elf);
}
