#include <common.h>
#include <mach/imx-regs.h>

/*
 *  GPIO Module and I/O Multiplexer
 *  x = 0..3 for reg_A, reg_B, reg_C, reg_D
 *
 *  i.MX1 and i.MXL: 0 <= x <= 3
 *  i.MX27         : 0 <= x <= 5
 */
#define DDIR(x)    __REG2(IMX_GPIO_BASE + 0x00, ((x) & 7) << 8)
#define OCR1(x)    __REG2(IMX_GPIO_BASE + 0x04, ((x) & 7) << 8)
#define OCR2(x)    __REG2(IMX_GPIO_BASE + 0x08, ((x) & 7) << 8)
#define ICONFA1(x) __REG2(IMX_GPIO_BASE + 0x0c, ((x) & 7) << 8)
#define ICONFA2(x) __REG2(IMX_GPIO_BASE + 0x10, ((x) & 7) << 8)
#define ICONFB1(x) __REG2(IMX_GPIO_BASE + 0x14, ((x) & 7) << 8)
#define ICONFB2(x) __REG2(IMX_GPIO_BASE + 0x18, ((x) & 7) << 8)
#define DR(x)      __REG2(IMX_GPIO_BASE + 0x1c, ((x) & 7) << 8)
#define GIUS(x)    __REG2(IMX_GPIO_BASE + 0x20, ((x) & 7) << 8)
#define SSR(x)     __REG2(IMX_GPIO_BASE + 0x24, ((x) & 7) << 8)
#define ICR1(x)    __REG2(IMX_GPIO_BASE + 0x28, ((x) & 7) << 8)
#define ICR2(x)    __REG2(IMX_GPIO_BASE + 0x2c, ((x) & 7) << 8)
#define IMR(x)     __REG2(IMX_GPIO_BASE + 0x30, ((x) & 7) << 8)
#define ISR(x)     __REG2(IMX_GPIO_BASE + 0x34, ((x) & 7) << 8)
#define GPR(x)     __REG2(IMX_GPIO_BASE + 0x38, ((x) & 7) << 8)
#define SWR(x)     __REG2(IMX_GPIO_BASE + 0x3c, ((x) & 7) << 8)
#define PUEN(x)    __REG2(IMX_GPIO_BASE + 0x40, ((x) & 7) << 8)

void imx_gpio_mode(int gpio_mode)
{
	unsigned int pin = gpio_mode & GPIO_PIN_MASK;
	unsigned int port = (gpio_mode & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	unsigned int ocr = (gpio_mode & GPIO_OCR_MASK) >> GPIO_OCR_SHIFT;
	unsigned int aout = (gpio_mode & GPIO_AOUT_MASK) >> GPIO_AOUT_SHIFT;
	unsigned int bout = (gpio_mode & GPIO_BOUT_MASK) >> GPIO_BOUT_SHIFT;
	unsigned int tmp;

	/* Pullup enable */
	if(gpio_mode & GPIO_PUEN)
		PUEN(port) |= (1 << pin);
	else
		PUEN(port) &= ~(1 << pin);

	/* Data direction */
	if(gpio_mode & GPIO_OUT)
		DDIR(port) |= 1 << pin;
	else
		DDIR(port) &= ~(1 << pin);

	/* Primary / alternate function */
	if(gpio_mode & GPIO_AF)
		GPR(port) |= (1 << pin);
	else
		GPR(port) &= ~(1 << pin);

	/* use as gpio? */
	if(!(gpio_mode & (GPIO_PF | GPIO_AF)))
		GIUS(port) |= (1 << pin);
	else
		GIUS(port) &= ~(1 << pin);

	/* Output / input configuration */
	if (pin < 16) {
		tmp = OCR1(port);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		OCR1(port) = tmp;

		ICONFA1(port) &= ~(3 << (pin * 2));
		ICONFA1(port) |= aout << (pin * 2);
		ICONFB1(port) &= ~(3 << (pin * 2));
		ICONFB1(port) |= bout << (pin * 2);
	} else {
		pin -= 16;

		tmp = OCR2(port);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		OCR2(port) = tmp;

		ICONFA2(port) &= ~(3 << (pin * 2));
		ICONFA2(port) |= aout << (pin * 2);
		ICONFB2(port) &= ~(3 << (pin * 2));
		ICONFB2(port) |= bout << (pin * 2);
	}
}

