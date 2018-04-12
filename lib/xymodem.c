/*
 * Handles the X-Modem, Y-Modem and Y-Modem/G protocols
 *
 * Copyright (C) 2008 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This file provides functions to receive X-Modem or Y-Modem(/G) protocols.
 *
 * References:
 *   *-Modem: http://www.techfest.com/hardware/modem/xymodem.htm
 *            XMODEM/YMODEM PROTOCOL REFERENCE, Chuck Forsberg
 */
#include <common.h>
#include <xfuncs.h>
#include <errno.h>
#include <crc.h>
#include <clock.h>
#include <console.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <fs.h>
#include <kfifo.h>
#include <linux/byteorder/generic.h>
#include <xymodem.h>

#define xy_dbg(fmt, args...)

/* Values magic to the protocol */
#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define BSP 0x08
#define NAK 0x15
#define CAN 0x18

#define PROTO_XMODEM	0
#define PROTO_YMODEM	1
#define PROTO_YMODEM_G	2
#define MAX_PROTOS	3

#define CRC_NONE	0	/* No CRC checking */
#define CRC_ADD8	1	/* Add of all data bytes */
#define CRC_CRC16	2	/* CCCIT CRC16 */
#define MAX_CRCS	3

#define MAX_RETRIES		10
#define MAX_RETRIES_WITH_CRC	5
#define TIMEOUT_READ		(1 * SECOND)
#define TIMEOUT_FLUSH		(1 * SECOND)
#define MAX_CAN_BEFORE_ABORT	5
#define INPUT_FIFO_SIZE		(4 * 1024)	/* Should always be > 1029 */

enum proto_state {
	PROTO_STATE_GET_FILENAME = 0,
	PROTO_STATE_NEGOCIATE_CRC,
	PROTO_STATE_RECEIVE_BODY,
	PROTO_STATE_FINISHED_FILE,
	PROTO_STATE_FINISHED_XFER,
};

/**
 * struct xyz_ctxt - context of a x/y modem (g) transfer
 *
 * @cdev: console device to support *MODEM transfer
 * @fifo: fifo to buffer input from serial line
 *        This is necessary for low hardware FIFOs buffers as UARTs.
 * @mode: protocol (XMODEM, YMODEM or YMODEM/G)
 * @crc_mode: CRC_NONE, CRC_ADD8 or CRC_CRC16
 * @state: protocol state (as in "state machine")
 * @buf: buffer to store the last tranfered buffer chunk
 * @filename : filename transmitted by sender (YMODEM* only)
 * @fd : file descriptor of the current stored file
 * @file_len: length declared by sender (YMODEM* only)
 * @nb_received: number of data bytes received since session open
 *               (this doesn't count resends)
 * @total_SOH: number of SOH frames received (128 bytes chunks)
 * @total_STX: number of STX frames received (1024 bytes chunks)
 * @total_CAN: nubmer of CAN frames received (cancel frames)
 */
struct xyz_ctxt {
	struct console_device *cdev;
	struct kfifo *fifo;
	int mode;
	int crc_mode;
	enum proto_state state;
	char filename[1024];
	int fd;
	int file_len;
	int nb_received;
	int next_blk;
	int total_SOH, total_STX, total_CAN, total_retries;
};

/**
 * struct xy_block - one unitary block of x/y modem (g) transfer
 *
 * @buf: data buffer
 * @len: length of data buffer (can only be 128 or 1024)
 * @seq: block sequence number (as in X/Y/YG MODEM protocol)
 */
struct xy_block {
	unsigned char buf[1024];
	int len;
	int seq;
};

/*
 * For XMODEM/YMODEM, always try to use the CRC16 versions, called also
 * XMODEM/CRC and YMODEM.
 * Only fallback to additive CRC (8 bits) if sender doesn't cope with CRC16.
 */
static const char invite_filename_hdr[MAX_PROTOS][MAX_CRCS] = {
	{ 0, NAK, 'C' },	/* XMODEM */
	{ 0, NAK, 'C' },	/* YMODEM */
	{ 0, 'G', 'G' },	/* YMODEM-G */
};

static const char invite_file_body[MAX_PROTOS][MAX_CRCS] = {
	{ 0, NAK, 'C' },	/* XMODEM */
	{ 0, NAK, 'C' },	/* YMODEM */
	{ 0, 'G', 'G' },	/* YMODEM-G */
};

static const char block_ack[MAX_PROTOS][MAX_CRCS] = {
	{ 0, ACK, ACK },	/* XMODEM */
	{ 0, ACK, ACK },	/* YMODEM */
	{ 0, 0, 0 },		/* YMODEM-G */
};

