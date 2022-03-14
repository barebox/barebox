/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SOC_STM32_REBOOT_H_
#define __SOC_STM32_REBOOT_H_

#include <linux/compiler.h>

#ifdef CONFIG_RESET_STM32
void stm32mp_system_restart_init(void __iomem *rcc);
#else
static inline void stm32mp_system_restart_init(void __iomem *rcc)
{
}
#endif

#endif
