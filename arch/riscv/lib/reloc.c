// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: Copyright (c) 2021 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <linux/linkage.h>
#include <asm/sections.h>
#include <asm/barebox-riscv.h>
#include <asm/cache.h>
#include <debug_ll.h>
#include <asm-generic/module.h>

#include <elf.h>

#if __riscv_xlen == 64
#define Elf_Rela			Elf64_Rela
#define R_RISCV_ABSOLUTE		R_RISCV_64
#define DYNSYM_ENTRY(dynsym, rela)	dynsym[ELF_R_SYM(rela->r_info) * 3 + 1]
#elif __riscv_xlen == 32
#define Elf_Rela			Elf32_Rela
#define R_RISCV_ABSOLUTE		R_RISCV_32
#define DYNSYM_ENTRY(dynsym, rela)	dynsym[ELF_R_SYM(rela->r_info) * 4 + 1]
#else
#error unknown riscv target
#endif

#define RISC_R_TYPE(x)	((x) & 0xFF)

void sync_caches_for_execution(void)
{
	local_flush_icache_all();
}

static void relocate_image(unsigned long offset,
			   void *dstart, void *dend,
			   long *dynsym, long *dynend)
{
	Elf_Rela *rela;

	if (!offset)
		return;

	for (rela = dstart; (void *)rela < dend; rela++) {
		unsigned long *fixup;

		fixup = (unsigned long *)(rela->r_offset + offset);

		switch (RISC_R_TYPE(rela->r_info)) {
		case R_RISCV_RELATIVE:
			*fixup = rela->r_addend + offset;
			break;
		case R_RISCV_ABSOLUTE:
			*fixup = DYNSYM_ENTRY(dynsym, rela) + rela->r_addend + offset;
			break;
		default:
			putc_ll('>');
			puthex_ll(rela->r_info);
			putc_ll(' ');
			puthex_ll(rela->r_offset);
			putc_ll(' ');
			puthex_ll(rela->r_addend);
			putc_ll('\n');
			__hang();
		}
	}

}

void relocate_to_current_adr(void)
{
	relocate_image(get_runtime_offset(),
		       runtime_address(__rel_dyn_start),
		       runtime_address(__rel_dyn_end),
		       runtime_address(__dynsym_start),
		       NULL);

	sync_caches_for_execution();
}
