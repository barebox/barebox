/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "barebox-ratp: " fmt

#include <common.h>
#include <command.h>
#include <kfifo.h>
#include <malloc.h>
#include <init.h>
#include <ratp.h>
#include <command.h>
#include <byteorder.h>
#include <environment.h>
#include <kfifo.h>
#include <poller.h>
#include <linux/sizes.h>
#include <ratp_bb.h>
#include <fs.h>

#define BB_RATP_TYPE_COMMAND		1
#define BB_RATP_TYPE_COMMAND_RETURN	2
#define BB_RATP_TYPE_CONSOLEMSG		3
#define BB_RATP_TYPE_PING		4
#define BB_RATP_TYPE_PONG		5
#define BB_RATP_TYPE_GETENV		6
#define BB_RATP_TYPE_GETENV_RETURN	7
#define BB_RATP_TYPE_FS			8
#define BB_RATP_TYPE_FS_RETURN		9

struct ratp_bb {
	uint16_t type;
	uint16_t flags;
	uint8_t data[];
};

struct ratp_bb_command_return {
	uint32_t errno;
};

struct ratp_ctx {
	struct console_device *cdev;
	struct ratp ratp;
	int ratp_status;
	struct console_device ratp_console;
	int have_synch;
	int in_ratp_console;

	u8 sendbuf[256];
	u8 sendbuf_len;

	int old_active;

	struct kfifo *console_recv_fifo;
	struct kfifo *console_transmit_fifo;

	struct ratp_bb_pkt *fs_rx;

	struct poller_struct poller;
};

static int console_recv(struct ratp *r, uint8_t *data)
{
	struct ratp_ctx *ctx = container_of(r, struct ratp_ctx, ratp);
	struct console_device *cdev = ctx->cdev;

	if (ctx->have_synch) {
		ctx->have_synch = 0;
		*data = 0x01;
		return 0;
	}

	if (!cdev->tstc(cdev))
		return -EAGAIN;

	*data = cdev->getc(cdev);

	return 0;
}

static int console_send(struct ratp *r, void *pkt, int len)
{
	struct ratp_ctx *ctx = container_of(r, struct ratp_ctx, ratp);
	struct console_device *cdev = ctx->cdev;
	const uint8_t *buf = pkt;
	int i;

	for (i = 0; i < len; i++)
		cdev->putc(cdev, buf[i]);

	return 0;
}

static void *xmemdup_add_zero(const void *buf, int len)
{
	void *ret;

	ret = xzalloc(len + 1);
	*(uint8_t *)(ret + len) = 0;
	memcpy(ret, buf, len);

	return ret;
}

static void ratp_queue_console_tx(struct ratp_ctx *ctx)
{
	u8 buf[255];
	struct ratp_bb *rbb = (void *)buf;
	unsigned int now, maxlen = 255 - sizeof(*rbb);
	int ret;

	rbb->type = cpu_to_be16(BB_RATP_TYPE_CONSOLEMSG);

	while (1) {
		now = min(maxlen, kfifo_len(ctx->console_transmit_fifo));
		if (!now)
			break;

		kfifo_get(ctx->console_transmit_fifo, rbb->data, now);

		ret = ratp_send(&ctx->ratp, rbb, now + sizeof(*rbb));
		if (ret)
			return;
	}
}

static int ratp_bb_send_command_return(struct ratp_ctx *ctx, uint32_t errno)
{
	void *buf;
	struct ratp_bb *rbb;
	struct ratp_bb_command_return *rbb_ret;
	int len = sizeof(*rbb) + sizeof(*rbb_ret);
	int ret;

	ratp_queue_console_tx(ctx);

	buf = xzalloc(len);
	rbb = buf;
	rbb_ret = buf + sizeof(*rbb);

	rbb->type = cpu_to_be16(BB_RATP_TYPE_COMMAND_RETURN);
	rbb_ret->errno = cpu_to_be32(errno);

	ret = ratp_send(&ctx->ratp, buf, len);

	free(buf);

	return ret;
}

