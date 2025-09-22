// SPDX-License-Identifier: GPL-2.0
/*
 * Microchip / Atmel SHA204A (I2C) driver.
 *
 * Copyright (c) 2019 Linaro, Ltd. <ard.biesheuvel@linaro.org>
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <i2c/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "atmel-i2c.h"

static int atmel_sha204a_otp_read_full(struct atmel_i2c_client_priv *i2c_priv, void *buf)
{
	u16 addr;
	struct atmel_i2c_cmd cmd;
	struct i2c_client *client = i2c_priv->client;
	int ret;

	for (addr = 0; addr < OTP_ZONE_SIZE / 4; addr++) {
		ret = atmel_i2c_init_read_otp_cmd(&cmd, addr);
		if (ret) {
			dev_err(&client->dev, "failed, invalid otp address %04X\n",
				addr);
			return ret;
		}

		ret = atmel_i2c_send_receive(client, &cmd);

		if (cmd.data[0] == 0xff) {
			dev_err(&client->dev, "failed, device not ready\n");
			return -EINVAL;
		}

		memcpy(buf, cmd.data + 1, 4);
		buf += 4;
	}

	return 0;
}

static int atmel_sha204a_otp_read(void *ctx, unsigned off, void *val, size_t count)
{
	struct atmel_i2c_client_priv *i2c_priv = ctx;
	int ret;

	if (!i2c_priv->data) {
		i2c_priv->data = xzalloc(OTP_ZONE_SIZE);
		ret = atmel_sha204a_otp_read_full(i2c_priv, i2c_priv->data);
		if (ret) {
			free(i2c_priv->data);
			i2c_priv->data = NULL;
			return ret;
		}
	}

	memcpy(val, i2c_priv->data + off, count);

	return 0;
}

static int atmel_sha204a_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct atmel_i2c_client_priv *i2c_priv;
	int ret;

	ret = atmel_i2c_probe(client);
	if (ret)
		return ret;

	i2c_priv = i2c_get_clientdata(client);

	i2c_priv->nvmem_config.name = dev_name(&client->dev);
	i2c_priv->nvmem_config.dev = &client->dev;
	i2c_priv->nvmem_config.priv = i2c_priv;
	i2c_priv->nvmem_config.read_only = true;
	i2c_priv->nvmem_config.reg_read = atmel_sha204a_otp_read;

	i2c_priv->nvmem_config.stride = 4;
	i2c_priv->nvmem_config.word_size = 1;
	i2c_priv->nvmem_config.size = OTP_ZONE_SIZE;

	i2c_priv->nvmem = nvmem_register(&i2c_priv->nvmem_config);
	if (IS_ERR(i2c_priv->nvmem)) {
		ret = PTR_ERR(i2c_priv->nvmem);
		goto fail;
	}

fail:
	return ret;
}

static const struct of_device_id atmel_sha204a_dt_ids[] __maybe_unused = {
	{ .compatible = "atmel,atsha204", },
	{ .compatible = "atmel,atsha204a", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, atmel_sha204a_dt_ids);

static struct driver sha204a_driver = {
	.name  = "atmel-sha204a",
	.probe = atmel_sha204a_probe,
	.of_compatible = DRV_OF_COMPAT(atmel_sha204a_dt_ids),
};
device_i2c_driver(sha204a_driver);

MODULE_AUTHOR("Ard Biesheuvel <ard.biesheuvel@linaro.org>");
MODULE_DESCRIPTION("Microchip / Atmel SHA204A (I2C) driver");
MODULE_LICENSE("GPL v2");
