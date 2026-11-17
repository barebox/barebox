// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/usb/misc/onboard_usb_dev.c?id=a174da76d3625a5e32f816ee27924ad5d6c1f19f
/*
 * Driver for onboard USB devices
 *
 * Copyright (c) 2022, Google LLC
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <init.h>
#include <i2c/i2c.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <of.h>
#include <driver.h>
#include <regulator.h>
#include <linux/slab.h>
#include <linux/usb/usb.h>

#include "onboard_usb_dev.h"

/* USB5744 register offset and mask */
#define USB5744_CMD_ATTACH			0xAA
#define USB5744_CMD_ATTACH_LSB			0x56
#define USB5744_CMD_CREG_ACCESS			0x99
#define USB5744_CMD_CREG_ACCESS_LSB		0x37
#define USB5744_CREG_MEM_ADDR			0x00
#define USB5744_CREG_MEM_RD_ADDR		0x04
#define USB5744_CREG_WRITE			0x00
#define USB5744_CREG_READ			0x01
#define USB5744_CREG_RUNTIMEFLAGS2		0x411D
#define USB5744_CREG_BYPASS_UDC_SUSPEND		BIT(3)

/************************** Platform driver **************************/

struct onboard_dev {
	struct regulator_bulk_data supplies[MAX_SUPPLIES];
	struct device *dev;
	const struct onboard_dev_pdata *pdata;
	struct gpio_desc *reset_gpio;
	struct clk *clk;
};

static int onboard_dev_get_regulators(struct onboard_dev *onboard_dev)
{
	const char * const *supply_names = onboard_dev->pdata->supply_names;
	unsigned int num_supplies = onboard_dev->pdata->num_supplies;
	struct device *dev = onboard_dev->dev;
	unsigned int i;
	int err;

	if (num_supplies > MAX_SUPPLIES)
		return dev_err_probe(dev, -EINVAL, "max %d supplies supported!\n",
				     MAX_SUPPLIES);

	for (i = 0; i < num_supplies; i++)
		onboard_dev->supplies[i].supply = supply_names[i];

	err = regulator_bulk_get(dev, num_supplies, onboard_dev->supplies);
	if (err)
		dev_err(dev, "Failed to get regulator supplies: %pe\n",
			ERR_PTR(err));

	return err;
}

static int onboard_dev_power_on(struct onboard_dev *onboard_dev)
{
	int err;

	err = clk_prepare_enable(onboard_dev->clk);
	if (err) {
		dev_err(onboard_dev->dev, "failed to enable clock: %pe\n",
			ERR_PTR(err));
		return err;
	}

	err = regulator_bulk_enable(onboard_dev->pdata->num_supplies,
				    onboard_dev->supplies);
	if (err) {
		dev_err(onboard_dev->dev, "failed to enable supplies: %pe\n",
			ERR_PTR(err));
		goto disable_clk;
	}

	udelay(onboard_dev->pdata->reset_us);
	gpiod_set_value_cansleep(onboard_dev->reset_gpio, 0);
	udelay(onboard_dev->pdata->power_on_delay_us);

	return 0;

disable_clk:
	clk_disable_unprepare(onboard_dev->clk);
	return err;
}

static int onboard_dev_power_off(struct onboard_dev *onboard_dev)
{
	int err;

	gpiod_set_value_cansleep(onboard_dev->reset_gpio, 1);

	err = regulator_bulk_disable(onboard_dev->pdata->num_supplies,
				     onboard_dev->supplies);
	if (err) {
		dev_err(onboard_dev->dev, "failed to disable supplies: %pe\n",
			ERR_PTR(err));
		return err;
	}

	clk_disable_unprepare(onboard_dev->clk);

	return 0;
}

#if IS_ENABLED(CONFIG_USB_ONBOARD_DEV_USB5744)
static int onboard_dev_5744_i2c_read_byte(struct i2c_client *client, u16 addr, u8 *data)
{
	struct i2c_msg msg[2];
	u8 rd_buf[3];
	int ret;

	u8 wr_buf[7] = {0, USB5744_CREG_MEM_ADDR, 4,
			USB5744_CREG_READ, 1,
			addr >> 8 & 0xff,
			addr & 0xff};
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = sizeof(wr_buf);
	msg[0].buf = wr_buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		return ret;

	wr_buf[0] = USB5744_CMD_CREG_ACCESS;
	wr_buf[1] = USB5744_CMD_CREG_ACCESS_LSB;
	wr_buf[2] = 0;
	msg[0].len = 3;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		return ret;

	wr_buf[0] = 0;
	wr_buf[1] = USB5744_CREG_MEM_RD_ADDR;
	msg[0].len = 2;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = rd_buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0)
		return ret;
	*data = rd_buf[1];

	return 0;
}

