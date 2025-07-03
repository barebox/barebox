/*
 * tftp.c
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "tftp: " fmt

#include <common.h>
#include <command.h>
#include <net.h>
#include <driver.h>
#include <clock.h>
#include <fs.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <getopt.h>
#include <globalvar.h>
#include <init.h>
#include <linux/bitmap.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <kfifo.h>
#include <parseopt.h>
#include <linux/sizes.h>
#include <linux/netfs.h>

#include "tftp-selftest.h"

#define TFTP_PORT	69	/* Well known TFTP port number */

/* Seconds to wait before remote server is allowed to resend a lost packet */
#define TIMEOUT		5

/* After this time without a response from the server we will resend a packet */
#define TFTP_RESEND_TIMEOUT	SECOND

/* After this time without progress we will bail out */
#define TFTP_TIMEOUT		((TIMEOUT * 3) * SECOND)

/*
 *	TFTP operations.
 */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

#define STATE_INVALID	0
#define STATE_RRQ	1
#define STATE_WRQ	2
#define STATE_RDATA	3
#define STATE_WDATA	4
/* OACK from server has been received and we can begin to sent either the ACK
   (for RRQ) or data (for WRQ) */
#define STATE_START	5
#define STATE_WAITACK	6
#define STATE_LAST	7
#define STATE_DONE	8

#define TFTP_BLOCK_SIZE		512	/* default TFTP block size */
#define TFTP_MTU_SIZE		1432	/* MTU based block size */
#define TFTP_MAX_WINDOW_SIZE	CONFIG_FS_TFTP_MAX_WINDOW_SIZE

/* allocate this number of blocks more than needed in the fifo */
#define TFTP_EXTRA_BLOCKS	2

/* marker for an emtpy 'tftp_cache' */
#define TFTP_CACHE_NO_ID	(-1)

#define TFTP_ERR_RESEND	1

#if defined(DEBUG) || IS_ENABLED(CONFIG_SELFTEST_TFTP)
#  define debug_assert(_cond)	BUG_ON(!(_cond))
#else
#  define debug_assert(_cond) do {			\
		if (!(_cond))				\
			__builtin_unreachable();	\
	} while (0)
#endif

static int g_tftp_window_size = DIV_ROUND_UP(TFTP_MAX_WINDOW_SIZE, 2);

struct tftp_block {
	uint16_t id;
	uint16_t len;

	struct list_head list;
	uint8_t data[];
};

struct tftp_cache {
	struct list_head	blocks;
};

struct file_priv {
	struct net_connection *tftp_con;
	int push;
	uint16_t block;
	uint16_t last_block;
	uint16_t ack_block;
	int state;
	int err;
	char *filename;
	loff_t filesize;
	uint64_t resend_timeout;
	uint64_t progress_timeout;
	struct kfifo *fifo;
	void *buf;
	int blocksize;
	unsigned int windowsize;
	bool is_getattr;
	struct tftp_cache cache;
};

struct tftp_priv {
	IPaddr_t server;
};

struct tftp_inode {
	struct netfs_inode netfs_node;
};

static struct tftp_inode *to_tftp_inode(struct inode *inode)
{
	return container_of(inode, struct tftp_inode, netfs_node.inode);
}

static inline bool is_block_before(uint16_t a, uint16_t b)
{
	return (int16_t)(b - a) > 0;
}

static bool in_window(uint16_t block, uint16_t start, uint16_t end)
{
	/* handle the three cases:
	   - [ ......... | start | .. | BLOCK | .. | end | ......... ]
	   - [ ..| BLOCK | .. | end | ................. | start | .. ]
	   - [ ..| end | ................. | start | .. | BLOCK | .. ]
	*/
	return ((start <= block && block <= end) ||
		(block <= end   && end   <= start) ||
		(end   <= start && start <= block));
}

static void tftp_window_cache_free(struct tftp_cache *cache)
{
	struct tftp_block *block, *tmp;

	list_for_each_entry_safe(block, tmp, &cache->blocks, list)
		free(block);
}

