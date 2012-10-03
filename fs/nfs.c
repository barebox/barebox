/*
 * nfs.c - barebox NFS driver
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) Masami Komiya <mkomiya@sonare.it> 2004
 *
 * Based on U-Boot NFS code which is based on NetBSD code
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

#include <common.h>
#include <net.h>
#include <driver.h>
#include <fs.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <kfifo.h>
#include <sizes.h>

#define SUNRPC_PORT     111

#define PROG_PORTMAP    100000
#define PROG_NFS        100003
#define PROG_MOUNT      100005

#define MSG_CALL        0
#define MSG_REPLY       1

#define PORTMAP_GETPORT 3

#define MOUNT_ADDENTRY	1
#define MOUNT_UMOUNT	3

#define NFS_GETATTR     1
#define NFS_LOOKUP      4
#define NFS_READLINK    5
#define NFS_READ        6
#define NFS_READDIR	16

#define NFS_FHSIZE      32

enum nfs_stat {
	NFS_OK		= 0,
	NFSERR_PERM	= 1,
	NFSERR_NOENT	= 2,
	NFSERR_IO	= 5,
	NFSERR_NXIO	= 6,
	NFSERR_ACCES	= 13,
	NFSERR_EXIST	= 17,
	NFSERR_NODEV	= 19,
	NFSERR_NOTDIR	= 20,
	NFSERR_ISDIR	= 21,
	NFSERR_FBIG	= 27,
	NFSERR_NOSPC	= 28,
	NFSERR_ROFS	= 30,
	NFSERR_NAMETOOLONG=63,
	NFSERR_NOTEMPTY	= 66,
	NFSERR_DQUOT	= 69,
	NFSERR_STALE	= 70,
	NFSERR_WFLUSH	= 99,
};

static void *nfs_packet;
static int nfs_len;

struct rpc_call {
	uint32_t id;
	uint32_t type;
	uint32_t rpcvers;
	uint32_t prog;
	uint32_t vers;
	uint32_t proc;
	uint32_t data[0];
};

struct rpc_reply {
	uint32_t id;
	uint32_t type;
	uint32_t rstatus;
	uint32_t verifier;
	uint32_t v2;
	uint32_t astatus;
	uint32_t data[0];
};

#define NFS_TIMEOUT	(2 * SECOND)
#define NFS_MAX_RESEND	5

struct nfs_priv {
	struct net_connection *con;
	IPaddr_t server;
	char *path;
	int mount_port;
	int nfs_port;
	unsigned long rpc_id;
	char rootfh[NFS_FHSIZE];
};

struct file_priv {
	struct kfifo *fifo;
	void *buf;
	char filefh[NFS_FHSIZE];
	struct nfs_priv *npriv;
};

static uint64_t nfs_timer_start;

static int	nfs_state;
#define STATE_DONE			1
#define STATE_START			2

enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5
};

struct fattr {
	uint32_t type;
	uint32_t mode;
	uint32_t nlink;
	uint32_t uid;
	uint32_t gid;
	uint32_t size;
	uint32_t blocksize;
	uint32_t rdev;
	uint32_t blocks;
};

struct readdirargs {
	char filefh[NFS_FHSIZE];
	uint32_t cookie;
	uint32_t count;
};

struct xdr_stream {
	__be32 *p;
	void *buf;
	__be32 *end;
};

#define xdr_zero 0
#define XDR_QUADLEN(l)		(((l) + 3) >> 2)

static void xdr_init(struct xdr_stream *stream, void *buf, int len)
{
	stream->p = stream->buf = buf;
	stream->end = stream->buf + len;
}

static __be32 *__xdr_inline_decode(struct xdr_stream *xdr, size_t nbytes)
{
	__be32 *p = xdr->p;
	__be32 *q = p + XDR_QUADLEN(nbytes);

        if (q > xdr->end || q < p)
		return NULL;
	xdr->p = q;
	return p;
}

static __be32 *xdr_inline_decode(struct xdr_stream *xdr, size_t nbytes)
{
	__be32 *p;

	if (nbytes == 0)
		return xdr->p;
	if (xdr->p == xdr->end)
		return NULL;
	p = __xdr_inline_decode(xdr, nbytes);

	return p;
}

static int decode_filename(struct xdr_stream *xdr,
		char *name, u32 *length)
{
	__be32 *p;
	u32 count;

	p = xdr_inline_decode(xdr, 4);
	if (!p)
		goto out_overflow;
	count = be32_to_cpup(p);
	if (count > 255)
		goto out_nametoolong;
	p = xdr_inline_decode(xdr, count);
	if (!p)
		goto out_overflow;
	memcpy(name, p, count);
	name[count] = 0;
	*length = count;
	return 0;
out_nametoolong:
	printk("NFS: returned filename too long: %u\n", count);
	return -ENAMETOOLONG;
out_overflow:
	printf("%s overflow\n",__func__);
	return -EIO;
}

/*
 * rpc_add_credentials - Add RPC authentication/verifier entries
 */
