// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments' K3 System Controller Driver
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - https://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <driver.h>
#include <linux/remoteproc.h>
#include <linux/printk.h>
#include <errno.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <io.h>
#include <of.h>
#include <soc/ti/k3-sec-proxy.h>
#include <mailbox.h>

#define K3_MSG_R5_TO_M3_M3FW			0x8105
#define K3_MSG_M3_TO_R5_CERT_RESULT		0x8805
#define K3_MSG_M3_TO_R5_BOOT_NOTIFICATION	0x000A

#define K3_FLAGS_MSG_CERT_AUTH_PASS		0x555555
#define K3_FLAGS_MSG_CERT_AUTH_FAIL		0xffffff

/**
 * struct k3_sysctrler_msg_hdr - Generic Header for Messages and responses.
 * @cmd_id:	Message ID. One of K3_MSG_*
 * @host_id:	Host ID of the message
 * @seq_ne:	Message identifier indicating a transfer sequence.
 * @flags:	Flags for the message.
 */
struct k3_sysctrler_msg_hdr {
	u16 cmd_id;
	u8 host_id;
	u8 seq_nr;
	u32 flags;
} __packed;

/**
 * struct k3_sysctrler_load_msg - Message format for Firmware loading
 * @hdr:		Generic message hdr
 * @buffer_address:	Address at which firmware is located.
 * @buffer_size:	Size of the firmware.
 */
struct k3_sysctrler_load_msg {
	struct k3_sysctrler_msg_hdr hdr;
	u32 buffer_address;
	u32 buffer_size;
} __packed;

/**
 * struct k3_sysctrler_boot_notification_msg - Message format for boot
 *					       notification
 * @checksum:		Checksum for the entire message
 * @reserved:		Reserved for future use.
 * @hdr:		Generic message hdr
 */
struct k3_sysctrler_boot_notification_msg {
	u16 checksum;
	u16 reserved;
	struct k3_sysctrler_msg_hdr hdr;
} __packed;

/**
 * struct k3_sysctrler_desc - Description of SoC integration.
 * @host_id:	Host identifier representing the compute entity
 * @max_rx_timeout_ms:	Timeout for communication with SoC (in Milliseconds)
 * @max_msg_size: Maximum size of data per message that can be handled.
 */
struct k3_sysctrler_desc {
	u8 host_id;
	int max_rx_timeout_us;
	int max_msg_size;
};

/**
 * struct k3_sysctrler_privdata - Structure representing System Controller data.
 * @chan_tx:	Transmit mailbox channel
 * @chan_rx:	Receive mailbox channel
 * @chan_boot_notify:	Boot notification channel
 * @desc:	SoC description for this instance
 * @seq_nr:	Counter for number of messages sent.
 * @has_boot_notify:	Has separate boot notification channel
 */
struct k3_sysctrler_privdata {
	struct device *dev;
	struct mbox_chan *chan_tx;
	struct mbox_chan *chan_rx;
	struct mbox_chan *chan_boot_notify;
	const struct k3_sysctrler_desc *desc;
	u32 seq_nr;
	bool has_boot_notify;
};

static int k3_sysctrler_boot_notification_response(struct k3_sysctrler_privdata *priv,
						   void *buf)
{
	struct k3_sysctrler_boot_notification_msg *boot = buf;

	/* ToDo: Verify checksum */

	/* Check for proper response ID */
	if (boot->hdr.cmd_id != K3_MSG_M3_TO_R5_BOOT_NOTIFICATION) {
		dev_err(priv->dev, "%s: Command expected 0x%x, but received 0x%x\n",
			__func__, K3_MSG_M3_TO_R5_BOOT_NOTIFICATION,
			boot->hdr.cmd_id);
		return -EINVAL;
	}

	debug("%s: Boot notification received\n", __func__);

	return 0;
}

/**
 * k3_sysctrler_start() - Start the remote processor
 *		Note that while technically the K3 system controller starts up
 *		automatically after its firmware got loaded we still want to
 *		utilize the rproc start operation for other startup-related
 *		tasks.
 * @dev:	device to operate upon
 *
 * Return: 0 if all went ok, else return appropriate error
 */
static int k3_sysctrler_start(struct k3_sysctrler_privdata *priv)
{
	struct k3_sec_proxy_msg msg;
	int ret;

	/* Receive the boot notification. Note that it is sent only once. */
	ret = mbox_recv(priv->has_boot_notify ? priv->chan_boot_notify :
			priv->chan_rx, &msg, priv->desc->max_rx_timeout_us);
	if (ret) {
		dev_err(priv->dev, "%s: Boot Notification response failed. ret = %d\n",
			__func__, ret);
		return ret;
	}

	/* Process the response */
	ret = k3_sysctrler_boot_notification_response(priv, msg.buf);
	if (ret)
		return ret;

	dev_info(priv->dev, "%s: Boot notification received successfully\n",
	      __func__);

	return 0;
}

/**
 * k3_sysctrler_probe() - Basic probe
 * @dev:	corresponding k3 remote processor device
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_sysctrler_probe(struct device *dev)
{
	struct k3_sysctrler_privdata *priv;
	int ret;

	debug("%s(dev=%p)\n", __func__, dev);

	priv = xzalloc(sizeof(*priv));

	priv->dev = dev;

	priv->chan_tx = mbox_request_channel_byname(dev, "tx");
	if (IS_ERR(priv->chan_tx))
		return dev_err_probe(dev, PTR_ERR(priv->chan_tx), "No tx mbox\n");

	priv->chan_rx = mbox_request_channel_byname(dev, "rx");
	if (IS_ERR(priv->chan_rx))
		return dev_err_probe(dev, PTR_ERR(priv->chan_tx), "No rx mbox\n");

	/* Some SoCs may have a optional channel for boot notification. */
	priv->has_boot_notify = 1;
	priv->chan_boot_notify = mbox_request_channel_byname(dev, "boot_notify");
	if (IS_ERR(priv->chan_boot_notify)) {
		dev_dbg(dev, "%s: Acquiring optional Boot_notify failed. ret = %d. Using Rx\n",
			__func__, ret);
		priv->has_boot_notify = 0;
	}

	priv->desc = device_get_match_data(dev);
	priv->seq_nr = 0;

	k3_sysctrler_start(priv);

	return 0;
}

static const struct k3_sysctrler_desc k3_sysctrler_am654_desc = {
	.host_id = 4,				/* HOST_ID_R5_1 */
	.max_rx_timeout_us = 800000,
	.max_msg_size = 60,
};

static const struct of_device_id k3_sysctrler_ids[] = {
	{
		.compatible = "ti,am654-system-controller",
		.data = &k3_sysctrler_am654_desc,
	}, {
		/* sentinel */
	}
};

static struct driver ti_k3_arm64_rproc_driver = {
	.name = "ti-k3-systemcontroller",
	.probe = k3_sysctrler_probe,
	.of_compatible = k3_sysctrler_ids,
};
device_platform_driver(ti_k3_arm64_rproc_driver);