static const char block_nack[MAX_PROTOS][MAX_CRCS] = {
	{ 0, NAK, NAK },	/* XMODEM */
	{ 0, NAK, NAK },	/* YMODEM */
	{ 0, 0, 0 },		/* YMODEM-G */
};

static int xy_gets(struct console_device *cdev, struct kfifo *fifo,
		      unsigned char *buf, int len, uint64_t timeout)
{
	int rc;

	rc = console_drain(cdev, fifo, buf, len, timeout);
	if (rc != len)
		return -ETIMEDOUT;

	return len;
}

static void xy_putc(struct console_device *cdev, unsigned char c)
{
	cdev->putc(cdev, c);
}

static void xy_flush(struct console_device *cdev, struct kfifo *fifo)
{
	uint64_t start;

	start = get_time_ns();
	while (cdev->tstc(cdev) &&
	       !is_timeout(start, TIMEOUT_FLUSH))
		cdev->getc(cdev);
	mdelay(250);
	while (cdev->tstc(cdev) &&
	       !is_timeout(start, TIMEOUT_FLUSH))
		cdev->getc(cdev);
	kfifo_reset(fifo);
}

static int is_xmodem(struct xyz_ctxt *proto)
{
	return proto->mode == PROTO_XMODEM;
}

static void xy_block_ack(struct xyz_ctxt *proto)
{
	unsigned char c = block_ack[proto->mode][proto->crc_mode];

	if (c)
		xy_putc(proto->cdev, c);
}

static void xy_block_nack(struct xyz_ctxt *proto)
{
	unsigned char c = block_nack[proto->mode][proto->crc_mode];

	if (c)
		xy_putc(proto->cdev, c);
	proto->total_retries++;
}

static int check_crc(unsigned char *buf, int len, int crc, int crc_mode)
{
	unsigned char crc8 = 0;
	uint16_t crc16;
	int i;

	switch (crc_mode) {
	case CRC_ADD8:
		for (i = 0; i < len; i++)
			crc8 += buf[i];
		return crc8 == crc ? 0 : -EBADMSG;
	case CRC_CRC16:
		crc16 = cyg_crc16(buf, len);
		xy_dbg("crc16: received = %x, calculated=%x\n", crc, crc16);
		return crc16 == crc ? 0 : -EBADMSG;
	case CRC_NONE:
		return 0;
	default:
		return -EBADMSG;
	}
}

/**
 * xy_read_block - read a X-Modem or Y-Modem(G) block
 * @proto: protocol control structure
 * @blk: block read
 * @timeout: maximal time to get data
 *
 * This is the pivotal function for block receptions. It attempts to receive one
 * block, ie. one 128 bytes or one 1024 bytes block.  The received data can also
 * be an end of transmission, or a cancel.
 *
 * Returns :
 *  >0           : size of the received block
 *  0            : last block, ie. end of transmission, ie. EOT
 * -EBADMSG      : malformed message (ie. sequence bi-bytes are not
 *                 complementary), or CRC check error
 * -EILSEQ       : block sequence number error wrt previously received block
 * -ETIMEDOUT    : block not received before timeout passed
 * -ECONNABORTED : transfer aborted by sender, ie. CAN
 */
static ssize_t xy_read_block(struct xyz_ctxt *proto, struct xy_block *blk,
	uint64_t timeout)
{
	ssize_t rc, data_len = 0;
	unsigned char hdr, seqs[2], crcs[2];
	int crc = 0;
	bool hdr_found = 0;
	uint64_t start = get_time_ns();

	while (!hdr_found) {
		rc = xy_gets(proto->cdev, proto->fifo, &hdr, 1, timeout);
		xy_dbg("read 0x%x(%c) -> %d\n", hdr, hdr, rc);
		if (rc < 0)
			goto out;
		if (is_timeout(start, timeout))
			goto timeout;
		switch (hdr) {
		case SOH:
			data_len = 128;
			hdr_found = 1;
			proto->total_SOH++;
			break;
		case STX:
			data_len = 1024;
			hdr_found = 1;
			proto->total_STX++;
			break;
		case CAN:
			rc = -ECONNABORTED;
			if (proto->total_CAN++ > MAX_CAN_BEFORE_ABORT)
				goto out;
			break;
		case EOT:
			rc = 0;
			blk->len = 0;
			goto out;
		default:
			break;
		}
	}

	blk->seq = 0;
	rc = xy_gets(proto->cdev, proto->fifo, seqs, 2, timeout);
	if (rc < 0)
		goto out;
	blk->seq = seqs[0];
	if (255 - seqs[0] != seqs[1])
		return -EBADMSG;

	rc = xy_gets(proto->cdev, proto->fifo, blk->buf, data_len, timeout);
	if (rc < 0)
		goto out;
	blk->len = rc;

	switch (proto->crc_mode) {
	case CRC_ADD8:
		rc = xy_gets(proto->cdev, proto->fifo, crcs, 1, timeout);
		crc = crcs[0];
		break;
	case CRC_CRC16:
		rc = xy_gets(proto->cdev, proto->fifo, crcs, 2, timeout);
		crc = (crcs[0] << 8) + crcs[1];
		break;
	case CRC_NONE:
		rc = 0;
		break;
	}
	if (rc < 0)
		goto out;

	rc = check_crc(blk->buf, data_len, crc, proto->crc_mode);
	if (rc < 0)
		goto out;
	return data_len;
timeout:
	return -ETIMEDOUT;
out:
	return rc;
}

