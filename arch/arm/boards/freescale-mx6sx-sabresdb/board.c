/*
 * Copyright (C) 2014 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "imx6sx-sdb: " fmt

#include <environment.h>
#include <partition.h>
#include <common.h>
#include <linux/sizes.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <generated/mach-types.h>
#include <i2c/i2c.h>

#include <asm/armlinux.h>

#include <mach/devices-imx6.h>
#include <mach/imx6-regs.h>
#include <mach/iomux-mx6.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/bbu.h>

#define PFUZE100_DEVICEID	0x0
#define PFUZE100_REVID		0x3
#define PFUZE100_FABID		0x4

#define PFUZE100_SW1ABVOL	0x20
#define PFUZE100_SW1ABSTBY	0x21
#define PFUZE100_SW1ABCONF	0x24
#define PFUZE100_SW1CVOL	0x2e
#define PFUZE100_SW1CSTBY	0x2f
#define PFUZE100_SW1CCONF	0x32
#define PFUZE100_SW1ABC_SETP(x)	((x - 3000) / 250)
#define PFUZE100_VGEN5CTL	0x70

/* set all switches APS in normal and PFM mode in standby */
static int imx6sx_sdb_setup_pmic_mode(struct i2c_client *client, int chip)
{
	unsigned char offset, i, switch_num, value;

	if (!chip) {
		/* pfuze100 */
		switch_num = 6;
		offset = 0x31;
	} else {
		/* pfuze200 */
		switch_num = 4;
		offset = 0x38;
	}

	value = 0xc;
	if (i2c_write_reg(client, 0x23, &value, 1) < 0)
		return -EIO;

	for (i = 0; i < switch_num - 1; i++)
		if (i2c_write_reg(client, offset + i * 7, &value, 1) < 0)
			return -EIO;

	return 0;
}

static int imx6sx_sdb_setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = -1, bus = 0;

	if (!of_machine_is_compatible("fsl,imx6sx-sdb"))
		return 0;

	/* I2C2 bus (2-1 = 1 in barebox numbering) */
	bus = 0;

	/* PFUZE100 device address is 0x08 */
	addr = 0x08;

	adapter = i2c_get_adapter(bus);
	if (!adapter)
		return -ENODEV;

	client.adapter = adapter;
	client.addr = addr;

	if (i2c_read_reg(&client, PFUZE100_DEVICEID, &value, 1) < 0)
		goto err_out;
	if (i2c_read_reg(&client, PFUZE100_REVID, &rev_id, 1) < 0)
		goto err_out;

	pr_info("Found PFUZE100! deviceid 0x%x, revid 0x%x\n", value, rev_id);

	if (imx6sx_sdb_setup_pmic_mode(&client, value & 0xf))
		goto err_out;

	/* set SW1AB standby volatage 0.975V */
	if (i2c_read_reg(&client, PFUZE100_SW1ABSTBY, &value, 1) < 0)
		goto err_out;

	value &= ~0x3f;
	value |= PFUZE100_SW1ABC_SETP(9750);
	if (i2c_write_reg(&client, PFUZE100_SW1ABSTBY, &value, 1) < 0)
		goto err_out;

	/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
	if (i2c_read_reg(&client, PFUZE100_SW1ABCONF, &value, 1) < 0)
		goto err_out;

	value &= ~0xc0;
	value |= 0x40;
	if (i2c_write_reg(&client, PFUZE100_SW1ABCONF, &value, 1) < 0)
		goto err_out;


	/* set SW1C standby volatage 0.975V */
	if (i2c_read_reg(&client, PFUZE100_SW1CSTBY, &value, 1) < 0)
		goto err_out;

	value &= ~0x3f;
	value |= PFUZE100_SW1ABC_SETP(9750);
	if (i2c_write_reg(&client, PFUZE100_SW1CSTBY, &value, 1) < 0)
		goto err_out;


	/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
	if (i2c_read_reg(&client, PFUZE100_SW1CCONF, &value, 1) < 0)
		goto err_out;

	value &= ~0xc0;
	value |= 0x40;
	if (i2c_write_reg(&client, PFUZE100_SW1CCONF, &value, 1) < 0)
		goto err_out;

	/* Enable power of VGEN5 3V3, needed for SD3 */
	if (i2c_read_reg(&client, PFUZE100_VGEN5CTL, &value, 1) < 0)
		goto err_out;

	value &= ~0x1F;
	value |= 0x1F;
	if (i2c_write_reg(&client, PFUZE100_VGEN5CTL, &value, 1) < 0)
		goto err_out;

	return 0;

err_out:
	pr_err("Setting up PMIC failed\n");

	return -EIO;
}
fs_initcall(imx6sx_sdb_setup_pmic_voltages);

static int ar8031_phy_fixup(struct phy_device *phydev)
{
	/*
	 * Enable 1.8V(SEL_1P5_1P8_POS_REG) on
	 * Phy control debug reg 0
	 */
	phy_write(phydev, 0x1d, 0x1f);
	phy_write(phydev, 0x1e, 0x8);

	/* rgmii tx clock delay enable */
	phy_write(phydev, 0x1d, 0x05);
	phy_write(phydev, 0x1e, 0x100);

	return 0;
}

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

static void imx6sx_sdb_setup_fec(void)
{
	void __iomem *gprbase = (void *)MX6_IOMUXC_BASE_ADDR + 0x4000;
	uint32_t val;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
			ar8031_phy_fixup);

	/* Active high for ncp692 */
	gpio_direction_output(IMX_GPIO_NR(4, 16), 1);

	val = readl(gprbase + IOMUXC_GPR1);
	/* Use 125M anatop loopback REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
	val &= ~(1 << 13);
	val &= ~(1 << 17);
	/* Use 125M anatop loopback REF_CLK1 for ENET2, clear gpr1[14], gpr1[18]*/
	val &= ~(1 << 14);
	val &= ~(1 << 18);
	writel(val, gprbase + IOMUXC_GPR1);

	/* Enable the ENET power, active low */
	gpio_direction_output(IMX_GPIO_NR(2, 6), 0);

	/* Reset AR8031 PHY */
	gpio_direction_output(IMX_GPIO_NR(2, 7), 0);
	udelay(500);
	gpio_set_value(IMX_GPIO_NR(2, 7), 1);
}

static int imx6sx_sdb_coredevices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx6sx-sdb"))
		return 0;

	imx6sx_sdb_setup_fec();

	barebox_set_hostname("mx6sx-sabresdb");

	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc3",
			BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
coredevice_initcall(imx6sx_sdb_coredevices_init);
