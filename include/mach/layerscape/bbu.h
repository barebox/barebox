/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LAYERSCAPE_BBU_H
#define __MACH_LAYERSCAPE_BBU_H

#include <bbu.h>

static inline int ls1028a_bbu_mmc_register_handler(const char *name,
						   const char *devicefile,
						   unsigned long flags)
{
	return bbu_register_std_file_update(name, flags, devicefile,
					    filetype_layerscape_image);
}

static inline int ls1046a_bbu_mmc_register_handler(const char *name,
						   const char *devicefile,
						   unsigned long flags)
{
	return bbu_register_std_file_update(name, flags, devicefile,
					    filetype_layerscape_image);
}

static inline int ls1046a_bbu_qspi_register_handler(const char *name,
						    const char *devicefile,
						    unsigned long flags)
{
	return bbu_register_std_file_update(name, flags, devicefile,
					    filetype_layerscape_qspi_image);
}

#endif /* __MACH_LAYERSCAPE_BBU_H */
