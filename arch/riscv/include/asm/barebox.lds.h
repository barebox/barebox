/* SPDX-License-Identifier: GPL-2.0-only */

#define BAREBOX_OUTPUT_ARCH	"riscv"
#ifdef CONFIG_64BIT
#define BAREBOX_OUTPUT_FORMAT	"elf64-littleriscv"
#else
#define BAREBOX_OUTPUT_FORMAT	"elf32-littleriscv"
#endif

#include <asm-generic/barebox.lds.h>
