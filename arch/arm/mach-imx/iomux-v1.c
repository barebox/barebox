#include <common.h>
#include <io.h>
#include <mach/iomux-v1.h>

/*
 *  GPIO Module and I/O Multiplexer
 *  x = 0..3 for reg_A, reg_B, reg_C, reg_D
 *
 *  i.MX1 and i.MXL: 0 <= x <= 3
 *  i.MX27         : 0 <= x <= 5
 */
#define DDIR    0x00
#define OCR1    0x04
#define OCR2    0x08
#define ICONFA1 0x0c
#define ICONFA2 0x10
#define ICONFB1 0x14
#define ICONFB2 0x18
#define DR      0x1c
#define GIUS    0x20
#define SSR     0x24
#define ICR1    0x28
#define ICR2    0x2c
#define IMR     0x30
#define ISR     0x34
#define GPR     0x38
#define SWR     0x3c
#define PUEN    0x40

static void __iomem *iomuxv1_base;

void imx_gpio_mode(int gpio_mode)
{
	unsigned int pin = gpio_mode & GPIO_PIN_MASK;
	unsigned int port = (gpio_mode & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	unsigned int ocr = (gpio_mode & GPIO_OCR_MASK) >> GPIO_OCR_SHIFT;
	unsigned int aout = (gpio_mode & GPIO_AOUT_MASK) >> GPIO_AOUT_SHIFT;
	unsigned int bout = (gpio_mode & GPIO_BOUT_MASK) >> GPIO_BOUT_SHIFT;
	void __iomem *portbase = iomuxv1_base + port * 0x100;
	uint32_t val;

	if (!iomuxv1_base)
		return;

	/* Pullup enable */
	val = readl(portbase + PUEN);
	if (gpio_mode & GPIO_PUEN)
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + PUEN);

	/* Data direction */
	val = readl(portbase + DDIR);
	if (gpio_mode & GPIO_OUT)
		val |= 1 << pin;
	else
		val &= ~(1 << pin);
	writel(val, portbase + DDIR);

	/* Primary / alternate function */
	val = readl(portbase + GPR);
	if (gpio_mode & GPIO_AF)
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + GPR);

	/* use as gpio? */
	val = readl(portbase + GIUS);
	if (!(gpio_mode & (GPIO_PF | GPIO_AF)))
		val |= (1 << pin);
	else
		val &= ~(1 << pin);
	writel(val, portbase + GIUS);

	/* Output / input configuration */
	if (pin < 16) {
		val = readl(portbase + OCR1);
		val &= ~(3 << (pin * 2));
		val |= (ocr << (pin * 2));
		writel(val, portbase + OCR1);

		val = readl(portbase + ICONFA1);
		val &= ~(3 << (pin * 2));
		val |= aout << (pin * 2);
		writel(val, portbase + ICONFA1);

		val = readl(portbase + ICONFB1);
		val &= ~(3 << (pin * 2));
		val |= bout << (pin * 2);
		writel(val, portbase + ICONFB1);
	} else {
		pin -= 16;

		val = readl(portbase + OCR2);
		val &= ~(3 << (pin * 2));
		val |= (ocr << (pin * 2));
		writel(val, portbase + OCR2);

		val = readl(portbase + ICONFA2);
		val &= ~(3 << (pin * 2));
		val |= aout << (pin * 2);
		writel(val, portbase + ICONFA2);

		val = readl(portbase + ICONFB2);
		val &= ~(3 << (pin * 2));
		val |= bout << (pin * 2);
		writel(val, portbase + ICONFB2);
	}
}

void imx_iomuxv1_init(void __iomem *base)
{
	iomuxv1_base = base;
}
