// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Xilinx Zynq GPIO device driver
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.
 *
 * Based on the Linux kernel driver (drivers/gpio/gpio-zynq.c).
 */

#include <common.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <of.h>

/* Maximum banks */
#define ZYNQ_GPIO_MAX_BANK	     4
#define ZYNQMP_GPIO_MAX_BANK	     6

#define ZYNQ_GPIO_BANK0_NGPIO	     32
#define ZYNQ_GPIO_BANK1_NGPIO	     22
#define ZYNQ_GPIO_BANK2_NGPIO	     32
#define ZYNQ_GPIO_BANK3_NGPIO	     32

#define ZYNQMP_GPIO_BANK0_NGPIO	     26
#define ZYNQMP_GPIO_BANK1_NGPIO	     26
#define ZYNQMP_GPIO_BANK2_NGPIO	     26
#define ZYNQMP_GPIO_BANK3_NGPIO	     32
#define ZYNQMP_GPIO_BANK4_NGPIO	     32
#define ZYNQMP_GPIO_BANK5_NGPIO	     32

#define ZYNQ_GPIO_NR_GPIOS	     118
#define ZYNQMP_GPIO_NR_GPIOS	     174

#define ZYNQ_GPIO_BANK0_PIN_MIN(str) 0
#define ZYNQ_GPIO_BANK0_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK0_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK0_NGPIO - 1)
#define ZYNQ_GPIO_BANK1_PIN_MIN(str) (ZYNQ_GPIO_BANK0_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK1_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK1_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK1_NGPIO - 1)
#define ZYNQ_GPIO_BANK2_PIN_MIN(str) (ZYNQ_GPIO_BANK1_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK2_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK2_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK2_NGPIO - 1)
#define ZYNQ_GPIO_BANK3_PIN_MIN(str) (ZYNQ_GPIO_BANK2_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK3_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK3_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK3_NGPIO - 1)
#define ZYNQ_GPIO_BANK4_PIN_MIN(str) (ZYNQ_GPIO_BANK3_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK4_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK4_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK4_NGPIO - 1)
#define ZYNQ_GPIO_BANK5_PIN_MIN(str) (ZYNQ_GPIO_BANK4_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK5_PIN_MAX(str)                                           \
	(ZYNQ_GPIO_BANK5_PIN_MIN(str) + ZYNQ##str##_GPIO_BANK5_NGPIO - 1)

/* Register offsets for the GPIO device */
/* LSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_LSW_OFFSET(BANK) (0x000 + (8 * BANK))
/* MSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_MSW_OFFSET(BANK) (0x004 + (8 * BANK))
/* Data Register-RW */
#define ZYNQ_GPIO_DATA_OFFSET(BANK)	(0x040 + (4 * BANK))
#define ZYNQ_GPIO_DATA_RO_OFFSET(BANK)	(0x060 + (4 * BANK))
/* Direction mode reg-RW */
#define ZYNQ_GPIO_DIRM_OFFSET(BANK)	(0x204 + (0x40 * BANK))
/* Output enable reg-RW */
#define ZYNQ_GPIO_OUTEN_OFFSET(BANK)	(0x208 + (0x40 * BANK))
/* Interrupt mask reg-RO */
#define ZYNQ_GPIO_INTMASK_OFFSET(BANK)	(0x20C + (0x40 * BANK))
/* Interrupt enable reg-WO */
#define ZYNQ_GPIO_INTEN_OFFSET(BANK)	(0x210 + (0x40 * BANK))
/* Interrupt disable reg-WO */
#define ZYNQ_GPIO_INTDIS_OFFSET(BANK)	(0x214 + (0x40 * BANK))
/* Interrupt status reg-RO */
#define ZYNQ_GPIO_INTSTS_OFFSET(BANK)	(0x218 + (0x40 * BANK))
/* Interrupt type reg-RW */
#define ZYNQ_GPIO_INTTYPE_OFFSET(BANK)	(0x21C + (0x40 * BANK))
/* Interrupt polarity reg-RW */
#define ZYNQ_GPIO_INTPOL_OFFSET(BANK)	(0x220 + (0x40 * BANK))
/* Interrupt on any, reg-RW */
#define ZYNQ_GPIO_INTANY_OFFSET(BANK)	(0x224 + (0x40 * BANK))

/* Disable all interrupts mask */
#define ZYNQ_GPIO_IXR_DISABLE_ALL	0xFFFFFFFF

/* Mid pin number of a bank */
#define ZYNQ_GPIO_MID_PIN_NUM		16

/* GPIO upper 16 bit mask */
#define ZYNQ_GPIO_UPPER_MASK		0xFFFF0000

/* set to differentiate zynq from zynqmp, 0=zynqmp, 1=zynq */
#define ZYNQ_GPIO_QUIRK_IS_ZYNQ		BIT(0)
#define GPIO_QUIRK_DATA_RO_BUG		BIT(1)

/**
 * struct zynq_gpio - GPIO device private data structure
 * @chip:	instance of the gpio_chip
 * @base_addr:	base address of the GPIO device
 * @p_data:	pointer to platform data
 */
struct zynq_gpio {
	struct gpio_chip chip;
	void __iomem *base_addr;
	const struct zynq_platform_data *p_data;
};

/**
 * struct zynq_platform_data - Zynq GPIO platform data structure
 * @quirks:	Flags is used to identify the platform
 * @ngpio:	max number of gpio pins
 * @max_bank:	maximum number of gpio banks
 * @bank_min:	this array represents bank's min pin
 * @bank_max:	this array represents bank's max pin
 */
struct zynq_platform_data {
	u32 quirks;
	u16 ngpio;
	int max_bank;
	int bank_min[ZYNQMP_GPIO_MAX_BANK];
	int bank_max[ZYNQMP_GPIO_MAX_BANK];
};

/**
 * zynq_gpio_is_zynq - Test if HW is Zynq or ZynqMP
 * @gpio:	Pointer to driver data struct
 *
 * Return: 0 if ZynqMP, 1 if Zynq.
 */
static int zynq_gpio_is_zynq(struct zynq_gpio *gpio)
{
	return !!(gpio->p_data->quirks & ZYNQ_GPIO_QUIRK_IS_ZYNQ);
}

/**
 * gpio_data_ro_bug - Test if HW bug exists or not
 * @gpio:       Pointer to driver data struct
 *
 * Return: 0 if bug does not exist, 1 if bug exists.
 */
static int gpio_data_ro_bug(struct zynq_gpio *gpio)
{
	return !!(gpio->p_data->quirks & GPIO_QUIRK_DATA_RO_BUG);
}

/**
 * zynq_gpio_get_bank_pin - Get the bank number and pin number within that bank
 * for a given pin in the GPIO device
 * @pin_num:	gpio pin number within the device
 * @bank_num:	an output parameter used to return the bank number of the gpio
 *		pin
 * @bank_pin_num: an output parameter used to return pin number within a bank
 *		  for the given gpio pin
 * @gpio:	gpio device data structure
 *
 * Returns the bank number and pin offset within the bank.
 */
static int zynq_gpio_get_bank_pin(unsigned int pin_num, unsigned int *bank_num,
				  unsigned int *bank_pin_num,
				  struct zynq_gpio *gpio)
{
	int bank;

	for (bank = 0; bank < gpio->p_data->max_bank; bank++) {
		if ((pin_num >= gpio->p_data->bank_min[bank]) &&
		    (pin_num <= gpio->p_data->bank_max[bank])) {
			*bank_num = bank;
			*bank_pin_num = pin_num - gpio->p_data->bank_min[bank];
			return 0;
		}
	}

	*bank_num = 0;
	*bank_pin_num = 0;
	return -ENODEV;
}

/**
 * zynq_gpio_get_value - Get the state of the specified pin of GPIO device
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 *
 * This function reads the state of the specified pin of the GPIO device.
 *
 * Return: 0 if the pin is low, 1 if pin is high.
 */
static int zynq_gpio_get_value(struct gpio_chip *chip, unsigned int pin)
{
	u32 data;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = container_of(chip, struct zynq_gpio, chip);

	if (zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio) < 0)
		return -EINVAL;

	if (gpio_data_ro_bug(gpio)) {
		if (zynq_gpio_is_zynq(gpio)) {
			if (bank_num <= 1) {
				data = readl_relaxed(
					gpio->base_addr +
					ZYNQ_GPIO_DATA_RO_OFFSET(bank_num));
			} else {
				data = readl_relaxed(
					gpio->base_addr +
					ZYNQ_GPIO_DATA_OFFSET(bank_num));
			}
		} else {
			if (bank_num <= 2) {
				data = readl_relaxed(
					gpio->base_addr +
					ZYNQ_GPIO_DATA_RO_OFFSET(bank_num));
			} else {
				data = readl_relaxed(
					gpio->base_addr +
					ZYNQ_GPIO_DATA_OFFSET(bank_num));
			}
		}
	} else {
		data = readl_relaxed(gpio->base_addr +
				     ZYNQ_GPIO_DATA_RO_OFFSET(bank_num));
	}
	return (data >> bank_pin_num) & 1;
}

