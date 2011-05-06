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
#include <errno.h>
#include <linux/err.h>

#define OPT_SIZE 312	/* Minimum DHCP Options size per RFC2131 - results in 576 byte pkt */

struct bootp {
	uint8_t		bp_op;		/* Operation				*/
#define OP_BOOTREQUEST	1
#define OP_BOOTREPLY	2
	uint8_t		bp_htype;	/* Hardware type			*/
#define HWT_ETHER	1
	uint8_t		bp_hlen;	/* Hardware address length		*/
#define HWL_ETHER	6
	uint8_t		bp_hops;	/* Hop count (gateway thing)		*/
	uint32_t	bp_id;		/* Transaction ID			*/
	uint16_t	bp_secs;	/* Seconds since boot			*/
	uint16_t	bp_spare1;	/* Alignment				*/
	IPaddr_t	bp_ciaddr;	/* Client IP address			*/
	IPaddr_t	bp_yiaddr;	/* Your (client) IP address		*/
	IPaddr_t	bp_siaddr;	/* Server IP address			*/
	IPaddr_t	bp_giaddr;	/* Gateway IP address			*/
	uint8_t		bp_chaddr[16];	/* Client hardware address		*/
	char		bp_sname[64];	/* Server host name			*/
	char		bp_file[128];	/* Boot file name			*/
	char		bp_vend[0];	/* Vendor information			*/
};

/* DHCP States */
typedef enum {
	INIT,
	INIT_REBOOT,
	REBOOTING,
	SELECTING,
	REQUESTING,
	REBINDING,
	BOUND,
	RENEWING,
} dhcp_state_t;

#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7

#define BOOTP_VENDOR_MAGIC	0x63825363	/* RFC1048 Magic Cookie		*/

#define PORT_BOOTPS	67		/* BOOTP server UDP port		*/
#define PORT_BOOTPC	68		/* BOOTP client UDP port		*/

#define DHCP_MIN_EXT_LEN 64	/* minimal length of extension list	*/

static uint32_t Bootp_id;
static dhcp_state_t dhcp_state;
static uint32_t dhcp_leasetime;
static IPaddr_t net_dhcp_server_ip;
static uint64_t dhcp_start;

static int bootp_check_packet(unsigned char *pkt, unsigned src, unsigned len)
{
	struct bootp *bp = (struct bootp *) pkt;
	int retval = 0;

	if (src != PORT_BOOTPS)
		retval = -1;
	else if (len < sizeof(struct bootp))
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
	else if (net_read_uint32(&bp->bp_id) != Bootp_id) {
		retval = -6;
	}

	debug("Filtering pkt = %d\n", retval);

	return retval;
}

/*
 * Copy parameters of interest from BOOTP_REPLY/DHCP_OFFER packet
 */
static void bootp_copy_net_params(struct bootp *bp)
{
	IPaddr_t tmp_ip;

	tmp_ip = net_read_ip(&bp->bp_yiaddr);
	net_set_ip(tmp_ip);

	tmp_ip = net_read_ip(&bp->bp_siaddr);
	if (tmp_ip != 0)
		net_set_serverip(tmp_ip);

	if (strlen(bp->bp_file) > 0)
		setenv("bootfile", bp->bp_file);

	debug("bootfile: %s\n", bp->bp_file);
}

/*
 * Initialize BOOTP extension fields in the request.
 */
static int dhcp_extended (u8 *e, int message_type, IPaddr_t ServerID, IPaddr_t RequestedIP)
{
	u8 *start = e;
	u8 *cnt;

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

	*e++ = 55;		/* Parameter Request List */
	 cnt = e++;		/* Pointer to count of requested items */
	*cnt = 0;
	*e++  = 1;		/* Subnet Mask */
	*cnt += 1;
	*e++  = 3;		/* Router Option */
	*cnt += 1;
	*e++  = 6;		/* DNS Server(s) */
	*cnt += 1;
	*e++  = 12;		/* Hostname */
	*cnt += 1;
	*e++  = 15;		/* domain name */
	*cnt += 1;
	*e++  = 17;		/* Boot path */
	*cnt += 1;
	*e++  = 255;		/* End of the list */

	/* Pad to minimal length */
	while ((e - start) <= DHCP_MIN_EXT_LEN)
		*e++ = 0;

	return e - start;
}

static struct net_connection *dhcp_con;

