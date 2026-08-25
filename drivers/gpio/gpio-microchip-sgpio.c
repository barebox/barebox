// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/pinctrl-microchip-sgpio.c?id=b7e5ac83cb16f7ffd11dc23736f84276602100ed
/*
 * Microchip SGPIO driver — LAN969X / sparx5 subset.
 *
 * Copyright (c) 2020 Microchip Technology Inc.
 *
 * Ported from Linux drivers/pinctrl/pinctrl-microchip-sgpio.c. Identifiers
 * and register-layout tables are kept aligned with Linux so future fixes
 * can be backported with minimal context churn.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <io.h>
#include <of.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#define SGPIO_BITS_PER_WORD	32
#define SGPIO_MAX_BITS		4

enum {
	REG_INPUT_DATA,
	REG_PORT_CONFIG,
	REG_PORT_ENABLE,
	REG_SIO_CONFIG,
	REG_SIO_CLOCK,
	REG_INT_POLARITY,
	REG_INT_TRIGGER,
	REG_INT_ACK,
	REG_INT_ENABLE,
	REG_INT_IDENT,
	MAXREG
};

enum {
	SGPIO_ARCH_LUTON,
	SGPIO_ARCH_OCELOT,
	SGPIO_ARCH_SPARX5,
};

struct sgpio_properties {
	int arch;
	int flags;
	u8 regoff[MAXREG];
};

#define SGPIO_SPARX5_AUTO_REPEAT BIT(6)
#define SGPIO_SPARX5_PORT_WIDTH  GENMASK(4, 3)
#define SGPIO_SPARX5_CLK_FREQ    GENMASK(19, 8)
#define SGPIO_SPARX5_BIT_SOURCE  GENMASK(23, 12)

static const struct sgpio_properties properties_sparx5 = {
	.arch   = SGPIO_ARCH_SPARX5,
	.regoff = { 0x00, 0x06, 0x26, 0x04, 0x05, 0x2a, 0x32, 0x3a, 0x3e, 0x42 },
};

struct sgpio_priv;

struct sgpio_bank {
	struct sgpio_priv *priv;
	bool is_input;
	struct gpio_chip gpio;
};

struct sgpio_priv {
	struct device *dev;
	struct sgpio_bank in;
	struct sgpio_bank out;
	u32 bitcount;
	u32 ports;
	u32 clock;
	struct regmap *regs;
	const struct sgpio_properties *properties;
};

struct sgpio_port_addr {
	u8 port;
	u8 bit;
};

#define to_sgpio_bank(gc)  container_of(gc, struct sgpio_bank, gpio)

static inline void sgpio_pin_to_addr(struct sgpio_priv *priv, int pin,
				     struct sgpio_port_addr *addr)
{
	addr->port = pin / priv->bitcount;
	addr->bit = pin % priv->bitcount;
}

static inline int sgpio_addr_to_pin(struct sgpio_priv *priv, int port, int bit)
{
	return bit + port * priv->bitcount;
}

static inline u32 sgpio_get_addr(struct sgpio_priv *priv, u32 rno, u32 off)
{
	return (priv->properties->regoff[rno] + off) * 4;
}

static u32 sgpio_readl(struct sgpio_priv *priv, u32 rno, u32 off)
{
	u32 val = 0;
	int ret;

	ret = regmap_read(priv->regs, sgpio_get_addr(priv, rno, off), &val);
	WARN_ONCE(ret, "error reading sgpio reg %d\n", ret);

	return val;
}

static void sgpio_writel(struct sgpio_priv *priv, u32 val, u32 rno, u32 off)
{
	int ret;

	ret = regmap_write(priv->regs, sgpio_get_addr(priv, rno, off), val);
	WARN_ONCE(ret, "error writing sgpio reg %d\n", ret);
}

static inline void sgpio_clrsetbits(struct sgpio_priv *priv,
				    u32 rno, u32 off, u32 clear, u32 set)
{
	int ret;

	ret = regmap_update_bits(priv->regs, sgpio_get_addr(priv, rno, off),
				 clear | set, set);
	WARN_ONCE(ret, "error updating sgpio reg %d\n", ret);
}

static inline void sgpio_configure_bitstream(struct sgpio_priv *priv)
{
	int width = priv->bitcount - 1;
	u32 clr, set;

	/* Only SPARX5 architecture is implemented here. */
	clr = SGPIO_SPARX5_PORT_WIDTH;
	set = SGPIO_SPARX5_AUTO_REPEAT |
	      FIELD_PREP(SGPIO_SPARX5_PORT_WIDTH, width);

	sgpio_clrsetbits(priv, REG_SIO_CONFIG, 0, clr, set);
}

static inline void sgpio_configure_clock(struct sgpio_priv *priv, u32 clkfrq)
{
	u32 clr = SGPIO_SPARX5_CLK_FREQ;
	u32 set = FIELD_PREP(SGPIO_SPARX5_CLK_FREQ, clkfrq);

	sgpio_clrsetbits(priv, REG_SIO_CLOCK, 0, clr, set);
}