static int check_blk_seq(struct xyz_ctxt *proto, struct xy_block *blk,
	int read_rc)
{
	if (blk->seq == ((proto->next_blk - 1) % 256))
		return -EALREADY;
	if (blk->seq != proto->next_blk)
		return -EILSEQ;
	return read_rc;
}

static int parse_first_block(struct xyz_ctxt *proto, struct xy_block *blk)
{
	int filename_len;
	char *str_num;

	filename_len = strlen(blk->buf);
	if (filename_len > blk->len)
		return -EINVAL;
	strlcpy(proto->filename, blk->buf, sizeof(proto->filename));
	str_num = blk->buf + filename_len + 1;
	strsep(&str_num, " ");
	proto->file_len = simple_strtoul(blk->buf + filename_len + 1, NULL, 10);
	return 1;
}

static int xy_get_file_header(struct xyz_ctxt *proto)
{
	struct xy_block blk;
	int tries, rc = 0;

	memset(&blk, 0, sizeof(blk));
	proto->state = PROTO_STATE_GET_FILENAME;
	proto->crc_mode = CRC_CRC16;
	for (tries = 0; tries < MAX_RETRIES; tries++) {
		xy_putc(proto->cdev,
			   invite_filename_hdr[proto->mode][proto->crc_mode]);
		rc = xy_read_block(proto, &blk, 3 * SECOND);
		xy_dbg("read block returned %d\n", rc);
		switch (rc) {
		case -ECONNABORTED:
			goto fail;
		case -ETIMEDOUT:
		case -EBADMSG:
			if (proto->mode != PROTO_YMODEM_G)
				xy_flush(proto->cdev, proto->fifo);
			break;
		case -EALREADY:
		default:
			proto->next_blk = 1;
			xy_block_ack(proto);
			proto->state = PROTO_STATE_NEGOCIATE_CRC;
			rc = parse_first_block(proto, &blk);
			return rc;
		}

		if (rc < 0 && tries++ >= MAX_RETRIES_WITH_CRC)
			proto->crc_mode = CRC_ADD8;
	}
	rc = -ETIMEDOUT;
fail:
	proto->total_retries += tries;
	return rc;
}

static int xy_await_header(struct xyz_ctxt *proto)
{
	int rc;

	rc = xy_get_file_header(proto);
	if (rc < 0)
		return rc;
	proto->state = PROTO_STATE_NEGOCIATE_CRC;
	xy_dbg("header received, filename=%s, file length=%d\n",
	       proto->filename, proto->file_len);
	if (proto->filename[0])
		proto->fd = open(proto->filename, O_WRONLY | O_CREAT);
	else
		proto->state = PROTO_STATE_FINISHED_XFER;
	proto->nb_received = 0;
	return rc;
}

static void xy_finish_file(struct xyz_ctxt *proto)
{
	close(proto->fd);
	proto->fd = 0;
	proto->state = PROTO_STATE_FINISHED_FILE;
}

static struct xyz_ctxt *xymodem_open(struct console_device *cdev,
				     int proto_mode, int xmodem_fd)
{
	struct xyz_ctxt *proto;

	proto = xzalloc(sizeof(struct xyz_ctxt));
	proto->fifo = kfifo_alloc(INPUT_FIFO_SIZE);
	proto->mode = proto_mode;
	proto->cdev = cdev;
	proto->crc_mode = CRC_CRC16;

	if (is_xmodem(proto)) {
		proto->fd = xmodem_fd;
		proto->state = PROTO_STATE_NEGOCIATE_CRC;
	} else {
		proto->state = PROTO_STATE_GET_FILENAME;
	}
	xy_flush(proto->cdev, proto->fifo);
	return proto;
}