static int ratp_bb_send_pong(struct ratp_ctx *ctx)
{
	void *buf;
	struct ratp_bb *rbb;
	int len = sizeof(*rbb);
	int ret;

	buf = xzalloc(len);
	rbb = buf;

	rbb->type = cpu_to_be16(BB_RATP_TYPE_PONG);

	ret = ratp_send(&ctx->ratp, buf, len);

	free(buf);

	return ret;
}

static int ratp_bb_send_getenv_return(struct ratp_ctx *ctx, const char *val)
{
	void *buf;
	struct ratp_bb *rbb;
	int len, ret;

	if (!val)
	    val = "";

	len = sizeof(*rbb) + strlen(val);
	buf = xzalloc(len);
	rbb = buf;
	strcpy(rbb->data, val);

	rbb->type = cpu_to_be16(BB_RATP_TYPE_GETENV_RETURN);

	ret = ratp_send(&ctx->ratp, buf, len);

	free(buf);

	return ret;
}

static char *ratp_command;
static struct ratp_ctx *ratp_command_ctx;

static int ratp_bb_dispatch(struct ratp_ctx *ctx, const void *buf, int len)
{
	const struct ratp_bb *rbb = buf;
	struct ratp_bb_pkt *pkt;
	int dlen = len - sizeof(struct ratp_bb);
	char *varname;
	int ret = 0;

	switch (be16_to_cpu(rbb->type)) {
	case BB_RATP_TYPE_COMMAND:
		if (ratp_command)
			return 0;

		ratp_command = xmemdup_add_zero(&rbb->data, dlen);
		ratp_command_ctx = ctx;
		pr_debug("got command: %s\n", ratp_command);

		break;

	case BB_RATP_TYPE_COMMAND_RETURN:
	case BB_RATP_TYPE_PONG:
		break;

	case BB_RATP_TYPE_CONSOLEMSG:

		kfifo_put(ctx->console_recv_fifo, rbb->data, dlen);
		break;

	case BB_RATP_TYPE_PING:
		ret = ratp_bb_send_pong(ctx);
		break;

	case BB_RATP_TYPE_GETENV:
		varname = xmemdup_add_zero(&rbb->data, dlen);

		ret = ratp_bb_send_getenv_return(ctx, getenv(varname));
		break;

	case BB_RATP_TYPE_FS_RETURN:
		pkt = xzalloc(sizeof(*pkt) + dlen);
		pkt->len = dlen;
		memcpy(pkt->data, &rbb->data, dlen);
		ctx->fs_rx = pkt;
		break;
	default:
		printf("%s: unhandled packet type 0x%04x\n", __func__, be16_to_cpu(rbb->type));
		break;
	}

	return ret;
}

static int ratp_console_getc(struct console_device *cdev)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);
	unsigned char c;

	if (!kfifo_len(ctx->console_recv_fifo))
		return -1;

	kfifo_getc(ctx->console_recv_fifo, &c);

	return c;
}

static int ratp_console_tstc(struct console_device *cdev)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);

	return kfifo_len(ctx->console_recv_fifo) ? 1 : 0;
}

static int ratp_console_puts(struct console_device *cdev, const char *s)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);
	int len = 0;

	len = strlen(s);

	if (ratp_busy(&ctx->ratp))
		return len;

	kfifo_put(ctx->console_transmit_fifo, s, len);

	return len;
}

static void ratp_console_putc(struct console_device *cdev, char c)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);

	if (ratp_busy(&ctx->ratp))
		return;

	kfifo_putc(ctx->console_transmit_fifo, c);
}