/**
 * zynq_gpio_set_value - Modify the state of the pin with specified value
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 * @state:	value used to modify the state of the specified pin
 *
 * This function calculates the register offset (i.e to lower 16 bits or
 * upper 16 bits) based on the given pin number and sets the state of a
 * gpio pin to the specified value. The state is either 0 or non-zero.
 */
static void zynq_gpio_set_value(struct gpio_chip *chip, unsigned int pin,
				int state)
{
	unsigned int reg_offset, bank_num, bank_pin_num;
	struct zynq_gpio *gpio = container_of(chip, struct zynq_gpio, chip);

	if (zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio) < 0)
		return;

	if (bank_pin_num >= ZYNQ_GPIO_MID_PIN_NUM) {
		bank_pin_num -= ZYNQ_GPIO_MID_PIN_NUM;
		reg_offset = ZYNQ_GPIO_DATA_MSW_OFFSET(bank_num);
	} else {
		reg_offset = ZYNQ_GPIO_DATA_LSW_OFFSET(bank_num);
	}

	/*
	 * get the 32 bit value to be written to the mask/data register where
	 * the upper 16 bits is the mask and lower 16 bits is the data
	 */
	state = !!state;
	state = ~(1 << (bank_pin_num + ZYNQ_GPIO_MID_PIN_NUM)) &
		((state << bank_pin_num) | ZYNQ_GPIO_UPPER_MASK);

	writel_relaxed(state, gpio->base_addr + reg_offset);
}

