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
#include "tftp.h"

#undef	ET_DEBUG

#define WELL_KNOWN_PORT	69		/* Well known TFTP port #		*/
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


static int	TftpServerPort;		/* The UDP port at their end		*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrap;		/* count of sequence number wraparounds */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpState;

#define STATE_RRQ	1
#define STATE_DATA	2
#define STATE_OACK	3

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/
#define TFTP_SEQUENCE_SIZE	((ulong)(1<<16))    /* sequence number is 16 bit */

static char *tftp_filename;

extern int net_store_fd;

static int store_block(unsigned block, uchar * src, unsigned len)
{
	ulong offset = block * TFTP_BLOCK_SIZE + TftpBlockWrapOffset;
	ulong newsize = offset + len;
	int ret;

	ret = write(net_store_fd, src, len);
	if (ret < 0)
		return ret;

	if (NetBootFileXferSize < newsize)
		NetBootFileXferSize = newsize;
	return 0;
}

static void TftpSend(void)
{
	uchar *pkt;
	uchar *xp;
	int len = 0;
	ushort *s;

	/*
	 *	We will always be sending some sort of packet, so
	 *	cobble together the packet headers now.
	 */
	pkt = NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE;

	switch (TftpState) {
	case STATE_RRQ:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_RRQ);
		pkt = (uchar *)s;
		pkt += sprintf((uchar *)pkt, "%s%coctet%ctimeout%c%d",
				tftp_filename, 0, 0, 0, TIMEOUT) + 1;
		len = pkt - xp;
		break;

	case STATE_DATA:
	case STATE_OACK:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar *)s;
		len = pkt - xp;
		break;
	}

	NetSendUDPPacket(NetServerEther, NetServerIP, TftpServerPort,
			TftpOurPort, len);
}

static void TftpTimeout(void)
{
	puts("T ");
	NetSetTimeout(TIMEOUT * SECOND, TftpTimeout);
	TftpSend();
}

static void TftpHandler(uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	ushort proto;
	ushort *s;

	if (dest != TftpOurPort)
		return;

	if (TftpState != STATE_RRQ && src != TftpServerPort)
		return;

	if (len < 2)
		return;

	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort *)pkt;
	proto = *s++;
	pkt = (uchar *)s;
	switch (ntohs(proto)) {
	case TFTP_RRQ:
	case TFTP_WRQ:
	case TFTP_ACK:
		break;
	default:
		break;

	case TFTP_OACK:
		debug("Got OACK: %s %s\n", pkt, pkt+strlen(pkt)+1);
		TftpState = STATE_OACK;
		TftpServerPort = src;
		TftpSend(); /* Send ACK */
		break;
	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);

		/*
		 * RFC1350 specifies that the first data packet will
		 * have sequence number 1. If we receive a sequence
		 * number of 0 this means that there was a wrap
		 * around of the (16 bit) counter.
		 */
		if (TftpBlock == 0) {
			TftpBlockWrap++;
			TftpBlockWrapOffset += TFTP_BLOCK_SIZE * TFTP_SEQUENCE_SIZE;
			printf ("\n\t %lu MB received\n\t ", TftpBlockWrapOffset>>20);
		} else {
			if (((TftpBlock - 1) % 10) == 0) {
				putchar('#');
			} else if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0) {
				puts("\n\t ");
			}
		}

		if (TftpState == STATE_RRQ)
			debug("Server did not acknowledge timeout option!\n");

		if (TftpState == STATE_RRQ || TftpState == STATE_OACK) {
			/* first block received */
			TftpState = STATE_DATA;
			TftpServerPort = src;
			TftpLastBlock = 0;
			TftpBlockWrap = 0;
			TftpBlockWrapOffset = 0;

			if (TftpBlock != 1) {	/* Assertion */
				printf("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				NetState = NETLOOP_FAIL;
				break;
			}
		}

		if (TftpBlock == TftpLastBlock)
			/* Same block again; ignore it. */
			break;

		TftpLastBlock = TftpBlock;
		NetSetTimeout(TIMEOUT * SECOND, TftpTimeout);

		if (store_block(TftpBlock - 1, pkt + 2, len) < 0) {
			perror("write");
			NetState = NETLOOP_FAIL;
			return;
		}

		/*
		 *	Acknowledge the block just received, which will prompt
		 *	the server for the next one.
		 */
		TftpSend();

		if (len < TFTP_BLOCK_SIZE) {
			/*
			 *	We received the whole thing.  Try to
			 *	run it.
			 */
			puts("\ndone\n");
			NetState = NETLOOP_SUCCESS;
		}
		break;

	case TFTP_ERROR:
		printf("\nTFTP error: '%s' (%d)\n",
					pkt + 2, ntohs(*(ushort *)pkt));
		NetState = NETLOOP_FAIL;
		break;
	}
}

void TftpStart(char *filename)
{
	char ip1[16], ip2[16];

	tftp_filename = filename;

	printf("TFTP from server %s; our IP address is %s\n"
			"\nFilename '%s'.\nLoading: *\b",
			ip_to_string(NetServerIP, ip1),
			ip_to_string(NetOurIP, ip2),
			tftp_filename);

	NetSetTimeout(TIMEOUT * SECOND, TftpTimeout);
	NetSetHandler(TftpHandler);

	TftpServerPort = WELL_KNOWN_PORT;
	TftpState = STATE_RRQ;
	/* Use a pseudo-random port */
	TftpOurPort = 1024 + ((unsigned int)get_time_ns() % 3072);
	TftpBlock = 0;

	/* zero out server ether in case the server ip has changed */
	memset(NetServerEther, 0, 6);

	TftpSend();
}
