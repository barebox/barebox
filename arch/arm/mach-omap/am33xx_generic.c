/*
 * (C) Copyright 2012 Teresa Gámez, Phytec Messtechnik GmbH
 * (C) Copyright 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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
#include <bootsource.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/generic.h>
#include <mach/sys_info.h>
#include <mach/am33xx-generic.h>
#include <mach/gpmc.h>

void __noreturn reset_cpu(unsigned long addr)
{
	writel(AM33XX_PRM_RSTCTRL_RESET, AM33XX_PRM_RSTCTRL);

	while (1);
}

/**
 * @brief Get the upper address of current execution
 *
 * we can use this to figure out if we are running in SRAM /
 * XIP Flash or in SDRAM
 *
 * @return base address
 */
u32 get_base(void)
{
	u32 val;
	__asm__ __volatile__("mov %0, pc \n":"=r"(val)::"memory");
	val &= 0xF0000000;
	val >>= 28;
	return val;
}

/**
 * @brief Are we running in Flash XIP?
 *
 * If the base is in GPMC address space, we probably are!
 *
 * @return 1 if we are running in XIP mode, else return 0
 */
u32 running_in_flash(void)
{
	if (get_base() < 4)
		return 1;	/* in flash */
	return 0;		/* running in SRAM or SDRAM */
}

/**
 * @brief Are we running in OMAP internal SRAM?
 *
 * If in SRAM address, then yes!
 *
 * @return  1 if we are running in SRAM, else return 0
 */
u32 running_in_sram(void)
{
	if (get_base() == 4)
		return 1;	/* in SRAM */
	return 0;		/* running in FLASH or SDRAM */
}

/**
 * @brief Are we running in SDRAM?
 *
 * if we are not in GPMC nor in SRAM address space,
 * we are in SDRAM execution area
 *
 * @return 1 if we are running from SDRAM, else return 0
 */
u32 running_in_sdram(void)
{
	if (get_base() > 4)
		return 1;	/* in sdram */
	return 0;		/* running in SRAM or FLASH */
}

static int am33xx_bootsource(void)
{
	enum bootsource src;

	switch (omap_bootinfo[2] & 0xFF) {
	case 0x05:
		src = BOOTSOURCE_NAND;
		break;
	case 0x08:
		src = BOOTSOURCE_MMC;
		break;
	case 0x0b:
		src = BOOTSOURCE_SPI;
		break;
	default:
		src = BOOTSOURCE_UNKNOWN;
	}
	bootsource_set(src);
	bootsource_set_instance(0);
	return 0;
}
postcore_initcall(am33xx_bootsource);

int am33xx_register_ethaddr(int eth_id, int mac_id)
{
	void __iomem *mac_id_low = (void *)AM33XX_MAC_ID0_LO + mac_id * 8;
	void __iomem *mac_id_high = (void *)AM33XX_MAC_ID0_HI + mac_id * 8;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;

	mac_lo = readl(mac_id_low);
	mac_hi = readl(mac_id_high);
	mac_addr[0] = mac_hi & 0xff;
	mac_addr[1] = (mac_hi & 0xff00) >> 8;
	mac_addr[2] = (mac_hi & 0xff0000) >> 16;
	mac_addr[3] = (mac_hi & 0xff000000) >> 24;
	mac_addr[4] = mac_lo & 0xff;
	mac_addr[5] = (mac_lo & 0xff00) >> 8;

	if (is_valid_ether_addr(mac_addr)) {
		eth_register_ethaddr(eth_id, mac_addr);
		return 0;
	}

	return -ENODEV;
}

/* GPMC timing for AM33XX nand device */
const struct gpmc_config am33xx_nand_cfg = {
	.cfg = {
		0x00000800,	/* CONF1 */
		0x001e1e00,	/* CONF2 */
		0x001e1e00,	/* CONF3 */
		0x16051807,	/* CONF4 */
		0x00151e1e,	/* CONF5 */
		0x16000f80,	/* CONF6 */
	},
	.base = 0x08000000,
	.size = GPMC_SIZE_16M,
};

static int am33xx_gpio_init(void)
{
	add_generic_device("omap-gpio", 0, NULL, AM33XX_GPIO0_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 1, NULL, AM33XX_GPIO1_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 2, NULL, AM33XX_GPIO2_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 3, NULL, AM33XX_GPIO3_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	return 0;
}
coredevice_initcall(am33xx_gpio_init);
