// SPDX-License-Identifier: GPL-2.0
/*
 * Microchip / Atmel ECC (I2C) driver.
 *
 * Copyright (c) 2017, Microchip Technology Inc.
 * Author: Tudor Ambarus
 */

#include <linux/bitrev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <i2c/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "atmel-i2c.h"

static const struct {
	u8 value;
	const char *error_text;
} error_list[] = {
	{ 0x01, "CheckMac or Verify miscompare" },
	{ 0x03, "Parse Error" },
	{ 0x05, "ECC Fault" },
	{ 0x0F, "Execution Error" },
	{ 0xEE, "Watchdog about to expire" },
	{ 0xFF, "CRC or other communication error" },
};
u16 crc16(u16 crc, const u8 *buffer, size_t len);
/**
 * atmel_i2c_checksum() - Generate 16-bit CRC as required by ATMEL ECC.
 * CRC16 verification of the count, opcode, param1, param2 and data bytes.
 * The checksum is saved in little-endian format in the least significant
 * two bytes of the command. CRC polynomial is 0x8005 and the initial register
 * value should be zero.
 *
 * @cmd : structure used for communicating with the device.
 */
static void atmel_i2c_checksum(struct atmel_i2c_cmd *cmd)
{
	u8 *data = &cmd->count;
	size_t len = cmd->count - CRC_SIZE;
	__le16 *__crc16 = (__le16 *)(data + len);

	*__crc16 = cpu_to_le16(bitrev16(crc16(0, data, len)));
}

void atmel_i2c_init_read_config_cmd(struct atmel_i2c_cmd *cmd)
{
	cmd->word_addr = COMMAND;
	cmd->opcode = OPCODE_READ;
	/*
	 * Read the word from Configuration zone that contains the lock bytes
	 * (UserExtra, Selector, LockValue, LockConfig).
	 */
	cmd->param1 = CONFIGURATION_ZONE;
	cmd->param2 = cpu_to_le16(DEVICE_LOCK_ADDR);
	cmd->count = READ_COUNT;

	atmel_i2c_checksum(cmd);

	cmd->msecs = MAX_EXEC_TIME_READ;
	cmd->rxsize = READ_RSP_SIZE;
}
EXPORT_SYMBOL(atmel_i2c_init_read_config_cmd);

int atmel_i2c_init_read_otp_cmd(struct atmel_i2c_cmd *cmd, u16 addr)
{
	if (addr > OTP_ZONE_SIZE)
		return -EINVAL;

	cmd->word_addr = COMMAND;
	cmd->opcode = OPCODE_READ;
	/*
	 * Read the word from OTP zone that may contain e.g. serial
	 * numbers or similar if persistently pre-initialized and locked
	 */
	cmd->param1 = OTP_ZONE;
	cmd->param2 = cpu_to_le16(addr);
	cmd->count = READ_COUNT;

	atmel_i2c_checksum(cmd);

	cmd->msecs = MAX_EXEC_TIME_READ;
	cmd->rxsize = READ_RSP_SIZE;

	return 0;
}
EXPORT_SYMBOL(atmel_i2c_init_read_otp_cmd);

/*
 * After wake and after execution of a command, there will be error, status, or
 * result bytes in the device's output register that can be retrieved by the
 * system. When the length of that group is four bytes, the codes returned are
 * detailed in error_list.
 */
static int atmel_i2c_status(struct device *dev, u8 *status)
{
	size_t err_list_len = ARRAY_SIZE(error_list);
	int i;
	u8 err_id = status[1];

	if (*status != STATUS_SIZE)
		return 0;

	if (err_id == STATUS_WAKE_SUCCESSFUL || err_id == STATUS_NOERR)
		return 0;

	for (i = 0; i < err_list_len; i++)
		if (error_list[i].value == err_id)
			break;

	/* if err_id is not in the error_list then ignore it */
	if (i != err_list_len) {
		dev_err(dev, "%02x: %s:\n", err_id, error_list[i].error_text);
		return err_id;
	}

	return 0;
}

/**
 * i2c_transfer_buffer_flags - issue a single I2C message transferring data
 *                             to/from a buffer
 * @client: Handle to slave device
 * @buf: Where the data is stored
 * @count: How many bytes to transfer, must be less than 64k since msg.len is u16
 * @flags: The flags to be used for the message, e.g. I2C_M_RD for reads
 *
 * Returns negative errno, or else the number of bytes transferred.
 */
static int i2c_transfer_buffer_flags(const struct i2c_client *client, char *buf,
			      int count, u16 flags)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = flags,
		.len = count,
		.buf = buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transferred), return #bytes
	 * transferred, else error code.
	 */
	return (ret == 1) ? count : ret;
}

