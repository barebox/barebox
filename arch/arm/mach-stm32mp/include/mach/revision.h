/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2015-2017, STMicroelectronics - All Rights Reserved
 */

#ifndef __MACH_CPUTYPE_H__
#define __MACH_CPUTYPE_H__


/* ID = Device Version (bit31:16) + Device Part Number (RPN) (bit7:0)
 * 157X: 2x Cortex-A7, Cortex-M4, CAN FD, GPU, DSI
 * 153X: 2x Cortex-A7, Cortex-M4, CAN FD
 * 151X: 1x Cortex-A7, Cortex-M4
 * XXXA: Cortex-A7 @ 650 MHz
 * XXXC: Cortex-A7 @ 650 MHz + Secure Boot + HW Crypto
 * XXXD: Cortex-A7 @ 800 MHz
 * XXXF: Cortex-A7 @ 800 MHz + Secure Boot + HW Crypto
 */
#define CPU_STM32MP157Cxx	0x05000000
#define CPU_STM32MP157Axx	0x05000001
#define CPU_STM32MP153Cxx	0x05000024
#define CPU_STM32MP153Axx	0x05000025
#define CPU_STM32MP151Cxx	0x0500002E
#define CPU_STM32MP151Axx	0x0500002F
#define CPU_STM32MP157Fxx	0x05000080
#define CPU_STM32MP157Dxx	0x05000081
#define CPU_STM32MP153Fxx	0x050000A4
#define CPU_STM32MP153Dxx	0x050000A5
#define CPU_STM32MP151Fxx	0x050000AE
#define CPU_STM32MP151Dxx	0x050000AF

/* silicon revisions */
#define CPU_REV_A	0x1000
#define CPU_REV_B	0x2000
#define CPU_REV_Z	0x2001

int stm32mp_silicon_revision(void);
int stm32mp_cputype(void);
int stm32mp_package(void);

#define cpu_is_stm32mp157c() (stm32mp_cputype() == CPU_STM32MP157Cxx)
#define cpu_is_stm32mp157a() (stm32mp_cputype() == CPU_STM32MP157Axx)
#define cpu_is_stm32mp153c() (stm32mp_cputype() == CPU_STM32MP153Cxx)
#define cpu_is_stm32mp153a() (stm32mp_cputype() == CPU_STM32MP153Axx)
#define cpu_is_stm32mp151c() (stm32mp_cputype() == CPU_STM32MP151Cxx)
#define cpu_is_stm32mp151a() (stm32mp_cputype() == CPU_STM32MP151Axx)

#endif /* __MACH_CPUTYPE_H__ */
