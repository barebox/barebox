#ifndef __ENTRY_H__
#define __ENTRY_H__

#include <common.h>

void __noreturn barebox_non_pbl_start(unsigned long membase,
				      unsigned long memsize,
				      void *boarddata);

void __noreturn barebox_multi_pbl_start(unsigned long membase,
					unsigned long memsize,
					void *boarddata);

void __noreturn barebox_single_pbl_start(unsigned long membase,
					 unsigned long memsize,
					 void *boarddata);

#endif
