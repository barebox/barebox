/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright (c) 2021 Ahmad Fatoum, Pengutronix */

#ifndef __ASM_RISCV_HEAD_H
#define __ASM_RISCV_HEAD_H

#include <linux/kernel.h>
#include <asm/image.h>

#define ____barebox_riscv_header(instr, load_offset, version, magic1, magic2)         \
	__asm__ __volatile__ (                                                        \
		instr "\n"                     /* code0 */                            \
		"j 1f\n"                       /* code1 */                            \
		".balign 8\n"                                                         \
		".dword " #load_offset "\n"    /* Image load offset from RAM start */ \
		".dword _barebox_image_size\n" /* Effective Image size */             \
		".dword 0\n"                   /* Kernel flags */                     \
		".word " #version "\n"         /* version */                          \
		".word 0\n"                    /* reserved */                         \
		".dword 0\n"                   /* reserved */                         \
		".asciz \"" magic1 "\"\n"      /* magic 1 */                          \
		".balign 8\n"                                                         \
		".ascii \"" magic2 "\"\n"      /* magic 2 */                          \
		".word 0\n"                    /* reserved (PE-COFF offset) */        \
		"1:\n"                                                                \
	)

#define __barebox_riscv_header(instr, load_offset, version, magic1, magic2) \
        ____barebox_riscv_header(instr, load_offset, version, magic1, magic2)

#ifndef __barebox_riscv_head
#define __barebox_riscv_head() \
	__barebox_riscv_header("nop", 0x0, 0x0, "barebox", "RSCV")
#endif

#endif /* __ASM_RISCV_HEAD_H */
