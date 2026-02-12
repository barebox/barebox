// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix

/*
 * Multifunction core driver for Hexagon Geosystems EFI MCU that is connected
 * via dedicated UART port. The communication protocol between both parties is
 * called Sensor Protocol (SEP).
 *
 * Based on drivers/mfd/rave-sp.c
 */

#define pr_fmt(fmt) "hgs-efi: " fmt

#include <asm/unaligned.h>
#include <common.h>
#include <init.h>
#include <of_device.h>
#include <mfd/hgs-efi.h>
#include <linux/crc16.h>
#include <linux/ctype.h>
#include <linux/gpio/consumer.h>
#include <linux/string.h>
#include <mfd/hgs-efi.h>
#include <param.h>

#define HGS_EFI_SEP_ASCII_SYNCBYTE		'S'
#define HGS_EFI_SEP_ASCII_MSG_TYPE_CMD		'C'
#define HGS_EFI_SEP_ASCII_MSG_TYPE_EVENT	'E'
#define HGS_EFI_SEP_ASCII_MSG_TYPE_REPLY	'R'
#define HGS_EFI_SEP_ASCII_DELIM			','
#define HGS_EFI_SEP_ASCII_HDR_END		':'

/* Non addressed ascii header format */
struct hgs_efi_sep_ascii_hdr {
	u8 syncbyte;
	u8 msg_type;
	u8 msg_id[5]; /* u16 dec number */
	u8 delim;
	u8 crc16[4];  /* u16 hex number */
	u8 hdr_end;
} __packed;

#define HGS_EFI_SEP_RX_BUFFER_SIZE	64
#define HGS_EFI_SEP_FRAME_PREAMBLE_SZ	2
#define HGS_EFI_SEP_FRAME_POSTAMBLE_SZ	2

enum hgs_efi_sep_deframer_state {
	HGS_EFI_SEP_EXPECT_SOF,
	HGS_EFI_SEP_EXPECT_DATA,
};

/**
 * struct hgs_efi_deframer - Device protocol deframer
 *
 * @state:  Current state of the deframer
 * @data:   Buffer used to collect deframed data
 * @length: Number of bytes de-framed so far
 */
struct hgs_efi_deframer {
	enum hgs_efi_sep_deframer_state state;
	unsigned char data[HGS_EFI_SEP_RX_BUFFER_SIZE];
	size_t length;
};

/**
 * struct hgs_efi_reply - Reply as per SEP
 *
 * @length:	Expected reply length
 * @data:	Buffer to store reply payload in
 * @msg_id:	Expected SEP msg-id
 * @received:   Successful reply reception
 */
struct hgs_efi_reply {
	size_t length;
	void  *data;
	u16    msg_id;
	bool   received;
};

struct hgs_efi_sep_coder {
	int (*encode)(struct hgs_efi *efi, struct hgs_sep_cmd *cmd, u8 *buf);
	int (*process_frame)(struct hgs_efi *efi, void *buf, size_t size);
	unsigned int sep_header_hdrsize;
	char sep_sof_char;
};

struct hgs_efi {
	struct device dev;
	struct serdev_device *serdev;
	struct gpio_desc *cpu_rdy_gpio;
	const struct hgs_efi_sep_coder *coder;
	struct hgs_efi_deframer deframer;
	struct hgs_efi_reply *reply;

	unsigned int cpu_rdy_send;
};

static int hgs_efi_signal_cpu_set(struct param_d *p, void *priv)
{
	struct hgs_efi *efi = priv;

	gpiod_set_value(efi->cpu_rdy_gpio, 1);

	return 0;
}

static void hgs_efi_write(struct hgs_efi *efi, const u8 *data, size_t data_size)
{
	print_hex_dump_bytes("tx: ", DUMP_PREFIX_NONE, data, data_size);

	/* timeout is ignored, instead polling_window is used */
	serdev_device_write(efi->serdev, data, data_size, SECOND);
}

