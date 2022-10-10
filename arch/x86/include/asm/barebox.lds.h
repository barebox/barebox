/* SPDX-License-Identifier: GPL-2.0-only */

#ifdef CONFIG_X86_32
#define BAREBOX_OUTPUT_FORMAT	"elf32-i386", "elf32-i386", "elf32-i386"
#define BAREBOX_OUTPUT_ARCH	"i386"
#else
#define BAREBOX_OUTPUT_FORMAT	"elf64-x86-64", "elf64-x86-64", "elf64-x86-64"
#define BAREBOX_OUTPUT_ARCH	"i386:x86-64"
#endif

#include <asm-generic/barebox.lds.h>
