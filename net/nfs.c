/*
 * NFS support driver - based on etherboot and barebox's tftp.c
 *
 * Masami Komiya <mkomiya@sonare.it> 2004
 *
 */

/* NOTE: the NFS code is heavily inspired by the NetBSD netboot code (read:
 * large portions are copied verbatim) as distributed in OSKit 0.97.  A few
 * changes were necessary to adapt the code to Etherboot and to fix several
 * inconsistencies.  Also the RPC message preparation is done "by hand" to
 * avoid adding netsprintf() which I find hard to understand and use.  */

/* NOTE 2: Etherboot does not care about things beyond the kernel image, so
 * it loads the kernel image off the boot server (ARP_SERVER) and does not
 * access the client root disk (root-path in dhcpd.conf), which would use
 * ARP_ROOTSERVER.  The root disk is something the operating system we are
 * about to load needs to use.	This is different from the OSKit 0.97 logic.  */

/* NOTE 3: Symlink handling introduced by Anselm M Hoffmeister, 2003-July-14
 * If a symlink is encountered, it is followed as far as possible (recursion
 * possible, maximum 16 steps). There is no clearing of ".."'s inside the
 * path, so please DON'T DO THAT. thx. */

#include <common.h>
#include <command.h>
#include <clock.h>
#include <net.h>
#include <malloc.h>
#include <libgen.h>
#include <fs.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <progress.h>
#include <linux/err.h>

#define SUNRPC_PORT     111

#define PROG_PORTMAP    100000
#define PROG_NFS        100003
#define PROG_MOUNT      100005

#define MSG_CALL        0
#define MSG_REPLY       1

#define PORTMAP_GETPORT 3

#define MOUNT_ADDENTRY  1
#define MOUNT_UMOUNTALL 4

#define NFS_LOOKUP      4
#define NFS_READLINK    5
#define NFS_READ        6

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

/* Block size used for NFS read accesses.  A RPC reply packet (including  all
 * headers) must fit within a single Ethernet frame to avoid fragmentation.
 * Chosen to be a power of two, as most NFS servers are optimized for this.  */
#define NFS_READ_SIZE   1024

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

struct rpc_t {
	union {
		struct {
			uint32_t id;
			uint32_t type;
			uint32_t rpcvers;
			uint32_t prog;
			uint32_t vers;
			uint32_t proc;
			uint32_t data[1];
		} call;
		struct {
			uint32_t id;
			uint32_t type;
			uint32_t rstatus;
			uint32_t verifier;
			uint32_t v2;
			uint32_t astatus;
			uint32_t data[19];
		} reply;
	} u;
};

#define NFS_TIMEOUT 15

static unsigned long rpc_id = 0;
static int nfs_offset = -1;
static uint64_t nfs_timer_start;
static int nfs_err;

static char dirfh[NFS_FHSIZE];	/* file handle of directory */
static char filefh[NFS_FHSIZE]; /* file handle of kernel image */

static int	nfs_server_mount_port;
static int	nfs_server_nfs_port;
static int	nfs_state;
#define STATE_PRCLOOKUP_PROG_MOUNT_REQ	1
#define STATE_PRCLOOKUP_PROG_NFS_REQ	2
#define STATE_MOUNT_REQ			3
#define STATE_UMOUNT_REQ		4
#define STATE_LOOKUP_REQ		5
#define STATE_READ_REQ			6
#define STATE_READLINK_REQ		7
#define STATE_DONE			8

static char *nfs_filename;
static char *nfs_path;
static char nfs_path_buff[2048];

static int net_store_fd;
static struct net_connection *nfs_con;

