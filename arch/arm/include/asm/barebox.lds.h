/* SPDX-License-Identifier: GPL-2.0-only */

#if defined CONFIG_ARCH_EP93XX
#include <mach/barebox.lds.h>
#endif

#ifdef CONFIG_CPU_32
#define BAREBOX_OUTPUT_FORMAT	"elf32-littlearm", "elf32-littlearm", "elf32-littlearm"
#define BAREBOX_OUTPUT_ARCH	"arm"
#else
#define BAREBOX_OUTPUT_FORMAT	"elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64"
#define BAREBOX_OUTPUT_ARCH	"aarch64"
#endif

#include <asm-generic/barebox.lds.h>