/* ---- output / input value helpers ---- */

static int sgpio_output_set(struct sgpio_priv *priv,
			    struct sgpio_port_addr *addr, int value)
{
	unsigned int bit = 3 * addr->bit;
	u32 clr = FIELD_PREP(SGPIO_SPARX5_BIT_SOURCE, BIT(bit));
	u32 set = (value & 1) ? clr : 0;

	sgpio_clrsetbits(priv, REG_PORT_CONFIG, addr->port, clr, set);
	return 0;
}

static int sgpio_output_get(struct sgpio_priv *priv,
			    struct sgpio_port_addr *addr)
{
	u32 portval = sgpio_readl(priv, REG_PORT_CONFIG, addr->port);
	u32 bit_source = FIELD_GET(SGPIO_SPARX5_BIT_SOURCE, portval);

	return !!(bit_source & BIT(3 * addr->bit));
}

static int sgpio_input_get(struct sgpio_priv *priv,
			   struct sgpio_port_addr *addr)
{
	return !!(sgpio_readl(priv, REG_INPUT_DATA, addr->bit) & BIT(addr->port));
}

/* ---- gpio_chip ops ---- */

static int microchip_sgpio_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);

	/* in bank = input, out bank = output. Fixed direction per bank. */
	return bank->is_input ? 1 : 0;
}

static int microchip_sgpio_direction_input(struct gpio_chip *gc,
					   unsigned int gpio)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);

	return bank->is_input ? 0 : -EINVAL;
}

static int microchip_sgpio_direction_output(struct gpio_chip *gc,
					    unsigned int gpio, int value)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);
	struct sgpio_port_addr addr;

	if (bank->is_input)
		return -EINVAL;

	sgpio_pin_to_addr(bank->priv, gpio, &addr);
	return sgpio_output_set(bank->priv, &addr, value);
}

static int microchip_sgpio_get_value(struct gpio_chip *gc, unsigned int gpio)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);
	struct sgpio_port_addr addr;

	sgpio_pin_to_addr(bank->priv, gpio, &addr);
	return bank->is_input ? sgpio_input_get(bank->priv, &addr)
			      : sgpio_output_get(bank->priv, &addr);
}

static int microchip_sgpio_set_value(struct gpio_chip *gc,
				     unsigned int gpio, int value)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);
	struct sgpio_port_addr addr;

	if (bank->is_input)
		return -EINVAL;

	sgpio_pin_to_addr(bank->priv, gpio, &addr);
	return sgpio_output_set(bank->priv, &addr, value);
}

