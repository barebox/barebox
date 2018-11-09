/*
 * Copyright 2018 (C) Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/sizes.h>
#include <common.h>
#include <mach/iomux-mx6.h>
#include <mach/imx-gpio.h>
#include <mach/imx6.h>

#include "mem.h"

#define PCBVERSION_PIN IMX_GPIO_NR(2, 2)
#define PCBID0_PIN IMX_GPIO_NR(6, 7)
#define PCBID1_PIN IMX_GPIO_NR(6, 9)

#define MX6S_PAD_NANDF_CLE__GPIO_6_7 \
	IOMUX_PAD(0x0658, 0x0270, 5, 0x0000, 0, 0)
#define MX6S_PAD_NANDF_WP_B__GPIO_6_9 \
	IOMUX_PAD(0x0690, 0x02A8, 5, 0x0000, 0, 0)
#define MX6S_PAD_NANDF_D2__GPIO_2_2 \
	IOMUX_PAD(0x028c, 0x0674, 5, 0x0000, 0, 0)

resource_size_t samx6i_get_size(void)
{
	resource_size_t size = 0;
	int ver, id0, id1;
	int cpu_type = __imx6_cpu_type();
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *gpio6 = IOMEM(MX6_GPIO6_BASE_ADDR);
	void __iomem *gpio2 = IOMEM(MX6_GPIO2_BASE_ADDR);

	if (cpu_type == IMX6_CPUTYPE_IMX6D ||
			cpu_type == IMX6_CPUTYPE_IMX6Q) {
		imx_setup_pad(iomuxbase, MX6Q_PAD_NANDF_CLE__GPIO_6_7);
		imx_setup_pad(iomuxbase, MX6Q_PAD_NANDF_WP_B__GPIO_6_9);
		imx_setup_pad(iomuxbase, MX6Q_PAD_NANDF_D2__GPIO_2_2);
	} else if (cpu_type == IMX6_CPUTYPE_IMX6S ||
			cpu_type == IMX6_CPUTYPE_IMX6DL) {
		imx_setup_pad(iomuxbase, MX6S_PAD_NANDF_CLE__GPIO_6_7);
		imx_setup_pad(iomuxbase, MX6S_PAD_NANDF_WP_B__GPIO_6_9);
		imx_setup_pad(iomuxbase, MX6S_PAD_NANDF_D2__GPIO_2_2);
	};

	imx6_gpio_direction_input(gpio6, 6);
	imx6_gpio_direction_input(gpio6, 9);
	imx6_gpio_direction_input(gpio2, 2);

	ver = imx6_gpio_val(gpio2, 2);
	id0 = imx6_gpio_val(gpio6, 7);
	id1 = imx6_gpio_val(gpio6, 9);

	if (cpu_type == IMX6_CPUTYPE_IMX6D ||
			cpu_type == IMX6_CPUTYPE_IMX6Q) {
		if (ver)
			size = SZ_1G;
		else if (id0 && id1)
			size = SZ_2G;
		else if (id0)
			size = SZ_2G;
		else if (id1)
			size = SZ_1G;
		else
			size = SZ_512M;
	} else if (cpu_type == IMX6_CPUTYPE_IMX6S ||
			cpu_type == IMX6_CPUTYPE_IMX6DL) {
		if (ver)
			size = SZ_512M;
		if (id0 && id1)
			size = SZ_2G;
		else if (id0)
			size = SZ_1G;
		else if (id1)
			size = SZ_512M;
		else
			size = SZ_128M;
	}

	return size;
}
