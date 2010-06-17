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
#include <linux/err.h>

#define TFTP_PORT	69		/* Well known TFTP port #		*/
#define TIMEOUT		5		/* Seconds to timeout for a lost pkt	*/
# define TIMEOUT_COUNT	10		/* # of timeouts before giving up  */
					/* (for checking the image size)	*/
#define HASHES_PER_LINE	65		/* Number of "loading" hashes per line	*/

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
static unsigned int	tftp_block_wrap;	/* count of sequence number wraparounds */
static unsigned int	tftp_block_wrap_offset;	/* memory offset due to wrapping	*/
static int		tftp_state;
static uint64_t		tftp_timer_start;
static int		tftp_err;

#define STATE_RRQ	1
#define STATE_DATA	2
#define STATE_OACK	3
#define STATE_DONE	4

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/
#define TFTP_SEQUENCE_SIZE	((unsigned long)(1<<16))    /* sequence number is 16 bit */

static char *tftp_filename;
static struct net_connection *tftp_con;
static int net_store_fd;

static int tftp_send(void)
{
	unsigned char *pkt;
	unsigned char *xp;
	int len = 0;
	uint16_t *s;
	unsigned char *packet = net_udp_get_payload(tftp_con);
	int ret;

	pkt = packet;

	switch (tftp_state) {
	case STATE_RRQ:
		xp = pkt;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_RRQ);
		pkt = (unsigned char *)s;
		pkt += sprintf((unsigned char *)pkt, "%s%coctet%ctimeout%c%d",
				tftp_filename, 0, 0, 0, TIMEOUT) + 1;
		len = pkt - xp;
		break;

	case STATE_DATA:
	case STATE_OACK:
		xp = pkt;
		s = (uint16_t *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(tftp_block);
		pkt = (unsigned char *)s;
		len = pkt - xp;
		break;
	}

	ret = net_udp_send(tftp_con, len);

	return ret;
}

static void tftp_handler(char *packet, unsigned len)
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
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (uint16_t *)pkt;
	proto = *s++;
	pkt = (unsigned char *)s;
	switch (ntohs(proto)) {
	case TFTP_RRQ:
	case TFTP_WRQ:
	case TFTP_ACK:
		break;
	default:
		break;

	case TFTP_OACK:
		debug("Got OACK: %s %s\n", pkt, pkt + strlen(pkt) + 1);
		tftp_state = STATE_OACK;
		tftp_server_port = ntohs(udp->uh_sport);
		tftp_con->udp->uh_dport = udp->uh_sport;
		tftp_send(); /* Send ACK */
		break;
	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		tftp_block = ntohs(*(uint16_t *)pkt);

		/*
		 * RFC1350 specifies that the first data packet will
		 * have sequence number 1. If we receive a sequence
		 * number of 0 this means that there was a wrap
		 * around of the (16 bit) counter.
		 */
		if (tftp_block == 0) {
			tftp_block_wrap++;
			tftp_block_wrap_offset += TFTP_BLOCK_SIZE * TFTP_SEQUENCE_SIZE;
		} else {
			if (((tftp_block - 1) % 10) == 0) {
				putchar('#');
			} else if ((tftp_block % (10 * HASHES_PER_LINE)) == 0) {
				puts("\n\t ");
			}
		}

		if (tftp_state == STATE_RRQ)
			debug("Server did not acknowledge timeout option!\n");

		if (tftp_state == STATE_RRQ || tftp_state == STATE_OACK) {
			/* first block received */
			tftp_state = STATE_DATA;
			tftp_con->udp->uh_dport = udp->uh_sport;
			tftp_server_port = ntohs(udp->uh_sport);
			tftp_last_block = 0;
			tftp_block_wrap = 0;
			tftp_block_wrap_offset = 0;

			if (tftp_block != 1) {	/* Assertion */
				printf("error: First block is not block 1 (%ld)\n",
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
		tftp_timer_start = get_time_ns();

		ret = write(net_store_fd, pkt + 2, len);
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
	char *localfile;
	char *remotefile;
	char ip1[16];

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

	tftp_con = net_udp_new(net_get_serverip(), TFTP_PORT, tftp_handler);
	if (IS_ERR(tftp_con)) {
		tftp_err = PTR_ERR(tftp_con);
		goto out_close;
	}

	tftp_filename = remotefile;

	printf("TFTP from server %s; Filename: '%s'\nLoading: ",
			ip_to_string(net_get_serverip(), ip1),
			tftp_filename);

	tftp_timer_start = get_time_ns();
	tftp_state = STATE_RRQ;
	tftp_block = 0;

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
			tftp_timer_start = get_time_ns();
			printf("T ");
			tftp_send();
		}
	}
out_unreg:
	net_unregister(tftp_con);
out_close:
	close(net_store_fd);

	if (tftp_err) {
		printf("\ntftp failed: %s\n", strerror(-tftp_err));
		unlink(localfile);
	}

	printf("\n");

	return tftp_err == 0 ? 0 : 1;
}

static const __maybe_unused char cmd_tftp_help[] =
"Usage: tftp <file> [localfile]\n"
"Load a file via network using BootP/TFTP protocol.\n";

BAREBOX_CMD_START(tftp)
	.cmd		= do_tftpb,
	.usage		= "Load file using tftp protocol",
	BAREBOX_CMD_HELP(cmd_tftp_help)
BAREBOX_CMD_END

/**
 * @page tftp_command tftp
 *
 * Usage is: tftp \<filename\> [\<localfilename\>]
 *
 * Load a file via network using BootP/TFTP protocol. The loaded file you
 * can find after download in you current ramdisk. Refer \b ls command.
 *
 * \<localfile> can be the local filename only, or also a device name. In the
 * case of a device name, the will gets stored there. This works also for
 * partitions of flash memory. Refer \b erase, \b unprotect for flash
 * preparation.
 *
 * Note: This command is available only, if enabled in the menuconfig.
 */

