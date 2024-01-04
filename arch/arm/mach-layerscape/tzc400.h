/*
 * Copyright (c) 2014-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TZC400_H
#define TZC400_H

#include <linux/bits.h>

/*
 * Offset of core registers from the start of the base of configuration
 * registers for each region.
 */

/* ID Registers */
#define PID0_OFF					0xfe0
#define PID1_OFF					0xfe4
#define PID2_OFF					0xfe8
#define PID3_OFF					0xfec
#define PID4_OFF					0xfd0
#define CID0_OFF					0xff0
#define CID1_OFF					0xff4
#define CID2_OFF					0xff8
#define CID3_OFF					0xffc

/*
 * What type of action is expected when an access violation occurs.
 * The memory requested is returned as zero. But we can also raise an event to
 * let the system know it happened.
 * We can raise an interrupt(INT) and/or cause an exception(ERR).
 *  TZC_ACTION_NONE    - No interrupt, no Exception
 *  TZC_ACTION_ERR     - No interrupt, raise exception -> sync external
 *                       data abort
 *  TZC_ACTION_INT     - Raise interrupt, no exception
 *  TZC_ACTION_ERR_INT - Raise interrupt, raise exception -> sync
 *                       external data abort
 */
#define TZC_ACTION_NONE			0
#define TZC_ACTION_ERR			1
#define TZC_ACTION_INT			2
#define TZC_ACTION_ERR_INT		(TZC_ACTION_ERR | TZC_ACTION_INT)

/* Bit positions of TZC_ACTION registers */
#define TZC_ACTION_RV_SHIFT				0
#define TZC_ACTION_RV_MASK				0x3
#define TZC_ACTION_RV_LOWOK				0x0
#define TZC_ACTION_RV_LOWERR				0x1
#define TZC_ACTION_RV_HIGHOK				0x2
#define TZC_ACTION_RV_HIGHERR				0x3

/*
 * Controls secure access to a region. If not enabled secure access is not
 * allowed to region.
 */
#define TZC_REGION_S_NONE		0
#define TZC_REGION_S_RD			1
#define TZC_REGION_S_WR			2
#define TZC_REGION_S_RDWR		(TZC_REGION_S_RD | TZC_REGION_S_WR)

#define TZC_REGION_ATTR_S_RD_SHIFT			30
#define TZC_REGION_ATTR_S_WR_SHIFT			31
#define TZC_REGION_ATTR_F_EN_SHIFT			0
#define TZC_REGION_ATTR_SEC_SHIFT			30
#define TZC_REGION_ATTR_S_RD_MASK			0x1
#define TZC_REGION_ATTR_S_WR_MASK			0x1
#define TZC_REGION_ATTR_SEC_MASK			0x3

#define TZC_REGION_ACCESS_WR_EN_SHIFT			16
#define TZC_REGION_ACCESS_RD_EN_SHIFT			0
#define TZC_REGION_ACCESS_ID_MASK			0xf

/* Macros for allowing Non-Secure access to a region based on NSAID */
#define TZC_REGION_ACCESS_RD(nsaid)				\
	((U(1) << ((nsaid) & TZC_REGION_ACCESS_ID_MASK)) <<	\
	 TZC_REGION_ACCESS_RD_EN_SHIFT)
#define TZC_REGION_ACCESS_WR(nsaid)				\
	((U(1) << ((nsaid) & TZC_REGION_ACCESS_ID_MASK)) <<	\
	 TZC_REGION_ACCESS_WR_EN_SHIFT)
#define TZC_REGION_ACCESS_RDWR(nsaid)				\
	(TZC_REGION_ACCESS_RD(nsaid) |				\
	TZC_REGION_ACCESS_WR(nsaid))

/* Returns offset of registers to program for a given region no */
#define TZC_REGION_OFFSET(region_size, region_no)	\
				((region_size) * (region_no))

#define TZC400_BUILD_CONFIG			0x000
#define TZC400_GATE_KEEPER			0x008
#define TZC400_SPECULATION_CTRL			0x00c
#define TZC400_INT_STATUS			0x010
#define TZC400_INT_CLEAR			0x014

#define TZC400_FAIL_ADDRESS_LOW			0x020
#define TZC400_FAIL_ADDRESS_HIGH		0x024
#define TZC400_FAIL_CONTROL			0x028
#define TZC400_FAIL_ID				0x02c

#define TZC400_BUILD_CONFIG_NF			GENMASK(25, 24)
#define TZC400_BUILD_CONFIG_AW			GENMASK(13, 8)
#define TZC400_BUILD_CONFIG_NR			GENMASK(4, 0)

/*
 * Number of gate keepers is implementation defined. But we know the max for
 * this device is 4. Get implementation details from BUILD_CONFIG.
 */
#define TZC400_GATE_KEEPER_OS			GENMASK(19, 16)
#define TZC400_GATE_KEEPER_OR			GENMASK(3, 0)
#define GATE_KEEPER_FILTER_MASK			0x1

#define TZC400_FAIL_CONTROL_DIR_WRITE		BIT(24)
#define TZC400_FAIL_CONTROL_NS_NONSECURE	BIT(21)
#define TZC400_FAIL_CONTROL_PRIV		BIT(20)

#define TZC400_PERIPHERAL_ID			0x460

/* Filter enable bits in a TZC */
#define TZC400_REGION_ATTR_F_EN_MASK		0xf
#define TZC400_REGION_ATTR_FILTER_BIT(x)	(1) << (x))
#define TZC400_REGION_ATTR_FILTER_BIT_ALL	TZC400_REGION_ATTR_F_EN_MASK

/*
 * All TZC region configuration registers are placed one after another. It
 * depicts size of block of registers for programming each region.
 */
#define TZC400_REGION_SIZE			0x20
#define TZC400_ACTION				0x4

#define FILTER_OFFSET				0x10

#define TZC400_REGION_BASE_LOW_0	0x100
#define TZC400_REGION_BASE_HIGH_0	0x104
#define TZC400_REGION_TOP_LOW_0	0x108
#define TZC400_REGION_TOP_HIGH_0	0x10c
#define TZC400_REGION_ATTR_0		0x110
#define TZC400_REGION_ID_ACCESS_0	0x114

#define TZC_REGION_NS_NONE			0x00000000U

/*
 * NXP Platforms do not support NS Access ID (NSAID) based non-secure access.
 * Supports only non secure through generic NS ACCESS ID
 */
#define TZC_NS_ACCESS_ID			0xFFFFFFFFU

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/
void tzc400_init(uintptr_t base);
void tzc400_configure_region0(unsigned int sec_attr,
			   unsigned int ns_device_access);
void tzc400_configure_region(unsigned int filters,
			  unsigned int region,
			  unsigned long long region_base,
			  unsigned long long region_top,
			  unsigned int sec_attr,
			  unsigned int nsaid_permissions);
void tzc400_update_filters(unsigned int region, unsigned int filters);
void tzc400_set_action(unsigned int action);
void tzc400_enable_filters(void);
void tzc400_disable_filters(void);

#endif /* TZC400_H */
