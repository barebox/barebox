/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2009-2010 Digi International, Inc.
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * (c) 2011 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
 * Modified for barebox by Alexander Shiyan <shc_work@mail.ru>
 */

#include <common.h>
#include <envfs.h>
#include <init.h>
#include <net.h>
#include <notifier.h>
#include <asm/armlinux.h>
#include <linux/sizes.h>
#include <mfd/mc13xxx.h>
#include <mfd/mc13892.h>

#include <mach/bbu.h>
#include <mach/esdctl.h>
#include <mach/iim.h>
#include <mach/imx5.h>
#include <mach/imx51-regs.h>
#include <mach/revision.h>

static const struct ccxmx_ident {
	char		*id_string;
	unsigned int	mem_sz;
	unsigned int	cpu_mhz;
	unsigned char	eth0:1;
	unsigned char	eth1:1;
	unsigned char	wless:1;
	unsigned char	accel:1;
} *ccxmx_id, ccxmx51_ids[] = {
	[0x00] = { NULL /* Unknown */,					0,       0,   0, 0, 0, 0 },
	[0x01] = { NULL /* Not supported */,				0,       0,   0, 0, 0, 0 },
	[0x02] = { "i.MX515@800MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M, 800, 1, 1, 1, 1 },
	[0x03] = { "i.MX515@800MHz, PHY, Ext. Eth, Accel",		SZ_512M, 800, 1, 1, 0, 1 },
	[0x04] = { "i.MX515@600MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M, 600, 1, 1, 1, 1 },
	[0x05] = { "i.MX515@600MHz, PHY, Ext. Eth, Accel",		SZ_512M, 600, 1, 1, 0, 1 },
	[0x06] = { "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_512M, 800, 1, 0, 1, 1 },
	[0x07] = { "i.MX515@800MHz, PHY, Accel",			SZ_512M, 800, 1, 0, 0, 1 },
	[0x08] = { "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_256M, 800, 1, 0, 1, 1 },
	[0x09] = { "i.MX515@800MHz, PHY, Accel",			SZ_256M, 800, 1, 0, 0, 1 },
	[0x0a] = { "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_256M, 600, 1, 0, 1, 1 },
	[0x0b] = { "i.MX515@600MHz, PHY, Accel",			SZ_256M, 600, 1, 0, 0, 1 },
	[0x0c] = { "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_128M, 800, 1, 0, 1, 1 },
	[0x0d] = { "i.MX512@800MHz",					SZ_128M, 800, 0, 0, 0, 0 },
	[0x0e] = { "i.MX515@800MHz, Wireless, PHY, Accel",		SZ_512M, 800, 1, 0, 1, 1 },
	[0x0f] = { "i.MX515@600MHz, PHY, Accel",			SZ_128M, 600, 1, 0, 0, 1 },
	[0x10] = { "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_128M, 600, 1, 0, 1, 1 },
	[0x11] = { "i.MX515@800MHz, PHY, Accel",			SZ_128M, 800, 1, 0, 0, 1 },
	[0x12] = { "i.MX515@600MHz, Wireless, PHY, Accel",		SZ_512M, 600, 1, 0, 1, 1 },
	[0x13] = { "i.MX515@800MHz, PHY, Accel",			SZ_512M, 800, 1, 0, 0, 1 },
};

static u32 boardserial;

static void ccxmx51_power_init(struct mc13xxx *mc13xxx)
{
	u32 val;

	/* Clear GP01-GPO4, enable short circuit protection,  PWGT1SPIEN off */
	val = MC13892_POWER_MISC_REGSCPEN | MC13892_POWER_MISC_PWGT1SPIEN;
	val |= MC13892_POWER_MISC_GPO4ADIN;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	/* Set ICHRG in externally powered mode, 4.2V, Disable thermistor */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_CHARGE, 0xa3827b);

	/* Set core voltage (SW1) to 1.1V NORMAL, 1.05V STANDBY */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_0, &val);
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE1_SHIFT);
	val |= MC13892_SWx_SWx_1_100V << MC13892_SWMODE1_SHIFT;
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE2_SHIFT);
	val |= MC13892_SWx_SWx_1_050V << MC13892_SWMODE2_SHIFT;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_0, val);

	/* Setup VCC (SW2) to 1.225 NORMAL, 1.175V STANDBY */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_1, &val);
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE1_SHIFT);
	val |= MC13892_SWx_SWx_1_225V << MC13892_SWMODE1_SHIFT;
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE2_SHIFT);
	val |= MC13892_SWx_SWx_1_175V << MC13892_SWMODE2_SHIFT;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_1, val);

	/* Setup 1V2_DIG1 (SW3) to 1.2 NORMAL, 1.15V STANDBY */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_2, &val);
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE1_SHIFT);
	val |= MC13892_SWx_SWx_1_200V << MC13892_SWMODE1_SHIFT;
	val &= ~(MC13892_SWx_SWx_VOLT_MASK << MC13892_SWMODE2_SHIFT);
	val |= MC13892_SWx_SWx_1_150V << MC13892_SWMODE2_SHIFT;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_2, val);

	/* Set SW1-SW4 switcher in Auto in NORMAL & STANDBY mode */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_4, &val);
	val &= ~(MC13892_SWMODE_MASK << MC13892_SWMODE1_SHIFT);
	val |= MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE1_SHIFT;
	val &= ~(MC13892_SWMODE_MASK << MC13892_SWMODE2_SHIFT);
	val |= MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE2_SHIFT;
	/* Disable current limit */
	val |= 1 << 22;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_4, val);

	mc13xxx_reg_read(mc13xxx, MC13892_REG_SW_5, &val);
	val &= ~(MC13892_SWMODE_MASK << MC13892_SWMODE3_SHIFT);
	val |= MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE3_SHIFT;
	val &= ~(MC13892_SWMODE_MASK << MC13892_SWMODE4_SHIFT);
	val |= MC13892_SWMODE_AUTO_AUTO << MC13892_SWMODE4_SHIFT;
	/* Enable SWBST */
	val |= 1 << 20;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SW_5, val);

	/* Set VVIDEO=2.775V, VAUDIO=3V, VSD=3.15V */
	val = MC13892_SETTING_1_VVIDEO_2_775 | MC13892_SETTING_1_VAUDIO_3_0;
	val |= MC13892_SETTING_1_VSD_3_15;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_1, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = MC13892_MODE_1_VGEN3CONFIG | MC13892_MODE_1_VCAMCONFIG;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);
	udelay(200);

	/* Set VGEN2=3.15V, VGEN3=1.8V, VDIG=1.25V, VCAM=2.75V */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_SETTING_0, &val);
	val &= ~(MC13892_SETTING_0_VGEN2_MASK | MC13892_SETTING_0_VGEN3_MASK);
	val |= MC13892_SETTING_0_VGEN2_3_15 | MC13892_SETTING_0_VGEN3_1_8;
	val &= ~(MC13892_SETTING_0_VDIG_MASK | MC13892_SETTING_0_VCAM_MASK);
	val |= MC13892_SETTING_0_VDIG_1_25 | MC13892_SETTING_0_VCAM_2_75;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_SETTING_0, val);

	/* Enable OTG function */
	mc13xxx_reg_write(mc13xxx, MC13892_REG_USB1, 0x409);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = MC13892_MODE_1_VGEN3EN | MC13892_MODE_1_VGEN3CONFIG;
	val |= MC13892_MODE_1_VCAMEN | MC13892_MODE_1_VCAMCONFIG;
	val |= MC13892_MODE_1_VVIDEOEN | MC13892_MODE_1_VAUDIOEN;
	val |= MC13892_MODE_1_VSDEN;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_MODE_1, val);

	/* Set VCOIN=3.0V, Keeps VSRTC and CLK32KMCU */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_CTL0, &val);
	val &= ~(7 << 20);
	val |= (1 << 4) | (4 << 20) | (1 << 23);
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_CTL0, val);

	/* De-assert reset of external devices on GP01-GPO4 */
	mc13xxx_reg_read(mc13xxx, MC13892_REG_POWER_MISC, &val);
	/* GPO1 - External */
	/* GP02 - LAN9221 Power */
	/* GP03 - FEC Reset */
	/* GP04 - Wireless Power */
	if (ccxmx_id->eth1) {
		val |= MC13892_POWER_MISC_GPO2EN;
		mdelay(100);
	}
	if (ccxmx_id->eth0)
		val |= MC13892_POWER_MISC_GPO3EN;
	if (ccxmx_id->wless)
		val |= MC13892_POWER_MISC_GPO4EN;
	mc13xxx_reg_write(mc13xxx, MC13892_REG_POWER_MISC, val);

	udelay(100);

	console_flush();
	imx51_init_lowlevel(ccxmx_id->cpu_mhz);
	clock_notifier_call_chain();

	printf("MC13892 PMIC initialized.\n");
}

