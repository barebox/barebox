// SPDX-License-Identifier: GPL-2.0-only
/*
 * EFI I2C master driver
 *
 * Copyright (C) 2024 Elrest Solutions Company s.r.o.
 * Author: Tomas Marek <tomas.marek@elrest.cz>
 */

#include <common.h>
#include <i2c/i2c.h>
#include <driver.h>
#include <efi.h>
#include <efi/efi-device.h>
#include <efi/efi-util.h>
#include <linux/kernel.h>

/* Define EFI I2C transfer control flags */
#define EFI_I2C_FLAG_READ           0x00000001

#define EFI_I2C_ADDRESSING_10_BIT   0x80000000

/* The set_bus_frequency() EFI call doesn't work (doesn't alter SPI clock
 * frequency) if it's parameter is defined on the stack (observed with
 * American Megatrends EFI Revision 5.19) - let's define it globaly.
 */
static unsigned int bus_clock;

struct efi_i2c_capabilities {
	u32 StructureSizeInBytes;
	u32 MaximumReceiveBytes;
	u32 MaximumTransmitBytes;
	u32 MaximumTotalBytes;
};

struct efi_i2c_operation {
	u32 Flags;
	u32 LengthInBytes;
	u8  *Buffer;
};

struct efi_i2c_request_packet {
	unsigned int OperationCount;
	struct efi_i2c_operation Operation[];
};

struct efi_i2c_master_protocol {
	efi_status_t(EFIAPI * set_bus_frequency)(
		struct efi_i2c_master_protocol *this,
		unsigned int *bus_clock
	);
	efi_status_t(EFIAPI * reset)(
		struct efi_i2c_master_protocol *this
	);
	efi_status_t(EFIAPI * start_request)(
		struct efi_i2c_master_protocol *this,
		unsigned int slave_address,
		struct efi_i2c_request_packet *request_packet,
		void *event,
		efi_status_t *status
	);
	struct efi_i2c_capabilities *capabilities;
};

struct efi_i2c_priv {
	struct efi_i2c_master_protocol *efi_protocol;
	struct i2c_adapter adapter;
};

static inline struct efi_i2c_priv *
adapter_to_efi_i2c_priv(struct i2c_adapter *a)
{
	return container_of(a, struct efi_i2c_priv, adapter);
}

static efi_status_t efi_i2c_request(
	struct efi_i2c_request_packet *request,
	const struct efi_i2c_priv *i2c_priv,
	const unsigned int slave_address)
{
	const struct device *dev = &i2c_priv->adapter.dev;
	efi_status_t efiret;

	efiret = i2c_priv->efi_protocol->start_request(
		i2c_priv->efi_protocol,
		slave_address,
		request,
		NULL,
		NULL
	);

	if (EFI_ERROR(efiret))
		dev_err(dev, "I2C operation failed - %s (%lx)\n",
			efi_strerror(efiret), -efiret);

	return efiret;
}

static u32 efi_i2c_max_len(const struct efi_i2c_priv *i2c_priv,
			   const struct i2c_msg *msg)
{
	const struct efi_i2c_capabilities *capabilities =
		i2c_priv->efi_protocol->capabilities;

	if (msg->flags & I2C_M_RD)
		return capabilities->MaximumReceiveBytes;
	else
		return capabilities->MaximumTransmitBytes;
}

static unsigned int efi_i2c_msg_op_cnt(const struct efi_i2c_priv *i2c_priv,
				       const struct i2c_msg *msg)
{
	unsigned int max_len;

	max_len = efi_i2c_max_len(i2c_priv, msg);

	return (msg->len + max_len - 1) / max_len;
}

static unsigned int efi_i2c_req_op_cnt(
	const struct efi_i2c_priv *i2c_priv,
	const struct i2c_msg *msg,
	const int nmsgs)
{
	unsigned int op_cnt = 0;
	int i;

	for (i = nmsgs; i > 0; i--, msg++)
		op_cnt += efi_i2c_msg_op_cnt(i2c_priv, msg);

	return op_cnt;
}

static void i2c_msg_to_efi_op(
	const struct efi_i2c_priv *i2c_priv,
	const struct i2c_msg *msg,
	struct efi_i2c_operation **op)
{
	unsigned int max_len = efi_i2c_max_len(i2c_priv, msg);
	unsigned int remaining = msg->len;
	u32 flags;

	flags = (msg->flags & I2C_M_RD) ? EFI_I2C_FLAG_READ : 0;

	do {
		unsigned int len = min(remaining, max_len);

		(*op)->Flags = flags;
		(*op)->LengthInBytes = len;
		(*op)->Buffer = msg->buf + (msg->len - remaining);
		(*op)++;

		remaining -= len;
	} while (remaining > 0);
}