static int tftp_window_cache_insert(struct tftp_cache *cache, uint16_t id,
				    void const *data, size_t len)
{
	struct tftp_block *block, *new;

	list_for_each_entry(block, &cache->blocks, list) {
		if (block->id == id)
			return 0;
		if (is_block_before(block->id, id))
			continue;

		break;
	}

	new = xzalloc(sizeof(*new) + len);
	memcpy(new->data, data, len);
	new->id = id;
	new->len = len;
	list_add_tail(&new->list, &block->list);

	return 0;
}

static int tftp_truncate(struct device *dev, struct file *f, loff_t size)
{
	return 0;
}

static char const * const tftp_states[] = {
	[STATE_INVALID] = "INVALID",
	[STATE_RRQ] = "RRQ",
	[STATE_WRQ] = "WRQ",
	[STATE_RDATA] = "RDATA",
	[STATE_WDATA] = "WDATA",
	[STATE_WAITACK] = "WAITACK",
	[STATE_LAST] = "LAST",
	[STATE_DONE] = "DONE",
	[STATE_START] = "START",
};

static int tftp_send(struct file_priv *priv)
{
	unsigned char *xp;
	int len = 0;
	uint16_t *s;
	unsigned char *pkt = net_udp_get_payload(priv->tftp_con);
	unsigned int window_size;
	int ret;

	pr_vdebug("%s: state %s\n", __func__, tftp_states[priv->state]);

	switch (priv->state) {
	case STATE_RRQ:
	case STATE_WRQ:
		if (priv->push || priv->is_getattr)
			/* atm, windowsize is supported only for RRQ and there
			   is no need to request a full window when we are
			   just looking up file attributes */
			window_size = 1;
		else
			window_size = min_t(unsigned int, g_tftp_window_size,
					    TFTP_MAX_WINDOW_SIZE);

		xp = pkt;
		s = (uint16_t *)pkt;
		if (priv->state == STATE_RRQ)
			*s++ = htons(TFTP_RRQ);
		else
			*s++ = htons(TFTP_WRQ);
		pkt = (unsigned char *)s;
		pkt += sprintf((unsigned char *)pkt,
				"%s%c"
				"octet%c"
				"timeout%c"
				"%d%c"
				"blksize%c"
				"%u",
				priv->filename + 1, '\0',
				'\0',	/* "octet" */
				'\0',	/* "timeout" */
				TIMEOUT, '\0',
				'\0',	/* "blksize" */
				/* use only a minimal blksize for getattr
				   operations, */
				priv->is_getattr ? TFTP_BLOCK_SIZE : TFTP_MTU_SIZE);
		pkt++;

		if (!priv->push)
			/* we do not know the filesize in WRQ requests and
			   'priv->filesize' will always be zero */
			pkt += sprintf((unsigned char *)pkt,
				       "tsize%c%lld%c",
				       '\0', priv->filesize,
				       '\0');

		if (window_size > 1)
			pkt += sprintf((unsigned char *)pkt,
				       "windowsize%c%u%c",
				       '\0', window_size,
				       '\0');

		len = pkt - xp;
		break;

	case STATE_RDATA:
		xp = pkt;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(priv->last_block);
		priv->ack_block  = priv->last_block;
		priv->ack_block += priv->windowsize;
		pkt = (unsigned char *)s;
		len = pkt - xp;
		break;
	}

	ret = net_udp_send(priv->tftp_con, len);

	return ret;
}

static int tftp_send_write(struct file_priv *priv, void *buf, int len)
{
	uint16_t *s;
	unsigned char *pkt = net_udp_get_payload(priv->tftp_con);
	int ret;

	s = (uint16_t *)pkt;
	*s++ = htons(TFTP_DATA);
	*s++ = htons(priv->block);
	memcpy((void *)s, buf, len);
	if (len < priv->blocksize)
		priv->state = STATE_LAST;
	len += 4;

	ret = net_udp_send(priv->tftp_con, len);
	priv->last_block = priv->block;
	priv->state = STATE_WAITACK;

	return ret;
}

