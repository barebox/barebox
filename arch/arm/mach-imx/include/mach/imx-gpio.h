#ifndef __MACH_IMX_GPIO_H
#define __MACH_IMX_GPIO_H

#include <io.h>

/*
 * i.MX lowlevel gpio functions. Only for use with lowlevel code. Use
 * regular gpio functions outside of lowlevel code!
 */

static inline void imx_gpio_direction_output(void __iomem *gdir, void __iomem *dr,
		int gpio, int value)
{
	uint32_t val;

	val = readl(gdir);
	val |= 1 << gpio;
	writel(val, gdir);

	val = readl(dr);
	if (value)
		val |= 1 << gpio;
	else
		val &= ~(1 << gpio);

	writel(val, dr);
}

static inline void imx1_gpio_direction_output(void *base, int gpio, int value)
{
	imx_gpio_direction_output(base + 0x0, base + 0x1c, gpio, value);
}

#define imx21_gpio_direction_output(base, gpio, value) imx1_gpio_direction_output(base, gpio,value)
#define imx27_gpio_direction_output(base, gpio, value) imx1_gpio_direction_output(base, gpio,value)

static inline void imx31_gpio_direction_output(void *base, int gpio, int value)
{
	imx_gpio_direction_output(base + 0x4, base + 0x0, gpio, value);
}

#define imx25_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx35_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx51_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx53_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx6_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)

#endif /* __MACH_IMX_GPIO_H */
