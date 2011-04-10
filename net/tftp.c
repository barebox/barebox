/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <driver.h>
#include <clock.h>
#include <fs.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <progress.h>
#include <getopt.h>
#include <fs.h>
#include <linux/stat.h>
#include <linux/err.h>

#define TFTP_PORT	69		/* Well known TFTP port #		*/
#define TIMEOUT		5		/* Seconds to timeout for a lost pkt	*/

/*
 *	TFTP operations.
 */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6


static int		tftp_server_port;	/* The UDP port at their end		*/
static unsigned int	tftp_block;		/* packet sequence number		*/
static unsigned int	tftp_last_block;	/* last packet sequence number received */
static int		tftp_state;
static uint64_t		tftp_timer_start;
static int		tftp_err;

#define STATE_RRQ	1
#define STATE_WRQ	2
#define STATE_RDATA	3
#define STATE_WDATA	4
#define STATE_OACK	5
#define STATE_LAST	6
#define STATE_DONE	7

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/

static char *tftp_filename;
static struct net_connection *tftp_con;
static int tftp_fd;
static int tftp_size;

#ifdef CONFIG_NET_TFTP_PUSH
static int tftp_push;

static inline void do_tftp_push(int push)
{
	tftp_push = push;
}

#else

#define tftp_push	0

static inline void do_tftp_push(int push)
{
}
#endif

static int tftp_send(void)
{
	unsigned char *xp;
	int len = 0;
	uint16_t *s;
	unsigned char *pkt = net_udp_get_payload(tftp_con);
	int ret;
	static int last_len;

	switch (tftp_state) {
	case STATE_RRQ:
	case STATE_WRQ:
		xp = pkt;
		s = (uint16_t *)pkt;
		if (tftp_state == STATE_RRQ)
			*s++ = htons(TFTP_RRQ);
		else
			*s++ = htons(TFTP_WRQ);
		pkt = (unsigned char *)s;
		pkt += sprintf((unsigned char *)pkt, "%s%coctet%ctimeout%c%d",
				tftp_filename, 0, 0, 0, TIMEOUT) + 1;
		len = pkt - xp;
		break;

	case STATE_WDATA:
		if (!tftp_push)
			break;

		if (tftp_last_block == tftp_block) {
			len = last_len;
			break;
		}

		tftp_last_block = tftp_block;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_DATA);
		*s++ = htons(tftp_block);
		len = read(tftp_fd, s, 512);
		if (len < 0) {
			perror("read");
			tftp_err = -errno;
			tftp_state = STATE_DONE;
			return tftp_err;
		}
		tftp_size += len;
		if (len < 512)
			tftp_state = STATE_LAST;
		len += 4;
		last_len = len;
		break;

	case STATE_RDATA:
	case STATE_OACK:
		xp = pkt;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(tftp_block);
		pkt = (unsigned char *)s;
		len = pkt - xp;
		break;
	}

	tftp_timer_start = get_time_ns();
	show_progress(tftp_size);
	ret = net_udp_send(tftp_con, len);

	return ret;
}

static void tftp_handler(void *ctx, char *packet, unsigned len)
{
	uint16_t proto;
	uint16_t *s;
	char *pkt = net_eth_to_udp_payload(packet);
	struct udphdr *udp = net_eth_to_udphdr(packet);
	int ret;

	len = net_eth_to_udplen(packet);
	if (len < 2)
		return;

	len -= 2;

	s = (uint16_t *)pkt;
	proto = *s++;
	pkt = (unsigned char *)s;

	switch (ntohs(proto)) {
	case TFTP_RRQ:
	case TFTP_WRQ:
	default:
		break;
	case TFTP_ACK:
		if (!tftp_push)
			break;

		tftp_block = ntohs(*(uint16_t *)pkt);
		if (tftp_block != tftp_last_block) {
			debug("ack %d != %d\n", tftp_block, tftp_last_block);
			break;
		}
		tftp_block++;
		if (tftp_state == STATE_LAST) {
			tftp_state = STATE_DONE;
			break;
		}
		tftp_con->udp->uh_dport = udp->uh_sport;
		tftp_state = STATE_WDATA;
		tftp_send();
		break;

	case TFTP_OACK:
		debug("Got OACK: %s %s\n", pkt, pkt + strlen(pkt) + 1);
		tftp_server_port = ntohs(udp->uh_sport);
		tftp_con->udp->uh_dport = udp->uh_sport;

		if (tftp_push) {
			/* send first block */
			tftp_state = STATE_WDATA;
			tftp_block = 1;
		} else {
			/* send ACK */
			tftp_state = STATE_OACK;
			tftp_block = 0;
		}

		tftp_send();

		break;
	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		tftp_block = ntohs(*(uint16_t *)pkt);

		if (tftp_state == STATE_RRQ)
			debug("Server did not acknowledge timeout option!\n");

		if (tftp_state == STATE_RRQ || tftp_state == STATE_OACK) {
			/* first block received */
			tftp_state = STATE_RDATA;
			tftp_con->udp->uh_dport = udp->uh_sport;
			tftp_server_port = ntohs(udp->uh_sport);
			tftp_last_block = 0;

			if (tftp_block != 1) {	/* Assertion */
				printf("error: First block is not block 1 (%d)\n",
					tftp_block);
				tftp_err = -EINVAL;
				tftp_state = STATE_DONE;
				break;
			}
		}

		if (tftp_block == tftp_last_block)
			/* Same block again; ignore it. */
			break;

		tftp_last_block = tftp_block;

		if (!(tftp_block % 10))
			tftp_size++;

		ret = write(tftp_fd, pkt + 2, len);
		if (ret < 0) {
			perror("write");
			tftp_err = -errno;
			tftp_state = STATE_DONE;
			return;
		}

		/*
		 *	Acknowledge the block just received, which will prompt
		 *	the server for the next one.
		 */
		tftp_send();

		if (len < TFTP_BLOCK_SIZE)
			tftp_state = STATE_DONE;

		break;

	case TFTP_ERROR:
		debug("\nTFTP error: '%s' (%d)\n",
					pkt + 2, ntohs(*(uint16_t *)pkt));
		switch (ntohs(*(uint16_t *)pkt)) {
		case 1: tftp_err = -ENOENT; break;
		case 2: tftp_err = -EACCES; break;
		default: tftp_err = -EINVAL; break;
		}
		tftp_state = STATE_DONE;
		break;
	}
}