int hgs_efi_exec(struct hgs_efi *efi, struct hgs_sep_cmd *cmd)
{
	struct device *dev = efi->serdev->dev;
	struct hgs_efi_reply reply = {
		.msg_id   = cmd->msg_id,
		.data     = cmd->reply_data,
		.length   = cmd->reply_data_size,
		.received = false,
	};
	unsigned int max_msg_len;
	u8 *msg, *p;
	int ret;

	switch (cmd->type) {
	case HGS_SEP_MSG_TYPE_COMMAND:
	case HGS_SEP_MSG_TYPE_EVENT:
		break;
	case HGS_SEP_MSG_TYPE_REPLY:
		dev_warn(dev, "MCU initiated communication is not supported yet!\n");
		return -EINVAL;
	default:
		dev_warn(dev, "Unknown EFI msg-type %#x\n", cmd->type);
		return -EINVAL;
	}

	max_msg_len = HGS_EFI_SEP_FRAME_PREAMBLE_SZ +
		      HGS_EFI_SEP_FRAME_POSTAMBLE_SZ +
		      efi->coder->sep_header_hdrsize + cmd->payload_size;
	msg = p = xzalloc(max_msg_len);

	/* MCU serial flush preamble */
	*p++ = '\r';
	*p++ = '\n';

	ret = efi->coder->encode(efi, cmd, p);
	if (ret < 0) {
		free(msg);
		return ret;
	}

	p += ret;

	/* SEP postamble */
	*p++ = '\r';
	*p++ = '\n';

	efi->reply = &reply;
	hgs_efi_write(efi, msg, p - msg);

	free(msg);

	if (cmd->type == HGS_SEP_MSG_TYPE_EVENT) {
		efi->reply = NULL;
		return 0;
	}

	/*
	 * is_timeout will implicitly poll serdev via poller
	 * infrastructure
	 */
	ret = wait_on_timeout(SECOND, reply.received);
	if (ret)
		dev_err(dev, "Command timeout\n");

	efi->reply = NULL;

	return ret;
}

#define HGS_SEP_DOUBLE_QUOTE_SUB_VAL	0x1a

char *hgs_efi_extract_str_response(u8 *buf)
{
	unsigned char *start;
	unsigned char *end;
	unsigned char *p;
	size_t i;

	if (!buf || buf[0] != '"') {
		pr_warn("No start \" char found in string response\n");
		return ERR_PTR(-EINVAL);
	}

	start = &buf[1];
	end = strrchr(start, '"');
	if (!end) {
		pr_warn("No end \" char found in string response\n");
		return ERR_PTR(-EINVAL);
	}
	*end = '\0';

	/*
	 * Last step, check for substition val in string reply
	 * and re-substitute it.
	 */
	p = start;
	for (i = 0; i < strlen(start); i++)
		if (*p == HGS_SEP_DOUBLE_QUOTE_SUB_VAL)
			*p = '"';

	return start;
}

static int hgs_sep_ascii_encode(struct hgs_efi *efi, struct hgs_sep_cmd *cmd,
				u8 *buf)
{
	size_t hdr_len;
	char msg_type;

	switch (cmd->type) {
	case HGS_SEP_MSG_TYPE_COMMAND:
		msg_type = 'C';
		break;
	case HGS_SEP_MSG_TYPE_EVENT:
		msg_type = 'E';
		break;
	default:
		/* Should never happen */
		return -EINVAL;
	}

	/*
	 * The ASCII coder doesn't care about the CRC, also the CRC handling
	 * has a few flaws. Therefore skip it for now.
	 */
	hdr_len = sprintf(buf, "S%c%u:", msg_type, cmd->msg_id);
	memcpy(buf + hdr_len, cmd->payload, cmd->payload_size);

	return hdr_len + cmd->payload_size;
}

