/*
 * tftp.c
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <fs.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <kfifo.h>
#include <linux/sizes.h>

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
#define STATE_OACK	5
#define STATE_WAITACK	6
#define STATE_LAST	7
#define STATE_DONE	8

#define TFTP_BLOCK_SIZE		512	/* default TFTP block size */
#define TFTP_FIFO_SIZE		4096

#define TFTP_ERR_RESEND	1

struct file_priv {
	struct net_connection *tftp_con;
	int push;
	uint16_t block;
	uint16_t last_block;
	int state;
	int err;
	char *filename;
	loff_t filesize;
	uint64_t resend_timeout;
	uint64_t progress_timeout;
	struct kfifo *fifo;
	void *buf;
	int blocksize;
	int block_requested;
};

struct tftp_priv {
	IPaddr_t server;
};

static int tftp_truncate(struct device_d *dev, FILE *f, ulong size)
{
	return 0;
}

static char *tftp_states[] = {
	[STATE_INVALID] = "INVALID",
	[STATE_RRQ] = "RRQ",
	[STATE_WRQ] = "WRQ",
	[STATE_RDATA] = "RDATA",
	[STATE_WDATA] = "WDATA",
	[STATE_OACK] = "OACK",
	[STATE_WAITACK] = "WAITACK",
	[STATE_LAST] = "LAST",
	[STATE_DONE] = "DONE",
};

static int tftp_send(struct file_priv *priv)
{
	unsigned char *xp;
	int len = 0;
	uint16_t *s;
	unsigned char *pkt = net_udp_get_payload(priv->tftp_con);
	int ret;

	pr_vdebug("%s: state %s\n", __func__, tftp_states[priv->state]);

	switch (priv->state) {
	case STATE_RRQ:
	case STATE_WRQ:
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
				"tsize%c"
				"%lld%c"
				"blksize%c"
				"1432",
				priv->filename + 1, 0,
				0,
				0,
				TIMEOUT, 0,
				0,
				priv->filesize, 0,
				0);
		pkt++;
		len = pkt - xp;
		break;

	case STATE_RDATA:
		if (priv->block == priv->block_requested)
			return 0;
	case STATE_OACK:
		xp = pkt;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(priv->block);
		priv->block_requested = priv->block;
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
		priv->block_requested = -1;
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

static void tftp_parse_oack(struct file_priv *priv, unsigned char *pkt, int len)
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
			return;
		if (!strcmp(opt, "tsize"))
			priv->filesize = simple_strtoull(val, NULL, 10);
		if (!strcmp(opt, "blksize"))
			priv->blocksize = simple_strtoul(val, NULL, 10);
		pr_debug("OACK opt: %s val: %s\n", opt, val);
		s = val + strlen(val) + 1;
	}
}

static void tftp_timer_reset(struct file_priv *priv)
{
	priv->progress_timeout = priv->resend_timeout = get_time_ns();
}

static void tftp_recv(struct file_priv *priv,
			uint8_t *pkt, unsigned len, uint16_t uh_sport)
{
	uint16_t opcode;

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

		priv->block = ntohs(*(uint16_t *)pkt);
		if (priv->block != priv->last_block) {
			pr_vdebug("ack %d != %d\n", priv->block, priv->last_block);
			break;
		}

		priv->block++;

		tftp_timer_reset(priv);

		if (priv->state == STATE_LAST) {
			priv->state = STATE_DONE;
			break;
		}
		priv->tftp_con->udp->uh_dport = uh_sport;
		priv->state = STATE_WDATA;
		break;

	case TFTP_OACK:
		tftp_parse_oack(priv, pkt, len);
		priv->tftp_con->udp->uh_dport = uh_sport;

		if (priv->push) {
			/* send first block */
			priv->state = STATE_WDATA;
			priv->block = 1;
		} else {
			/* send ACK */
			priv->state = STATE_OACK;
			priv->block = 0;
			tftp_send(priv);
		}

		break;
	case TFTP_DATA:
		len -= 2;
		priv->block = ntohs(*(uint16_t *)pkt);

		if (priv->state == STATE_RRQ || priv->state == STATE_OACK) {
			/* first block received */
			priv->state = STATE_RDATA;
			priv->tftp_con->udp->uh_dport = uh_sport;
			priv->last_block = 0;

			if (priv->block != 1) {	/* Assertion */
				pr_err("error: First block is not block 1 (%d)\n",
					priv->block);
				priv->err = -EINVAL;
				priv->state = STATE_DONE;
				break;
			}
		}

		if (priv->block == priv->last_block)
			/* Same block again; ignore it. */
			break;

		priv->last_block = priv->block;

		tftp_timer_reset(priv);

		kfifo_put(priv->fifo, pkt + 2, len);

		if (len < priv->blocksize) {
			tftp_send(priv);
			priv->err = 0;
			priv->state = STATE_DONE;
		}

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

static struct file_priv *tftp_do_open(struct device_d *dev,
		int accmode, struct dentry *dentry)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct file_priv *priv;
	struct tftp_priv *tpriv = dev->priv;
	int ret;

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
	priv->block_requested = -1;

	priv->fifo = kfifo_alloc(TFTP_FIFO_SIZE);
	if (!priv->fifo) {
		ret = -ENOMEM;
		goto out;
	}

	priv->tftp_con = net_udp_new(tpriv->server, TFTP_PORT, tftp_handler,
			priv);
	if (IS_ERR(priv->tftp_con)) {
		ret = PTR_ERR(priv->tftp_con);
		goto out1;
	}

	ret = tftp_send(priv);
	if (ret)
		goto out2;

	tftp_timer_reset(priv);
	while (priv->state != STATE_RDATA &&
			priv->state != STATE_DONE &&
			priv->state != STATE_WDATA) {
		ret = tftp_poll(priv);
		if (ret == TFTP_ERR_RESEND)
			tftp_send(priv);
		if (ret < 0)
			goto out2;
	}

	if (priv->state == STATE_DONE && priv->err) {
		ret = priv->err;
		goto out2;
	}

	priv->buf = xmalloc(priv->blocksize);

	return priv;
out2:
	net_unregister(priv->tftp_con);
out1:
	kfifo_free(priv->fifo);
out:
	free(priv);

	return ERR_PTR(ret);
}