static int onboard_dev_5744_i2c_write_byte(struct i2c_client *client, u16 addr, u8 data)
{
	struct i2c_msg msg[2];
	int ret;

	u8 wr_buf[8] = {0, USB5744_CREG_MEM_ADDR, 5,
			USB5744_CREG_WRITE, 1,
			addr >> 8 & 0xff,
			addr & 0xff,
			data};
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = sizeof(wr_buf);
	msg[0].buf = wr_buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		return ret;

	msg[0].len = 3;
	wr_buf[0] = USB5744_CMD_CREG_ACCESS;
	wr_buf[1] = USB5744_CMD_CREG_ACCESS_LSB;
	wr_buf[2] = 0;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int onboard_dev_5744_i2c_init(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int ret;
	u8 reg;

	/*
	 * Set BYPASS_UDC_SUSPEND bit to ensure MCU is always enabled
	 * and ready to respond to SMBus runtime commands.
	 * The command writes 5 bytes to memory and single data byte in
	 * configuration register.
	 */
	ret = onboard_dev_5744_i2c_read_byte(client,
					     USB5744_CREG_RUNTIMEFLAGS2, &reg);
	if (ret)
		return dev_err_probe(dev, ret, "CREG_RUNTIMEFLAGS2 read failed\n");

	reg |= USB5744_CREG_BYPASS_UDC_SUSPEND;
	ret = onboard_dev_5744_i2c_write_byte(client,
					      USB5744_CREG_RUNTIMEFLAGS2, reg);
	if (ret)
		return dev_err_probe(dev, ret, "BYPASS_UDC_SUSPEND bit configuration failed\n");

	/* Send SMBus command to boot hub. */
	ret = i2c_smbus_write_word_data(client, USB5744_CMD_ATTACH,
					USB5744_CMD_ATTACH_LSB);
	if (ret < 0)
		return dev_err_probe(dev, ret, "USB Attach with SMBus command failed\n");

	return ret;
}
#else
static int onboard_dev_5744_i2c_init(struct i2c_client *client)
{
	return -ENODEV;
}
#endif

static int onboard_dev_probe(struct device *dev)
{
	struct onboard_dev *onboard_dev;
	struct device_node *i2c_node;
	struct device_node *peer_node;
	struct device *peer_dev;
	int err;

	/* the USB 2 and USB 3 halves of one hub share reset and supplies */
	peer_node = of_parse_phandle(dev->of_node, "peer-hub", 0);
	if (peer_node) {
		peer_dev = of_find_device_by_node(peer_node);
		if (peer_dev && dev_get_drvdata(peer_dev))
			return 0;
	}

	onboard_dev = devm_kzalloc(dev, sizeof(*onboard_dev), GFP_KERNEL);
	if (!onboard_dev)
		return -ENOMEM;

	onboard_dev->pdata = device_get_match_data(dev);
	if (!onboard_dev->pdata)
		return -EINVAL;

	onboard_dev->dev = dev;

	err = onboard_dev_get_regulators(onboard_dev);
	if (err)
		return err;

	onboard_dev->clk = clk_get_optional(dev, NULL);
	if (IS_ERR(onboard_dev->clk))
		return dev_err_probe(dev, PTR_ERR(onboard_dev->clk),
				     "failed to get clock\n");

	onboard_dev->reset_gpio = gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(onboard_dev->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(onboard_dev->reset_gpio),
				     "failed to get reset GPIO\n");

	dev_set_drvdata(dev, onboard_dev);

	err = onboard_dev_power_on(onboard_dev);
	if (err)
		return err;

	i2c_node = of_parse_phandle(dev->of_node, "i2c-bus", 0);
	if (i2c_node) {
		struct i2c_client *client = NULL;

#if IS_ENABLED(CONFIG_USB_ONBOARD_DEV_USB5744)
		client = of_find_i2c_device_by_node(i2c_node);
#endif
		of_node_put(i2c_node);

		if (!client) {
			err = -EPROBE_DEFER;
			goto err_power_off;
		}

		if (of_device_is_compatible(dev->of_node, "usb424,2744") ||
		    of_device_is_compatible(dev->of_node, "usb424,5744")) {
			err = onboard_dev_5744_i2c_init(client);
		}

		put_device(&client->dev);
		if (err < 0)
			goto err_power_off;
	}

	return 0;

err_power_off:
	onboard_dev_power_off(onboard_dev);
	return err;
}

MODULE_DEVICE_TABLE(of, onboard_dev_match);

static struct driver onboard_dev_driver = {
	.probe = onboard_dev_probe,
	.name = "onboard-usb-dev",
	.of_match_table = onboard_dev_match,
};
device_platform_driver(onboard_dev_driver);

void of_usb_host_probe_devs(struct usb_host *host)
{
	struct device_node *np;

	np = dev_of_node(host->hw_dev);
	if (!np)
		return;

	of_platform_populate(np, onboard_dev_match, host->hw_dev);
}

MODULE_AUTHOR("Matthias Kaehlcke <mka@chromium.org>");
MODULE_DESCRIPTION("Driver for discrete onboard USB devices");
MODULE_LICENSE("GPL v2");
