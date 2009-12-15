/*
 *	Based on LiMon - BOOTP.
 *
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000 Roland Borde
 *	Copyright 2000 Paolo Scaffardi
 *	Copyright 2000-2004 Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <clock.h>
#include <net.h>
#include <libbb.h>
#include "tftp.h"
#include "nfs.h"

#define OPT_SIZE 312	/* Minimum DHCP Options size per RFC2131 - results in 576 byte pkt */

typedef struct
{
	uchar		bp_op;		/* Operation				*/
# define OP_BOOTREQUEST	1
# define OP_BOOTREPLY	2
	uchar		bp_htype;	/* Hardware type			*/
# define HWT_ETHER	1
	uchar		bp_hlen;	/* Hardware address length		*/
# define HWL_ETHER	6
	uchar		bp_hops;	/* Hop count (gateway thing)		*/
	ulong		bp_id;		/* Transaction ID			*/
	ushort		bp_secs;	/* Seconds since boot			*/
	ushort		bp_spare1;	/* Alignment				*/
	IPaddr_t	bp_ciaddr;	/* Client IP address			*/
	IPaddr_t	bp_yiaddr;	/* Your (client) IP address		*/
	IPaddr_t	bp_siaddr;	/* Server IP address			*/
	IPaddr_t	bp_giaddr;	/* Gateway IP address			*/
	uchar		bp_chaddr[16];	/* Client hardware address		*/
	char		bp_sname[64];	/* Server host name			*/
	char		bp_file[128];	/* Boot file name			*/
	char		bp_vend[OPT_SIZE];	/* Vendor information			*/
}	Bootp_t;

#define BOOTP_HDR_SIZE	sizeof (Bootp_t)
#define BOOTP_SIZE	(ETHER_HDR_SIZE + IP_HDR_SIZE + BOOTP_HDR_SIZE)

/**********************************************************************/
/*
 *	Global functions and variables.
 */

/* bootp.c */
extern ulong	BootpID;		/* ID of cur BOOTP request		*/
extern char	BootFile[128];		/* Boot file name			*/
#ifdef CONFIG_BOOTP_RANDOM_DELAY
ulong		seed1, seed2;		/* seed for random BOOTP delay		*/
#endif


/* Send a BOOTP request */
extern void	BootpRequest (void);

/****************** DHCP Support *********************/

/* DHCP States */
typedef enum { INIT,
	       INIT_REBOOT,
	       REBOOTING,
	       SELECTING,
	       REQUESTING,
	       REBINDING,
	       BOUND,
	       RENEWING } dhcp_state_t;

#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7

#define SELECT_TIMEOUT 3	/* Seconds to wait for offers */

/**********************************************************************/

#define BOOTP_VENDOR_MAGIC	0x63825363	/* RFC1048 Magic Cookie		*/

#define TIMEOUT		5		/* Seconds before trying BOOTP again	*/
#ifndef CONFIG_NET_RETRY_COUNT
# define TIMEOUT_COUNT	5		/* # of timeouts before giving up  */
#else
# define TIMEOUT_COUNT	(CONFIG_NET_RETRY_COUNT)
#endif

#define PORT_BOOTPS	67		/* BOOTP server UDP port		*/
#define PORT_BOOTPC	68		/* BOOTP client UDP port		*/

#ifndef CONFIG_DHCP_MIN_EXT_LEN		/* minimal length of extension list	*/
#define CONFIG_DHCP_MIN_EXT_LEN 64
#endif

ulong		BootpID;
#ifdef CONFIG_BOOTP_RANDOM_DELAY
ulong		seed1, seed2;
#endif

dhcp_state_t dhcp_state = INIT;
unsigned long dhcp_leasetime = 0;
IPaddr_t NetDHCPServerIP = 0;
static void DhcpHandler(uchar * pkt, unsigned dest, unsigned src, unsigned len);

