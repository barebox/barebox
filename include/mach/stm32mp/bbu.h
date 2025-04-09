/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MACH_STM32MP_BBU_H_
#define MACH_STM32MP_BBU_H_

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE

int stm32mp_bbu_mmc_fip_register(const char *name, const char *devicefile,
				 unsigned long flags);

#else

static inline int stm32mp_bbu_mmc_fip_register(const char *name,
					       const char *devicefile,
					       unsigned long flags)
{
	return -ENOSYS;
}

#endif

#endif /* MACH_STM32MP_BBU_H_ */
