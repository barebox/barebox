/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <malloc.h>
#include <init.h>
#include <ratp.h>
#include <byteorder.h>
#include <environment.h>
#include <kfifo.h>
#include <poller.h>
#include <work.h>
#include <linux/sizes.h>
#include <ratp_bb.h>
#include <fs.h>
#include <console_countdown.h>

LIST_HEAD(ratp_command_list);
EXPORT_SYMBOL(ratp_command_list);

#define for_each_ratp_command(cmd) list_for_each_entry(cmd, &ratp_command_list, list)

struct ratp_bb_command_return {
	uint32_t errno;
};

struct ratp_ctx {
	struct console_device *cdev;
	struct ratp ratp;
	struct console_device ratp_console;
	int have_synch;
	int old_active;

	struct kfifo *console_recv_fifo;
	struct kfifo *console_transmit_fifo;

	struct ratp_bb_pkt *fs_rx;

	struct poller_struct poller;
	struct work_queue wq;

	bool console_registered;
	bool poller_registered;
	bool wq_registered;
};

static int compare_ratp_command(struct list_head *a, struct list_head *b)
{
	int id_a = list_entry(a, struct ratp_command, list)->request_id;
	int id_b = list_entry(b, struct ratp_command, list)->request_id;

	return (id_a - id_b);
}

int register_ratp_command(struct ratp_command *cmd)
{
	debug("register ratp command: request %hu, response %hu\n",
	      cmd->request_id, cmd->response_id);
	list_add_sort(&cmd->list, &ratp_command_list, compare_ratp_command);
	return 0;
}
EXPORT_SYMBOL(register_ratp_command);

static struct ratp_command *find_ratp_request(uint16_t request_id)
{
	struct ratp_command *cmdtp;

	for_each_ratp_command(cmdtp)
		if (request_id == cmdtp->request_id)
			return cmdtp;

	return NULL;	/* not found */
}

extern struct ratp_command __barebox_ratp_cmd_start;
extern struct ratp_command __barebox_ratp_cmd_end;

static int init_ratp_command_list(void)
{
	struct ratp_command *cmdtp;

	for (cmdtp = &__barebox_ratp_cmd_start;
			cmdtp != &__barebox_ratp_cmd_end;
			cmdtp++)
		register_ratp_command(cmdtp);

	return 0;
}

late_initcall(init_ratp_command_list);

static bool console_exists(struct console_device *cdev)
{
	struct console_device *cs;

	list_for_each_entry(cs, &console_list, list)
		if (cs == cdev)
			return true;
	return false;
}