static int tftp_poll(struct file_priv *priv)
{
	if (ctrlc()) {
		priv->state = STATE_DONE;
		priv->err = -EINTR;
		return -EINTR;
	}

	if (is_timeout(priv->resend_timeout, TFTP_RESEND_TIMEOUT)) {
		printf("T ");
		priv->resend_timeout = get_time_ns();
		return TFTP_ERR_RESEND;
	}

	if (is_timeout(priv->progress_timeout, TFTP_TIMEOUT)) {
		priv->state = STATE_DONE;
		priv->err = -ETIMEDOUT;
		return -ETIMEDOUT;
	}

	net_poll();

	return 0;
}

static int tftp_parse_oack(struct file_priv *priv, unsigned char *pkt, int len)
{
	unsigned char *opt, *val, *s;

	pkt[len - 1] = 0;

	pr_debug("got OACK\n");
#ifdef DEBUG
	memory_display(pkt, 0, len, 1, 0);
#endif

	s = pkt;

	while (s < pkt + len) {
		opt = s;
		val = s + strlen(s) + 1;
		if (val > s + len)
			break;
		if (!strcmp(opt, "tsize"))
			priv->filesize = simple_strtoull(val, NULL, 10);
		if (!strcmp(opt, "blksize"))
			priv->blocksize = simple_strtoul(val, NULL, 10);
		if (!strcmp(opt, "windowsize"))
			priv->windowsize = simple_strtoul(val, NULL, 10);
		pr_debug("OACK opt: %s val: %s\n", opt, val);
		s = val + strlen(val) + 1;
	}

	if (priv->blocksize > TFTP_MTU_SIZE ||
	    priv->windowsize > TFTP_MAX_WINDOW_SIZE ||
	    priv->windowsize == 0) {
		pr_warn("tftp: invalid oack response\n");
		return -EINVAL;
	}

	return 0;
}

static void tftp_timer_reset(struct file_priv *priv)
{
	priv->progress_timeout = priv->resend_timeout = get_time_ns();
}

static int tftp_allocate_transfer(struct file_priv *priv)
{
	debug_assert(!priv->fifo);
	debug_assert(!priv->buf);

	/* multiplication is safe; both operands were checked in tftp_parse_oack()
	   and are small integers */
	priv->fifo = kfifo_alloc(priv->blocksize *
				 (priv->windowsize + TFTP_EXTRA_BLOCKS));
	if (!priv->fifo)
		goto err;

	if (priv->push) {
		priv->buf = xmalloc(priv->blocksize);
		if (!priv->buf) {
			kfifo_free(priv->fifo);
			priv->fifo = NULL;
			goto err;
		}
	}

	INIT_LIST_HEAD(&priv->cache.blocks);

	return 0;

err:
	priv->err = -ENOMEM;
	priv->state = STATE_DONE;

	return priv->err;
}

static void tftp_put_data(struct file_priv *priv, uint16_t block,
			  void const *pkt, size_t len)
{
	unsigned int sz;

	if (len > priv->blocksize) {
		pr_warn("tftp: oversized packet (%zu > %d) received\n",
			len, priv->blocksize);
		return;
	}

	priv->last_block = block;

	sz = kfifo_put(priv->fifo, pkt, len);

	if (sz != len) {
		pr_err("tftp: not enough room in kfifo (only %u out of %zu written)\n",
		       sz, len);
		priv->err = -ENOMEM;
		priv->state = STATE_DONE;
	} else if (len < priv->blocksize) {
		tftp_send(priv);
		priv->err = 0;
		priv->state = STATE_DONE;
	}
}

static void tftp_apply_window_cache(struct file_priv *priv)
{
	struct tftp_cache *cache = &priv->cache;

	while (1) {
		struct tftp_block *block;

		/* can be changed by tftp_put_data() below and must be
		   checked in each loop */
		if (priv->state != STATE_RDATA)
			return;

		if (list_empty(&cache->blocks))
			return;

		block = list_first_entry(&cache->blocks, struct tftp_block, list);

		if (is_block_before(block->id, priv->last_block + 1)) {
			/* shouldn't happen, but be sure */
			list_del(&block->list);
			free(block);
			continue;
		}

		if (block->id != (uint16_t)(priv->last_block + 1))
			return;

		tftp_put_data(priv, block->id, block->data, block->len);

		list_del(&block->list);

		free(block);
	}
}