static int microchip_sgpio_of_xlate(struct gpio_chip *gc,
				    const struct of_phandle_args *gpiospec,
				    u32 *flags)
{
	struct sgpio_bank *bank = to_sgpio_bank(gc);
	struct sgpio_priv *priv = bank->priv;
	int pin;

	/*
	 * SGPIO pin is identified by *two* numbers: a port between 0..31
	 * and a bit index 0..3. Cells: <port bit flags>.
	 */
	if (gpiospec->args[0] > SGPIO_BITS_PER_WORD ||
	    gpiospec->args[1] > priv->bitcount)
		return -EINVAL;

	pin = sgpio_addr_to_pin(priv, gpiospec->args[0], gpiospec->args[1]);
	if (pin > gc->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[2];

	/*
	 * Convert chip-local pin to global GPIO number, like
	 * of_gpio_simple_xlate().
	 */
	return gc->base + pin;
}

static struct gpio_ops microchip_sgpio_gpio_ops = {
	.direction_input = microchip_sgpio_direction_input,
	.direction_output = microchip_sgpio_direction_output,
	.get_direction = microchip_sgpio_get_direction,
	.get = microchip_sgpio_get_value,
	.set = microchip_sgpio_set_value,
	.of_xlate = microchip_sgpio_of_xlate,
};

/* ---- probe-time setup ---- */

static int microchip_sgpio_get_ports(struct sgpio_priv *priv)
{
	const char *prop = "microchip,sgpio-port-ranges";
	struct device *dev = priv->dev;
	u32 range_params[64];
	int i, nranges, ret;

	nranges = of_property_count_elems_of_size(dev->of_node, prop, sizeof(u32));
	if (nranges < 2 || nranges % 2 || nranges > ARRAY_SIZE(range_params)) {
		dev_err(dev, "%s port range: '%s' property\n",
			nranges < 0 ? "Missing" : "Invalid", prop);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(dev->of_node, prop, range_params, nranges);
	if (ret) {
		dev_err(dev, "failed to parse '%s' property: %d\n", prop, ret);
		return ret;
	}
	for (i = 0; i < nranges; i += 2) {
		int start = range_params[i];
		int end = range_params[i + 1];

		if (start > end || end >= SGPIO_BITS_PER_WORD) {
			dev_err(dev, "Ill-formed port-range [%d:%d]\n", start, end);
			continue;
		}
		priv->ports |= GENMASK(end, start);
	}

	return 0;
}

/*
 * Child bank probe - runs once per `microchip,sparx5-sgpio-bank` node
 * spawned by the parent via of_platform_populate.
 */
static int microchip_sgpio_bank_probe(struct device *dev)
{
	struct sgpio_priv *priv;
	struct sgpio_bank *bank;
	struct gpio_chip *gc;
	u32 ngpios, reg;
	int ret;

	if (!dev->parent || !dev->parent->priv) {
		dev_err(dev, "no parent SGPIO controller\n");
		return -ENODEV;
	}
	priv = dev->parent->priv;

	ret = of_property_read_u32(dev->of_node, "reg", &reg);
	if (ret) {
		dev_err(dev, "missing 'reg' on bank node\n");
		return ret;
	}
	bank = reg ? &priv->out : &priv->in;
	bank->priv = priv;
	bank->is_input = (reg == 0);

	if (of_property_read_u32(dev->of_node, "ngpios", &ngpios))
		ngpios = SGPIO_BITS_PER_WORD * priv->bitcount;

	gc = &bank->gpio;
	gc->dev = dev;
	gc->base = -1;
	gc->ngpio = ngpios;
	gc->ops = &microchip_sgpio_gpio_ops;
	gc->of_gpio_n_cells = 3;

	ret = gpiochip_add(gc);
	if (ret)
		dev_err(dev, "failed to register sgpio bank (reg=%u): %d\n",
			reg, ret);
	else
		dev_dbg(dev, "sgpio bank (%s) registered, %d gpios\n",
			bank->is_input ? "in" : "out", ngpios);
	return ret;
}

static int microchip_sgpio_probe(struct device *dev)
{
	struct sgpio_priv *priv;
	struct reset_control *reset;
	struct clk *clk;
	struct resource *iores;
	void __iomem *base;
	int ret, port;
	u32 div_clock, val;

	static const struct regmap_config sgpio_regmap_config = {
		.reg_bits = 32,
		.val_bits = 32,
		.reg_stride = 4,
	};

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;
	priv->properties = device_get_match_data(dev);
	if (!priv->properties)
		return -EINVAL;

	reset = reset_control_get_optional(dev, "switch");
	if (!IS_ERR(reset))
		reset_control_reset(reset);

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(dev, "failed to get clock\n");
		return PTR_ERR(clk);
	}
	div_clock = clk_get_rate(clk);

	if (of_property_read_u32(dev->of_node, "bus-frequency", &priv->clock))
		priv->clock = 12500000;
	if (priv->clock == 0 || priv->clock > (div_clock / 2)) {
		dev_err(dev, "Invalid frequency %d\n", priv->clock);
		return -EINVAL;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	priv->regs = regmap_init_mmio(dev, base, &sgpio_regmap_config);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	ret = microchip_sgpio_get_ports(priv);
	if (ret)
		return ret;

	/* Bit count is fixed for sparx5/lan969x (4 bits per port). */
	priv->bitcount = SGPIO_MAX_BITS;

	sgpio_configure_bitstream(priv);

	val = max(2U, div_clock / priv->clock);
	sgpio_configure_clock(priv, val);

	for (port = 0; port < SGPIO_BITS_PER_WORD; port++)
		sgpio_writel(priv, 0, REG_PORT_CONFIG, port);
	sgpio_writel(priv, priv->ports, REG_PORT_ENABLE, 0);

	dev->priv = priv;

	ret = of_platform_populate(dev->of_node, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to populate bank children: %d\n", ret);
		return ret;
	}

	dev_dbg(dev, "registered: clk %u Hz, %u ports\n", priv->clock,
		hweight32(priv->ports));
	return 0;
}

static const struct of_device_id microchip_sgpio_of_match[] = {
	{ .compatible = "microchip,sparx5-sgpio", .data = &properties_sparx5 },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, microchip_sgpio_of_match);

static struct driver microchip_sgpio_driver = {
	.name = "microchip-sgpio",
	.probe = microchip_sgpio_probe,
	.of_compatible = DRV_OF_COMPAT(microchip_sgpio_of_match),
};
coredevice_platform_driver(microchip_sgpio_driver);

static const struct of_device_id microchip_sgpio_bank_of_match[] = {
	{ .compatible = "microchip,sparx5-sgpio-bank" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, microchip_sgpio_bank_of_match);

static struct driver microchip_sgpio_bank_driver = {
	.name = "microchip-sgpio-bank",
	.probe = microchip_sgpio_bank_probe,
	.of_compatible = DRV_OF_COMPAT(microchip_sgpio_bank_of_match),
};
coredevice_platform_driver(microchip_sgpio_bank_driver);
