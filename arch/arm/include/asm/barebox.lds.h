/* SPDX-License-Identifier: GPL-2.0-only */

#ifdef CONFIG_CPU_32
#define BAREBOX_OUTPUT_FORMAT	"elf32-littlearm", "elf32-littlearm", "elf32-littlearm"
#define BAREBOX_OUTPUT_ARCH	"arm"
#else
#define BAREBOX_OUTPUT_FORMAT	"elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64"
#define BAREBOX_OUTPUT_ARCH	"aarch64"
#endif

#ifdef CONFIG_CPU_32
#define BAREBOX_RELOCATION_TYPE	rel
#else
#define BAREBOX_RELOCATION_TYPE	rela
#endif

#define BAREBOX_RELOCATION_TABLE					\
	.rel_dyn_start : { *(.__rel_dyn_start) }			\
	.BAREBOX_RELOCATION_TYPE.dyn : { *(.BAREBOX_RELOCATION_TYPE*) }	\
	.rel_dyn_end : { *(.__rel_dyn_end) }				\
	.__relr_dyn_start : { *(.__relr_dyn_start) }			\
	.relr.dyn : ALIGN(8) { *(.relr.dyn) }				\
	.__relr_dyn_end : { *(.__relr_dyn_end) }			\
	.__dynsym_start :  { *(.__dynsym_start) }			\
	.dynsym : { *(.dynsym) }					\
	.__dynsym_end : { *(.__dynsym_end) }


#include <asm-generic/barebox.lds.h>
