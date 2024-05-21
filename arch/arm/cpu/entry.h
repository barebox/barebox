/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ENTRY_H__
#define __ENTRY_H__

#include <common.h>

struct handoff_data;

void __noreturn barebox_non_pbl_start(unsigned long membase,
				      unsigned long memsize,
				      struct handoff_data *hd);

void __noreturn barebox_pbl_start(unsigned long membase,
				  unsigned long memsize,
				  void *boarddata);

#endif
