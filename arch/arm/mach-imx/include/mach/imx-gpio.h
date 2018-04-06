#ifndef __MACH_IMX_GPIO_H
#define __MACH_IMX_GPIO_H

#include <io.h>

/*
 * i.MX lowlevel gpio functions. Only for use with lowlevel code. Use
 * regular gpio functions outside of lowlevel code!
 */

static inline void imx_gpio_direction(void __iomem *gdir, void __iomem *dr,
		int gpio, int out, int value)
{
	uint32_t val;

	val = readl(gdir);
	if (out)
		val |= 1 << gpio;
	else
		val &= ~(1 << gpio);
	writel(val, gdir);

	if (!out)
		return;

	val = readl(dr);
	if (value)
		val |= 1 << gpio;
	else
		val &= ~(1 << gpio);

	writel(val, dr);
}

static inline void imx1_gpio_direction_output(void *base, int gpio, int value)
{
	imx_gpio_direction(base + 0x0, base + 0x1c, gpio, 1, value);
}

#define imx21_gpio_direction_output(base, gpio, value) imx1_gpio_direction_output(base, gpio,value)
#define imx27_gpio_direction_output(base, gpio, value) imx1_gpio_direction_output(base, gpio,value)

static inline void imx31_gpio_direction_output(void *base, int gpio, int value)
{
	imx_gpio_direction(base + 0x4, base + 0x0, gpio, 1, value);
}

#define imx25_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx35_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx51_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx53_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)
#define imx6_gpio_direction_output(base, gpio, value) imx31_gpio_direction_output(base, gpio,value)

static inline void imx1_gpio_direction_input(void *base, int gpio, int value)
{
	imx_gpio_direction(base + 0x0, base + 0x1c, gpio, 0, 0);
}

#define imx21_gpio_direction_input(base, gpio, value) imx1_gpio_direction_input(base, gpio)
#define imx27_gpio_direction_input(base, gpio, value) imx1_gpio_direction_input(base, gpio)

static inline void imx31_gpio_direction_input(void *base, int gpio)
{
	imx_gpio_direction(base + 0x4, base + 0x0, gpio, 0, 0);
}

#define imx25_gpio_direction_input(base, gpio, value) imx31_gpio_direction_input(base, gpio)
#define imx35_gpio_direction_input(base, gpio, value) imx31_gpio_direction_input(base, gpio)
#define imx51_gpio_direction_input(base, gpio, value) imx31_gpio_direction_input(base, gpio)
#define imx53_gpio_direction_input(base, gpio, value) imx31_gpio_direction_input(base, gpio)
#define imx6_gpio_direction_input(base, gpio) imx31_gpio_direction_input(base, gpio)

#define imx1_gpio_val(base, gpio) readl(base + 0x1c) & (1 << gpio) ? 1 : 0
#define imx21_gpio_val(base, gpio) imx1_gpio_val(base, gpio)
#define imx27_gpio_val(base, gpio) imx1_gpio_val(base, gpio)

#define imx31_gpio_val(base, gpio) readl(base) & (1 << gpio) ? 1 : 0
#define imx25_gpio_val(base, gpio) imx31_gpio_val(base, gpio)
#define imx35_gpio_val(base, gpio) imx31_gpio_val(base, gpio)
#define imx51_gpio_val(base, gpio) imx31_gpio_val(base, gpio)
#define imx53_gpio_val(base, gpio) imx31_gpio_val(base, gpio)
#define imx6_gpio_val(base, gpio) imx31_gpio_val(base, gpio)

#endif /* __MACH_IMX_GPIO_H */