/**************************************************************************
RPC_ADD_CREDENTIALS - Add RPC authentication/verifier entries
**************************************************************************/
static uint32_t *rpc_add_credentials(uint32_t *p)
{
	int hl;
	int hostnamelen = 0;

	/* Here's the executive summary on authentication requirements of the
	 * various NFS server implementations:	Linux accepts both AUTH_NONE
	 * and AUTH_UNIX authentication (also accepts an empty hostname field
	 * in the AUTH_UNIX scheme).  *BSD refuses AUTH_NONE, but accepts
	 * AUTH_UNIX (also accepts an empty hostname field in the AUTH_UNIX
	 * scheme).  To be safe, use AUTH_UNIX and pass the hostname if we have
	 * it (if the BOOTP/DHCP reply didn't give one, just use an empty
	 * hostname).  */

	hl = (hostnamelen + 3) & ~3;

	/* Provide an AUTH_UNIX credential.  */
	*p++ = htonl(1);		/* AUTH_UNIX */
	*p++ = htonl(hl+20);		/* auth length */
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

/**************************************************************************
RPC_LOOKUP - Lookup RPC Port numbers
**************************************************************************/
static int rpc_req(int rpc_prog, int rpc_proc, uint32_t *data, int datalen)
{
	struct rpc_call pkt;
	unsigned long id;
	int sport;
	int ret;
	unsigned char *payload = net_udp_get_payload(nfs_con);

	id = ++rpc_id;
	pkt.id = htonl(id);
	pkt.type = htonl(MSG_CALL);
	pkt.rpcvers = htonl(2);	/* use RPC version 2 */
	pkt.prog = htonl(rpc_prog);
	pkt.vers = htonl(2);	/* portmapper is version 2 */
	pkt.proc = htonl(rpc_proc);

	memcpy(payload, &pkt, sizeof(pkt));
	memcpy(payload + sizeof(pkt), data, datalen * sizeof(uint32_t));

	if (rpc_prog == PROG_PORTMAP)
		sport = SUNRPC_PORT;
	else if (rpc_prog == PROG_MOUNT)
		sport = nfs_server_mount_port;
	else
		sport = nfs_server_nfs_port;

	nfs_con->udp->uh_dport = htons(sport);
	ret = net_udp_send(nfs_con, sizeof(pkt) + datalen * sizeof(uint32_t));

	return ret;
}

/**************************************************************************
RPC_LOOKUP - Lookup RPC Port numbers
**************************************************************************/
static void rpc_lookup_req(int prog, int ver)
{
	uint32_t data[16];

	data[0] = 0; data[1] = 0;	/* auth credential */
	data[2] = 0; data[3] = 0;	/* auth verifier */
	data[4] = htonl(prog);
	data[5] = htonl(ver);
	data[6] = htonl(17);	/* IP_UDP */
	data[7] = 0;

	rpc_req(PROG_PORTMAP, PORTMAP_GETPORT, data, 8);
}

/**************************************************************************
NFS_MOUNT - Mount an NFS Filesystem
**************************************************************************/
static void nfs_mount_req(char *path)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int pathlen;

	pathlen = strlen (path);

	p = &(data[0]);
	p = rpc_add_credentials(p);

	*p++ = htonl(pathlen);
	if (pathlen & 3)
		*(p + pathlen / 4) = 0;
	memcpy (p, path, pathlen);
	p += (pathlen + 3) / 4;

	len = p - &(data[0]);

	rpc_req(PROG_MOUNT, MOUNT_ADDENTRY, data, len);
}

/**************************************************************************
NFS_UMOUNTALL - Unmount all our NFS Filesystems on the Server
**************************************************************************/
static void nfs_umountall_req(void)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	if (nfs_server_mount_port < 0)
		/* Nothing mounted, nothing to umount */
		return;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	len = p - &(data[0]);

	rpc_req(PROG_MOUNT, MOUNT_UMOUNTALL, data, len);
}

/***************************************************************************
 * NFS_READLINK (AH 2003-07-14)
 * This procedure is called when read of the first block fails -
 * this probably happens when it's a directory or a symlink
 * In case of successful readlink(), the dirname is manipulated,
 * so that inside the nfs() function a recursion can be done.
 **************************************************************************/
static void nfs_readlink_req(void)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy (p, filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);

	len = p - &(data[0]);

	rpc_req(PROG_NFS, NFS_READLINK, data, len);
}

/**************************************************************************
NFS_LOOKUP - Lookup Pathname
**************************************************************************/
static void nfs_lookup_req(char *fname)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;
	int fnamelen;

	fnamelen = strlen (fname);

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy (p, dirfh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(fnamelen);
	if (fnamelen & 3)
		*(p + fnamelen / 4) = 0;
	memcpy (p, fname, fnamelen);
	p += (fnamelen + 3) / 4;

	len = p - &(data[0]);

	rpc_req(PROG_NFS, NFS_LOOKUP, data, len);
}

/**************************************************************************
NFS_READ - Read File on NFS Server
**************************************************************************/
static void nfs_read_req(int offset, int readlen)
{
	uint32_t data[1024];
	uint32_t *p;
	int len;

	p = &(data[0]);
	p = rpc_add_credentials(p);

	memcpy (p, filefh, NFS_FHSIZE);
	p += (NFS_FHSIZE / 4);
	*p++ = htonl(offset);
	*p++ = htonl(readlen);
	*p++ = 0;

	len = p - &(data[0]);

	rpc_req(PROG_NFS, NFS_READ, data, len);
}

