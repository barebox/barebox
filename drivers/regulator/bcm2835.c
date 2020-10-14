/*
 * bcm2835 regulator support
 *
 * Copyright (c) 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPLv2 Only
 */
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <regulator.h>

#include <mach/mbox.h>

#define REG_DEV(_id, _name)		\
	{				\
		.id		= _id,	\
		.devname	= _name,\
	}

static struct regulator_bcm2835 {
	int id;
	char *devname;

	struct regulator_dev rdev;
	struct regulator_desc rdesc;
} regs[] = {
	REG_DEV(BCM2835_MBOX_POWER_DEVID_SDHCI, "bcm2835_mci0"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_UART0, "uart0-pl0110"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_UART1, "uart0-pl0111"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_USB_HCD, "bcm2835_usb"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_I2C0, "bcm2835_i2c0"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_I2C1, "bcm2835_i2c1"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_I2C2, "bcm2835_i2c2"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_SPI, "bcm2835_spi"),
	REG_DEV(BCM2835_MBOX_POWER_DEVID_CCP2TX, "bcm2835_ccp2tx"),
};

struct msg_set_power_state {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_set_power_state set_power_state;
	u32 end_tag;
};

static int regulator_bcm2835_set(struct regulator_dev *rdev, int state)
{
	struct regulator_bcm2835 *rb = container_of(rdev, struct regulator_bcm2835, rdev);
	BCM2835_MBOX_STACK_ALIGN(struct msg_set_power_state, msg_pwr);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg_pwr);
	BCM2835_MBOX_INIT_TAG(&msg_pwr->set_power_state,
			      SET_POWER_STATE);
	msg_pwr->set_power_state.body.req.device_id = rb->id;
	msg_pwr->set_power_state.body.req.state =
		state |
		BCM2835_MBOX_SET_POWER_STATE_REQ_WAIT;

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN,
				     &msg_pwr->hdr);
	if (ret) {
		dev_err(rdev->dev, "bcm2835: Could not set module %u power state\n",
			rb->id);
		return ret;
	}

	return 0;
}

static int regulator_bcm2835_enable(struct regulator_dev *rdev)
{
	return regulator_bcm2835_set(rdev, BCM2835_MBOX_SET_POWER_STATE_REQ_ON);
}

static int regulator_bcm2835_disable(struct regulator_dev *rdev)
{
	return regulator_bcm2835_set(rdev, BCM2835_MBOX_SET_POWER_STATE_REQ_OFF);
}

struct msg_get_power_state {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_power_state get_power_state;
	u32 end_tag;
};

static int regulator_bcm2835_is_enabled(struct regulator_dev *rdev)
{
	struct regulator_bcm2835 *rb = container_of(rdev, struct regulator_bcm2835, rdev);
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_power_state, msg_pwr);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg_pwr);
	BCM2835_MBOX_INIT_TAG(&msg_pwr->get_power_state,
			      GET_POWER_STATE);
	msg_pwr->get_power_state.body.req.device_id = rb->id;

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN,
				     &msg_pwr->hdr);
	if (ret) {
		dev_err(rdev->dev, "bcm2835: Could not get module %u power state\n",
			rb->id);
		return ret;
	}

	return msg_pwr->get_power_state.body.resp.state;
}

const static struct regulator_ops bcm2835_ops = {
	.enable = regulator_bcm2835_enable,
	.disable = regulator_bcm2835_disable,
	.is_enabled = regulator_bcm2835_is_enabled,
};

static int regulator_bcm2835_probe(struct device_d *dev)
{
	struct regulator_bcm2835 *rb;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		rb = &regs[i];

		rb->rdesc.ops = &bcm2835_ops;
		rb->rdev.desc = &rb->rdesc;
		rb->rdev.dev = dev;

		ret = dev_regulator_register(&rb->rdev, rb->devname, NULL);
		if (ret)
			return ret;
	}

	return 0;
}

static struct driver_d regulator_bcm2835_driver = {
	.name  = "regulator-bcm2835",
	.probe = regulator_bcm2835_probe,
};
postcore_platform_driver(regulator_bcm2835_driver);

static int regulator_bcm2835_init(void)
{
	add_generic_device("regulator-bcm2835", DEVICE_ID_SINGLE, NULL,
			   0, 0, 0, NULL);

	return 0;
}
postcore_initcall(regulator_bcm2835_init);