static uint32_t *rpc_add_credentials(uint32_t *p)
{
	int hl;
	int hostnamelen = 0;

	/*
	 * Here's the executive summary on authentication requirements of the
	 * various NFS server implementations:	Linux accepts both AUTH_NONE
	 * and AUTH_UNIX authentication (also accepts an empty hostname field
	 * in the AUTH_UNIX scheme).  *BSD refuses AUTH_NONE, but accepts
	 * AUTH_UNIX (also accepts an empty hostname field in the AUTH_UNIX
	 * scheme).  To be safe, use AUTH_UNIX and pass the hostname if we have
	 * it (if the BOOTP/DHCP reply didn't give one, just use an empty
	 * hostname).
	 */

	hl = (hostnamelen + 3) & ~3;

	/* Provide an AUTH_UNIX credential.  */
	*p++ = htonl(1);		/* AUTH_UNIX */
	*p++ = htonl(hl + 20);		/* auth length */
	*p++ = htonl(0);		/* stamp */
	*p++ = htonl(hostnamelen);	/* hostname string */

	if (hostnamelen & 3)
		*(p + hostnamelen / 4) = 0; /* add zero padding */

	/* memcpy(p, hostname, hostnamelen); */ /* empty hostname */

	p += hl / 4;
	*p++ = 0;			/* uid */
	*p++ = 0;			/* gid */
	*p++ = 0;			/* auxiliary gid list */

	/* Provide an AUTH_NONE verifier.  */
	*p++ = 0;			/* AUTH_NONE */
	*p++ = 0;			/* auth length */

	return p;
}

static int rpc_check_reply(unsigned char *pkt, int rpc_prog, unsigned long rpc_id, int *nfserr)
{
	uint32_t *data;
	struct rpc_reply rpc;

	*nfserr = 0;

	if (!pkt)
		return -EAGAIN;

	memcpy(&rpc, pkt, sizeof(rpc));

	if (ntohl(rpc.id) != rpc_id)
		return -EINVAL;

	if (rpc.rstatus  ||
	    rpc.verifier ||
	    rpc.astatus ) {
		return -EINVAL;
	}

	if (rpc_prog == PROG_PORTMAP)
		return 0;

	data = (uint32_t *)(pkt + sizeof(struct rpc_reply));
	*nfserr = ntohl(net_read_uint32(data));
	*nfserr = -*nfserr;

	debug("%s: state: %d, err %d\n", __func__, nfs_state, *nfserr);

	return 0;
}

/*
 * rpc_req - synchronous RPC request
 */
