// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Raspberry Pi 3 expander GPIO driver
 *
 *  Uses the firmware mailbox service to communicate with the
 *  GPIO expander on the VPU.
 *
 *  Copyright (C) 2017 Raspberry Pi Trading Ltd.
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <mach/bcm283x/mbox.h>

#define NUM_GPIO 8

#define RPI_EXP_GPIO_BASE	128

#define RPI_EXP_GPIO_DIR_IN	0
#define RPI_EXP_GPIO_DIR_OUT	1

struct rpi_exp_gpio {
	struct gpio_chip gc;
	struct rpi_firmware *fw;
};

/* VC4 firmware mailbox interface data structures */
struct gpio_set_config {
	struct bcm2835_mbox_hdr hdr;		/* buf_size, code */
	struct bcm2835_mbox_tag_hdr tag_hdr;	/* tag, val_buf_size, val_len */
        union {
                struct {
			u32 gpio;
			u32 direction;
			u32 polarity;
			u32 term_en;
			u32 term_pull_up;
			u32 state;
                } req;
	} body;
	u32 end_tag;
};

struct gpio_get_config {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_hdr tag_hdr;
        union {
                struct {
			u32 gpio;
			u32 direction;
			u32 polarity;
			u32 term_en;
			u32 term_pull_up;
                } req;
	} body;
	u32 end_tag;
};

struct gpio_get_set_state {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_hdr tag_hdr;
        union {
                struct {
			u32 gpio;
			u32 state;
                } req;
	} body;
	u32 end_tag;
};

static inline struct rpi_exp_gpio *to_rpi_gpio(struct gpio_chip *gc)
{
	return container_of(gc, struct rpi_exp_gpio, gc);
}

static int rpi_exp_gpio_get_polarity(struct gpio_chip *gc, unsigned int off)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_get_config, get);
	BCM2835_MBOX_INIT_HDR(get);
	BCM2835_MBOX_INIT_TAG(get, GET_GPIO_CONFIG);

	gpio = to_rpi_gpio(gc);

	get->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &get->hdr);
	if (ret || get->body.req.gpio != 0) {
		dev_err(gc->dev, "Failed to get GPIO %u config (%d %x)\n",
			off, ret, get->body.req.gpio);
		return ret ? ret : -EIO;
	}
	return get->body.req.polarity;
}

static int rpi_exp_gpio_dir_in(struct gpio_chip *gc, unsigned int off)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_set_config, set_in);
	BCM2835_MBOX_INIT_HDR(set_in);
	BCM2835_MBOX_INIT_TAG(set_in, SET_GPIO_CONFIG);

	gpio = to_rpi_gpio(gc);

	set_in->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */
	set_in->body.req.direction = RPI_EXP_GPIO_DIR_IN;
	set_in->body.req.term_en = 0;		/* termination disabled */
	set_in->body.req.term_pull_up = 0;	/* n/a as termination disabled */
	set_in->body.req.state = 0;		/* n/a as configured as an input */

	ret = rpi_exp_gpio_get_polarity(gc, off);
	if (ret < 0)
		return ret;

	set_in->body.req.polarity = ret;		/* Retain existing setting */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &set_in->hdr);
	if (ret || set_in->body.req.gpio != 0) {
		dev_err(gc->dev, "Failed to set GPIO %u to input (%d %x)\n",
			off, ret, set_in->body.req.gpio);
		return ret ? ret : -EIO;
	}

	return 0;
}

static int rpi_exp_gpio_dir_out(struct gpio_chip *gc, unsigned int off, int val)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_set_config, set_out);
	BCM2835_MBOX_INIT_HDR(set_out);
	BCM2835_MBOX_INIT_TAG(set_out, SET_GPIO_CONFIG);

	gpio = to_rpi_gpio(gc);

	set_out->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */
	set_out->body.req.direction = RPI_EXP_GPIO_DIR_OUT;
	set_out->body.req.term_en = 0;		/* n/a as an output */
	set_out->body.req.term_pull_up = 0;	/* n/a as termination disabled */
	set_out->body.req.state = val;		/* Output state */

	ret = rpi_exp_gpio_get_polarity(gc, off);
	if (ret < 0)
		return ret;
	set_out->body.req.polarity = ret;		/* Retain existing setting */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &set_out->hdr);
	if (ret || set_out->body.req.gpio != 0) {
		dev_err(gc->dev, "Failed to set GPIO %u to output (%d %x)\n",
			off, ret, set_out->body.req.gpio);
		return ret ? ret : -EIO;
	}
	return 0;
}