/**
 * zynq_gpio_dir_in - Set the direction of the specified GPIO pin as input
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 *
 * This function uses the read-modify-write sequence to set the direction of
 * the gpio pin as input.
 *
 * Return: 0 always
 */
static int zynq_gpio_dir_in(struct gpio_chip *chip, unsigned int pin)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = container_of(chip, struct zynq_gpio, chip);

	if (zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio) < 0)
		return -EINVAL;
	/*
	 * On zynq bank 0 pins 7 and 8 are special and cannot be used
	 * as inputs.
	 */
	if (zynq_gpio_is_zynq(gpio) && bank_num == 0 &&
	    (bank_pin_num == 7 || bank_pin_num == 8))
		return -EINVAL;

	/* clear the bit in direction mode reg to set the pin as input */
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));
	reg &= ~BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));

	return 0;
}

/**
 * zynq_gpio_dir_out - Set the direction of the specified GPIO pin as output
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 * @state:	value to be written to specified pin
 *
 * This function sets the direction of specified GPIO pin as output, configures
 * the Output Enable register for the pin and uses zynq_gpio_set to set
 * the state of the pin to the value specified.
 *
 * Return: 0 always
 */
static int zynq_gpio_dir_out(struct gpio_chip *chip, unsigned int pin,
			     int state)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = container_of(chip, struct zynq_gpio, chip);

	if (zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio) < 0)
		return -EINVAL;

	/* set the GPIO pin as output */
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));
	reg |= BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));

	/* configure the output enable reg for the pin */
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_OUTEN_OFFSET(bank_num));
	reg |= BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_OUTEN_OFFSET(bank_num));

	/* set the state of the pin */
	zynq_gpio_set_value(chip, pin, state);
	return 0;
}

/**
 * zynq_gpio_get_direction - Read the direction of the specified GPIO pin
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 *
 * This function returns the direction of the specified GPIO.
 *
 * Return: 0 for output, 1 for input
 */
static int zynq_gpio_get_direction(struct gpio_chip *chip, unsigned int pin)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = container_of(chip, struct zynq_gpio, chip);

	if (zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio) < 0)
		return -EINVAL;

	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));

	return !(reg & BIT(bank_pin_num));
}