static int rpc_req(struct nfs_priv *npriv, int rpc_prog, int rpc_proc,
		uint32_t *data, int datalen)
{
	struct rpc_call pkt;
	unsigned long id;
	int dport;
	int ret;
	unsigned char *payload = net_udp_get_payload(npriv->con);
	int nfserr;
	int tries = 0;

	npriv->rpc_id++;
	id = npriv->rpc_id;

	pkt.id = htonl(id);
	pkt.type = htonl(MSG_CALL);
	pkt.rpcvers = htonl(2);	/* use RPC version 2 */
	pkt.prog = htonl(rpc_prog);
	pkt.vers = htonl(2);	/* portmapper is version 2 */
	pkt.proc = htonl(rpc_proc);

	memcpy(payload, &pkt, sizeof(pkt));
	memcpy(payload + sizeof(pkt), data, datalen * sizeof(uint32_t));

	if (rpc_prog == PROG_PORTMAP)
		dport = SUNRPC_PORT;
	else if (rpc_prog == PROG_MOUNT)
		dport = npriv->mount_port;
	else
		dport = npriv->nfs_port;

	npriv->con->udp->uh_dport = htons(dport);

again:
	ret = net_udp_send(npriv->con, sizeof(pkt) + datalen * sizeof(uint32_t));

	nfs_timer_start = get_time_ns();

	nfs_state = STATE_START;
	nfs_packet = NULL;

	while (nfs_state != STATE_DONE) {
		if (ctrlc()) {
			ret = -EINTR;
			break;
		}
		net_poll();

		if (is_timeout(nfs_timer_start, NFS_TIMEOUT)) {
			tries++;
			if (tries == NFS_MAX_RESEND)
				return -ETIMEDOUT;
			goto again;
		}

		ret = rpc_check_reply(nfs_packet, rpc_prog,
				npriv->rpc_id, &nfserr);
		if (!ret) {
			ret = nfserr;
			break;
		}
	}

	return ret;
}

/*
 * rpc_lookup_req - Lookup RPC Port numbers
 */
static int rpc_lookup_req(struct nfs_priv *npriv, int prog, int ver)
{
	uint32_t data[16];
	int ret;
	uint32_t port;

	data[0] = 0; data[1] = 0;	/* auth credential */
	data[2] = 0; data[3] = 0;	/* auth verifier */
	data[4] = htonl(prog);
	data[5] = htonl(ver);
	data[6] = htonl(17);	/* IP_UDP */
	data[7] = 0;

	ret = rpc_req(npriv, PROG_PORTMAP, PORTMAP_GETPORT, data, 8);
	if (ret)
		return ret;

	port = net_read_uint32((uint32_t *)(nfs_packet + sizeof(struct rpc_reply)));

	switch (prog) {
	case PROG_MOUNT:
		npriv->mount_port = ntohl(port);
		debug("mount port: %d\n", npriv->mount_port);
		break;
	case PROG_NFS:
		npriv->nfs_port = ntohl(port);
		debug("nfs port: %d\n", npriv->nfs_port);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * nfs_mount_req - Mount an NFS Filesystem
 */
static int nfs_mount_req(struct nfs_priv *npriv)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int pathlen;
	int ret;

	pathlen = strlen(npriv->path);

	debug("%s: %s\n", __func__, npriv->path);

	p = &(data[0]);
	p = rpc_add_credentials(p);

	*p++ = htonl(pathlen);
	if (pathlen & 3)
		*(p + pathlen / 4) = 0;

	memcpy (p, npriv->path, pathlen);
	p += (pathlen + 3) / 4;

	len = p - &(data[0]);

	ret = rpc_req(npriv, PROG_MOUNT, MOUNT_ADDENTRY, data, len);
	if (ret)
		return ret;

	memcpy(npriv->rootfh, nfs_packet + sizeof(struct rpc_reply) + 4, NFS_FHSIZE);

	return 0;
}

/*
 * nfs_umountall_req - Unmount all our NFS Filesystems on the Server
 */
static void nfs_umount_req(struct nfs_priv *npriv)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int pathlen;

	pathlen = strlen(npriv->path);

	p = &(data[0]);
	p = rpc_add_credentials(p);

	*p++ = htonl(pathlen);
	if (pathlen & 3)
		*(p + pathlen / 4) = 0;

	memcpy (p, npriv->path, pathlen);
	p += (pathlen + 3) / 4;

	len = p - &(data[0]);

	rpc_req(npriv, PROG_MOUNT, MOUNT_UMOUNT, data, len);
}

/*
 * nfs_lookup_req - Lookup Pathname
 */