static void tftp_handle_data(struct file_priv *priv, uint16_t block,
			     void const *data, size_t len)
{
	uint16_t exp_block;
	int rc;

	exp_block = priv->last_block + 1;

	if (exp_block == block) {
		/* datagram over network is the expected one; put it in the
		   fifo directly and try to apply cached items then */
		tftp_timer_reset(priv);
		tftp_put_data(priv, block, data, len);
		tftp_apply_window_cache(priv);
	} else if (!in_window(block, exp_block, priv->ack_block)) {
		/* completely unexpected and unrelated to actual window;
		   ignore the packet. */
		printf("B");
		if (g_tftp_window_size > 1)
			pr_warn_once("Unexpected packet. global.tftp.windowsize set too high?\n");
	} else {
		/* The 'rc < 0' below happens e.g. when datagrams in the first
		   part of the transfer window are dropped.

		   TODO: this will usually result in a timeout
		   (TFTP_RESEND_TIMEOUT).  It should be possible to bypass
		   this timeout by acknowledging the last packet (e.g. by
		   doing 'priv->ack_block = priv->last_block' here). */
		rc = tftp_window_cache_insert(&priv->cache, block, data, len);
		if (rc < 0)
			printf("M");
	}
}

static void tftp_recv(struct file_priv *priv,
			uint8_t *pkt, unsigned len, uint16_t uh_sport)
{
	uint16_t opcode;
	uint16_t block;
	int rc;

	/* according to RFC1350 minimal tftp packet length is 4 bytes */
	if (len < 4)
		return;

	opcode = ntohs(*(uint16_t *)pkt);

	/* skip tftp opcode 2-byte field */
	len -= 2;
	pkt += 2;

	pr_vdebug("%s: opcode 0x%04x\n", __func__, opcode);

	switch (opcode) {
	case TFTP_RRQ:
	case TFTP_WRQ:
	default:
		break;
	case TFTP_ACK:
		if (!priv->push)
			break;

		block = ntohs(*(uint16_t *)pkt);
		if (block != priv->last_block) {
			pr_vdebug("ack %d != %d\n", block, priv->last_block);
			break;
		}

		switch (priv->state) {
		case STATE_WRQ:
			priv->tftp_con->udp->uh_dport = uh_sport;
			priv->state = STATE_START;
			break;

		case STATE_WAITACK:
			priv->state = STATE_WDATA;
			break;

		case STATE_LAST:
			priv->state = STATE_DONE;
			break;

		default:
			pr_warn("ACK packet in %s state\n",
				tftp_states[priv->state]);
			goto ack_out;
		}

		priv->block = block + 1;
		tftp_timer_reset(priv);

	ack_out:
		break;

	case TFTP_OACK:
		if (priv->state != STATE_RRQ && priv->state != STATE_WRQ) {
			pr_warn("OACK packet in %s state\n",
				tftp_states[priv->state]);
			break;
		}

		priv->tftp_con->udp->uh_dport = uh_sport;

		if (tftp_parse_oack(priv, pkt, len) < 0) {
			priv->err = -EINVAL;
			priv->state = STATE_DONE;
			break;
		}

		priv->state = STATE_START;
		break;

	case TFTP_DATA:
		len -= 2;
		block = ntohs(*(uint16_t *)pkt);

		if (priv->state == STATE_RRQ) {
			/* first block received; entered only with non rfc
			   2347 (TFTP Option extension) compliant servers */
			priv->tftp_con->udp->uh_dport = uh_sport;
			priv->state = STATE_RDATA;
			priv->last_block = 0;
			priv->ack_block = priv->windowsize;

			rc = tftp_allocate_transfer(priv);
			if (rc < 0)
				break;
		}

		if (priv->state != STATE_RDATA) {
			pr_warn("DATA packet in %s state\n",
				tftp_states[priv->state]);
			break;
		}

		tftp_handle_data(priv, block, pkt + 2, len);
		break;

	case TFTP_ERROR:
		pr_debug("error: '%s' (%d)\n", pkt + 2, ntohs(*(uint16_t *)pkt));
		switch (ntohs(*(uint16_t *)pkt)) {
		case 1:
			priv->err = -ENOENT;
			break;
		case 2:
			priv->err = -EACCES;
			break;
		default:
			priv->err = -EINVAL;
			break;
		}
		priv->state = STATE_DONE;
		break;
	}
}

