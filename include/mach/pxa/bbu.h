/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_PXA_BBU_H
#define __MACH_PXA_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE_PXA_NAND
int pxa_bbu_nand_register_handler(const char *name, const char *devicefile);
#else
static inline int pxa_bbu_nand_register_handler(const char *name,
						const char *devicefile)
{
	return 0;
}
#endif

#endif /* __MACH_PXA_BBU_H */