static int nfs_lookup_req(struct file_priv *priv, const char *filename,
		int fnamelen)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int ret;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy(p, priv->filefh, NFS_FHSIZE);

	p += (NFS_FHSIZE / 4);
	*p++ = htonl(fnamelen);

	if (fnamelen & 3)
		*(p + fnamelen / 4) = 0;

	memcpy(p, filename, fnamelen);
	p += (fnamelen + 3) / 4;

	len = p - &(data[0]);

	ret = rpc_req(priv->npriv, PROG_NFS, NFS_LOOKUP, data, len);
	if (ret)
		return ret;

	memcpy(priv->filefh, nfs_packet + sizeof(struct rpc_reply) + 4, NFS_FHSIZE);

	return 0;
}

static int nfs_attr_req(struct file_priv *priv, struct stat *s)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int ret;
	struct fattr *fattr;
	uint32_t type;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy(p, priv->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = 0;

	len = p - &(data[0]);

	ret = rpc_req(priv->npriv, PROG_NFS, NFS_GETATTR, data, len);
	if (ret)
		return ret;

	fattr = nfs_packet + sizeof(struct rpc_reply) + 4;

	type = ntohl(net_read_uint32(&fattr->type));

	s->st_size = ntohl(net_read_uint32(&fattr->size));
	s->st_mode = ntohl(net_read_uint32(&fattr->mode));

	return 0;
}

static void *nfs_readdirattr_req(struct file_priv *priv, int *plen, uint32_t cookie)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int ret;
	void *buf;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy(p, priv->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(cookie); /* cookie */
	*p++ = htonl(1024); /* count */
	*p++ = 0;

	len = p - &(data[0]);

	ret = rpc_req(priv->npriv, PROG_NFS, NFS_READDIR, data, len);
	if (ret)
		return NULL;

	*plen = nfs_len - sizeof(struct rpc_reply) + 4;

	buf = xzalloc(*plen);

	memcpy(buf, nfs_packet + sizeof(struct rpc_reply) + 4, *plen);

	return buf;
}

/*
 * nfs_read_req - Read File on NFS Server
 */
static int nfs_read_req(struct file_priv *priv, int offset, int readlen)
{
	uint32_t data[1024];
	uint32_t *p;
	uint32_t *filedata;
	int len;
	int ret;
	int rlen;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy (p, priv->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(offset);
	*p++ = htonl(readlen);
	*p++ = 0;

	len = p - &(data[0]);

	ret = rpc_req(priv->npriv, PROG_NFS, NFS_READ, data, len);
	if (ret)
		return ret;

	filedata = (uint32_t *)(nfs_packet + sizeof(struct rpc_reply));

	rlen = ntohl(net_read_uint32(filedata + 18));

	kfifo_put(priv->fifo, (char *)(filedata + 19), rlen);

	return 0;
}

static void nfs_handler(void *ctx, char *packet, unsigned len)
{
	char *pkt = net_eth_to_udp_payload(packet);

	nfs_state = STATE_DONE;
	nfs_packet = pkt;
	nfs_len = len;
}

static int nfs_create(struct device_d *dev, const char *pathname, mode_t mode)
{
	return -ENOSYS;
}

static int nfs_unlink(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int nfs_mkdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int nfs_rmdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int nfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	return -ENOSYS;
}

static struct file_priv *nfs_do_open(struct device_d *dev, const char *filename)
{
	struct file_priv *priv;
	struct nfs_priv *npriv = dev->priv;
	int ret;
	const char *fname, *tok;

	priv = xzalloc(sizeof(*priv));

	priv->npriv = npriv;

	if (!*filename) {
		memcpy(priv->filefh, npriv->rootfh, NFS_FHSIZE);
		return priv;
	}

	filename++;

	fname = filename;

	memcpy(priv->filefh, npriv->rootfh, NFS_FHSIZE);

	while (*fname) {
		int flen;

		tok = strchr(fname, '/');
		if (tok)
			flen = tok - fname;
		else
			flen = strlen(fname);

		ret = nfs_lookup_req(priv, fname, flen);
		if (ret)
			goto out;

		if (tok)
			fname += flen + 1;
		else
			break;
	}

	return priv;

out:
	free(priv);

	return ERR_PTR(ret);
}

static void nfs_do_close(struct file_priv *priv)
{
	if (priv->fifo)
		kfifo_free(priv->fifo);

	free(priv);
}

static struct file_priv *nfs_do_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct file_priv *priv;
	int ret;

	priv = nfs_do_open(dev, filename);
	if (IS_ERR(priv))
		return priv;

	ret = nfs_attr_req(priv, s);
	if (ret) {
		nfs_do_close(priv);
		return ERR_PTR(ret);
	}

	return priv;
}