static void tftp_handler(void *ctx, char *packet, unsigned len)
{
	struct file_priv *priv = ctx;
	char *pkt = net_eth_to_udp_payload(packet);
	struct udphdr *udp = net_eth_to_udphdr(packet);

	(void)len;
	tftp_recv(priv, pkt, net_eth_to_udplen(packet), udp->uh_sport);
}

static int tftp_start_transfer(struct file_priv *priv)
{
	int rc;

	rc = tftp_allocate_transfer(priv);
	if (rc < 0)
		/* function sets 'priv->state = STATE_DONE' and 'priv->err' in
		   error case */
		return rc;

	if (priv->push) {
		/* send first block */
		priv->state = STATE_WDATA;
		priv->block = 1;
	} else {
		/* send ACK */
		priv->state = STATE_RDATA;
		priv->last_block = 0;
		tftp_send(priv);
	}

	return 0;
}

static struct file_priv *tftp_do_open(struct device *dev,
				      int accmode, struct dentry *dentry,
				      bool is_getattr)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct file_priv *priv;
	struct tftp_priv *tpriv = dev->priv;
	int ret;
	unsigned short port = TFTP_PORT;

	priv = xzalloc(sizeof(*priv));

	switch (accmode & O_ACCMODE) {
	case O_RDONLY:
		priv->push = 0;
		priv->state = STATE_RRQ;
		break;
	case O_WRONLY:
		priv->push = 1;
		priv->state = STATE_WRQ;
		break;
	case O_RDWR:
		ret = -ENOSYS;
		goto out;
	}

	priv->block = 1;
	priv->err = -EINVAL;
	priv->filename = dpath(dentry, fsdev->vfsmount.mnt_root);
	priv->blocksize = TFTP_BLOCK_SIZE;
	priv->windowsize = 1;
	priv->is_getattr = is_getattr;

	parseopt_hu(fsdev->options, "port", &port);

	priv->tftp_con = net_udp_new(tpriv->server, port, tftp_handler, priv);
	if (IS_ERR(priv->tftp_con)) {
		ret = PTR_ERR(priv->tftp_con);
		goto out;
	}

	ret = tftp_send(priv);
	if (ret)
		goto out1;

	tftp_timer_reset(priv);

	/* - 'ret < 0'  ... error
	   - 'ret == 0' ... further tftp_poll() required
	   - 'ret == 1' ... startup finished */
	do {
		switch (priv->state) {
		case STATE_DONE:
			/* branch is entered in two situations:
			   - non rfc 2347 compliant servers finished the
			     transfer by sending a small file
			   - some error occurred */
			if (priv->err < 0)
				ret = priv->err;
			else
				ret = 1;
			break;

		case STATE_START:
			ret = tftp_start_transfer(priv);
			if (!ret)
				ret = 1;
			break;

		case STATE_RDATA:
			/* first data block of non rfc 2347 servers */
			ret = 1;
			break;

		case STATE_RRQ:
		case STATE_WRQ:
			ret = tftp_poll(priv);
			if (ret == TFTP_ERR_RESEND) {
				tftp_send(priv);
				ret = 0;
			}
			break;

		default:
			debug_assert(false);
			break;
		}
	} while (ret == 0);

	if (ret < 0)
		goto out1;

	return priv;
out1:
	net_unregister(priv->tftp_con);
out:
	if (priv->fifo)
		kfifo_free(priv->fifo);

	free(priv->filename);
	free(priv->buf);
	free(priv);

	return ERR_PTR(ret);
}

