/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ASM_RELOC_H_
#define _ASM_RELOC_H_

unsigned long get_runtime_offset(void);

void relocate_to_current_adr(void);
void relocate_to_adr(unsigned long target);

void setup_c(void);

#include <asm-generic/reloc.h>

#endif	/* _BAREBOX_RISCV_H_ */