static int nfs_readlink_req(struct file_priv *priv, char* buf, size_t size)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int ret;
	char *path;
	uint32_t *filedata;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy(p, priv->filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);

	len = p - &(data[0]);

	ret = rpc_req(priv->npriv, PROG_NFS, NFS_READLINK, data, len);
	if (ret)
		return ret;

	filedata = nfs_packet + sizeof(struct rpc_reply);
	filedata++;

	len = ntohl(net_read_uint32(filedata)); /* new path length */
	filedata++;

	path = (char *)filedata;

	if (len > size)
		len = size;

	memcpy(buf, path, len);

	return 0;
}

static int nfs_readlink(struct device_d *dev, const char *filename,
			char *realname, size_t size)
{
	struct file_priv *priv;
	int ret;
	struct stat s;

	priv = nfs_do_stat(dev, filename, &s);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	ret = nfs_readlink_req(priv, realname, size);
	if (ret) {
		nfs_do_close(priv);
		return ret;
	}

	return 0;
}

static int nfs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct file_priv *priv;
	struct stat s;

	priv = nfs_do_stat(dev, filename, &s);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->inode = priv;
	file->size = s.st_size;

	priv->fifo = kfifo_alloc(1024);
	if (!priv->fifo) {
		free(priv);
		return -ENOMEM;
	}

	return 0;
}

static int nfs_close(struct device_d *dev, FILE *file)
{
	struct file_priv *priv = file->inode;

	nfs_do_close(priv);

	return 0;
}

static int nfs_write(struct device_d *_dev, FILE *file, const void *inbuf,
		size_t insize)
{
	return -ENOSYS;
}

static int nfs_read(struct device_d *dev, FILE *file, void *buf, size_t insize)
{
	struct file_priv *priv = file->inode;
	int now, outsize = 0, ret, pos = file->pos;

	while (insize) {
		now = kfifo_get(priv->fifo, buf, insize);
		outsize += now;
		buf += now;
		insize -= now;

		if (insize) {
			/* do not use min as insize is a size_t */
			if (insize < 1024)
				now = insize;
			else
				now = 1024;

			if (pos + now > file->size)
				now = file->size - pos;

			ret = nfs_read_req(priv, pos, now);
			if (ret)
				return ret;
			pos += now;
		}
	}

	return outsize;
}

static loff_t nfs_lseek(struct device_d *dev, FILE *file, loff_t pos)
{
	struct file_priv *priv = file->inode;

	file->pos = pos;
	kfifo_reset(priv->fifo);

	return file->pos;
}

struct nfs_dir {
	DIR dir;
	struct xdr_stream stream;
	struct dirent ent;
	struct file_priv *priv;
	uint32_t cookie;
};

static DIR *nfs_opendir(struct device_d *dev, const char *pathname)
{
	struct file_priv *priv;
	struct stat s;
	int ret;
	void *buf;
	struct nfs_dir *dir;
	int len;

	priv = nfs_do_open(dev, pathname);
	if (IS_ERR(priv))
		return NULL;

	ret = nfs_attr_req(priv, &s);
	if (ret)
		return NULL;

	if (!S_ISDIR(s.st_mode))
		return NULL;

	dir = xzalloc(sizeof(*dir));
	dir->priv = priv;

	buf = nfs_readdirattr_req(priv, &len, 0);
	if (!buf)
		return NULL;

	xdr_init(&dir->stream, buf, len);

	return &dir->dir;
}

static struct dirent *nfs_readdir(struct device_d *dev, DIR *dir)
{
	struct nfs_dir *ndir = (void *)dir;
	__be32 *p;
	int ret;
	int len;
	struct xdr_stream *xdr = &ndir->stream;

again:
	p = xdr_inline_decode(xdr, 4);
	if (!p)
		goto out_overflow;