static int console_recv(struct ratp *r, uint8_t *data)
{
	struct ratp_ctx *ctx = container_of(r, struct ratp_ctx, ratp);
	struct console_device *cdev = ctx->cdev;

	if (!console_exists(cdev))
		return -ENODEV;

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

	if (!console_exists(cdev))
		return -ENODEV;

	for (i = 0; i < len; i++)
		cdev->putc(cdev, buf[i]);

	return 0;
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

static struct ratp_ctx *ratp_ctx;

struct ratp_work {
	struct work_struct work;
	struct ratp_ctx *ctx;
	char *command;
};

static int ratp_bb_dispatch(struct ratp_ctx *ctx, const void *buf, int len)
{
	const struct ratp_bb *rbb = buf;
	struct ratp_bb_pkt *pkt;
	int dlen = len - sizeof(struct ratp_bb);
	int ret = 0;
	uint16_t type = be16_to_cpu(rbb->type);
	struct ratp_command *cmd;
	struct ratp_work *rw;

	/* See if there's a command registered to this type */
	cmd = find_ratp_request(type);
	if (cmd) {
		struct ratp_bb *rsp = NULL;
		int rsp_len = 0;

		ret = cmd->cmd(rbb, len, &rsp, &rsp_len);
		if (!ret)
			ret = ratp_send(&ctx->ratp, rsp, rsp_len);

		free(rsp);
		return ret;
	}

	switch (type) {
	case BB_RATP_TYPE_COMMAND:
		if (!IS_ENABLED(CONFIG_CONSOLE_RATP))
			return 0;

		rw = xzalloc(sizeof(*rw));
		rw->ctx = ctx;
		rw->command = xstrndup((const char *)rbb->data, dlen);

		wq_queue_work(&ctx->wq, &rw->work);

		pr_debug("got command: %s\n", rw->command);

		break;

	case BB_RATP_TYPE_COMMAND_RETURN:
		break;

	case BB_RATP_TYPE_CONSOLEMSG:
		if (!IS_ENABLED(CONFIG_CONSOLE_RATP))
			return 0;

		kfifo_put(ctx->console_recv_fifo, rbb->data, dlen);
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

static int ratp_console_puts(struct console_device *cdev, const char *s,
			     size_t nbytes)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);

	if (ratp_busy(&ctx->ratp))
		return nbytes;

	kfifo_put(ctx->console_transmit_fifo, s, nbytes);

	return nbytes;
}

static void ratp_console_putc(struct console_device *cdev, char c)
{
	struct ratp_ctx *ctx = container_of(cdev, struct ratp_ctx, ratp_console);

	if (ratp_busy(&ctx->ratp))
		return;

	kfifo_putc(ctx->console_transmit_fifo, c);
}

static void ratp_command_run(struct work_struct *w)
{
	struct ratp_work *rw = container_of(w, struct ratp_work, work);
	struct ratp_ctx *ctx = rw->ctx;
	int ret;

	pr_debug("running command: %s\n", rw->command);

	ret = run_command(rw->command);

	free(rw->command);
	free(rw);

	ratp_bb_send_command_return(ctx, ret);
}

static const char *ratpfs_mount_path;

int barebox_ratp_fs_mount(const char *path)
{
	if (path && ratpfs_mount_path)
		return -EBUSY;

	ratpfs_mount_path = path;

	return 0;
}

static void ratp_unregister(struct ratp_ctx *ctx)
{
	int ret;

	if (ctx->console_registered)
		console_unregister(&ctx->ratp_console);

	if (ctx->poller_registered)
		poller_unregister(&ctx->poller);

	if (ctx->wq_registered)
		wq_unregister(&ctx->wq);

	ratp_close(&ctx->ratp);
	console_set_active(ctx->cdev, ctx->old_active);

	if (ctx->console_recv_fifo)
		kfifo_free(ctx->console_recv_fifo);

	if (ctx->console_transmit_fifo)
		kfifo_free(ctx->console_transmit_fifo);

	free(ctx);
	ratp_ctx = NULL;

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

	if (IS_ENABLED(CONFIG_CONSOLE_RATP))
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
	ratp_unregister(ctx);
}

int barebox_ratp_fs_call(struct ratp_bb_pkt *tx, struct ratp_bb_pkt **rx)
{
	struct ratp_ctx *ctx = ratp_ctx;
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
		if (ratp_closed(&ctx->ratp))
			return -EIO;
		if (is_timeout(start, 10 * SECOND))
			return -ETIMEDOUT;
	}

	*rx = ctx->fs_rx;

	pr_debug("%s: len %i\n", __func__, ctx->fs_rx->len);

	return 0;
}

static void ratp_work_cancel(struct work_struct *w)
{
	struct ratp_work *rw = container_of(w, struct ratp_work, work);

	free(rw->command);
	free(rw);
}

int barebox_ratp(struct console_device *cdev)
{
	int ret;
	struct ratp_ctx *ctx;

	if (!cdev->getc || !cdev->putc)
		return -EINVAL;

	if (ratp_ctx)
		return -EBUSY;

	ctx = xzalloc(sizeof(*ctx));
	ratp_ctx = ctx;
	ctx->ratp.send = console_send;
	ctx->ratp.recv = console_recv;
	ctx->console_recv_fifo = kfifo_alloc(512);
	ctx->console_transmit_fifo = kfifo_alloc(SZ_128K);
	ctx->poller.func = ratp_poller;
	ctx->ratp_console.tstc = ratp_console_tstc;
	ctx->ratp_console.puts = ratp_console_puts;
	ctx->ratp_console.putc = ratp_console_putc;
	ctx->ratp_console.getc = ratp_console_getc;
	ctx->ratp_console.devname = "ratpconsole";
	ctx->ratp_console.devid = DEVICE_ID_SINGLE;

	ret = console_register(&ctx->ratp_console);
	if (ret) {
		pr_err("registering console failed with %s\n", strerror(-ret));
		return ret;
	}

	ctx->console_registered = true;

	ctx->old_active = console_get_active(cdev);
	console_set_active(cdev, 0);

	ctx->cdev = cdev;
	ctx->have_synch = 1;

	ret = ratp_establish(&ctx->ratp, false, 100);
	if (ret < 0)
		goto out;

	ctx->wq.fn = ratp_command_run;
	ctx->wq.cancel = ratp_work_cancel;
	wq_register(&ctx->wq);
	ctx->wq_registered = true;

	ret = poller_register(&ctx->poller, "ratp");
	if (ret)
		goto out;

	ctx->poller_registered = true;

	console_countdown_abort();

	console_set_active(&ctx->ratp_console, CONSOLE_STDOUT | CONSOLE_STDERR |
			CONSOLE_STDIN);

	return 0;

out:
	ratp_unregister(ctx);

	return ret;
}

static void barebox_ratp_close(void)
{
	if (ratp_ctx)
		ratp_unregister(ratp_ctx);
}
predevshutdown_exitcall(barebox_ratp_close);