static int
hgs_sep_process_ascii_frame(struct hgs_efi *efi, void *_buf, size_t size)
{
	struct device *dev = efi->serdev->dev;
	unsigned char *payload;
	unsigned int copy_bytes;
	unsigned int msgid;
	size_t payload_len;
	u8 *buf = _buf;
	size_t hdrlen;
	char *p;
	int ret;

	/*
	 * Non addressing ASCII format:
	 * S[MsgType][MsgID](,[CRC]):[Payload]
	 */
	if (buf[0] != HGS_EFI_SEP_ASCII_SYNCBYTE) {
		dev_warn(dev, "Invalid SOF detected\n");
		return -EINVAL;
	}

	if (buf[1] != HGS_EFI_SEP_ASCII_MSG_TYPE_REPLY) {
		dev_warn(dev, "Invalid MsgType: %c(%#x)\n", buf[1], buf[1]);
		return -EINVAL;
	}

	/* Split header from payload first for the following str-ops on buf */
	payload = strchr(buf, ':');
	if (!payload) {
		dev_warn(dev, "Failed to find header delim\n");
		return -EINVAL;
	}

	hdrlen = payload - buf;
	if (hdrlen > sizeof(struct hgs_efi_sep_ascii_hdr)) {
		dev_warn(dev, "Invalid header len detected\n");
		return -EINVAL;
	}

	*payload = 0;
	payload++;

	/*
	 * Albeit the CRC is optional and the calc has a few flaws the coder may
	 * has added it. Skip the CRC check but do the MsgID check.
	 */
	p = strchr(buf, ',');
	if (p)
		*p = 0;

	ret = kstrtouint(&buf[2], 10, &msgid);
	if (ret) {
		dev_warn(dev, "Failed to parse MsgID, ret:%d\n", ret);
		return -EINVAL;
	}

	if (msgid != efi->reply->msg_id) {
		dev_warn(dev, "Wrong MsgID received, ignore frame (%u != %u)\n",
			 msgid, efi->reply->msg_id);
		return -EINVAL;
	}

	payload_len = size - hdrlen;
	copy_bytes = payload_len;
	if (payload_len > efi->reply->length) {
		dev_warn(dev, "Reply buffer to small, dropping remaining %zu bytes\n",
			 payload_len - efi->reply->length);
		copy_bytes = efi->reply->length;
	}

	memcpy(efi->reply->data, payload, copy_bytes);

	return 0;
}

static const struct hgs_efi_sep_coder hgs_efi_ascii_coder = {
	.encode = hgs_sep_ascii_encode,
	.process_frame = hgs_sep_process_ascii_frame,
	.sep_header_hdrsize = sizeof(struct hgs_efi_sep_ascii_hdr),
	.sep_sof_char = HGS_EFI_SEP_ASCII_SYNCBYTE,
};

static bool hgs_efi_eof_received(struct hgs_efi_deframer *deframer)
{
	const char eof_seq[] = { '\r', '\n' };

	if (deframer->length <= 2)
		return false;

	if (memcmp(&deframer->data[deframer->length - 2], eof_seq, 2))
		return false;

	return true;
}

static void hgs_efi_receive_frame(struct hgs_efi *efi,
				  struct hgs_efi_deframer *deframer)
{
	int ret;

	if (deframer->length < efi->coder->sep_header_hdrsize) {
		dev_warn(efi->serdev->dev, "Bad frame: Too short\n");
		return;
	}

	print_hex_dump_bytes("rx-frame: ", DUMP_PREFIX_NONE,
			     deframer->data, deframer->length);

	ret = efi->coder->process_frame(efi, deframer->data,
			      deframer->length - HGS_EFI_SEP_FRAME_PREAMBLE_SZ);
	if (!ret)
		efi->reply->received = true;
}