static int BootpCheckPkt(uchar *pkt, unsigned dest, unsigned src, unsigned len)
{
	Bootp_t *bp = (Bootp_t *) pkt;
	int retval = 0;

	if (dest != PORT_BOOTPC || src != PORT_BOOTPS)
		retval = -1;
	else if (len < sizeof (Bootp_t) - OPT_SIZE)
		retval = -2;
	else if (bp->bp_op != OP_BOOTREQUEST &&
	    bp->bp_op != OP_BOOTREPLY &&
	    bp->bp_op != DHCP_OFFER &&
	    bp->bp_op != DHCP_ACK &&
	    bp->bp_op != DHCP_NAK ) {
		retval = -3;
	}
	else if (bp->bp_htype != HWT_ETHER)
		retval = -4;
	else if (bp->bp_hlen != HWL_ETHER)
		retval = -5;
	else if (NetReadLong((ulong*)&bp->bp_id) != BootpID) {
		retval = -6;
	}

	debug ("Filtering pkt = %d\n", retval);

	return retval;
}

/*
 * Copy parameters of interest from BOOTP_REPLY/DHCP_OFFER packet
 */
static void BootpCopyNetParams(Bootp_t *bp)
{
	IPaddr_t tmp_ip;

	NetCopyIP(&NetOurIP, &bp->bp_yiaddr);
	NetCopyIP(&tmp_ip, &bp->bp_siaddr);
	if (tmp_ip != 0)
		NetCopyIP(&NetServerIP, &bp->bp_siaddr);
	memcpy (NetServerEther, ((Ethernet_t *)NetRxPkt)->et_src, 6);
	if (strlen(bp->bp_file) > 0)
		safe_strncpy (BootFile, bp->bp_file, sizeof(BootFile));

	debug ("Bootfile: %s\n", BootFile);

	/* Propagate to environment:
	 * don't delete exising entry when BOOTP / DHCP reply does
	 * not contain a new value
	 */
	if (*BootFile) {
		setenv ("bootfile", BootFile);
	}
}

static int truncate_sz (const char *name, int maxlen, int curlen)
{
	if (curlen >= maxlen) {
		printf("*** WARNING: %s is too long (%d - max: %d) - truncated\n",
			name, curlen, maxlen);
		curlen = maxlen - 1;
	}
	return (curlen);
}

/*
 *	Timeout on BOOTP/DHCP request.
 */
static void
BootpTimeout(void)
{
	NetSetTimeout (TIMEOUT * SECOND, BootpTimeout);
	BootpRequest ();
}

/*
 *	Initialize BOOTP extension fields in the request.
 */
static int DhcpExtended (u8 * e, int message_type, IPaddr_t ServerID, IPaddr_t RequestedIP)
{
	u8 *start = e;
	u8 *cnt;

#ifdef CONFIG_BOOTP_VENDOREX
	u8 *x;
#endif
#ifdef CONFIG_BOOTP_SEND_HOSTNAME
	char *hostname;
#endif

	*e++ = 99;		/* RFC1048 Magic Cookie */
	*e++ = 130;
	*e++ = 83;
	*e++ = 99;

	*e++ = 53;		/* DHCP Message Type */
	*e++ = 1;
	*e++ = message_type;

	*e++ = 57;		/* Maximum DHCP Message Size */
	*e++ = 2;
	*e++ = (576 - 312 + OPT_SIZE) >> 8;
	*e++ = (576 - 312 + OPT_SIZE) & 0xff;

	if (ServerID) {
		int tmp = ntohl (ServerID);

		*e++ = 54;	/* ServerID */
		*e++ = 4;
		*e++ = tmp >> 24;
		*e++ = tmp >> 16;
		*e++ = tmp >> 8;
		*e++ = tmp & 0xff;
	}

	if (RequestedIP) {
		int tmp = ntohl (RequestedIP);

		*e++ = 50;	/* Requested IP */
		*e++ = 4;
		*e++ = tmp >> 24;
		*e++ = tmp >> 16;
		*e++ = tmp >> 8;
		*e++ = tmp & 0xff;
	}
#ifdef CONFIG_BOOTP_SEND_HOSTNAME
	if ((hostname = getenv ("hostname"))) {
		int hostnamelen = strlen (hostname);

		*e++ = 12;	/* Hostname */
		*e++ = hostnamelen;
		memcpy (e, hostname, hostnamelen);
		e += hostnamelen;
	}
#endif

	*e++ = 55;		/* Parameter Request List */
	 cnt = e++;		/* Pointer to count of requested items */
	*cnt = 0;
#ifdef CONFIG_BOOTP_SUBNETMASK
	*e++  = 1;		/* Subnet Mask */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_TIMEOFFSET
	*e++  = 2;
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_GATEWAY
	*e++  = 3;		/* Router Option */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_DNS
	*e++  = 6;		/* DNS Server(s) */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_HOSTNAME
	*e++  = 12;		/* Hostname */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_BOOTFILESIZE
	*e++  = 13;		/* Boot File Size */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_BOOTPATH
	*e++  = 17;		/* Boot path */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_NISDOMAIN
	*e++  = 40;		/* NIS Domain name request */
	*cnt += 1;
#endif
#ifdef CONFIG_BOOTP_NTPSERVER
	*e++  = 42;
	*cnt += 1;
#endif
	*e++  = 255;		/* End of the list */

	/* Pad to minimal length */
#ifdef	CONFIG_DHCP_MIN_EXT_LEN
	while ((e - start) <= CONFIG_DHCP_MIN_EXT_LEN)
		*e++ = 0;
#endif

	return e - start;
}

