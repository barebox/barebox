// SPDX-License-Identifier: (GPL-2.0-only OR MIT)
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/pinctrl-ocelot.c?id=b7e5ac83cb16f7ffd11dc23736f84276602100ed
/*
 * Microchip ocelot family pinmux driver — LAN969X subset.
 *
 * Copyright (c) 2017 Microsemi Corporation
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * Ported from Linux drivers/pinctrl/pinctrl-ocelot.c. Identifiers, struct
 * layouts, and pin/function tables are kept aligned with Linux so future
 * fixes can be backported with minimal context churn.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <io.h>
#include <of.h>
#include <pinctrl.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/regmap.h>

#define OCELOT_FUNC_PER_PIN	4

/* GPIO standard registers - byte offsets, identical to Linux. REG/REG_ALT
 * scale these by info->stride to step over the 32-bit-bank groups.
 */
#define OCELOT_GPIO_OUT_SET	0x0
#define OCELOT_GPIO_OUT_CLR	0x4
#define OCELOT_GPIO_OUT		0x8
#define OCELOT_GPIO_IN		0xc
#define OCELOT_GPIO_OE		0x10
#define OCELOT_GPIO_INTR	0x14
#define OCELOT_GPIO_INTR_ENA	0x18
#define OCELOT_GPIO_INTR_IDENT	0x1c
#define OCELOT_GPIO_ALT0	0x20
#define OCELOT_GPIO_ALT1	0x24
#define OCELOT_GPIO_SD_MAP	0x28

/*
 * Function enum — LAN969X subset of Linux's full enum, kept in Linux's
 * ordering. Other SoCs' FUNC_* entries are intentionally omitted; add
 * them when porting more SoCs.
 */
enum {
	FUNC_CAN0_a,
	FUNC_CAN0_b,
	FUNC_CAN1,
	FUNC_CLKMON,
	FUNC_NONE,
	FUNC_FAN,
	FUNC_FC,
	FUNC_FC_SHRD,
	FUNC_FUSA,
	FUNC_GPIO,
	FUNC_IRQ0,
	FUNC_IRQ1,
	FUNC_IRQ3,
	FUNC_IRQ4,
	FUNC_MIIM,
	FUNC_MIIM_Sa,
	FUNC_MIIM_IRQ,
	FUNC_PCIE_PERST,
	FUNC_PTPSYNC_0,
	FUNC_PTPSYNC_1,
	FUNC_PTPSYNC_2,
	FUNC_PTPSYNC_3,
	FUNC_PTPSYNC_4,
	FUNC_PTPSYNC_5,
	FUNC_PTPSYNC_6,
	FUNC_PTPSYNC_7,
	FUNC_QSPI1,
	FUNC_R,
	FUNC_SD,
	FUNC_SFP_SD,
	FUNC_SGPIO_a,
	FUNC_SYNCE,
	FUNC_TWI,
	FUNC_USB_POWER,
	FUNC_USB2PHY_RST,
	FUNC_USB_OVER_DETECT,
	FUNC_USB_ULPI,
	FUNC_EMMC_SD,
	FUNC_MAX
};

static const char *const ocelot_function_names[] = {
	[FUNC_CAN0_a]		= "can0_a",
	[FUNC_CAN0_b]		= "can0_b",
	[FUNC_CAN1]		= "can1",
	[FUNC_CLKMON]		= "clkmon",
	[FUNC_NONE]		= "none",
	[FUNC_FAN]		= "fan",
	[FUNC_FC]		= "fc",
	[FUNC_FC_SHRD]		= "fc_shrd",
	[FUNC_FUSA]		= "fusa",
	[FUNC_GPIO]		= "gpio",
	[FUNC_IRQ0]		= "irq0",
	[FUNC_IRQ1]		= "irq1",
	[FUNC_IRQ3]		= "irq3",
	[FUNC_IRQ4]		= "irq4",
	[FUNC_MIIM]		= "miim",
	[FUNC_MIIM_Sa]		= "miim_slave_a",
	[FUNC_MIIM_IRQ]		= "miim_irq",
	[FUNC_PCIE_PERST]	= "pcie_perst",
	[FUNC_PTPSYNC_0]	= "ptpsync_0",
	[FUNC_PTPSYNC_1]	= "ptpsync_1",
	[FUNC_PTPSYNC_2]	= "ptpsync_2",
	[FUNC_PTPSYNC_3]	= "ptpsync_3",
	[FUNC_PTPSYNC_4]	= "ptpsync_4",
	[FUNC_PTPSYNC_5]	= "ptpsync_5",
	[FUNC_PTPSYNC_6]	= "ptpsync_6",
	[FUNC_PTPSYNC_7]	= "ptpsync_7",
	[FUNC_QSPI1]		= "qspi1",
	[FUNC_R]		= "reserved",
	[FUNC_SD]		= "sd",
	[FUNC_SFP_SD]		= "sfp_sd",
	[FUNC_SGPIO_a]		= "sgpio_a",
	[FUNC_SYNCE]		= "synce",
	[FUNC_TWI]		= "twi",
	[FUNC_USB_POWER]	= "usb_power",
	[FUNC_USB2PHY_RST]	= "usb2phy_rst",
	[FUNC_USB_OVER_DETECT]	= "usb_over_detect",
	[FUNC_USB_ULPI]		= "usb_ulpi",
	[FUNC_EMMC_SD]		= "emmc_sd",
};

