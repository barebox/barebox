/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2015-2017, STMicroelectronics - All Rights Reserved
 */

#ifndef __MACH_CPUTYPE_H__
#define __MACH_CPUTYPE_H__

/* ID = Device Version (bit31:16) + Device Part Number (RPN) (bit15:0)*/
#define CPU_STM32MP157Cxx	0x05000000
#define CPU_STM32MP157Axx	0x05000001
#define CPU_STM32MP153Cxx	0x05000024
#define CPU_STM32MP153Axx	0x05000025
#define CPU_STM32MP151Cxx	0x0500002E
#define CPU_STM32MP151Axx	0x0500002F

/* silicon revisions */
#define CPU_REV_A	0x1000
#define CPU_REV_B	0x2000

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