static int tftp_open(struct inode *inode, struct file *file)
{
	struct file_priv *priv;

	priv = tftp_do_open(&file->fsdev->dev, file->f_flags, file->f_path.dentry, false);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->private_data = priv;

	return 0;
}

static int tftp_do_close(struct file_priv *priv)
{
	int ret;

	if (priv->push && priv->state != STATE_DONE) {
		int len;

		len = kfifo_get(priv->fifo, priv->buf, priv->blocksize);
		tftp_send_write(priv, priv->buf, len);
		priv->state = STATE_LAST;

		tftp_timer_reset(priv);

		while (priv->state != STATE_DONE) {
			ret = tftp_poll(priv);
			if (ret == TFTP_ERR_RESEND)
				tftp_send_write(priv, priv->buf, len);
			if (ret < 0)
				break;
		}
	}

	if (!priv->push && priv->state != STATE_DONE) {
		uint16_t *pkt = net_udp_get_payload(priv->tftp_con);
		*pkt++ = htons(TFTP_ERROR);
		*pkt++ = 0;
		*pkt++ = 0;
		net_udp_send(priv->tftp_con, 6);
	}

	net_unregister(priv->tftp_con);
	tftp_window_cache_free(&priv->cache);
	kfifo_free(priv->fifo);
	free(priv->filename);
	free(priv->buf);
	free(priv);

	return 0;
}

static int tftp_close(struct inode *inode, struct file *f)
{
	struct file_priv *priv = f->private_data;

	return tftp_do_close(priv);
}

static int tftp_write(struct device *_dev, struct file *f, const void *inbuf,
		      size_t insize)
{
	struct file_priv *priv = f->private_data;
	size_t size, now;
	int ret;

	pr_vdebug("%s: %zu\n", __func__, insize);

	size = insize;

	while (size) {
		now = kfifo_put(priv->fifo, inbuf, size);

		while (kfifo_len(priv->fifo) >= priv->blocksize) {
			kfifo_get(priv->fifo, priv->buf, priv->blocksize);

			tftp_send_write(priv, priv->buf, priv->blocksize);
			tftp_timer_reset(priv);

			while (priv->state == STATE_WAITACK) {
				ret = tftp_poll(priv);
				if (ret == TFTP_ERR_RESEND)
					tftp_send_write(priv, priv->buf,
							priv->blocksize);
				if (ret < 0)
					return ret;
			}
		}
		size -= now;
		inbuf += now;
	}

	return insize;
}

static int tftp_read(struct device *dev, struct file *f, void *buf, size_t insize)
{
	struct file_priv *priv = f->private_data;
	size_t outsize = 0, now;
	int ret = 0;

	pr_vdebug("%s %zu\n", __func__, insize);

	while (insize) {
		now = kfifo_get(priv->fifo, buf, insize);
		outsize += now;
		buf += now;
		insize -= now;

		if (priv->state == STATE_DONE) {
			ret = priv->err;
			break;
		}

		/* send the ACK only when fifo has been nearly depleted; else,
		   when tftp_read() is called with small 'insize' values, it
		   is possible that there is read more data from the network
		   than consumed by kfifo_get() and the fifo overflows */
		if (priv->last_block == priv->ack_block &&
		    kfifo_len(priv->fifo) <= TFTP_EXTRA_BLOCKS * priv->blocksize)
			tftp_send(priv);

		ret = tftp_poll(priv);
		if (ret == TFTP_ERR_RESEND)
			tftp_send(priv);
		if (ret < 0)
			break;
	}

	if (ret < 0)
		return ret;

	return outsize;
}

static int tftp_lseek(struct device *dev, struct file *f, loff_t pos)
{
	/* We cannot seek backwards without reloading or caching the file */
	loff_t f_pos = f->f_pos;

	if (pos >= f_pos) {
		int ret = 0;
		char *buf = xmalloc(1024);

		while (pos > f_pos) {
			size_t len = min_t(size_t, 1024, pos - f_pos);

			ret = tftp_read(dev, f, buf, len);

			if (!ret)
				/* EOF, so the desired pos is invalid. */
				ret = -EINVAL;
			if (ret < 0)
				goto out_free;

			f_pos += ret;
		}

out_free:
		free(buf);
		if (ret < 0) {
			/*
			 * Update f->pos even if the overall request
			 * failed since we can't move backwards
			 */
			f->f_pos = f_pos;
			return ret;
		}

		return 0;
	}

	return -ENOSYS;
}