/**************************************************************************
RPC request dispatcher
**************************************************************************/
static void nfs_send(void)
{
	debug("%s\n", __func__);

	switch (nfs_state) {
	case STATE_PRCLOOKUP_PROG_MOUNT_REQ:
		rpc_lookup_req(PROG_MOUNT, 1);
		break;
	case STATE_PRCLOOKUP_PROG_NFS_REQ:
		rpc_lookup_req(PROG_NFS, 2);
		break;
	case STATE_MOUNT_REQ:
		nfs_mount_req(nfs_path);
		break;
	case STATE_UMOUNT_REQ:
		nfs_umountall_req();
		break;
	case STATE_LOOKUP_REQ:
		nfs_lookup_req(nfs_filename);
		break;
	case STATE_READ_REQ:
		nfs_read_req(nfs_offset, NFS_READ_SIZE);
		break;
	case STATE_READLINK_REQ:
		nfs_readlink_req();
		break;
	}

	nfs_timer_start = get_time_ns();
}

static int rpc_check_reply(unsigned char *pkt, int isnfs)
{
	uint32_t *data;
	int nfserr;
	struct rpc_reply rpc;

	memcpy(&rpc, pkt, sizeof(rpc));

	if (ntohl(rpc.id) != rpc_id)
		return -EINVAL;

	if (rpc.rstatus  ||
	    rpc.verifier ||
	    rpc.astatus ) {
		return -EINVAL;
	}

	if (!isnfs)
		return 0;

	data = (uint32_t *)(pkt + sizeof(struct rpc_reply));
	nfserr = ntohl(net_read_uint32(data));

	debug("%s: state: %d, err %d\n", __func__, nfs_state, -nfserr);

	if (nfserr <= 30)
		/* These nfs codes correspond with those in errno.h */
		return -nfserr;
	if (nfserr == NFSERR_STALE)
		return -ESTALE;

	return -EINVAL;
}

static int rpc_lookup_reply(int prog, unsigned char *pkt, unsigned len)
{
	uint32_t port;
	int ret;

	ret = rpc_check_reply(pkt, 0);
	if (ret)
		return ret;

	port = net_read_uint32((uint32_t *)(pkt + sizeof(struct rpc_reply)));
	switch (prog) {
	case PROG_MOUNT:
		nfs_server_mount_port = ntohl(port);
		break;
	case PROG_NFS:
		nfs_server_nfs_port = ntohl(port);
		break;
	}

	return 0;
}

static int nfs_mount_reply(unsigned char *pkt, unsigned len)
{
	int ret;

	ret = rpc_check_reply(pkt, 1);
	if (ret)
		return ret;

	memcpy(dirfh, pkt + sizeof(struct rpc_reply) + 4, NFS_FHSIZE);

	return 0;
}

static int nfs_umountall_reply(unsigned char *pkt, unsigned len)
{
	int ret;

	ret = rpc_check_reply(pkt, 0);
	if (ret)
		return ret;

	memset(dirfh, 0, sizeof(dirfh));

	return 0;
}

static int nfs_lookup_reply(unsigned char *pkt, unsigned len)
{
	int ret;

	ret = rpc_check_reply(pkt, 1);
	if (ret)
		return ret;

	memcpy(filefh, pkt + sizeof(struct rpc_reply) + 4, NFS_FHSIZE);

	return 0;
}

static int nfs_readlink_reply(unsigned char *pkt, unsigned len)
{
	uint32_t *data;
	char *path;
	int rlen;
	int ret;

	ret = rpc_check_reply(pkt, 1);
	if (ret)
		return ret;

	data = (uint32_t *)(pkt + sizeof(struct rpc_reply));

	data++;

	rlen = ntohl(net_read_uint32(data)); /* new path length */

	data++;
	path = (char *)data;

	if (*path != '/') {
		strcat(nfs_path, "/");
		strncat(nfs_path, path, rlen);
	} else {
		memcpy(nfs_path, path, rlen);
		nfs_path[rlen] = 0;
	}
	return 0;
}

static int nfs_read_reply(unsigned char *pkt, unsigned len)
{
	int rlen;
	uint32_t *data;
	int ret;

	debug("%s\n", __func__);

	ret = rpc_check_reply(pkt, 1);
	if (ret)
		return ret;

	data = (uint32_t *)(pkt + sizeof(struct rpc_reply));

	if (!nfs_offset) {
		uint32_t filesize = ntohl(net_read_uint32(data + 6));
		init_progression_bar(filesize);
	}

	rlen = ntohl(net_read_uint32(data + 18));

	ret = write(net_store_fd, (char *)(data + 19), rlen);
	if (ret < 0) {
		perror("write");
		return ret;
	}

	return rlen;
}