static int ratp_console_register(struct ratp_ctx *ctx)
{
	int ret;

	ctx->ratp_console.tstc = ratp_console_tstc;
	ctx->ratp_console.puts = ratp_console_puts;
	ctx->ratp_console.putc = ratp_console_putc;
	ctx->ratp_console.getc = ratp_console_getc;
	ctx->ratp_console.devname = "ratpconsole";
	ctx->ratp_console.devid = DEVICE_ID_SINGLE;

	ret = console_register(&ctx->ratp_console);
	if (ret) {
		pr_err("registering failed with %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}

void ratp_run_command(void)
{
	int ret;

	if (!ratp_command)
		return;

	pr_debug("running command: %s\n", ratp_command);

	ret = run_command(ratp_command);

	free(ratp_command);
	ratp_command = NULL;

	ratp_bb_send_command_return(ratp_command_ctx, ret);
}

static const char *ratpfs_mount_path;

int barebox_ratp_fs_mount(const char *path)
{
	if (path && ratpfs_mount_path)
		return -EBUSY;

	ratpfs_mount_path = path;

	return 0;
}

static void ratp_console_unregister(struct ratp_ctx *ctx)
{
	int ret;

	console_set_active(&ctx->ratp_console, 0);
	poller_unregister(&ctx->poller);
	ratp_close(&ctx->ratp);
	console_set_active(ctx->cdev, ctx->old_active);
	ctx->cdev = NULL;

	if (ratpfs_mount_path) {
		ret = umount(ratpfs_mount_path);
		if (!ret)
			ratpfs_mount_path = NULL;
	}
}

static void ratp_poller(struct poller_struct *poller)
{
	struct ratp_ctx *ctx = container_of(poller, struct ratp_ctx, poller);
	int ret;
	size_t len;
	void *buf;

	ratp_queue_console_tx(ctx);

	ret = ratp_poll(&ctx->ratp);
	if (ret == -EINTR)
		goto out;
	if (ratp_closed(&ctx->ratp))
		goto out;

	ret = ratp_recv(&ctx->ratp, &buf, &len);
	if (ret < 0)
		return;

	ratp_bb_dispatch(ctx, buf, len);

	free(buf);

	return;

out:
	ratp_console_unregister(ctx);
}

int barebox_ratp_fs_call(struct ratp_bb_pkt *tx, struct ratp_bb_pkt **rx)
{
	struct ratp_ctx *ctx = ratp_command_ctx;
	struct ratp_bb *rbb;
	int len;
	u64 start;

	if (!ctx)
		return -EINVAL;

	ctx->fs_rx = NULL;

	len = sizeof(*rbb) + tx->len;
	rbb = xzalloc(len);
	rbb->type = cpu_to_be16(BB_RATP_TYPE_FS);
	memcpy(rbb->data, tx->data, tx->len);

	if (ratp_send(&ctx->ratp, rbb, len) != 0)
		pr_debug("failed to send port pkt\n");

	free(rbb);

	start = get_time_ns();

	while (!ctx->fs_rx) {
		poller_call();
		if (ratp_closed(&ctx->ratp))
			return -EIO;
		if (is_timeout(start, 10 * SECOND))
			return -ETIMEDOUT;
	}

	*rx = ctx->fs_rx;

	pr_debug("%s: len %i\n", __func__, ctx->fs_rx->len);

	return 0;
}

int barebox_ratp(struct console_device *cdev)
{
	int ret;
	struct ratp_ctx *ctx;
	struct ratp *ratp;

	if (!cdev->getc || !cdev->putc)
		return -EINVAL;

	if (ratp_command_ctx) {
		ctx = ratp_command_ctx;
	} else {
		ctx = xzalloc(sizeof(*ctx));
		ratp_command_ctx = ctx;
		ctx->ratp.send = console_send;
		ctx->ratp.recv = console_recv;
		ctx->console_recv_fifo = kfifo_alloc(512);
		ctx->console_transmit_fifo = kfifo_alloc(SZ_128K);
		ctx->poller.func = ratp_poller;
		ratp_console_register(ctx);
	}

	if (ctx->cdev)
		return -EBUSY;

	ratp = &ctx->ratp;

	ctx->old_active = console_get_active(cdev);
	console_set_active(cdev, 0);

	ctx->cdev = cdev;
	ctx->have_synch = 1;

	ret = ratp_establish(ratp, false, 100);
	if (ret < 0)
		goto out;

	ret = poller_register(&ctx->poller);
	if (ret)
		goto out1;

	console_set_active(&ctx->ratp_console, CONSOLE_STDOUT | CONSOLE_STDERR |
			CONSOLE_STDIN);

	return 0;

out1:
	ratp_close(ratp);
out:
	console_set_active(ctx->cdev, ctx->old_active);
	ctx->cdev = NULL;

	return ret;
}

static void barebox_ratp_close(void)
{
	if (ratp_command_ctx && ratp_command_ctx->cdev)
		ratp_console_unregister(ratp_command_ctx);
}
predevshutdown_exitcall(barebox_ratp_close);
