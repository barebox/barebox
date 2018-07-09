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
				 Elf32_Phdr *phdr)
{
	void *dst = (void *)phdr->p_paddr;
	int ret;

	/* we care only about PT_LOAD segments */
	if (phdr->p_type != PT_LOAD)
		return 0;

	if (!phdr->p_filesz)
		return 0;

	pr_debug("Loading phdr to 0x%p (%i bytes)\n", dst, phdr->p_filesz);

	ret = elf_request_region(elf, (resource_size_t)dst, phdr->p_filesz);
	if (ret)
		return ret;

	memcpy(dst, src, phdr->p_filesz);

	if (phdr->p_filesz < phdr->p_memsz)
		memset(dst + phdr->p_filesz, 0x00,
		       phdr->p_memsz - phdr->p_filesz);

	return 0;
}

static int load_elf_image_phdr(struct elf_image *elf)
{
	void *buf = elf->buf;
	Elf32_Ehdr *ehdr = buf;
	Elf32_Phdr *phdr = (Elf32_Phdr *)(buf + ehdr->e_phoff);
	int i, ret;

	elf->entry = ehdr->e_entry;

	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *src = buf + phdr->p_offset;

		ret = load_elf_phdr_segment(elf, src, phdr);
		/* in case of error elf_load_image() caller should clean up and
		 * call elf_release_image() */
		if (ret)
			return ret;

		++phdr;
	}

	return 0;
}

static int elf_check_image(void *buf)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;

	if (strncmp(buf, ELFMAG, SELFMAG)) {
		pr_err("ELF magic not found.\n");
		return -EINVAL;
	}

	if (ehdr->e_type != ET_EXEC) {
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

	ret = elf_check_image(buf);
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
