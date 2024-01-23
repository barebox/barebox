// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2016-2022, ARM Limited and Contributors. All rights reserved.
 *
 */
#define pr_fmt(fmt) "tzc400: " fmt

#include <common.h>
#include <linux/bitfield.h>
#include <linux/sizes.h>
#include <mach/layerscape/lowlevel.h>
#include <mach/layerscape/layerscape.h>

#include "tzc400.h"

static inline void mmio_write_32(uintptr_t addr, uint32_t value)
{
	out_le32(addr, value);
}

static inline uint32_t mmio_read_32(uintptr_t addr)
{
	return in_le32(addr);
}

static inline void mmio_clrsetbits_32(uintptr_t addr,
                                uint32_t clear,
                                uint32_t set)
{
	clrsetbits_le32(addr, clear, set);
}

static inline unsigned int tzc_read_peripheral_id(uintptr_t base)
{
	unsigned int id;

	id = mmio_read_32(base + PID0_OFF);
	/* Masks DESC part in PID1 */
	id |= ((mmio_read_32(base + PID1_OFF) & 0xFU) << 8U);

	return id;
}

/*
 * Implementation defined values used to validate inputs later.
 * Filters : max of 4 ; 0 to 3
 * Regions : max of 9 ; 0 to 8
 * Address width : Values between 32 to 64
 */
struct tzc400_instance {
	uintptr_t base;
	uint8_t addr_width;
	uint8_t num_filters;
	uint8_t num_regions;
};

static struct tzc400_instance tzc400;

static inline unsigned int tzc400_read_gate_keeper(void)
{
	uintptr_t base = tzc400.base;

	return mmio_read_32(base + TZC400_GATE_KEEPER);
}

static inline void tzc400_write_gate_keeper(unsigned int val)
{
	uintptr_t base = tzc400.base;

	mmio_write_32(base + TZC400_GATE_KEEPER, val);
}

static unsigned int tzc400_open_status(void)
{
	return FIELD_GET(TZC400_GATE_KEEPER_OS, tzc400_read_gate_keeper());
}

static unsigned int tzc400_get_gate_keeper(unsigned int filter)
{
	return (tzc400_open_status() >> filter) & GATE_KEEPER_FILTER_MASK;
}

/* This function is not MP safe. */
static void tzc400_set_gate_keeper(unsigned int filter, int val)
{
	unsigned int os;

	/* Upper half is current state. Lower half is requested state. */
	os = tzc400_open_status();

	if (val != 0)
		os |=  (1UL << filter);
	else
		os &= ~(1UL << filter);

	tzc400_write_gate_keeper(FIELD_PREP(TZC400_GATE_KEEPER_OR, os));

	/* Wait here until we see the change reflected in the TZC status. */
	while ((tzc400_open_status()) != os)
		;
}

void tzc400_set_action(unsigned int action)
{
	uintptr_t base = tzc400.base;

	ASSERT(base != 0U);
	ASSERT(action <= TZC_ACTION_ERR_INT);

	mmio_write_32(base + TZC400_ACTION, action);
}

void tzc400_init(uintptr_t base)
{
	unsigned int tzc400_id;
	unsigned int tzc400_build;

	tzc400.base = base;

	tzc400_id = tzc_read_peripheral_id(base);
	if (tzc400_id != TZC400_PERIPHERAL_ID)
		panic("TZC-400 : Wrong device ID (0x%x).\n", tzc400_id);

	/* Save values we will use later. */
	tzc400_build = mmio_read_32(base + TZC400_BUILD_CONFIG);
	tzc400.num_filters = FIELD_GET(TZC400_BUILD_CONFIG_NF, tzc400_build) + 1;
	tzc400.addr_width  = FIELD_GET(TZC400_BUILD_CONFIG_AW, tzc400_build) + 1;
	tzc400.num_regions = FIELD_GET(TZC400_BUILD_CONFIG_NR, tzc400_build) + 1;
}

/*
 * `tzc400_configure_region` is used to program regions into the TrustZone
 * controller. A region can be associated with more than one filter. The
 * associated filters are passed in as a bitmap (bit0 = filter0), except that
 * the value TZC400_REGION_ATTR_FILTER_BIT_ALL selects all filters, based on
 * the value of tzc400.num_filters.
 * NOTE:
 * Region 0 is special; it is preferable to use tzc400_configure_region0
 * for this region (see comment for that function).
 */
void tzc400_configure_region(unsigned int filters, unsigned int region, uint64_t region_base,
			     uint64_t region_top, unsigned int sec_attr,
			     unsigned int nsaid_permissions)
{
	uintptr_t rbase = tzc400.base + TZC_REGION_OFFSET(TZC400_REGION_SIZE, region);

	/* Adjust filter mask by real filter number */
	if (filters == TZC400_REGION_ATTR_FILTER_BIT_ALL)
		filters = (1U << tzc400.num_filters) - 1U;

	/* Do range checks on filters and regions. */
	ASSERT(((filters >> tzc400.num_filters) == 0U) &&
	       (region < tzc400.num_regions));

	/*
	 * Do address range check based on TZC configuration. A 64bit address is
	 * the max and expected case.
	 */
	ASSERT((region_top <= (U64_MAX >> (64U - tzc400.addr_width))) &&
		(region_base < region_top));

	/* region_base and (region_top + 1) must be 4KB aligned */
	ASSERT(((region_base | (region_top + 1U)) & (4096U - 1U)) == 0U);

	ASSERT(sec_attr <= TZC_REGION_S_RDWR);

	pr_debug("TrustZone : Configuring region %u\n", region);
	pr_debug("TrustZone : ... base = %llx, top = %llx,\n", region_base, region_top);
	pr_debug("TrustZone : ... sec_attr = 0x%x, ns_devs = 0x%x)\n",
		sec_attr, nsaid_permissions);

	/***************************************************/
	/* Inputs look ok, start programming registers.    */
	/* All the address registers are 32 bits wide and  */
	/* have a LOW and HIGH				   */
	/* component used to construct an address up to a  */
	/* 64bit.					   */
	/***************************************************/
	mmio_write_32(rbase + TZC400_REGION_BASE_LOW_0, (uint32_t)region_base);
	mmio_write_32(rbase + TZC400_REGION_BASE_HIGH_0, (uint32_t)(region_base >> 32));
	mmio_write_32(rbase + TZC400_REGION_TOP_LOW_0, (uint32_t)region_top);
	mmio_write_32(rbase + TZC400_REGION_TOP_HIGH_0, (uint32_t)(region_top >> 32));

	/* Enable filter to the region and set secure attributes */
	mmio_write_32(rbase + TZC400_REGION_ATTR_0,
		      (sec_attr << TZC_REGION_ATTR_SEC_SHIFT) | (filters << TZC_REGION_ATTR_F_EN_SHIFT));

	/***************************************************/
	/* Specify which non-secure devices have permission*/
	/* to access this region.			   */
	/***************************************************/
	mmio_write_32(rbase + TZC400_REGION_ID_ACCESS_0, nsaid_permissions);
}

