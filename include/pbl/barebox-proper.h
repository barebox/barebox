/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PBL_BAREBOX_PROPER_H_
#define __PBL_BAREBOX_PROPER_H_

#include <linux/compiler.h>

struct handoff_data;

void *barebox_boot_dtb(void);

void __noreturn barebox_non_pbl_start(unsigned long membase,
				      unsigned long memsize,
				      struct handoff_data *hd);

/* called with the end of the malloc area before its start is chosen */
void arch_adjust_malloc_end(unsigned long *malloc_end);

#endif