static const struct inode_operations tftp_file_inode_operations;
static const struct inode_operations tftp_dir_inode_operations;
static const struct file_operations tftp_file_operations = {
	.open = tftp_open,
	.release = tftp_close,
};

static struct inode *tftp_get_inode(struct super_block *sb, const struct inode *dir,
                                     umode_t mode)
{
	struct inode *inode = new_inode(sb);
	struct tftp_inode *node;

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;

	node = to_tftp_inode(inode);
	netfs_inode_init(&node->netfs_node);

	switch (mode & S_IFMT) {
	default:
		return NULL;
	case S_IFREG:
		inode->i_op = &tftp_file_inode_operations;
		inode->i_fop = &tftp_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &tftp_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
		inc_nlink(inode);
		break;
	}

	return inode;
}

static int tftp_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode;

	inode = tftp_get_inode(dir->i_sb, dir, mode);
	if (!inode)
		return -EPERM;

	inode->i_size = 0;

	d_instantiate(dentry, inode);

	return 0;
}

static struct dentry *tftp_lookup(struct inode *dir, struct dentry *dentry,
			    unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	struct inode *inode;
	struct file_priv *priv;
	loff_t filesize;

	priv = tftp_do_open(&fsdev->dev, O_RDONLY, dentry, true);
	if (IS_ERR(priv))
		return NULL;

	filesize = priv->filesize;

	tftp_do_close(priv);

	inode = tftp_get_inode(dir->i_sb, dir, S_IFREG | S_IRWXUGO);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	if (filesize)
		inode->i_size = filesize;
	else
		inode->i_size = FILE_SIZE_STREAM;

	d_add(dentry, inode);

	return NULL;
}

static const struct inode_operations tftp_dir_inode_operations =
{
	.lookup = tftp_lookup,
	.create = tftp_create,
};

static struct inode *tftp_alloc_inode(struct super_block *sb)
{
	struct tftp_inode *node;

	node = xzalloc(sizeof(*node));
	if (!node)
		return NULL;

	return &node->netfs_node.inode;
}

static void tftp_destroy_inode(struct inode *inode)
{
	struct tftp_inode *node = to_tftp_inode(inode);

	free(node);
}

static const struct super_operations tftp_ops = {
	.alloc_inode = tftp_alloc_inode,
	.destroy_inode = tftp_destroy_inode,
};

static int tftp_probe(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct tftp_priv *priv = xzalloc(sizeof(struct tftp_priv));
	struct super_block *sb = &fsdev->sb;
	struct inode *inode;
	int ret;

	dev->priv = priv;

	ret = resolv(fsdev->backingstore, &priv->server);
	if (ret) {
		pr_err("Cannot resolve \"%s\": %pe\n", fsdev->backingstore, ERR_PTR(ret));
		goto err;
	}

	sb->s_op = &tftp_ops;
	sb->s_d_op = &netfs_dentry_operations_timed;

	inode = tftp_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
err:
	free(priv);

	return ret;
}

static void tftp_remove(struct device *dev)
{
	struct tftp_priv *priv = dev->priv;

	free(priv);
}

static struct fs_driver tftp_driver = {
	.read      = tftp_read,
	.lseek     = tftp_lseek,
	.write     = tftp_write,
	.truncate  = tftp_truncate,
	.drv = {
		.probe  = tftp_probe,
		.remove = tftp_remove,
		.name = "tftp",
	}
};

static int tftp_init(void)
{
	globalvar_add_simple_int("tftp.windowsize", &g_tftp_window_size, "%u");

	return register_fs_driver(&tftp_driver);
}
coredevice_initcall(tftp_init);