static void ccxmx51_disable_device(struct device_node *root, const char *label)
{
	struct device_node *np = of_find_node_by_name(root, label);
	if (np)
		of_device_disable(np);
}

static int ccxmx51_board_fixup(struct device_node *root, void *unused)
{
	char *serial;

	if (!ccxmx_id->accel)
		ccxmx51_disable_device(root, "mma7455l@1d");

	if (!ccxmx_id->eth0)
		ccxmx51_disable_device(root, "ethernet@83fec000");

	if (!ccxmx_id->eth1)
		ccxmx51_disable_device(root, "lan9221@5,0");

	if (!ccxmx_id->wless)
		ccxmx51_disable_device(root, "esdhc@70008000");

	serial = basprintf("%08x%08x", 0, boardserial);
	of_set_property(root, "serial-number", serial, strlen(serial) + 1, 1);
	free(serial);

	return 0;
}

static __init int ccxmx51_is_compatible(void)
{
	return of_machine_is_compatible("digi,connectcore-ccxmx51-som");
}

static __init int ccxmx51_sdram_fixup(void)
{
	if (!ccxmx51_is_compatible())
		return 0;

	arm_add_mem_device("ram0", MX51_CSD0_BASE_ADDR, SZ_128M);

	/*
	 * On this board the SDRAM is always configured for 512Mib. The real
	 * size is determined by the board id read from the IIM module.
	 */
	imx_esdctl_disable();

	of_register_fixup(ccxmx51_board_fixup, NULL);

	return 0;
}
postcore_initcall(ccxmx51_sdram_fixup);