	if (*p++ == xdr_zero) {
		p = xdr_inline_decode(xdr, 4);
		if (!p)
			goto out_overflow;
		if (*p++ == xdr_zero) {
			void *buf;
			int len;

			/*
			 * End of current entries, read next chunk.
			 */

			free(ndir->stream.buf);

			buf = nfs_readdirattr_req(ndir->priv, &len, ndir->cookie);
			if (!buf)
				return NULL;

			xdr_init(&ndir->stream, buf, len);

			goto again;
		}
		return NULL; /* -EINVAL */
	}

	p = xdr_inline_decode(xdr, 4);
	if (!p)
		goto out_overflow;

	ret = decode_filename(xdr, ndir->ent.d_name, &len);
	if (ret)
		return NULL;

	/*
	 * The type (size and byte order) of nfscookie isn't defined in
	 * RFC 1094.  This implementation assumes that it's an XDR uint32.
	 */
	p = xdr_inline_decode(xdr, 4);
	if (!p)
		goto out_overflow;

	ndir->cookie = be32_to_cpup(p);

	return &ndir->ent;

out_overflow:

	printf("nfs: overflow error\n");

	return NULL;

}

static int nfs_closedir(struct device_d *dev, DIR *dir)
{
	struct nfs_dir *ndir = (void *)dir;

	nfs_do_close(ndir->priv);
	free(ndir->stream.buf);
	free(ndir);

	return 0;
}

static int nfs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct file_priv *priv;

	priv = nfs_do_stat(dev, filename, s);
	if (IS_ERR(priv)) {
		return PTR_ERR(priv);
	} else {
		nfs_do_close(priv);
		return 0;
	}
}

static int nfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct nfs_priv *npriv = xzalloc(sizeof(struct nfs_priv));
	char *tmp = xstrdup(fsdev->backingstore);
	char *path;
	int ret;

	dev->priv = npriv;

	debug("nfs: mount: %s\n", fsdev->backingstore);

	path = strchr(tmp, ':');
	if (!path) {
		ret = -EINVAL;
		goto err;
	}

	*path = 0;

	npriv->path = xstrdup(path + 1);

	npriv->server = resolv(tmp);

	debug("nfs: server: %s path: %s\n", tmp, npriv->path);

	npriv->con = net_udp_new(npriv->server, 0, nfs_handler, npriv);
	if (IS_ERR(npriv->con)) {
		ret = PTR_ERR(npriv->con);
		goto err1;
	}

	/* Need a priviliged source port */
	net_udp_bind(npriv->con, 1000);

	ret = rpc_lookup_req(npriv, PROG_MOUNT, 1);
	if (ret) {
		printf("lookup mount port failed with %d\n", ret);
		goto err2;
	}

	ret = rpc_lookup_req(npriv, PROG_NFS, 2);
	if (ret) {
		printf("lookup nfs port failed with %d\n", ret);
		goto err2;
	}

	ret = nfs_mount_req(npriv);
	if (ret) {
		printf("mounting failed with %d\n", ret);
		goto err2;
	}

	free(tmp);

	return 0;

err2:
	net_unregister(npriv->con);
err1:
	free(npriv->path);
err:
	free(tmp);
	free(npriv);

	return ret;
}

static void nfs_remove(struct device_d *dev)
{
	struct nfs_priv *npriv = dev->priv;

	nfs_umount_req(npriv);

	net_unregister(npriv->con);
	free(npriv->path);
	free(npriv);
}

static struct fs_driver_d nfs_driver = {
	.open      = nfs_open,
	.close     = nfs_close,
	.read      = nfs_read,
	.lseek     = nfs_lseek,
	.opendir   = nfs_opendir,
	.readdir   = nfs_readdir,
	.closedir  = nfs_closedir,
	.stat      = nfs_stat,
	.create    = nfs_create,
	.unlink    = nfs_unlink,
	.mkdir     = nfs_mkdir,
	.rmdir     = nfs_rmdir,
	.write     = nfs_write,
	.truncate  = nfs_truncate,
	.readlink  = nfs_readlink,
	.flags     = 0,
	.drv = {
		.probe  = nfs_probe,
		.remove = nfs_remove,
		.name = "nfs",
	}
};

static int nfs_init(void)
{
	return register_fs_driver(&nfs_driver);
}
coredevice_initcall(nfs_init);