static int rpi_exp_gpio_get_direction(struct gpio_chip *gc, unsigned int off)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_get_config, get);
	BCM2835_MBOX_INIT_HDR(get);
	BCM2835_MBOX_INIT_TAG(get, GET_GPIO_CONFIG);

	gpio = to_rpi_gpio(gc);

	get->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &get->hdr);
	if (ret || get->body.req.gpio != 0) {
		dev_err(gc->dev,
			"Failed to get GPIO %u config (%d %x)\n", off, ret,
			get->body.req.gpio);
		return ret ? ret : -EIO;
	}
	if (get->body.req.direction)
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static int rpi_exp_gpio_get(struct gpio_chip *gc, unsigned int off)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_get_set_state, get);
	BCM2835_MBOX_INIT_HDR(get);
	BCM2835_MBOX_INIT_TAG(get, GET_GPIO_STATE);

	gpio = to_rpi_gpio(gc);

	get->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */
	get->body.req.state = 0;		/* storage for returned value */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &get->hdr);
	if (ret || get->body.req.gpio != 0) {
		dev_err(gc->dev,
			"Failed to get GPIO %u state (%d %x)\n", off, ret,
			get->body.req.gpio);
		return ret ? ret : -EIO;
	}
	return !!get->body.req.state;
}

static void rpi_exp_gpio_set(struct gpio_chip *gc, unsigned int off, int val)
{
	struct rpi_exp_gpio *gpio;
	int ret;
	BCM2835_MBOX_STACK_ALIGN(struct gpio_get_set_state, set);
	BCM2835_MBOX_INIT_HDR(set);
	BCM2835_MBOX_INIT_TAG(set, SET_GPIO_STATE);

	gpio = to_rpi_gpio(gc);

	set->body.req.gpio = off + RPI_EXP_GPIO_BASE;	/* GPIO to update */
	set->body.req.state = val;	/* Output state */

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &set->hdr);
	if (ret || set->body.req.gpio != 0)
		dev_err(gc->dev,
			"Failed to set GPIO %u state (%d %x)\n", off, ret,
			set->body.req.gpio);
}

static struct gpio_ops rpi_exp_gpio_ops = {
	.direction_input = rpi_exp_gpio_dir_in,
	.direction_output = rpi_exp_gpio_dir_out,
	.get_direction = rpi_exp_gpio_get_direction,
	.get = rpi_exp_gpio_get,
	.set = rpi_exp_gpio_set,
};

static int rpi_exp_gpio_probe(struct device *dev)
{
	struct rpi_exp_gpio *rpi_gpio;
	int ret;

	rpi_gpio = xzalloc(sizeof(*rpi_gpio));

	rpi_gpio->gc.dev = dev;
	rpi_gpio->gc.base = -1;
	rpi_gpio->gc.ngpio = NUM_GPIO;
	rpi_gpio->gc.ops = &rpi_exp_gpio_ops;

	ret = gpiochip_add(&rpi_gpio->gc);
	if (ret)
		return ret;

	dev_dbg(dev, "probed gpiochip with %d gpios, base %d\n",
			rpi_gpio->gc.ngpio, rpi_gpio->gc.base);

	return 0;
}

static __maybe_unused struct of_device_id rpi_exp_gpio_ids[] = {
	{
		.compatible = "raspberrypi,firmware-gpio",
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, rpi_exp_gpio_ids);

static struct driver rpi_exp_gpio_driver = {
	.name = "rpi-exp-gpio",
	.probe = rpi_exp_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(rpi_exp_gpio_ids),
};

device_platform_driver(rpi_exp_gpio_driver);