static int i2c_msgs_to_efi_transaction(struct i2c_adapter *adapter,
				       struct efi_i2c_operation *op,
				       const struct i2c_msg *msg,
				       const int nmsgs)
{
	struct efi_i2c_priv *i2c_priv = adapter_to_efi_i2c_priv(adapter);
	const struct i2c_msg *msg_tmp;
	int ret = 0;
	int i;

	msg_tmp = msg;
	for (i = nmsgs; i > 0; i--, msg_tmp++) {
		if (msg_tmp->flags & I2C_M_DATA_ONLY) {
			ret = -ENOTSUPP;
			break;
		}

		if (i > 0 && msg_tmp->addr != msg->addr) {
			dev_err(&adapter->dev, "Different I2C addresses in one request not supported!\n");
			ret = -ENOTSUPP;
			break;
		}

		i2c_msg_to_efi_op(i2c_priv, msg_tmp, &op);
	}

	return ret;
}

static int efi_i2c_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msg, int nmsgs)
{
	struct efi_i2c_priv *i2c_priv = adapter_to_efi_i2c_priv(adapter);
	struct efi_i2c_request_packet *request_packet;
	unsigned int slave_address;
	efi_status_t efiret;
	unsigned int op_cnt;
	int ret = nmsgs;
	int len;

	op_cnt = efi_i2c_req_op_cnt(i2c_priv, msg, nmsgs);

	len = sizeof(*request_packet) + op_cnt * sizeof(struct efi_i2c_operation);
	request_packet = malloc(len);
	if (!request_packet)
		return -ENOMEM;

	request_packet->OperationCount = op_cnt;
	ret = i2c_msgs_to_efi_transaction(adapter, request_packet->Operation,
					  msg, nmsgs);
	if (ret)
		goto out_free;

	slave_address = msg->addr;
	if (msg->flags & I2C_M_TEN)
		slave_address |= EFI_I2C_ADDRESSING_10_BIT;

	efiret = efi_i2c_request(request_packet, i2c_priv, slave_address);
	if (EFI_ERROR(efiret)) {
		ret = -efi_errno(efiret);
		goto out_free;
	}

	ret = nmsgs;

out_free:
	free(request_packet);

	return ret;
}

static int efi_i2c_probe(struct efi_device *efidev)
{
	struct i2c_platform_data *pdata;
	struct efi_i2c_priv *efi_i2c;
	struct i2c_timings timings;
	efi_status_t efiret;
	int ret;

	efi_i2c = xzalloc(sizeof(*efi_i2c));

	efi_i2c->efi_protocol = efidev->protocol;

	efi_i2c->adapter.master_xfer = efi_i2c_xfer;
	efi_i2c->adapter.nr = efidev->dev.id;
	efi_i2c->adapter.dev.parent = &efidev->dev;
	efi_i2c->adapter.dev.of_node = efidev->dev.of_node;

	i2c_parse_fw_timings(&efidev->dev, &timings, true);

	pdata = efidev->dev.platform_data;
	if (pdata && pdata->bitrate)
		timings.bus_freq_hz = pdata->bitrate;

	efiret = efi_i2c->efi_protocol->reset(efi_i2c->efi_protocol);
	if (EFI_ERROR(efiret)) {
		dev_err(&efidev->dev, "controller reset failed - %ld\n",
			efiret);
		ret = -efi_errno(efiret);
		goto out_free;
	}

	bus_clock = timings.bus_freq_hz;
	efiret = efi_i2c->efi_protocol->set_bus_frequency(
			efi_i2c->efi_protocol,
			&bus_clock);
	if (EFI_ERROR(efiret)) {
		dev_err(&efidev->dev, "I2C clock frequency %u update failed - %s (%lx)\n",
			timings.bus_freq_hz, efi_strerror(efiret), -efiret);
		ret = -efi_errno(efiret);
		goto out_free;
	}

	dev_dbg(&efidev->dev, "I2C clock frequency %u\n", bus_clock);

	ret = i2c_add_numbered_adapter(&efi_i2c->adapter);

out_free:
	if (ret < 0)
		kfree(efi_i2c);

	return ret;
}

static struct efi_driver efi_i2c_driver = {
	.driver = {
		.name  = "efi-i2c",
	},
	.probe = efi_i2c_probe,
	.guid = EFI_I2C_MASTER_PROTOCOL_GUID
};
device_efi_driver(efi_i2c_driver);

MODULE_AUTHOR("Tomas Marek <tomas.marek@elrest.cz>");
MODULE_DESCRIPTION("EFI I2C master driver");
MODULE_LICENSE("GPL");