void
BootpRequest (void)
{
	uchar *pkt, *iphdr;
	Bootp_t *bp;
	int ext_len, pktlen, iplen;

	dhcp_state = INIT;

	printf("BOOTP broadcast\n");
	pkt = NetTxPacket;
	memset ((void*)pkt, 0, PKTSIZE);

	pkt += NetSetEther(pkt, NetBcastAddr, PROT_IP);

	/*
	 * Next line results in incorrect packet size being transmitted, resulting
	 * in errors in some DHCP servers, reporting missing bytes.  Size must be
	 * set in packet header after extension length has been determined.
	 * C. Hallinan, DS4.COM, Inc.
	 */
	/* NetSetIP(pkt, 0xFFFFFFFFL, PORT_BOOTPS, PORT_BOOTPC, sizeof (Bootp_t)); */
	iphdr = pkt;	/* We need this later for NetSetIP() */
	pkt += IP_HDR_SIZE;

	bp = (Bootp_t *)pkt;
	bp->bp_op = OP_BOOTREQUEST;
	bp->bp_htype = HWT_ETHER;
	bp->bp_hlen = HWL_ETHER;
	bp->bp_hops = 0;
	bp->bp_secs = htons(get_time_ns() >> 30);
	NetWriteIP(&bp->bp_ciaddr, 0);
	NetWriteIP(&bp->bp_yiaddr, 0);
	NetWriteIP(&bp->bp_siaddr, 0);
	NetWriteIP(&bp->bp_giaddr, 0);
	memcpy (bp->bp_chaddr, NetOurEther, 6);
	safe_strncpy (bp->bp_file, BootFile, sizeof(bp->bp_file));

	/* Request additional information from the BOOTP/DHCP server */
	ext_len = DhcpExtended((u8 *)bp->bp_vend, DHCP_DISCOVER, 0, 0);

	/*
	 *	Bootp ID is the lower 4 bytes of our ethernet address
	 *	plus the current time in HZ.
	 */
	BootpID = ((ulong)NetOurEther[2] << 24)
		| ((ulong)NetOurEther[3] << 16)
		| ((ulong)NetOurEther[4] << 8)
		| (ulong)NetOurEther[5];
	BootpID += (uint32_t)get_time_ns();
	BootpID	 = htonl(BootpID);
	NetCopyLong(&bp->bp_id, &BootpID);

	/*
	 * Calculate proper packet lengths taking into account the
	 * variable size of the options field
	 */
	pktlen = BOOTP_SIZE - sizeof(bp->bp_vend) + ext_len;
	iplen = BOOTP_HDR_SIZE - sizeof(bp->bp_vend) + ext_len;
	NetSetIP(iphdr, 0xFFFFFFFFL, PORT_BOOTPS, PORT_BOOTPC, iplen);
	NetSetTimeout(SELECT_TIMEOUT * SECOND, BootpTimeout);

	dhcp_state = SELECTING;
	NetSetHandler(DhcpHandler);
	NetSendPacket(NetTxPacket, pktlen);
}

