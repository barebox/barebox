/*
 * (C) Copyright 2007 Pengutronix
 * Sascha Hauer, <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <mach/AT91RM9200.h>
#include <at91rm9200_net.h>
#include <dm9161.h>
#include <miiphy.h>
#include <splash.h>
#include <asm/armlinux.h>
#include <s1d13706fb.h>
#include <net.h>
#include <cfi_flash.h>
#include <init.h>

/*
 * Miscelaneous platform dependent initialisations
 */

static struct cfi_platform_data cfi_info = {
};

struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.map_base = 0x11000000,
	.size     = 16 * 1024 * 1024,

	.platform_data = &cfi_info,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

struct device_d sdram_dev = {
	.name     = "mem",
	.map_base = 0x20000000,
	.size     = 32 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct device_d at91_ath_dev = {
	.name     = "at91_eth",
};

static int devices_init (void)
{
	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&at91_ath_dev);

	armlinux_set_bootparams((void *)(PHYS_SDRAM + 0x100));
	armlinux_set_architecture(MACH_TYPE_ECO920);

	return 0;
}

device_initcall(devices_init);

static unsigned int phy_is_connected (AT91PS_EMAC p_mac)
{
	return 1;
}

static unsigned char phy_init_bogus (AT91PS_EMAC p_mac)
{
	unsigned short val;
	int timeout, adr, speed, fullduplex;

	at91rm9200_EmacEnableMDIO (p_mac);

	/* Scan through phy addresses to find a phy */
	for (adr = 0; adr < 16; adr++) {
		at91rm9200_EmacReadPhy(p_mac, PHY_PHYIDR1 | (adr << 5), &val);
		if (val != 0xffff)
			break;
	}

	adr <<= 5;

	val = PHY_BMCR_RESET;
	at91rm9200_EmacWritePhy(p_mac, PHY_BMCR | adr, &val);

	udelay(1000);

	val = 0x01e1; /* ADVERTISE_100FULL | ADVERTISE_100HALF |
	               * ADVERTISE_10FULL | ADVERTISE_10HALF |
		       * ADVERTISE_CSMA */
	at91rm9200_EmacWritePhy(p_mac, PHY_ANAR | adr, &val);

	at91rm9200_EmacReadPhy(p_mac, PHY_BMCR | adr, &val);
	val |= PHY_BMCR_AUTON | PHY_BMCR_RST_NEG;
	at91rm9200_EmacWritePhy(p_mac, PHY_BMCR | adr, &val);

	timeout = 500;
	do {
		/* at91rm9200_EmacReadPhy() has a udelay(10000)
		 * in it, so this should be about 5 deconds
		 */
		if ((timeout--) == 0) {
			printf("Autonegotiation timeout\n");
			goto out;
		}

		at91rm9200_EmacReadPhy(p_mac, PHY_BMSR | adr, &val);
	} while (!(val & PHY_BMSR_LS));

	at91rm9200_EmacReadPhy(p_mac, PHY_ANLPAR | adr, &val);

	if (val & PHY_ANLPAR_100) {
		speed = 100;
		p_mac->EMAC_CFG |= AT91C_EMAC_SPD;
	} else {
		speed = 10;
		p_mac->EMAC_CFG &= ~AT91C_EMAC_SPD;
	}

	if (val & (PHY_ANLPAR_TXFD | PHY_ANLPAR_10FD)) {
		fullduplex = 1;
		p_mac->EMAC_CFG |= AT91C_EMAC_FD;
	} else {
		fullduplex = 0;
		p_mac->EMAC_CFG &= ~AT91C_EMAC_FD;
	}

	printf("running at %d-%sDuplex\n",speed, fullduplex ? "FUll" : "Half");

out:
	at91rm9200_EmacDisableMDIO (p_mac);

	return 1;
}

void at91rm9200_GetPhyInterface(AT91PS_PhyOps p_phyops)
{
	p_phyops->Init = phy_init_bogus;
	p_phyops->IsPhyConnected = phy_is_connected;
	/* This is not used anywhere */
	p_phyops->GetLinkSpeed = NULL;
	/* ditto */
	p_phyops->AutoNegotiate = NULL;
}

#ifdef CONFIG_DRIVER_VIDEO_S1D13706
static int efb_init(struct efb_info *efb)
{
	writeb(GPIO_CONTROL0_GPO, efb->regs + EFB_GPIO_CONTROL1);
	writeb(PCLK_SOURCE_CLKI2, efb->regs + EFB_PCLK_CONF);
	writeb(0x1, efb->regs + 0x26); /* FIXME: display specific, should be set to zero
					*        according to datasheet
					*/
	return 0;
}

/* Nanya STN Display */
static struct efb_info efb = {
	.fbd = {
		.xres = 320,
		.yres = 240,
		.bpp = 8,
		.fb = (void*)0x40020000,
	},

	.init			= efb_init,
	.regs			= (void*)0x40000000,
	.pixclock		= 100000,
	.hsync_len		= 1,
	.left_margin		= 22,
	.right_margin		= 1,
	.vsync_len		= 1,
	.upper_margin		= 0,
	.lower_margin		= 1,
	.sync			= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.panel_type		= PANEL_TYPE_STN | PANEL_TYPE_WIDTH_8 |
				  PANEL_TYPE_COLOR | PANEL_TYPE_FORMAT_2,
};
#endif
#define SMC_CSR3	0xFFFFFF7C

int misc_init_r(void)
{
	/* Initialization of the Static Memory Controller for Chip Select 3 */
	*(volatile unsigned long*)SMC_CSR3 = 0x00002185;
#ifdef CONFIG_DRIVER_VIDEO_S1D13706
	s1d13706fb_init(&efb);
#endif
#ifdef CONFIG_CMD_SPLASH
	splash_set_fb_data(&efb.fbd);
#endif
	return 0;
}
