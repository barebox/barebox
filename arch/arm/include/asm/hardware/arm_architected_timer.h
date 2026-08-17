/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_HARDWARE_ARCH_TIMER_H
#define __ASM_ARM_HARDWARE_ARCH_TIMER_H

#include <linux/types.h>

/**
 * arm_arch_timer_init() - Initialize the ARM architected timer as global
 * clocksource
 * @cntfrq: The timer frequency, if zero the frequency is read from the
 *          CNTFRQ_EL0 register
 *
 * This function is meant to be called in a PBL constructor or in the driver
 * probe function.
 *
 * Return: 0 on success, -ENODEV if the timer frequency can not be determined
 */
int arm_arch_timer_init(uint64_t cntfrq);

#endif