static int tftp_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct file_priv *priv;

	priv = tftp_do_open(dev, file->flags, file->dentry);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->priv = priv;

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
	kfifo_free(priv->fifo);
	free(priv->filename);
	free(priv->buf);
	free(priv);

	return 0;
}

static int tftp_close(struct device_d *dev, FILE *f)
{
	struct file_priv *priv = f->priv;

	return tftp_do_close(priv);
}

static int tftp_write(struct device_d *_dev, FILE *f, const void *inbuf,
		size_t insize)
{
	struct file_priv *priv = f->priv;
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

static int tftp_read(struct device_d *dev, FILE *f, void *buf, size_t insize)
{
	struct file_priv *priv = f->priv;
	size_t outsize = 0, now;
	int ret;

	pr_vdebug("%s %zu\n", __func__, insize);

	while (insize) {
		now = kfifo_get(priv->fifo, buf, insize);
		outsize += now;
		buf += now;
		insize -= now;
		if (priv->state == STATE_DONE)
			return outsize;

		if (TFTP_FIFO_SIZE - kfifo_len(priv->fifo) >= priv->blocksize)
			tftp_send(priv);

		ret = tftp_poll(priv);
		if (ret == TFTP_ERR_RESEND)
			tftp_send(priv);
		if (ret < 0)
			return ret;
	}

	return outsize;
}

static loff_t tftp_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	/* We cannot seek backwards without reloading or caching the file */
	if (pos >= f->pos) {
		loff_t ret;
		char *buf = xmalloc(1024);

		while (pos > f->pos) {
			size_t len = min_t(size_t, 1024, pos - f->pos);

			ret = tftp_read(dev, f, buf, len);

			if (!ret)
				/* EOF, so the desired pos is invalid. */
				ret = -EINVAL;
			if (ret < 0)
				goto out_free;

			f->pos += ret;
		}

		ret = pos;

out_free:
		free(buf);
		return ret;
	}

	return -ENOSYS;
}

static const struct inode_operations tftp_file_inode_operations;
static const struct inode_operations tftp_dir_inode_operations;
static const struct file_operations tftp_file_operations;

static struct inode *tftp_get_inode(struct super_block *sb, const struct inode *dir,
                                     umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;

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
	struct fs_device_d *fsdev = container_of(sb, struct fs_device_d, sb);
	struct inode *inode;
	struct file_priv *priv;
	loff_t filesize;

	priv = tftp_do_open(&fsdev->dev, O_RDONLY, dentry);
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

static const struct super_operations tftp_ops;

static int tftp_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct tftp_priv *priv = xzalloc(sizeof(struct tftp_priv));
	struct super_block *sb = &fsdev->sb;
	struct inode *inode;
	int ret;

	dev->priv = priv;

	ret = resolv(fsdev->backingstore, &priv->server);
	if (ret) {
		pr_err("Cannot resolve \"%s\": %s\n", fsdev->backingstore, strerror(-ret));
		goto err;
	}

	sb->s_op = &tftp_ops;

	inode = tftp_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
err:
	free(priv);

	return ret;
}

static void tftp_remove(struct device_d *dev)
{
	struct tftp_priv *priv = dev->priv;

	free(priv);
}

static struct fs_driver_d tftp_driver = {
	.open      = tftp_open,
	.close     = tftp_close,
	.read      = tftp_read,
	.lseek     = tftp_lseek,
	.write     = tftp_write,
	.truncate  = tftp_truncate,
	.flags     = 0,
	.drv = {
		.probe  = tftp_probe,
		.remove = tftp_remove,
		.name = "tftp",
	}
};

static int tftp_init(void)
{
	return register_fs_driver(&tftp_driver);
}
coredevice_initcall(tftp_init);