static __init int ccxmx51_init(void)
{
	char manloc = 'N';
	u8 hwid[6];

	if (!ccxmx51_is_compatible())
		return 0;

	if ((imx_iim_read(1, 9, hwid, sizeof(hwid)) != sizeof(hwid)) ||
	    (hwid[0] < 0x02) || (hwid[0] >= ARRAY_SIZE(ccxmx51_ids))) {
		printf("Unknown board variant (0x%02x). System halted.\n", hwid[0]);
		hang();
	}

	ccxmx_id = &ccxmx51_ids[hwid[0]];

	switch (hwid[2] & 0xc0) {
	case 0x00:
		manloc = 'B';
		break;
	case 0x40:
		manloc = 'W';
		break;
	case 0x80:
		manloc = 'S';
		break;
	default:
		break;
	}

	eth_register_ethaddr(0, hwid);

	boardserial = ((hwid[2] & 0x3f) << 24) | (hwid[3] << 16) | (hwid[4] << 8) | hwid[5];

	printf("Module Variant: %s (0x%02x)\n", ccxmx_id->id_string, hwid[0]);
	printf("Module HW Rev : %02x\n", hwid[1] + 1);
	printf("Module Serial : %c%d\n", manloc, boardserial);

	if ((ccxmx_id->mem_sz - SZ_128M) > 0)
		arm_add_mem_device("ram1", MX51_CSD0_BASE_ADDR + SZ_128M,
				   ccxmx_id->mem_sz - SZ_128M);

	mc13xxx_register_init_callback(ccxmx51_power_init);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
						BBU_HANDLER_FLAG_DEFAULT);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT))
		defaultenv_append_directory(defaultenv_ccxmx51);

	return 0;
}
fs_initcall(ccxmx51_init);