static int hgs_efi_receive_buf(struct serdev_device *serdev,
			       const unsigned char *buf, size_t size)
{
	struct device *dev = serdev->dev;
	struct hgs_efi *efi = dev->priv;
	struct hgs_efi_deframer *deframer = &efi->deframer;
	const unsigned char *src = buf;
	const unsigned char *end = buf + size;

	print_hex_dump_bytes("rx-bytes: ", DUMP_PREFIX_NONE, buf, size);

	while (src < end) {
		const unsigned char byte = *src++;

		switch (deframer->state) {
		case HGS_EFI_SEP_EXPECT_SOF:
			if (byte == efi->coder->sep_sof_char)
				deframer->state = HGS_EFI_SEP_EXPECT_DATA;
			deframer->data[deframer->length++] = byte;
			break;
		case HGS_EFI_SEP_EXPECT_DATA:
			if (deframer->length >= sizeof(deframer->data)) {
				dev_warn(dev, "Bad frame: Too long\n");
				goto frame_reset;
			}

			deframer->data[deframer->length++] = byte;
			if (hgs_efi_eof_received(deframer)) {
				hgs_efi_receive_frame(efi, deframer);
				goto frame_reset;
			}
		}
	}

	/*
	 * All bytes processed but no EOF detected yet because the serdev
	 * poller may called us to early. Keep the deframer state to continue
	 * the work where we finished.
	 */
	return size;

frame_reset:
	memset(deframer->data, 0, deframer->length);
	deframer->length = 0;
	deframer->state = HGS_EFI_SEP_EXPECT_SOF;

	return src - buf;
}

static int hgs_efi_register_dev(struct hgs_efi *efi)
{
	struct device *dev = &efi->dev;
	struct param_d *p;
	int ret;

	dev->parent = efi->serdev->dev;
	dev_set_name(dev, "efi");
	dev->id = DEVICE_ID_SINGLE;

	ret = register_device(dev);
	if (ret)
		return ret;

	p = dev_add_param_bool(dev, "cpu_rdy", hgs_efi_signal_cpu_set, NULL,
			       &efi->cpu_rdy_send, efi);
	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto err_unregister_dev;
	}

	return 0;

err_unregister_dev:
	unregister_device(dev);
	return ret;
}

static void hgs_efi_unregister_dev(struct hgs_efi *efi)
{
	unregister_device(&efi->dev);
}

static int hgs_efi_probe(struct device *dev)
{
	struct serdev_device *serdev = to_serdev_device(dev->parent);
	struct hgs_efi *efi;
	u32 baud;
	int ret;

	if (of_property_read_u32(dev->of_node, "current-speed", &baud)) {
		dev_err(dev,
			"'current-speed' is not specified in device node\n");
		return -EINVAL;
	}

	efi = xzalloc(sizeof(*efi));
	efi->serdev = serdev;
	efi->coder = of_device_get_match_data(dev);
	efi->cpu_rdy_gpio = gpiod_get(dev, "cpu-rdy", GPIOD_OUT_LOW);
	if (IS_ERR(efi->cpu_rdy_gpio)) {
		dev_err(dev, "Failed to get cpu-rdy GPIO\n");
		ret = PTR_ERR(efi->cpu_rdy_gpio);
		goto err_free;
	}

	dev->priv = efi;
	serdev->dev = dev;
	serdev->receive_buf = hgs_efi_receive_buf;
	serdev->polling_interval = 200 * MSECOND;
	serdev->polling_window = 10 * MSECOND;

	ret = serdev_device_open(serdev);
	if (ret)
		goto err_gpiod_put;

	serdev_device_set_baudrate(serdev, baud);

	ret = hgs_efi_register_dev(efi);
	if (ret) {
		dev_err(dev, "Failed to register EFI device\n");
		goto err_serdev_close;
	};

	ret = of_platform_populate(dev->of_node, NULL, dev);
	if (ret) {
		dev_err(dev, "OF populate failed\n");
		goto err_hgs_efi_unregister_dev;
	}

	return 0;

err_hgs_efi_unregister_dev:
	hgs_efi_unregister_dev(efi);
err_serdev_close:
	serdev_device_close(serdev);
err_gpiod_put:
	gpiod_put(efi->cpu_rdy_gpio);
err_free:
	free(efi);
	return ret;
}

static const struct of_device_id __maybe_unused hgs_efi_dt_ids[] = {
	{ .compatible = "hgs,efi-gs05", .data = &hgs_efi_ascii_coder },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, lgs_efi_dt_ids);

static struct driver hgs_efi_drv = {
	.name = "hgs-efi",
	.probe = hgs_efi_probe,
	.of_compatible = DRV_OF_COMPAT(hgs_efi_dt_ids),
};
console_platform_driver(hgs_efi_drv);