static int bootp_request(void)
{
	struct bootp *bp;
	int ext_len;
	int ret;
	unsigned char *payload = net_udp_get_payload(dhcp_con);
	const char *bfile;

	dhcp_state = INIT;

	debug("BOOTP broadcast\n");

	bp = (struct bootp *)payload;
	bp->bp_op = OP_BOOTREQUEST;
	bp->bp_htype = HWT_ETHER;
	bp->bp_hlen = HWL_ETHER;
	bp->bp_hops = 0;
	bp->bp_secs = htons(get_time_ns() >> 30);
	net_write_ip(&bp->bp_ciaddr, 0);
	net_write_ip(&bp->bp_yiaddr, 0);
	net_write_ip(&bp->bp_siaddr, 0);
	net_write_ip(&bp->bp_giaddr, 0);
	memcpy(bp->bp_chaddr, dhcp_con->et->et_src, 6);

	bfile = getenv("bootfile");
	if (bfile)
		safe_strncpy (bp->bp_file, bfile, sizeof(bp->bp_file));

	/* Request additional information from the BOOTP/DHCP server */
	ext_len = dhcp_extended((u8 *)bp->bp_vend, DHCP_DISCOVER, 0, 0);

	Bootp_id = (uint32_t)get_time_ns();
	net_copy_uint32(&bp->bp_id, &Bootp_id);

	dhcp_state = SELECTING;

	ret = net_udp_send(dhcp_con, sizeof(*bp) + ext_len);

	return ret;
}

static void dhcp_options_process(unsigned char *popt, struct bootp *bp)
{
	unsigned char *end = popt + sizeof(*bp) + OPT_SIZE;
	int oplen;
	IPaddr_t ip;
	char str[256];

	while (popt < end && *popt != 0xff) {
		oplen = *(popt + 1);
		switch (*popt) {
		case 1:
			ip = net_read_ip(popt + 2);
			net_set_netmask(ip);
			break;
		case 3:
			ip = net_read_ip(popt + 2);
			net_set_gateway(ip);
			break;
		case 6:
			ip = net_read_ip(popt + 2);
			setenv_ip("nameserver", ip);
			break;
		case 12:
			memcpy(str, popt + 2, oplen);
			str[oplen] = 0;
			setenv("hostname", str);
			break;
		case 15:
			memcpy(str, popt + 2, oplen);
			str[oplen] = 0;
			setenv("domainname", str);
			break;
		case 17:
			memcpy(str, popt + 2, oplen);
			str[oplen] = 0;
			setenv("rootpath", str);
			break;
		case 51:
			net_copy_uint32 (&dhcp_leasetime, (uint32_t *)(popt + 2));
			break;
		case 53:	/* Ignore Message Type Option */
			break;
		case 54:
			net_copy_ip(&net_dhcp_server_ip, (popt + 2));
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
			memcpy(str, popt + 2, oplen);
			str[oplen] = 0;
			if (bp->bp_file[0] == '\0') {
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
					setenv("bootfile", str);
			}
			break;
		default:
#ifdef CONFIG_BOOTP_VENDOREX
			if (dhcp_vendorex_proc (popt))
				break;
#endif
			debug("*** Unhandled DHCP Option in OFFER/ACK: %d\n", *popt);
			break;
		}
		popt += oplen + 2;	/* Process next option */
	}
}

static int dhcp_message_type(unsigned char *popt)
{
	if (net_read_uint32((uint32_t *)popt) != htonl(BOOTP_VENDOR_MAGIC))
		return -1;

	popt += 4;
	while ( *popt != 0xff ) {
		if ( *popt == 53 )	/* DHCP Message Type */
			return *(popt + 2);
		popt += *(popt + 1) + 2;	/* Scan through all options */
	}
	return -1;
}

