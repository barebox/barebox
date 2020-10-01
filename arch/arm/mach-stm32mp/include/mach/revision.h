/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2015-2017, STMicroelectronics - All Rights Reserved
 */

#ifndef __MACH_CPUTYPE_H__
#define __MACH_CPUTYPE_H__

#include <mach/bsec.h>
#include <asm/io.h>
#include <mach/stm32.h>

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

/* DBGMCU register */
#define DBGMCU_APB4FZ1		(STM32_DBGMCU_BASE + 0x2C)
#define DBGMCU_IDC		(STM32_DBGMCU_BASE + 0x00)
#define DBGMCU_IDC_DEV_ID_MASK	GENMASK(11, 0)
#define DBGMCU_IDC_DEV_ID_SHIFT	0
#define DBGMCU_IDC_REV_ID_MASK	GENMASK(31, 16)
#define DBGMCU_IDC_REV_ID_SHIFT	16

#define RCC_DBGCFGR		(STM32_RCC_BASE + 0x080C)
#define RCC_DBGCFGR_DBGCKEN	BIT(8)

/* BSEC OTP index */
#define BSEC_OTP_RPN	1
#define BSEC_OTP_PKG	16

/* Device Part Number (RPN) = OTP_DATA1 lower 8 bits */
#define RPN_SHIFT	0
#define RPN_MASK	GENMASK(7, 0)

static inline u32 stm32mp_read_idc(void)
{
	setbits_le32(RCC_DBGCFGR, RCC_DBGCFGR_DBGCKEN);
	return readl(IOMEM(DBGMCU_IDC));
}

/* Get Device Part Number (RPN) from OTP */
static inline int __stm32mp_get_cpu_rpn(u32 *rpn)
{
	int ret = bsec_read_field(BSEC_OTP_RPN, rpn);
	if (ret)
		return ret;

	*rpn = (*rpn >> RPN_SHIFT) & RPN_MASK;
	return 0;
}

static inline int __stm32mp_get_cpu_type(u32 *type)
{
	u32 id;
	int ret = __stm32mp_get_cpu_rpn(type);
	if (ret)
		return ret;

	id = (stm32mp_read_idc() & DBGMCU_IDC_DEV_ID_MASK) >> DBGMCU_IDC_DEV_ID_SHIFT;
	*type |= id << 16;
	return 0;
}

#endif /* __MACH_CPUTYPE_H__ */
