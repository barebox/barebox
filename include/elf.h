/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ELF_H
#define __ELF_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/elf.h>
#include <lib/elf.h>

static inline size_t elf_get_mem_size(struct elf_image *elf)
{
	return elf->high_addr - elf->low_addr;
}

struct elf_image *elf_open_binary(void *buf);
struct elf_image *elf_open(const char *filename);
void elf_close(struct elf_image *elf);
int elf_load(struct elf_image *elf);

#endif /* __ELF_H */
