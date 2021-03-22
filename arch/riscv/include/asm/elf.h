/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_RISCV_ELF_H__
#define __ASM_RISCV_ELF_H__

#if __SIZEOF_POINTER__ == 8
#define ELF_CLASS	ELFCLASS64
#else
#define ELF_CLASS	ELFCLASS32
#endif

/* Relocation types used by the dynamic linker */
#define R_RISCV_NONE		0
#define R_RISCV_32		1
#define R_RISCV_64		2
#define R_RISCV_RELATIVE	3

#endif /* __ASM_RISCV_ELF_H__ */