struct ocelot_pin_caps {
	unsigned int pin;
	unsigned char functions[OCELOT_FUNC_PER_PIN];
	unsigned char a_functions[OCELOT_FUNC_PER_PIN];	/* Additional functions */
};

struct ocelot_pin_desc {
	unsigned int number;
	const char *name;
	struct ocelot_pin_caps *drv_data;
};

#define LAN969X_P(p, f0, f1, f2, f3, f4, f5, f6, f7)           \
static struct ocelot_pin_caps lan969x_pin_##p = {              \
	.pin = p,                                              \
	.functions = {                                         \
		FUNC_##f0, FUNC_##f1, FUNC_##f2,               \
		FUNC_##f3                                      \
	},                                                     \
	.a_functions = {                                       \
		FUNC_##f4, FUNC_##f5, FUNC_##f6,               \
		FUNC_##f7                                      \
	},                                                     \
}

/* Pinmuxing table taken from data sheet */
/*        Pin   FUNC0      FUNC1   FUNC2         FUNC3                  FUNC4     FUNC5      FUNC6        FUNC7 */
LAN969X_P(0,    GPIO,      IRQ0,   FC_SHRD,      PCIE_PERST,            NONE,     NONE,      NONE,        R);
LAN969X_P(1,    GPIO,      IRQ1,   FC_SHRD,       USB_POWER,            NONE,     NONE,      NONE,        R);
LAN969X_P(2,    GPIO,        FC,      NONE,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(3,    GPIO,        FC,      NONE,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(4,    GPIO,        FC,      NONE,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(5,    GPIO,   SGPIO_a,      NONE,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(6,    GPIO,   SGPIO_a,      NONE,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(7,    GPIO,   SGPIO_a,      NONE,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(8,    GPIO,   SGPIO_a,      NONE,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(9,    GPIO,      MIIM,   MIIM_Sa,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(10,   GPIO,      MIIM,   MIIM_Sa,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(11,   GPIO,  MIIM_IRQ,   MIIM_Sa,          CLKMON,            NONE,     NONE,      NONE,        R);
LAN969X_P(12,   GPIO,      IRQ3,   FC_SHRD,     USB2PHY_RST,            NONE,     NONE,      NONE,        R);
LAN969X_P(13,   GPIO,      IRQ4,   FC_SHRD, USB_OVER_DETECT,            NONE,     NONE,      NONE,        R);
LAN969X_P(14,   GPIO,   EMMC_SD,     QSPI1,              FC,            NONE,     NONE,      NONE,        R);
LAN969X_P(15,   GPIO,   EMMC_SD,     QSPI1,              FC,            NONE,     NONE,      NONE,        R);
LAN969X_P(16,   GPIO,   EMMC_SD,     QSPI1,              FC,            NONE,     NONE,      NONE,        R);
LAN969X_P(17,   GPIO,   EMMC_SD,     QSPI1,       PTPSYNC_0,       USB_POWER,     NONE,      NONE,        R);
LAN969X_P(18,   GPIO,   EMMC_SD,     QSPI1,       PTPSYNC_1,     USB2PHY_RST,     NONE,      NONE,        R);
LAN969X_P(19,   GPIO,   EMMC_SD,     QSPI1,       PTPSYNC_2, USB_OVER_DETECT,     NONE,      NONE,        R);
LAN969X_P(20,   GPIO,   EMMC_SD,      NONE,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(21,   GPIO,   EMMC_SD,      NONE,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(22,   GPIO,   EMMC_SD,      NONE,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(23,   GPIO,   EMMC_SD,      NONE,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(24,   GPIO,   EMMC_SD,      NONE,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(25,   GPIO,       FAN,      FUSA,          CAN0_a,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(26,   GPIO,       FAN,      FUSA,          CAN0_a,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(27,   GPIO,     SYNCE,        FC,            MIIM,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(28,   GPIO,     SYNCE,        FC,            MIIM,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(29,   GPIO,     SYNCE,        FC,        MIIM_IRQ,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(30,   GPIO, PTPSYNC_0,  USB_ULPI,         FC_SHRD,           QSPI1,     NONE,      NONE,        R);
LAN969X_P(31,   GPIO, PTPSYNC_1,  USB_ULPI,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(32,   GPIO, PTPSYNC_2,  USB_ULPI,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(33,   GPIO,        SD,  USB_ULPI,         FC_SHRD,            NONE,     NONE,      NONE,        R);
LAN969X_P(34,   GPIO,        SD,  USB_ULPI,            CAN1,         FC_SHRD,     NONE,      NONE,        R);
LAN969X_P(35,   GPIO,        SD,  USB_ULPI,            CAN1,         FC_SHRD,     NONE,      NONE,        R);
LAN969X_P(36,   GPIO,        SD,  USB_ULPI,      PCIE_PERST,         FC_SHRD,     NONE,      NONE,        R);
LAN969X_P(37,   GPIO,        SD,  USB_ULPI,          CAN0_b,            NONE,     NONE,      NONE,        R);
LAN969X_P(38,   GPIO,        SD,  USB_ULPI,          CAN0_b,            NONE,     NONE,      NONE,        R);
LAN969X_P(39,   GPIO,        SD,  USB_ULPI,            MIIM,            NONE,     NONE,      NONE,        R);
LAN969X_P(40,   GPIO,        SD,  USB_ULPI,            MIIM,            NONE,     NONE,      NONE,        R);
LAN969X_P(41,   GPIO,        SD,  USB_ULPI,        MIIM_IRQ,            NONE,     NONE,      NONE,        R);
LAN969X_P(42,   GPIO, PTPSYNC_3,      CAN1,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(43,   GPIO, PTPSYNC_4,      CAN1,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(44,   GPIO, PTPSYNC_5,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(45,   GPIO, PTPSYNC_6,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(46,   GPIO, PTPSYNC_7,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(47,   GPIO,      NONE,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(48,   GPIO,      NONE,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(49,   GPIO,      NONE,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(50,   GPIO,      NONE,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(51,   GPIO,      NONE,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(52,   GPIO,       FAN,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(53,   GPIO,       FAN,    SFP_SD,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(54,   GPIO,     SYNCE,        FC,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(55,   GPIO,     SYNCE,        FC,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(56,   GPIO,     SYNCE,        FC,            NONE,            NONE,     NONE,      NONE,        R);
LAN969X_P(57,   GPIO,    SFP_SD,   FC_SHRD,             TWI,       PTPSYNC_3,     NONE,      NONE,        R);
LAN969X_P(58,   GPIO,    SFP_SD,   FC_SHRD,             TWI,       PTPSYNC_4,     NONE,      NONE,        R);
LAN969X_P(59,   GPIO,    SFP_SD,   FC_SHRD,             TWI,       PTPSYNC_5,     NONE,      NONE,        R);
LAN969X_P(60,   GPIO,    SFP_SD,   FC_SHRD,             TWI,       PTPSYNC_6,     NONE,      NONE,        R);
LAN969X_P(61,   GPIO,      MIIM,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);
LAN969X_P(62,   GPIO,      MIIM,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);
LAN969X_P(63,   GPIO,  MIIM_IRQ,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);
LAN969X_P(64,   GPIO,        FC,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);
LAN969X_P(65,   GPIO,        FC,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);
LAN969X_P(66,   GPIO,        FC,   FC_SHRD,             TWI,            NONE,     NONE,      NONE,        R);

#define LAN969X_PIN(n) {                                       \
	.number = n,                                           \
	.name = "GPIO_"#n,                                     \
	.drv_data = &lan969x_pin_##n                           \
}

static const struct ocelot_pin_desc lan969x_pins[] = {
	LAN969X_PIN(0),  LAN969X_PIN(1),  LAN969X_PIN(2),  LAN969X_PIN(3),
	LAN969X_PIN(4),  LAN969X_PIN(5),  LAN969X_PIN(6),  LAN969X_PIN(7),
	LAN969X_PIN(8),  LAN969X_PIN(9),  LAN969X_PIN(10), LAN969X_PIN(11),
	LAN969X_PIN(12), LAN969X_PIN(13), LAN969X_PIN(14), LAN969X_PIN(15),
	LAN969X_PIN(16), LAN969X_PIN(17), LAN969X_PIN(18), LAN969X_PIN(19),
	LAN969X_PIN(20), LAN969X_PIN(21), LAN969X_PIN(22), LAN969X_PIN(23),
	LAN969X_PIN(24), LAN969X_PIN(25), LAN969X_PIN(26), LAN969X_PIN(27),
	LAN969X_PIN(28), LAN969X_PIN(29), LAN969X_PIN(30), LAN969X_PIN(31),
	LAN969X_PIN(32), LAN969X_PIN(33), LAN969X_PIN(34), LAN969X_PIN(35),
	LAN969X_PIN(36), LAN969X_PIN(37), LAN969X_PIN(38), LAN969X_PIN(39),
	LAN969X_PIN(40), LAN969X_PIN(41), LAN969X_PIN(42), LAN969X_PIN(43),
	LAN969X_PIN(44), LAN969X_PIN(45), LAN969X_PIN(46), LAN969X_PIN(47),
	LAN969X_PIN(48), LAN969X_PIN(49), LAN969X_PIN(50), LAN969X_PIN(51),
	LAN969X_PIN(52), LAN969X_PIN(53), LAN969X_PIN(54), LAN969X_PIN(55),
	LAN969X_PIN(56), LAN969X_PIN(57), LAN969X_PIN(58), LAN969X_PIN(59),
	LAN969X_PIN(60), LAN969X_PIN(61), LAN969X_PIN(62), LAN969X_PIN(63),
	LAN969X_PIN(64), LAN969X_PIN(65), LAN969X_PIN(66),
};

struct ocelot_pinctrl_desc {
	const char *name;
	const struct ocelot_pin_desc *pins;
	unsigned int npins;
};

struct ocelot_pinctrl {
	struct device *dev;
	struct pinctrl_device pctl;
	struct gpio_chip gpio_chip;
	struct regmap *map;
	struct regmap *pincfg;
	const struct ocelot_pinctrl_desc *desc;
	u8 stride;
	u8 altm_stride;
};

#define to_ocelot_pctl(pdev) container_of(pdev, struct ocelot_pinctrl, pctl)
#define to_ocelot_gpio(gc)   container_of(gc,   struct ocelot_pinctrl, gpio_chip)

static const struct ocelot_pinctrl_desc lan969x_desc = {
	.name = "lan969x-pinctrl",
	.pins = lan969x_pins,
	.npins = ARRAY_SIZE(lan969x_pins),
};

/*
 * Register stride: each ALT0/ALT1/ALT2 group of pin-mux bits spans
 * (info->stride) 32-bit words to cover all pins; the LAN969X 78-pin layout
 * also uses altm_stride == stride.
 */
#define REG(r, info, p) ((r) * (info)->stride + (4 * ((p) / 32)))
#define REG_ALT(msb, info, p) \
	(OCELOT_GPIO_ALT0 * (info)->stride + 4 * ((msb) + ((info)->altm_stride * ((p) / 32))))

static int ocelot_pin_function_idx(struct ocelot_pinctrl *info,
				   unsigned int pin, unsigned int function)
{
	struct ocelot_pin_caps *p = info->desc->pins[pin].drv_data;
	int i;

	for (i = 0; i < OCELOT_FUNC_PER_PIN; i++) {
		if (function == p->functions[i])
			return i;
		if (function == p->a_functions[i])
			return i + OCELOT_FUNC_PER_PIN;
	}

	return -1;
}

/* 3-bit pinmux encoding across ALT0/ALT1/ALT2, used by LAN966X and LAN969X. */
static int lan969x_pinmux_set_mux(struct ocelot_pinctrl *info,
				  unsigned int group, unsigned int selector)
{
	struct ocelot_pin_caps *pin = info->desc->pins[group].drv_data;
	unsigned int p = pin->pin % 32;
	int f;

	f = ocelot_pin_function_idx(info, group, selector);
	if (f < 0)
		return -EINVAL;

	/*
	 * f is encoded on three bits.
	 * bit 0 of f goes in BIT(pin) of ALT[0], bit 1 of f goes in BIT(pin) of
	 * ALT[1], bit 2 of f goes in BIT(pin) of ALT[2]
	 * Note: ALT0/ALT1/ALT2 are organized specially for 78 gpio targets
	 */
	regmap_update_bits(info->map, REG_ALT(0, info, pin->pin),
			   BIT(p), f << p);
	regmap_update_bits(info->map, REG_ALT(1, info, pin->pin),
			   BIT(p), (f >> 1) << p);
	regmap_update_bits(info->map, REG_ALT(2, info, pin->pin),
			   BIT(p), (f >> 2) << p);

	return 0;
}

/* ----- GPIO ops ----- */

static int ocelot_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct ocelot_pinctrl *info = to_ocelot_gpio(chip);
	unsigned int val;

	regmap_read(info->map, REG(OCELOT_GPIO_IN, info, offset), &val);

	return !!(val & BIT(offset % 32));
}

static int ocelot_gpio_set(struct gpio_chip *chip, unsigned int offset,
			   int value)
{
	struct ocelot_pinctrl *info = to_ocelot_gpio(chip);

	if (value)
		regmap_write(info->map, REG(OCELOT_GPIO_OUT_SET, info, offset),
			     BIT(offset % 32));
	else
		regmap_write(info->map, REG(OCELOT_GPIO_OUT_CLR, info, offset),
			     BIT(offset % 32));

	return 0;
}

static int ocelot_gpio_get_direction(struct gpio_chip *chip,
				     unsigned int offset)
{
	struct ocelot_pinctrl *info = to_ocelot_gpio(chip);
	unsigned int val;

	regmap_read(info->map, REG(OCELOT_GPIO_OE, info, offset), &val);

	/* GPIOF_DIR_OUT == 0, GPIOF_DIR_IN == 1 */
	return (val & BIT(offset % 32)) ? 0 : 1;
}

static int ocelot_gpio_direction_input(struct gpio_chip *chip,
				       unsigned int offset)
{
	struct ocelot_pinctrl *info = to_ocelot_gpio(chip);
	unsigned int pin = BIT(offset % 32);

	regmap_update_bits(info->map, REG(OCELOT_GPIO_OE, info, offset),
			   pin, 0);

	return 0;
}

static int ocelot_gpio_direction_output(struct gpio_chip *chip,
					unsigned int offset, int value)
{
	struct ocelot_pinctrl *info = to_ocelot_gpio(chip);
	unsigned int pin = BIT(offset % 32);

	if (value)
		regmap_write(info->map, REG(OCELOT_GPIO_OUT_SET, info, offset),
			     pin);
	else
		regmap_write(info->map, REG(OCELOT_GPIO_OUT_CLR, info, offset),
			     pin);

	regmap_update_bits(info->map, REG(OCELOT_GPIO_OE, info, offset),
			   pin, pin);

	return 0;
}

static struct gpio_ops ocelot_gpio_ops = {
	.direction_input = ocelot_gpio_direction_input,
	.direction_output = ocelot_gpio_direction_output,
	.get_direction = ocelot_gpio_get_direction,
	.get = ocelot_gpio_get,
	.set = ocelot_gpio_set,
};

/* ----- pinctrl ops ----- */

static int ocelot_lookup_function(const char *name)
{
	int i;

	for (i = 0; i < FUNC_MAX; i++) {
		if (ocelot_function_names[i] &&
		    strcmp(name, ocelot_function_names[i]) == 0)
			return i;
	}
	return -1;
}

static int ocelot_lookup_pin(struct ocelot_pinctrl *info, const char *name)
{
	unsigned int i;

	for (i = 0; i < info->desc->npins; i++)
		if (strcmp(info->desc->pins[i].name, name) == 0)
			return i;
	return -1;
}

static int ocelot_pinctrl_set_state(struct pinctrl_device *pdev,
				    struct device_node *np)
{
	struct ocelot_pinctrl *info = to_ocelot_pctl(pdev);
	const char *func_name;
	const char **pin_names;
	int func, ret, n, i;

	ret = of_property_read_string(np, "function", &func_name);
	if (ret < 0)
		return 0;

	func = ocelot_lookup_function(func_name);
	if (func < 0) {
		dev_err(pdev->dev, "unknown function %s\n", func_name);
		return -EINVAL;
	}

	n = of_property_count_strings(np, "pins");
	if (n <= 0)
		return 0;

	pin_names = xzalloc(n * sizeof(*pin_names));
	ret = of_property_read_string_array(np, "pins", pin_names, n);
	if (ret < 0)
		goto out;

	for (i = 0; i < n; i++) {
		int idx = ocelot_lookup_pin(info, pin_names[i]);

		if (idx < 0) {
			dev_err(pdev->dev, "unknown pin %s\n", pin_names[i]);
			ret = -EINVAL;
			goto out;
		}
		ret = lan969x_pinmux_set_mux(info, idx, func);
		if (ret < 0) {
			dev_err(pdev->dev,
				"failed to set pin %s to function %s\n",
				pin_names[i], func_name);
			goto out;
		}
	}
	ret = 0;
out:
	free(pin_names);
	return ret;
}

static struct pinctrl_ops ocelot_pinctrl_ops = {
	.set_state = ocelot_pinctrl_set_state,
};

/* ----- probe ----- */

static const struct regmap_config ocelot_pinctrl_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static int ocelot_pinctrl_probe(struct device *dev)
{
	struct ocelot_pinctrl *info;
	const struct ocelot_pinctrl_desc *desc;
	struct resource *iores;
	void __iomem *base;
	int ret;

	desc = device_get_match_data(dev);
	if (!desc)
		return -EINVAL;

	info = xzalloc(sizeof(*info));
	info->dev = dev;
	info->desc = desc;
	info->stride = 1 + (desc->npins - 1) / 32;
	info->altm_stride = info->stride;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	base = IOMEM(iores->start);

	info->map = regmap_init_mmio(dev, base, &ocelot_pinctrl_regmap_config);
	if (IS_ERR(info->map)) {
		ret = PTR_ERR(info->map);
		goto err_free;
	}

	/* PINCFG resource (optional). */
	iores = dev_request_mem_resource(dev, 1);
	if (!IS_ERR(iores)) {
		void __iomem *pincfg_base = IOMEM(iores->start);

		info->pincfg = regmap_init_mmio(dev, pincfg_base,
						&ocelot_pinctrl_regmap_config);
		if (IS_ERR(info->pincfg))
			info->pincfg = NULL;
	}

	info->pctl.dev = dev;
	info->pctl.ops = &ocelot_pinctrl_ops;
	info->pctl.base = 0;
	info->pctl.npins = desc->npins;
	ret = pinctrl_register(&info->pctl);
	if (ret) {
		dev_err(dev, "failed to register pinctrl: %d\n", ret);
		goto err_free;
	}

	info->gpio_chip.dev = dev;
	info->gpio_chip.base = -1;
	info->gpio_chip.ngpio = desc->npins;
	info->gpio_chip.ops = &ocelot_gpio_ops;
	ret = gpiochip_add(&info->gpio_chip);
	if (ret) {
		dev_err(dev, "failed to register gpio chip: %d\n", ret);
		pinctrl_unregister(&info->pctl);
		goto err_free;
	}

	dev_info(dev, "%s registered: %u pins\n", desc->name, desc->npins);
	return 0;

err_free:
	free(info);
	return ret;
}

static const struct of_device_id ocelot_pinctrl_of_match[] = {
	{ .compatible = "microchip,lan9691-pinctrl", .data = &lan969x_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ocelot_pinctrl_of_match);

static struct driver ocelot_pinctrl_driver = {
	.name = "ocelot-pinctrl",
	.probe = ocelot_pinctrl_probe,
	.of_compatible = DRV_OF_COMPAT(ocelot_pinctrl_of_match),
};
coredevice_platform_driver(ocelot_pinctrl_driver);
