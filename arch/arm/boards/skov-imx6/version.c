// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "skov-imx6: " fmt

#include <common.h>
#include <mach/iomux-mx6.h>
#include <mach/imx-gpio.h>
#include <mach/imx6.h>

#include "version.h"

#define V_PAD_CTRL MX6_PAD_CTL_PUS_100K_UP | MX6_PAD_CTL_PUE | MX6_PAD_CTL_PKE | \
	MX6_PAD_CTL_SPEED_LOW

unsigned skov_imx6_get_version(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *gpiobase = IOMEM(MX6_GPIO2_BASE_ADDR);
	unsigned reg;
	unsigned var = 0;
	int cpu_type = __imx6_cpu_type();

	/* mux pins as GPIOs */
	switch (cpu_type) {
	case IMX6_CPUTYPE_IMX6S:
	case IMX6_CPUTYPE_IMX6DL:
		writel(0x05, iomuxbase + 0x348); /* VERSION_0, GPIO2/10 */
		writel(V_PAD_CTRL, iomuxbase + 0x730);
		writel(0x05, iomuxbase + 0x35c); /* VERSION_1, GPIO2/15 */
		writel(V_PAD_CTRL, iomuxbase + 0x744);
		writel(0x05, iomuxbase + 0x340); /* VERSION_2, GPIO2/8 */
		writel(V_PAD_CTRL, iomuxbase + 0x728);
		writel(0x05, iomuxbase + 0x344); /* VERSION_3, GPIO2/9 */
		writel(V_PAD_CTRL, iomuxbase + 0x72C);
		writel(0x05, iomuxbase + 0x350); /* VERSION_4, GPIO2/12 */
		writel(V_PAD_CTRL, iomuxbase + 0x738);
		writel(0x05, iomuxbase + 0x358); /* VERSION_5, GPIO2/14 */
		writel(V_PAD_CTRL, iomuxbase + 0x740);
		writel(0x05, iomuxbase + 0x34c); /* VERSION_6, GPIO2/11 */
		writel(V_PAD_CTRL, iomuxbase + 0x734);
		writel(0x05, iomuxbase + 0x354); /* VERSION_7, GPIO2/13 */
		writel(V_PAD_CTRL, iomuxbase + 0x73C);
		break;
	case IMX6_CPUTYPE_IMX6D:
	case IMX6_CPUTYPE_IMX6Q:
		writel(0x05, iomuxbase + 0x324); /* VERSION_0, GPIO2/10 */
		writel(V_PAD_CTRL, iomuxbase + 0x70c);
		writel(0x05, iomuxbase + 0x338); /* VERSION_1, GPIO2/15 */
		writel(V_PAD_CTRL, iomuxbase + 0x720);
		writel(0x05, iomuxbase + 0x31c); /* VERSION_2, GPIO2/8 */
		writel(V_PAD_CTRL, iomuxbase + 0x704);
		writel(0x05, iomuxbase + 0x320); /* VERSION_3, GPIO2/9 */
		writel(V_PAD_CTRL, iomuxbase + 0x708);
		writel(0x05, iomuxbase + 0x32c); /* VERSION_4, GPIO2/12 */
		writel(V_PAD_CTRL, iomuxbase + 0x714);
		writel(0x05, iomuxbase + 0x334); /* VERSION_5, GPIO2/14 */
		writel(V_PAD_CTRL, iomuxbase + 0x71c);
		writel(0x05, iomuxbase + 0x328); /* VERSION_6, GPIO2/11 */
		writel(V_PAD_CTRL, iomuxbase + 0x710);
		writel(0x05, iomuxbase + 0x330); /* VERSION_7, GPIO2/13 */
		writel(V_PAD_CTRL, iomuxbase + 0x718);
		break;
	default:
		pr_err("Invalid SoC! i.MX6S/DL or i.MX6Q expected (found %d)\n", cpu_type);
		return -1;
	}

	imx6_gpio_direction_input(gpiobase, 13);
	imx6_gpio_direction_input(gpiobase, 11);
	imx6_gpio_direction_input(gpiobase, 14);
	imx6_gpio_direction_input(gpiobase, 12);
	imx6_gpio_direction_input(gpiobase, 9);
	imx6_gpio_direction_input(gpiobase, 8);
	imx6_gpio_direction_input(gpiobase, 15);
	imx6_gpio_direction_input(gpiobase, 10);

	reg = readl(gpiobase + 0x00);
	var |= imx6_gpio_val(gpiobase, 13) << 7;
	var |= imx6_gpio_val(gpiobase, 11) << 6;
	var |= imx6_gpio_val(gpiobase, 14) << 5;
	var |= imx6_gpio_val(gpiobase, 12) << 4;
	var |= imx6_gpio_val(gpiobase, 9) << 3;
	var |= imx6_gpio_val(gpiobase, 8) << 2;
	var |= imx6_gpio_val(gpiobase, 15) << 1;
	var |= imx6_gpio_val(gpiobase, 10) << 0;

	return (~var) & 0xff;
}