static struct gpio_ops zynq_gpio_ops = {
	.direction_input = zynq_gpio_dir_in,
	.direction_output = zynq_gpio_dir_out,
	.get = zynq_gpio_get_value,
	.set = zynq_gpio_set_value,
	.get_direction = zynq_gpio_get_direction,
};

static int zynqmp_gpio_probe(struct device *dev)
{
	struct resource *iores;
	struct zynq_gpio *gpio;
	const struct zynq_platform_data *p_data;
	int ret;

	gpio = xzalloc(sizeof(*gpio));
	p_data = device_get_match_data(dev);
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto free_gpio;
	}

	gpio->base_addr = IOMEM(iores->start);
	gpio->chip.base = of_alias_get_id(dev->of_node, "gpio");
	gpio->chip.ops = &zynq_gpio_ops;
	gpio->chip.ngpio = p_data->ngpio;
	gpio->chip.dev = dev;
	gpio->p_data = p_data;

	return gpiochip_add(&gpio->chip);

free_gpio:
	kfree(gpio);
	return ret;
}

static const struct zynq_platform_data zynqmp_gpio_def = {
	.quirks = GPIO_QUIRK_DATA_RO_BUG,
	.ngpio = ZYNQMP_GPIO_NR_GPIOS,
	.max_bank = ZYNQMP_GPIO_MAX_BANK,
	.bank_min[0] = ZYNQ_GPIO_BANK0_PIN_MIN(MP),
	.bank_max[0] = ZYNQ_GPIO_BANK0_PIN_MAX(MP),
	.bank_min[1] = ZYNQ_GPIO_BANK1_PIN_MIN(MP),
	.bank_max[1] = ZYNQ_GPIO_BANK1_PIN_MAX(MP),
	.bank_min[2] = ZYNQ_GPIO_BANK2_PIN_MIN(MP),
	.bank_max[2] = ZYNQ_GPIO_BANK2_PIN_MAX(MP),
	.bank_min[3] = ZYNQ_GPIO_BANK3_PIN_MIN(MP),
	.bank_max[3] = ZYNQ_GPIO_BANK3_PIN_MAX(MP),
	.bank_min[4] = ZYNQ_GPIO_BANK4_PIN_MIN(MP),
	.bank_max[4] = ZYNQ_GPIO_BANK4_PIN_MAX(MP),
	.bank_min[5] = ZYNQ_GPIO_BANK5_PIN_MIN(MP),
	.bank_max[5] = ZYNQ_GPIO_BANK5_PIN_MAX(MP),
};

static const struct zynq_platform_data zynq_gpio_def = {
	.quirks = ZYNQ_GPIO_QUIRK_IS_ZYNQ | GPIO_QUIRK_DATA_RO_BUG,
	.ngpio = ZYNQ_GPIO_NR_GPIOS,
	.max_bank = ZYNQ_GPIO_MAX_BANK,
	.bank_min[0] = ZYNQ_GPIO_BANK0_PIN_MIN(),
	.bank_max[0] = ZYNQ_GPIO_BANK0_PIN_MAX(),
	.bank_min[1] = ZYNQ_GPIO_BANK1_PIN_MIN(),
	.bank_max[1] = ZYNQ_GPIO_BANK1_PIN_MAX(),
	.bank_min[2] = ZYNQ_GPIO_BANK2_PIN_MIN(),
	.bank_max[2] = ZYNQ_GPIO_BANK2_PIN_MAX(),
	.bank_min[3] = ZYNQ_GPIO_BANK3_PIN_MIN(),
	.bank_max[3] = ZYNQ_GPIO_BANK3_PIN_MAX(),
};

static const struct of_device_id zynq_gpio_of_match[] = {
	{ .compatible = "xlnx,zynq-gpio-1.0", .data = &zynq_gpio_def },
	{ .compatible = "xlnx,zynqmp-gpio-1.0", .data = &zynqmp_gpio_def },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, zynq_gpio_of_match);

static struct driver zynqmp_gpio_driver = {
	.name = "zynqmp-gpio",
	.of_compatible = zynq_gpio_of_match,
	.probe = zynqmp_gpio_probe,
};

postcore_platform_driver(zynqmp_gpio_driver);