static int atmel_i2c_wakeup(struct i2c_client *client)
{
	struct atmel_i2c_client_priv *i2c_priv = i2c_get_clientdata(client);
	u8 status[STATUS_RSP_SIZE];
	int ret;

	/*
	 * The device ignores any levels or transitions on the SCL pin when the
	 * device is idle, asleep or during waking up. Don't check for error
	 * when waking up the device.
	 */
	i2c_transfer_buffer_flags(client, i2c_priv->wake_token,
				i2c_priv->wake_token_sz, I2C_M_IGNORE_NAK);

	/*
	 * Wait to wake the device. Typical execution times for ecdh and genkey
	 * are around tens of milliseconds. Delta is chosen to 50 microseconds.
	 */
	udelay(TWHI_MIN);

	ret = i2c_master_recv(client, status, STATUS_SIZE);
	if (ret < 0)
		return ret;

	return atmel_i2c_status(&client->dev, status);
}

static int atmel_i2c_sleep(struct i2c_client *client)
{
	u8 sleep = SLEEP_TOKEN;

	return i2c_master_send(client, &sleep, 1);
}

/*
 * atmel_i2c_send_receive() - send a command to the device and receive its
 *                            response.
 * @client: i2c client device
 * @cmd   : structure used to communicate with the device
 *
 * After the device receives a Wake token, a watchdog counter starts within the
 * device. After the watchdog timer expires, the device enters sleep mode
 * regardless of whether some I/O transmission or command execution is in
 * progress. If a command is attempted when insufficient time remains prior to
 * watchdog timer execution, the device will return the watchdog timeout error
 * code without attempting to execute the command. There is no way to reset the
 * counter other than to put the device into sleep or idle mode and then
 * wake it up again.
 */
int atmel_i2c_send_receive(struct i2c_client *client, struct atmel_i2c_cmd *cmd)
{
	int ret;

	ret = atmel_i2c_wakeup(client);
	if (ret)
		goto err;

	/* send the command */
	ret = i2c_master_send(client, (u8 *)cmd, cmd->count + WORD_ADDR_SIZE);
	if (ret < 0)
		goto err;

	/* delay the appropriate amount of time for command to execute */
	mdelay(cmd->msecs);

	/* receive the response */
	ret = i2c_master_recv(client, cmd->data, cmd->rxsize);
	if (ret < 0)
		goto err;

	/* put the device into low-power mode */
	ret = atmel_i2c_sleep(client);
	if (ret < 0)
		goto err;

	return atmel_i2c_status(&client->dev, cmd->data);
err:
	return ret;
}
EXPORT_SYMBOL(atmel_i2c_send_receive);

static inline size_t atmel_i2c_wake_token_sz(u32 bus_clk_rate)
{
	u32 no_of_bits = DIV_ROUND_UP(TWLO_USEC * bus_clk_rate, USEC_PER_SEC);

	/* return the size of the wake_token in bytes */
	return DIV_ROUND_UP(no_of_bits, 8);
}

static int device_sanity_check(struct i2c_client *client)
{
	struct atmel_i2c_cmd *cmd;
	int ret;

	cmd = kmalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	atmel_i2c_init_read_config_cmd(cmd);

	ret = atmel_i2c_send_receive(client, cmd);
	if (ret)
		goto free_cmd;

	/*
	 * It is vital that the Configuration, Data and OTP zones be locked
	 * prior to release into the field of the system containing the device.
	 * Failure to lock these zones may permit modification of any secret
	 * keys and may lead to other security problems.
	 */
	if (cmd->data[LOCK_CONFIG_IDX] || cmd->data[LOCK_VALUE_IDX]) {
		dev_err(&client->dev, "Configuration or Data and OTP zones are unlocked!\n");
		ret = -ENOTSUPP;
	}

	/* fall through */
free_cmd:
	kfree(cmd);
	return ret;
}

int atmel_i2c_probe(struct i2c_client *client)
{
	struct atmel_i2c_client_priv *i2c_priv;
	struct device *dev = &client->dev;
	int bus_clk_rate = 1000000; /* maximum */

	i2c_priv = devm_kzalloc(dev, sizeof(*i2c_priv), GFP_KERNEL);
	if (!i2c_priv)
		return -ENOMEM;

	i2c_priv->client = client;

	/*
	 * WAKE_TOKEN_MAX_SIZE was calculated for the maximum bus_clk_rate -
	 * 1MHz. The previous bus_clk_rate check ensures us that wake_token_sz
	 * will always be smaller than or equal to WAKE_TOKEN_MAX_SIZE.
	 */
	i2c_priv->wake_token_sz = atmel_i2c_wake_token_sz(bus_clk_rate);

	memset(i2c_priv->wake_token, 0, sizeof(i2c_priv->wake_token));

	i2c_set_clientdata(client, i2c_priv);

	return device_sanity_check(client);
}
EXPORT_SYMBOL(atmel_i2c_probe);

MODULE_AUTHOR("Tudor Ambarus");
MODULE_DESCRIPTION("Microchip / Atmel ECC (I2C) driver");
MODULE_LICENSE("GPL v2");