static void DhcpOptionsProcess (uchar * popt, Bootp_t *bp)
{
	uchar *end = popt + BOOTP_HDR_SIZE;
	int oplen, size;

	while (popt < end && *popt != 0xff) {
		oplen = *(popt + 1);
		switch (*popt) {
		case 1:
			NetCopyIP (&NetOurSubnetMask, (popt + 2));
			break;
		case 3:
			NetCopyIP (&NetOurGatewayIP, (popt + 2));
			break;
		case 6:
			NetCopyIP (&NetOurDNSIP, (popt + 2));
#ifdef CONFIG_BOOTP_DNS2
			if (*(popt + 1) > 4) {
				NetCopyIP (&NetOurDNS2IP, (popt + 2 + 4));
			}
#endif
			break;
		case 12:
			size = truncate_sz ("Host Name", sizeof (NetOurHostName), oplen);
			memcpy (&NetOurHostName, popt + 2, size);
			NetOurHostName[size] = 0;
			break;
		case 15:	/* Ignore Domain Name Option */
			break;
		case 17:
			size = truncate_sz ("Root Path", sizeof (NetOurRootPath), oplen);
			memcpy (&NetOurRootPath, popt + 2, size);
			NetOurRootPath[size] = 0;
			break;
		case 51:
			NetCopyLong (&dhcp_leasetime, (ulong *) (popt + 2));
			break;
		case 53:	/* Ignore Message Type Option */
			break;
		case 54:
			NetCopyIP (&NetDHCPServerIP, (popt + 2));
			break;
		case 58:	/* Ignore Renewal Time Option */
			break;
		case 59:	/* Ignore Rebinding Time Option */
			break;
		case 66:	/* Ignore TFTP server name */
			break;
		case 67:	/* vendor opt bootfile */
			/*
			 * I can't use dhcp_vendorex_proc here because I need
			 * to write into the bootp packet - even then I had to
			 * pass the bootp packet pointer into here as the
			 * second arg
			 */
			size = truncate_sz ("Opt Boot File",
					    sizeof(bp->bp_file),
					    oplen);
			if (bp->bp_file[0] == '\0' && size > 0) {
				/*
				 * only use vendor boot file if we didn't
				 * receive a boot file in the main non-vendor
				 * part of the packet - god only knows why
				 * some vendors chose not to use this perfectly
				 * good spot to store the boot file (join on
				 * Tru64 Unix) it seems mind bogglingly crazy
				 * to me
				 */
				printf("*** WARNING: using vendor "
					"optional boot file\n");
				memcpy(bp->bp_file, popt + 2, size);
				bp->bp_file[size] = '\0';
			}
			break;
		default:
#ifdef CONFIG_BOOTP_VENDOREX
			if (dhcp_vendorex_proc (popt))
				break;
#endif
			printf ("*** Unhandled DHCP Option in OFFER/ACK: %d\n", *popt);
			break;
		}
		popt += oplen + 2;	/* Process next option */
	}
}

static int DhcpMessageType(unsigned char *popt)
{
	if (NetReadLong((ulong*)popt) != htonl(BOOTP_VENDOR_MAGIC))
		return -1;

	popt += 4;
	while ( *popt != 0xff ) {
		if ( *popt == 53 )	/* DHCP Message Type */
			return *(popt + 2);
		popt += *(popt + 1) + 2;	/* Scan through all options */
	}
	return -1;
}