static void dhcp_send_request_packet(struct bootp *bp_offer)
{
	struct bootp *bp;
	int extlen;
	IPaddr_t OfferedIP;
	unsigned char *payload = net_udp_get_payload(dhcp_con);

	debug("%s: Sending DHCPREQUEST\n", __func__);

	bp = (struct bootp *)payload;
	bp->bp_op = OP_BOOTREQUEST;
	bp->bp_htype = HWT_ETHER;
	bp->bp_hlen = HWL_ETHER;
	bp->bp_hops = 0;
	/* FIXME what is this? */
//	bp->bp_secs = htons(get_timer(0) / CFG_HZ);
	net_copy_ip(&bp->bp_ciaddr, &bp_offer->bp_ciaddr); /* both in network byte order */
	net_copy_ip(&bp->bp_yiaddr, &bp_offer->bp_yiaddr);
	net_copy_ip(&bp->bp_siaddr, &bp_offer->bp_siaddr);
	/*
	 * RFC3046 requires Relay Agents to discard packets with
	 * nonzero and offered giaddr
	 */
	net_write_ip(&bp->bp_giaddr, 0);

	memcpy(bp->bp_chaddr, dhcp_con->et->et_src, 6);

	/*
	 * ID is the id of the OFFER packet
	 */

	net_copy_uint32(&bp->bp_id, &bp_offer->bp_id);

	/*
	 * Copy options from OFFER packet if present
	 */
	net_copy_ip(&OfferedIP, &bp->bp_yiaddr);
	extlen = dhcp_extended((u8 *)bp->bp_vend, DHCP_REQUEST, net_dhcp_server_ip, OfferedIP);

	debug("Transmitting DHCPREQUEST packet\n");
	net_udp_send(dhcp_con, sizeof(*bp) + extlen);
}

/*
 *	Handle DHCP received packets.
 */
static void dhcp_handler(void *ctx, char *packet, unsigned int len)
{
	char *pkt = net_eth_to_udp_payload(packet);
	struct udphdr *udp = net_eth_to_udphdr(packet);
	struct bootp *bp = (struct bootp *)pkt;

	len = net_eth_to_udplen(packet);

	debug("DHCPHandler: got packet: (len=%d) state: %d\n",
		len, dhcp_state);

	if (bootp_check_packet(pkt, ntohs(udp->uh_sport), len)) /* Filter out pkts we don't want */
		return;

	switch (dhcp_state) {
	case SELECTING:
		/*
		 * Wait an appropriate time for any potential DHCPOFFER packets
		 * to arrive.  Then select one, and generate DHCPREQUEST response.
		 * If filename is in format we recognize, assume it is a valid
		 * OFFER from a server we want.
		 */
		debug ("%s: state SELECTING, bp_file: \"%s\"\n", __func__, bp->bp_file);
		dhcp_state = REQUESTING;

		if (net_read_uint32((uint32_t *)&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
			dhcp_options_process((u8 *)&bp->bp_vend[4], bp);

		bootp_copy_net_params(bp); /* Store net params from reply */

		dhcp_start = get_time_ns();
		dhcp_send_request_packet(bp);

		break;
	case REQUESTING:
		debug ("%s: State REQUESTING\n", __func__);

		if (dhcp_message_type((u8 *)bp->bp_vend) == DHCP_ACK ) {
			if (net_read_uint32((uint32_t *)&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
				dhcp_options_process((u8 *)&bp->bp_vend[4], bp);
			bootp_copy_net_params(bp); /* Store net params from reply */
			dhcp_state = BOUND;
			puts ("DHCP client bound to address ");
			print_IPaddr(net_get_ip());
			putchar('\n');
			return;
		}
		break;
	default:
		debug("%s: INVALID STATE\n", __func__);
		break;
	}
}

static int do_dhcp(struct command *cmdtp, int argc, char *argv[])
{
	int ret;

	dhcp_con = net_udp_new(0xffffffff, PORT_BOOTPS, dhcp_handler, NULL);
	if (IS_ERR(dhcp_con)) {
		ret = PTR_ERR(dhcp_con);
		goto out;
	}

	ret = net_udp_bind(dhcp_con, PORT_BOOTPC);
	if (ret)
		goto out1;

	net_set_ip(0);

	ret = bootp_request(); /* Basically same as BOOTP */
	if (ret)
		goto out1;

	while (dhcp_state != BOUND) {
		if (ctrlc())
			break;
		net_poll();
		if (is_timeout(dhcp_start, 3 * SECOND)) {
			dhcp_start = get_time_ns();
			printf("T ");
			ret = bootp_request();
			if (ret)
				goto out1;
		}
	}

out1:
	net_unregister(dhcp_con);
out:
	if (ret)
		printf("dhcp failed: %s\n", strerror(-ret));

	return ret ? 1 : 0;
}

BAREBOX_CMD_START(dhcp)
	.cmd		= do_dhcp,
	.usage		= "invoke dhcp client to obtain ip/boot params",
BAREBOX_CMD_END

