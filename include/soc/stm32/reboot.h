/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SOC_STM32_REBOOT_H_
#define __SOC_STM32_REBOOT_H_

#include <linux/compiler.h>

struct device;

#ifdef CONFIG_RESET_STM32
void stm32mp_system_restart_init(struct device *rcc);
#else
static inline void stm32mp_system_restart_init(struct device *rcc)
{
}
#endif

#endif