void tzc400_update_filters(unsigned int region, unsigned int filters)
{
	uintptr_t rbase = tzc400.base + TZC_REGION_OFFSET(TZC400_REGION_SIZE, region);
	uint32_t filters_mask = GENMASK(tzc400.num_filters - 1U, 0);

	/* Do range checks on filters and regions. */
	ASSERT(((filters >> tzc400.num_filters) == 0U) &&
	       (region < tzc400.num_regions));

	mmio_clrsetbits_32(rbase + TZC400_REGION_ATTR_0,
			   filters_mask << TZC_REGION_ATTR_F_EN_SHIFT,
			   filters << TZC_REGION_ATTR_F_EN_SHIFT);
}

void tzc400_enable_filters(void)
{
	unsigned int state;
	unsigned int filter;

	ASSERT(tzc400.base != 0U);

	for (filter = 0U; filter < tzc400.num_filters; filter++) {
		state = tzc400_get_gate_keeper(filter);
		if (state != 0U) {
			/* Filter 0 is special and cannot be disabled.
			 * So here we allow it being already enabled. */
			if (filter == 0U)
				continue;

			/*
			 * The TZC filter is already configured. Changing the
			 * programmer's view in an active system can cause
			 * unpredictable behavior therefore panic for now rather
			 * than try to determine whether this is safe in this
			 * instance.
			 *
			 * See the 'ARM (R) CoreLink TM TZC-400 TrustZone (R)
			 * Address Space Controller' Technical Reference Manual.
			 */
			panic("TZC-400 : Filter %u Gatekeeper already enabled.\n",
			      filter);
		}
		tzc400_set_gate_keeper(filter, 1);
	}
}

void tzc400_disable_filters(void)
{
	unsigned int filter;
	unsigned int state;
	unsigned int start = 0U;

	ASSERT(tzc400.base != 0U);

	/* Filter 0 is special and cannot be disabled. */
	state = tzc400_get_gate_keeper(0);
	if (state != 0U)
		start++;

	for (filter = start; filter < tzc400.num_filters; filter++)
		tzc400_set_gate_keeper(filter, 0);
}

unsigned long ls1028a_tzc400_init(unsigned long memsize)
{
	unsigned long lowmem, highmem, lowmem_end;

	tzc400_init(LS1028A_TZC400_BASE);
	tzc400_disable_filters();

	/* Region 0 set to no access by default */
	mmio_write_32(tzc400.base + TZC400_REGION_ATTR_0, TZC_REGION_S_NONE << TZC_REGION_ATTR_SEC_SHIFT);
	mmio_write_32(tzc400.base + TZC400_REGION_ID_ACCESS_0, 0);

	lowmem = min_t(unsigned long, LS1028A_DDR_SDRAM_LOWMEM_SIZE, memsize);
	lowmem_end = LS1028A_DDR_SDRAM_BASE + lowmem;
	highmem = memsize - lowmem;

	/* region 1: secure memory */
	tzc400_configure_region(1, 1,
		lowmem_end - LS1028A_SECURE_DRAM_SIZE,
		lowmem_end - 1,
		TZC_REGION_S_RDWR, TZC_REGION_NS_NONE);

	/* region 2: shared memory */
	tzc400_configure_region(1, 2,
		lowmem_end - LS1028A_SECURE_DRAM_SIZE - LS1028A_SP_SHARED_DRAM_SIZE,
		lowmem_end - LS1028A_SECURE_DRAM_SIZE - 1,
		TZC_REGION_S_RDWR, TZC_NS_ACCESS_ID);

	/* region 3: nonsecure low memory */
	tzc400_configure_region(1, 3,
		LS1028A_DDR_SDRAM_BASE,
		lowmem_end - LS1028A_SECURE_DRAM_SIZE - LS1028A_SP_SHARED_DRAM_SIZE - 1,
		TZC_REGION_S_RDWR, TZC_NS_ACCESS_ID);

	if (highmem)
		/* nonsecure high memory */
		tzc400_configure_region(1, 4,
			LS1028A_DDR_SDRAM_HIGHMEM_BASE,
			LS1028A_DDR_SDRAM_HIGHMEM_BASE + highmem - 1,
			TZC_REGION_S_RDWR, TZC_NS_ACCESS_ID);

	tzc400_set_action(TZC_ACTION_ERR);

	tzc400_enable_filters();

	return lowmem - LS1028A_SECURE_DRAM_SIZE - LS1028A_SP_SHARED_DRAM_SIZE;
}
