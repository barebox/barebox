// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: (C) Copyright 2023 Ametek Inc.
// SPDX-FileCopyrightText: 2023 Renaud Barbier <renaud.barbier@ametek.com>,

#include <common.h>
#include <init.h>
#include <bbu.h>
#include <net.h>
#include <crc.h>
#include <fs.h>
#include <io.h>
#include <envfs.h>
#include <libfile.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <asm/system.h>
#include <mach/layerscape/layerscape.h>
#include <of_address.h>
#include <soc/fsl/immap_lsch2.h>

#define PHY_ID_AR8031	0x004dd074

/* Currently 1000FD is not working. Below is a bit of guess work
 * from reading MMD3/MMD7 of the AR8033
 */
static int phy_fixup(struct phy_device *phydev)
{
	unsigned short val;
	int advertise = SUPPORTED_1000baseT_Full | SUPPORTED_1000baseT_Half;

	phydev->advertising &= ~advertise;

	/* Ar8031 phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	phy_write(phydev, 0xd, 0x3);
	phy_write(phydev, 0xe, 0x805d);
	phy_write(phydev, 0xd, 0x4003);
	val = phy_read(phydev, 0xe);
	val &= ~(0x1 << 8);
	phy_write(phydev, 0xe, val);

	/* Use XTAL */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x8016);
	phy_write(phydev, 0xd, 0x4007);
	val = phy_read(phydev, 0xe);
	val &= 0xffe3;
	phy_write(phydev, 0xe, val);

	return 0;
}

static int iot_mem_init(void)
{
	if (!of_machine_is_compatible("fsl,ls1021a"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, 0x40000000);

	return 0;
}
mem_initcall(iot_mem_init);

static int iot_postcore_init(void)
{
	struct ls102xa_ccsr_scfg *scfg = IOMEM(LSCH2_SCFG_ADDR);

	if (!of_machine_is_compatible("fsl,ls1021a"))
		return 0;

	/* clear BD & FR bits for BE BD's and frame data */
	clrbits_be32(&scfg->etsecdmamcr, SCFG_ETSECDMAMCR_LE_BD_FR);
	out_be32(&scfg->etsecmcr, SCFG_ETSECCMCR_GE2_CLK125);

	phy_register_fixup_for_uid(PHY_ID_AR8031, 0xffffffff, phy_fixup);

	return 0;
}
coredevice_initcall(iot_postcore_init);