static int xymodem_handle(struct xyz_ctxt *proto)
{
	int rc = 0, xfer_max, len = 0, again = 1, remain;
	int crc_tries = 0, same_blk_retries = 0;
	unsigned char invite;
	struct xy_block blk;

	while (again) {
		switch (proto->state) {
		case PROTO_STATE_GET_FILENAME:
			crc_tries = 0;
			rc = xy_await_header(proto);
			if (rc < 0)
				goto fail;
			continue;
		case PROTO_STATE_FINISHED_FILE:
			if (is_xmodem(proto))
				proto->state = PROTO_STATE_FINISHED_XFER;
			else
				proto->state = PROTO_STATE_GET_FILENAME;
			xy_putc(proto->cdev, ACK);
			continue;
		case PROTO_STATE_FINISHED_XFER:
			again = 0;
			rc = 0;
			goto out;
		case PROTO_STATE_NEGOCIATE_CRC:
			invite = invite_file_body[proto->mode][proto->crc_mode];
			proto->next_blk = 1;
			if (crc_tries++ > MAX_RETRIES_WITH_CRC)
				proto->crc_mode = CRC_ADD8;
			xy_putc(proto->cdev, invite);
			/* Fall through */
		case PROTO_STATE_RECEIVE_BODY:
			rc = xy_read_block(proto, &blk, 3 * SECOND);
			if (rc > 0) {
				rc = check_blk_seq(proto, &blk, rc);
				proto->state = PROTO_STATE_RECEIVE_BODY;
			}
			break;
		}

		if (proto->state != PROTO_STATE_RECEIVE_BODY)
			continue;

		switch (rc) {
		case -ECONNABORTED:
			goto fail;
		case -ETIMEDOUT:
			if (proto->mode == PROTO_YMODEM_G)
				goto fail;
			xy_flush(proto->cdev, proto->fifo);
			xy_block_nack(proto);
			break;
		case -EBADMSG:
		case -EILSEQ:
			if (proto->mode == PROTO_YMODEM_G)
				goto fail;
			xy_flush(proto->cdev, proto->fifo);
			xy_block_nack(proto);
			break;
		case -EALREADY:
			xy_block_ack(proto);
			break;
		case 0:
			xy_finish_file(proto);
			break;
		default:
			remain = proto->file_len - proto->nb_received;
			if (is_xmodem(proto))
				xfer_max = blk.len;
			else
				xfer_max = min(blk.len, remain);
			rc = write(proto->fd, blk.buf, xfer_max);
			proto->next_blk = ((blk.seq + 1) % 256);
			proto->nb_received += rc;
			len += rc;
			xy_block_ack(proto);
			break;
		}
		if (rc < 0)
			same_blk_retries++;
		else
			same_blk_retries = 0;
		if (same_blk_retries > MAX_RETRIES)
			goto fail;
	}
out:
	return rc;
fail:
	if (proto->fd)
		close(proto->fd);
	return rc;
}

static void xymodem_close(struct xyz_ctxt *proto)
{
	xy_flush(proto->cdev, proto->fifo);
	printf("\nxyModem - %d(SOH)/%d(STX)/%d(CAN) packets,"
	       " %d retries\n",
	       proto->total_SOH, proto->total_STX,
	       proto->total_CAN, proto->total_retries);
	kfifo_free(proto->fifo);
}

int do_load_serial_xmodem(struct console_device *cdev, int fd)
{
	struct xyz_ctxt *proto;
	int rc;

	proto = xymodem_open(cdev, PROTO_XMODEM, fd);
	do {
		rc = xymodem_handle(proto);
	} while (rc > 0);
	xymodem_close(proto);
	return rc < 0 ? rc : 0;
}
EXPORT_SYMBOL(do_load_serial_xmodem);

int do_load_serial_ymodem(struct console_device *cdev)
{
	struct xyz_ctxt *proto;
	int rc;

	proto = xymodem_open(cdev, PROTO_YMODEM, 0);
	do {
		rc = xymodem_handle(proto);
	} while (rc > 0);
	xymodem_close(proto);
	return rc < 0 ? rc : 0;
}
EXPORT_SYMBOL(do_load_serial_ymodem);

int do_load_serial_ymodemg(struct console_device *cdev)
{
	struct xyz_ctxt *proto;
	int rc;

	proto = xymodem_open(cdev, PROTO_YMODEM_G, 0);
	do {
		rc = xymodem_handle(proto);
	} while (rc > 0);
	xymodem_close(proto);
	return rc < 0 ? rc : 0;
}
EXPORT_SYMBOL(do_load_serial_ymodemg);