static void DhcpSendRequestPkt(Bootp_t *bp_offer)
{
	uchar *pkt, *iphdr;
	Bootp_t *bp;
	int pktlen, iplen, extlen;
	IPaddr_t OfferedIP;

	debug ("DhcpSendRequestPkt: Sending DHCPREQUEST\n");
	pkt = NetTxPacket;
	memset ((void*)pkt, 0, PKTSIZE);

	pkt += NetSetEther(pkt, NetBcastAddr, PROT_IP);

	iphdr = pkt;		/* We'll need this later to set proper pkt size */
	pkt += IP_HDR_SIZE;

	bp = (Bootp_t *)pkt;
	bp->bp_op = OP_BOOTREQUEST;
	bp->bp_htype = HWT_ETHER;
	bp->bp_hlen = HWL_ETHER;
	bp->bp_hops = 0;
	/* FIXME what is this? */
//	bp->bp_secs = htons(get_timer(0) / CFG_HZ);
	NetCopyIP(&bp->bp_ciaddr, &bp_offer->bp_ciaddr); /* both in network byte order */
	NetCopyIP(&bp->bp_yiaddr, &bp_offer->bp_yiaddr);
	NetCopyIP(&bp->bp_siaddr, &bp_offer->bp_siaddr);
	/*
	 * RFC3046 requires Relay Agents to discard packets with
	 * nonzero and offered giaddr
	 */
	NetWriteIP(&bp->bp_giaddr, 0);

	memcpy (bp->bp_chaddr, NetOurEther, 6);

	/*
	 * ID is the id of the OFFER packet
	 */

	NetCopyLong(&bp->bp_id, &bp_offer->bp_id);

	/*
	 * Copy options from OFFER packet if present
	 */
	NetCopyIP(&OfferedIP, &bp->bp_yiaddr);
	extlen = DhcpExtended((u8 *)bp->bp_vend, DHCP_REQUEST, NetDHCPServerIP, OfferedIP);

	pktlen = BOOTP_SIZE - sizeof(bp->bp_vend) + extlen;
	iplen = BOOTP_HDR_SIZE - sizeof(bp->bp_vend) + extlen;
	NetSetIP(iphdr, 0xFFFFFFFFL, PORT_BOOTPS, PORT_BOOTPC, iplen);

	debug ("Transmitting DHCPREQUEST packet: len = %d\n", pktlen);
	NetSendPacket(NetTxPacket, pktlen);
}

/*
 *	Handle DHCP received packets.
 */
static void
DhcpHandler(uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	Bootp_t *bp = (Bootp_t *)pkt;

	debug ("DHCPHandler: got packet: (src=%d, dst=%d, len=%d) state: %d\n",
		src, dest, len, dhcp_state);

	if (BootpCheckPkt(pkt, dest, src, len)) /* Filter out pkts we don't want */
		return;

	debug ("DHCPHandler: got DHCP packet: (src=%d, dst=%d, len=%d) state: %d\n",
		src, dest, len, dhcp_state);

	switch (dhcp_state) {
	case SELECTING:
		/*
		 * Wait an appropriate time for any potential DHCPOFFER packets
		 * to arrive.  Then select one, and generate DHCPREQUEST response.
		 * If filename is in format we recognize, assume it is a valid
		 * OFFER from a server we want.
		 */
		debug ("DHCP: state=SELECTING bp_file: \"%s\"\n", bp->bp_file);
#ifdef CFG_BOOTFILE_PREFIX
		if (strncmp(bp->bp_file,
			    CFG_BOOTFILE_PREFIX,
			    strlen(CFG_BOOTFILE_PREFIX)) == 0 ) {
#endif	/* CFG_BOOTFILE_PREFIX */

			debug ("TRANSITIONING TO REQUESTING STATE\n");
			dhcp_state = REQUESTING;

			if (NetReadLong((ulong*)&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
				DhcpOptionsProcess((u8 *)&bp->bp_vend[4], bp);

			BootpCopyNetParams(bp); /* Store net params from reply */

			NetSetTimeout(TIMEOUT * SECOND, BootpTimeout);
			DhcpSendRequestPkt(bp);
#ifdef CFG_BOOTFILE_PREFIX
		}
#endif	/* CFG_BOOTFILE_PREFIX */

		return;
		break;
	case REQUESTING:
		debug ("DHCP State: REQUESTING\n");

		if ( DhcpMessageType((u8 *)bp->bp_vend) == DHCP_ACK ) {
			if (NetReadLong((ulong*)&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
				DhcpOptionsProcess((u8 *)&bp->bp_vend[4], bp);
			BootpCopyNetParams(bp); /* Store net params from reply */
			dhcp_state = BOUND;
			puts ("DHCP client bound to address ");
			print_IPaddr(NetOurIP);
			putchar('\n');

			NetState = NETLOOP_SUCCESS;
			return;
		}
		break;
	default:
		puts ("DHCP: INVALID STATE\n");
		break;
	}

}

static int do_dhcp (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int size;

	if (NetLoopInit(DHCP) < 0)
		return 1;

	NetOurIP = 0;
	BootpRequest();		/* Basically same as BOOTP */

	if ((size = NetLoop()) < 0)
		return 1;

	/* NetLoop ok, update environment */
	netboot_update_env();

	return 0;
}

BAREBOX_CMD_START(dhcp)
	.cmd		= do_dhcp,
	.usage		= "invoke dhcp client to obtain ip/boot params",
BAREBOX_CMD_END