/**************************************************************************
Interfaces of barebox
**************************************************************************/

static void nfs_handler(void *ctx, char *packet, unsigned len)
{
	char *pkt = net_eth_to_udp_payload(packet);
	int ret;

	debug("%s\n", __func__);

	switch (nfs_state) {
	case STATE_PRCLOOKUP_PROG_MOUNT_REQ:
		ret = rpc_lookup_reply(PROG_MOUNT, pkt, len);
		if (ret)
			goto err_out;
		nfs_state = STATE_PRCLOOKUP_PROG_NFS_REQ;
		break;

	case STATE_PRCLOOKUP_PROG_NFS_REQ:
		ret = rpc_lookup_reply(PROG_NFS, pkt, len);
		if (ret)
			goto err_out;
		nfs_state = STATE_MOUNT_REQ;
		break;

	case STATE_MOUNT_REQ:
		ret = nfs_mount_reply(pkt, len);
		if (ret)
			goto err_out;

		nfs_state = STATE_LOOKUP_REQ;
		break;

	case STATE_UMOUNT_REQ:
		ret = nfs_umountall_reply(pkt, len);
		if (ret)
			nfs_err = ret;
		nfs_state = STATE_DONE;
		return;

	case STATE_LOOKUP_REQ:
		ret = nfs_lookup_reply(pkt, len);
		if (ret)
			goto err_umount;

		nfs_state = STATE_READ_REQ;
		nfs_offset = 0;
		break;

	case STATE_READLINK_REQ:
		ret = nfs_readlink_reply(pkt, len);
		if (ret)
			goto err_umount;

		debug("Symlink --> %s\n", nfs_path);

		nfs_filename = basename(nfs_path);
		nfs_path     = dirname(nfs_path);

		nfs_state = STATE_MOUNT_REQ;
		break;

	case STATE_READ_REQ:
		ret = nfs_read_reply(pkt, len);
		nfs_timer_start = get_time_ns();
		if (ret > 0)
			nfs_offset += ret;
		else if (ret == -EISDIR || ret == -EINVAL)
			/* symbolic link */
			nfs_state = STATE_READLINK_REQ;
		else
			goto err_umount;
		show_progress(nfs_offset);
		break;
	}

	nfs_send();

	return;

err_umount:
	nfs_state = STATE_UMOUNT_REQ;
	nfs_err = ret;
	nfs_send();
	return;

err_out:
	nfs_state = STATE_DONE;
	nfs_err = ret;
}

static void nfs_start(char *p)
{
	debug("%s\n", __func__);

	nfs_path = (char *)nfs_path_buff;

	strcpy(nfs_path, p);

	nfs_filename = basename (nfs_path);
	nfs_path     = dirname (nfs_path);

	printf("\nFilename '%s/%s'.\n", nfs_path, nfs_filename);

	nfs_state = STATE_PRCLOOKUP_PROG_MOUNT_REQ;

	nfs_send();
}

static int do_nfs(int argc, char *argv[])
{
	char  *localfile;
	char  *remotefile;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	remotefile = argv[1];

	if (argc == 2)
		localfile = basename(remotefile);
	else
		localfile = argv[2];

	net_store_fd = open(localfile, O_WRONLY | O_CREAT);
	if (net_store_fd < 0) {
		perror("open");
		return 1;
	}

	nfs_con = net_udp_new(net_get_serverip(), 0, nfs_handler, NULL);
	if (IS_ERR(nfs_con)) {
		nfs_err = PTR_ERR(nfs_con);
		goto err_udp;
	}
	net_udp_bind(nfs_con, 1000);

	nfs_err = 0;

	nfs_start(remotefile);

	while (nfs_state != STATE_DONE) {
		if (ctrlc()) {
			nfs_err = -EINTR;
			break;
		}
		net_poll();
		if (is_timeout(nfs_timer_start, NFS_TIMEOUT * SECOND)) {
			show_progress(-1);
			nfs_send();
		}
	}

	net_unregister(nfs_con);
err_udp:
	close(net_store_fd);
	if (nfs_err) {
		printf("NFS failed: %s\n", strerror(-nfs_err));
		unlink(localfile);
	}

	printf("\n");

	return nfs_err == 0 ? 0 : 1;
}

static const __maybe_unused char cmd_nfs_help[] =
"Usage: nfs <file> [localfile]\n"
"Load a file via network using nfs protocol.\n";

BAREBOX_CMD_START(nfs)
	.cmd		= do_nfs,
	.usage		= "boot image via network using nfs protocol",
	BAREBOX_CMD_HELP(cmd_nfs_help)
BAREBOX_CMD_END