static int do_tftpb(struct command *cmdtp, int argc, char *argv[])
{
	char *localfile, *remotefile, *file1, *file2;
	char ip1[16];
	int opt;
	struct stat s;
	unsigned long flags;

	do_tftp_push(0);
	tftp_last_block = 0;
	tftp_size = 0;

	while((opt = getopt(argc, argv, "p")) > 0) {
		switch(opt) {
		case 'p':
			do_tftp_push(1);
			break;
		}
	}

	if (argc <= optind)
		return COMMAND_ERROR_USAGE;

	file1 = argv[optind++];

	if (argc == optind)
		file2 = basename(file1);
	else
		file2 = argv[optind];

	if (tftp_push) {
		localfile = file1;
		remotefile = file2;
		stat(localfile, &s);
		flags = O_RDONLY;
	} else {
		localfile = file2;
		remotefile = file1;
		flags = O_WRONLY | O_CREAT;
	}

	tftp_fd = open(localfile, flags);
	if (tftp_fd < 0) {
		perror("open");
		return 1;
	}

	tftp_con = net_udp_new(net_get_serverip(), TFTP_PORT, tftp_handler, NULL);
	if (IS_ERR(tftp_con)) {
		tftp_err = PTR_ERR(tftp_con);
		goto out_close;
	}

	tftp_filename = remotefile;

	printf("TFTP %s server %s ('%s' -> '%s')\n",
			tftp_push ? "to" : "from",
			ip_to_string(net_get_serverip(), ip1),
			file1, file2);

	init_progression_bar(tftp_push ? s.st_size : 0);

	tftp_timer_start = get_time_ns();
	tftp_state = tftp_push ? STATE_WRQ : STATE_RRQ;
	tftp_block = 1;

	tftp_err = tftp_send();
	if (tftp_err)
		goto out_unreg;

	while (tftp_state != STATE_DONE) {
		if (ctrlc()) {
			tftp_err = -EINTR;
			break;
		}
		net_poll();
		if (is_timeout(tftp_timer_start, SECOND)) {
			show_progress(-1);
			tftp_send();
		}
	}
out_unreg:
	net_unregister(tftp_con);
out_close:
	close(tftp_fd);

	if (tftp_err) {
		printf("\ntftp failed: %s\n", strerror(-tftp_err));
		if (!tftp_push)
			unlink(localfile);
	}

	printf("\n");

	return tftp_err == 0 ? 0 : 1;
}

BAREBOX_CMD_HELP_START(tftp)
#ifdef CONFIG_NET_TFTP_PUSH
BAREBOX_CMD_HELP_USAGE("tftp <remotefile> [localfile], tftp -p <localfile> [remotefile]\n")
BAREBOX_CMD_HELP_SHORT("Load a file from or upload to TFTP server.\n")
BAREBOX_CMD_HELP_END
#else
BAREBOX_CMD_HELP_USAGE("tftp <remotefile> [localfile]\n")
BAREBOX_CMD_HELP_SHORT("Load a file from a TFTP server.\n")
BAREBOX_CMD_HELP_END
#endif

/**
 * @page tftp_command

The second file argument can be skipped in which case the first filename
is used (without the directory part).

\<localfile> can be the local filename or a device file under /dev.
This also works for flash memory. Refer to \ref erase_command and \ref
unprotect_command for flash preparation.

\note This command is available only if enabled in menuconfig.
 */

BAREBOX_CMD_START(tftp)
	.cmd		= do_tftpb,
	.usage		=
#ifdef CONFIG_NET_TFTP_PUSH
			"(up-)"
#endif
			"Load file using tftp protocol",
	BAREBOX_CMD_HELP(cmd_tftp_help)
BAREBOX_CMD_END

