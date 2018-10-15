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
#include <complete.h>
#include <environment.h>
#include <clock.h>
#include <net.h>
#include <libbb.h>
#include <errno.h>
#include <magicvar.h>
#include <linux/err.h>
#include <getopt.h>
#include <globalvar.h>
#include <init.h>
#include <dhcp.h>

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
} __packed;

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
static uint64_t dhcp_start;
static struct eth_device *dhcp_edev;
struct dhcp_req_param dhcp_param;
struct dhcp_result *dhcp_result;

struct dhcp_receivce_opts {
	IPaddr_t netmask;
	IPaddr_t gateway;
	IPaddr_t nameserver;
	IPaddr_t serverip;
	char *hostname;
	char *domainname;
	char *rootpath;
	char *devicetree;
	char *bootfile;
};

#define DHCP_HOSTNAME		12
#define DHCP_VENDOR_ID		60
#define DHCP_CLIENT_ID		61
#define DHCP_USER_CLASS		77
#define DHCP_CLIENT_UUID	97
#define DHCP_OPTION224		224

static int dhcp_set_ip_options(int option, u8 *e, IPaddr_t ip)
{
	int tmp;

	if (!ip)
		return 0;

	tmp = ntohl(ip);

	*e++ = option;
	*e++ = 4;
	*e++ = tmp >> 24;
	*e++ = tmp >> 16;
	*e++ = tmp >> 8;
	*e++ = tmp & 0xff;

	return 6;
}

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

	dhcp_result->ip = net_read_ip(&bp->bp_yiaddr);
	dhcp_result->serverip = net_read_ip(&bp->bp_siaddr);

	if (strlen(bp->bp_file) > 0)
		dhcp_result->bootfile = xstrdup(bp->bp_file);
}

static int dhcp_set_string_options(int option, const char *str, u8 *e)
{
	int str_len;

	if (!str)
		return 0;

	str_len = strlen(str);
	if (!str_len)
		return 0;

	*e++ = option;
	*e++ = str_len;
	memcpy(e, str, str_len);

	return str_len + 2;
}

/*
 * Initialize BOOTP extension fields in the request.
 */
static int dhcp_extended(u8 *e, int message_type, IPaddr_t ServerID,
			  IPaddr_t RequestedIP)
{
	int i;
	u8 *start = e;
	u8 *cnt;
	u8 dhcp_options[] = {1, 3, 6, DHCP_HOSTNAME, 15, 17, 51, DHCP_VENDOR_ID, 66, 67, DHCP_CLIENT_ID,
			     DHCP_USER_CLASS, DHCP_CLIENT_UUID, 224};

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

	e += dhcp_set_ip_options(50, e, RequestedIP);
	e += dhcp_set_ip_options(54, e, ServerID);

	e += dhcp_set_string_options(DHCP_HOSTNAME, dhcp_param.hostname, e);
	e += dhcp_set_string_options(DHCP_VENDOR_ID, dhcp_param.vendor_id, e);
	e += dhcp_set_string_options(DHCP_CLIENT_ID, dhcp_param.client_id, e);
	e += dhcp_set_string_options(DHCP_USER_CLASS, dhcp_param.user_class, e);
	e += dhcp_set_string_options(DHCP_CLIENT_UUID, dhcp_param.client_uuid, e);
	e += dhcp_set_string_options(DHCP_OPTION224, dhcp_param.option224, e);

	*e++ = 55;		/* Parameter Request List */
	cnt = e++;		/* Pointer to count of requested items */

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++)
		*e++ = dhcp_options[i];

	*cnt = ARRAY_SIZE(dhcp_options);

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
	const char *bfile;

	dhcp_state = INIT;

	debug("BOOTP broadcast\n");

	bp = net_udp_get_payload(dhcp_con);;
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
		safe_strncpy(bp->bp_file, bfile, sizeof(bp->bp_file));

	/* Request additional information from the BOOTP/DHCP server */
	ext_len = dhcp_extended(bp->bp_vend, DHCP_DISCOVER, 0, 0);

	Bootp_id = (uint32_t)get_time_ns();
	net_copy_uint32(&bp->bp_id, &Bootp_id);

	dhcp_state = SELECTING;

	ret = net_udp_send(dhcp_con, sizeof(*bp) + ext_len);

	return ret;
}

static void dhcp_options_handle(unsigned char option, void *popt,
			       int optlen, struct bootp *bp)
{
	switch (option) {
		case 1:
			dhcp_result->netmask = net_read_ip(popt);
			break;
		case 3:
			dhcp_result->gateway = net_read_ip(popt);
			break;
		case 6:
			dhcp_result->nameserver = net_read_ip(popt);
			break;
		case DHCP_HOSTNAME:
			dhcp_result->hostname = xstrndup(popt, optlen);
			break;
		case 15:
			dhcp_result->domainname = xstrndup(popt, optlen);
			break;
		case 17:
			dhcp_result->rootpath = xstrndup(popt, optlen);
			break;
		case 51:
			net_copy_uint32(&dhcp_result->leasetime, popt);
			break;
		case 54:
			dhcp_result->serverip = net_read_ip(popt);
			break;
		case 66:
			dhcp_result->tftp_server_name = xstrndup(popt, optlen);
			break;
		case 67:
			if (!dhcp_result->bootfile)
				dhcp_result->bootfile = xstrndup(popt, optlen);
			break;
		case 224:
			dhcp_result->devicetree = xstrndup(popt, optlen);
			break;
		default:
			debug("*** Unhandled DHCP Option in OFFER/ACK: %d\n", option);
	}
}

static void dhcp_options_process(unsigned char *popt, struct bootp *bp)
{
	unsigned char *end = popt + sizeof(*bp) + OPT_SIZE;
	int oplen;
	unsigned char option;

	while (popt < end && *popt != 0xff) {
		oplen = *(popt + 1);
		option = *popt;

		dhcp_options_handle(option, popt + 2, oplen, bp);

		popt += oplen + 2;	/* Process next option */
	}
}

static int dhcp_message_type(unsigned char *popt)
{
	if (net_read_uint32((uint32_t *)popt) != htonl(BOOTP_VENDOR_MAGIC))
		return -1;

	popt += 4;
	while (*popt != 0xff) {
		if (*popt == 53)	/* DHCP Message Type */
			return *(popt + 2);
		popt += *(popt + 1) + 2;	/* Scan through all options */
	}
	return -1;
}

static void dhcp_send_request_packet(struct bootp *bp_offer)
{
	struct bootp *bp;
	int extlen;

	debug("%s: Sending DHCPREQUEST\n", __func__);

	bp = net_udp_get_payload(dhcp_con);
	bp->bp_op = OP_BOOTREQUEST;
	bp->bp_htype = HWT_ETHER;
	bp->bp_hlen = HWL_ETHER;
	bp->bp_hops = 0;
	bp->bp_secs = htons(get_time_ns() >> 30);

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
	extlen = dhcp_extended(bp->bp_vend, DHCP_REQUEST, dhcp_result->serverip,
				dhcp_result->ip);

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
		debug("%s: state SELECTING, bp_file: \"%s\"\n", __func__, bp->bp_file);
		dhcp_state = REQUESTING;

		if (net_read_uint32(&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
			dhcp_options_process((u8 *)&bp->bp_vend[4], bp);

		bootp_copy_net_params(bp); /* Store net params from reply */

		dhcp_start = get_time_ns();
		dhcp_send_request_packet(bp);

		break;
	case REQUESTING:
		debug("%s: State REQUESTING\n", __func__);

		if (dhcp_message_type((u8 *)bp->bp_vend) == DHCP_ACK ) {
			if (net_read_uint32(&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
				dhcp_options_process(&bp->bp_vend[4], bp);
			bootp_copy_net_params(bp); /* Store net params from reply */
			dhcp_state = BOUND;
			dev_info(&dhcp_edev->dev, "DHCP client bound to address %pI4\n", &dhcp_result->ip);
			return;
		}
		break;
	default:
		debug("%s: INVALID STATE\n", __func__);
		break;
	}
}

static char *global_dhcp_user_class;
static char *global_dhcp_vendor_id;
static char *global_dhcp_client_uuid;
static char *global_dhcp_client_id;
static char *global_dhcp_bootfile;
static char *global_dhcp_oftree_file;
static char *global_dhcp_rootpath;
static char *global_dhcp_tftp_server_name;
static int global_dhcp_retries = DHCP_DEFAULT_RETRY;
static char *global_dhcp_option224;

static void set_res(char **var, const char *res)
{
	free(*var);

	if (res)
		*var = xstrdup(res);
	else
		*var = xstrdup("");
}

int dhcp_request(struct eth_device *edev, const struct dhcp_req_param *param,
		 struct dhcp_result **res)
{
	int ret = 0;

	dhcp_edev = edev;
	if (param)
		dhcp_param = *param;
	else
		memset(&dhcp_param, 0, sizeof(dhcp_param));

	dhcp_result = xzalloc(sizeof(*dhcp_result));

	if (!dhcp_param.user_class)
		dhcp_param.user_class = global_dhcp_user_class;
	if (!dhcp_param.vendor_id)
		dhcp_param.vendor_id = global_dhcp_vendor_id;
	if (!dhcp_param.client_uuid)
		dhcp_param.client_uuid = global_dhcp_client_uuid;
	if (!dhcp_param.client_id)
		dhcp_param.client_id = global_dhcp_client_id;
	if (!dhcp_param.option224)
		dhcp_param.option224 = global_dhcp_option224;
	if (!dhcp_param.retries)
		dhcp_param.retries = global_dhcp_retries;

	dhcp_con = net_udp_eth_new(edev, IP_BROADCAST, PORT_BOOTPS, dhcp_handler, NULL);
	if (IS_ERR(dhcp_con)) {
		ret = PTR_ERR(dhcp_con);
		goto out;
	}

	ret = net_udp_bind(dhcp_con, PORT_BOOTPC);
	if (ret)
		goto out1;

	net_set_ip(edev, 0);

	dhcp_start = get_time_ns();
	ret = bootp_request(); /* Basically same as BOOTP */
	if (ret)
		goto out1;

	while (dhcp_state != BOUND) {
		if (ctrlc()) {
			ret = -EINTR;
			goto out1;
		}
		if (!dhcp_param.retries) {
			ret = -ETIMEDOUT;
			goto out1;
		}
		net_poll();
		if (is_timeout(dhcp_start, 3 * SECOND)) {
			dhcp_start = get_time_ns();
			printf("T ");
			ret = bootp_request();
			/* no need to check if retries > 0 as we check if != 0 */
			dhcp_param.retries--;
			if (ret)
				goto out1;
		}
	}

	pr_debug("DHCP result:\n"
		"  ip: %pI4\n"
		"  netmask: %pI4\n"
		"  gateway: %pI4\n"
		"  serverip: %pI4\n"
		"  nameserver: %pI4\n"
		"  hostname: %s\n"
		"  domainname: %s\n"
		"  rootpath: %s\n"
		"  devicetree: %s\n"
		"  tftp_server_name: %s\n",
		&dhcp_result->ip,
		&dhcp_result->netmask,
		&dhcp_result->gateway,
		&dhcp_result->serverip,
		&dhcp_result->nameserver,
		dhcp_result->hostname ? dhcp_result->hostname : "",
		dhcp_result->domainname ? dhcp_result->domainname : "",
		dhcp_result->rootpath ? dhcp_result->rootpath : "",
		dhcp_result->devicetree ? dhcp_result->devicetree : "",
		dhcp_result->tftp_server_name ? dhcp_result->tftp_server_name : "");

out1:
	net_unregister(dhcp_con);
out:
	if (ret) {
		debug("dhcp failed: %s\n", strerror(-ret));
		free(dhcp_result);
	} else {
		*res = dhcp_result;
	}

	return ret;
}

int dhcp_set_result(struct eth_device *edev, struct dhcp_result *res)
{
	net_set_ip(edev, res->ip);
	net_set_netmask(edev, res->netmask);
	net_set_gateway(res->gateway);
	net_set_nameserver(res->nameserver);

	set_res(&global_dhcp_bootfile, res->bootfile);
	set_res(&global_dhcp_oftree_file, res->devicetree);
	set_res(&global_dhcp_rootpath, res->rootpath);
	set_res(&global_dhcp_tftp_server_name, res->tftp_server_name);

	if (res->hostname)
		barebox_set_hostname(res->hostname);
	if (res->domainname)
		net_set_domainname(res->domainname);

	if (res->tftp_server_name) {
		IPaddr_t ip;

		ip = resolv(res->tftp_server_name);
		if (ip)
			net_set_serverip_empty(ip);
	} else if (res->serverip) {
		net_set_serverip_empty(res->serverip);
	}

	return 0;
}

void dhcp_result_free(struct dhcp_result *res)
{
	free(res->hostname);
	free(res->domainname);
	free(res->rootpath);
	free(res->devicetree);
	free(res->bootfile);
	free(res->tftp_server_name);

	free(res);
}

int dhcp(struct eth_device *edev, const struct dhcp_req_param *param)
{
	struct dhcp_result *res;
	int ret;

	ret = dhcp_request(edev, param, &res);
	if (ret)
		return ret;

	ret = dhcp_set_result(edev, res);

	dhcp_result_free(res);

	if (!ret)
		edev->ifup = true;

	return ret;
}

#ifdef CONFIG_GLOBALVAR

static int dhcp_global_init(void)
{
	globalvar_add_simple_string("dhcp.bootfile", &global_dhcp_bootfile);
	globalvar_add_simple_string("dhcp.rootpath", &global_dhcp_rootpath);
	globalvar_add_simple_string("dhcp.vendor_id", &global_dhcp_vendor_id);
	globalvar_add_simple_string("dhcp.client_uuid", &global_dhcp_client_uuid);
	globalvar_add_simple_string("dhcp.client_id", &global_dhcp_client_id);
	globalvar_add_simple_string("dhcp.user_class", &global_dhcp_user_class);
	globalvar_add_simple_string("dhcp.oftree_file", &global_dhcp_oftree_file);
	globalvar_add_simple_string("dhcp.tftp_server_name", &global_dhcp_tftp_server_name);
	globalvar_add_simple_int("dhcp.retries", &global_dhcp_retries, "%u");
	globalvar_add_simple_string("dhcp.option224", &global_dhcp_option224);

	return 0;
}
late_initcall(dhcp_global_init);
#endif

BAREBOX_MAGICVAR_NAMED(global_dhcp_bootfile, global.dhcp.bootfile, "bootfile returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_rootpath, global.dhcp.rootpath, "rootpath returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_vendor_id, global.dhcp.vendor_id, "vendor id to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_client_uuid, global.dhcp.client_uuid, "client uuid to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_client_id, global.dhcp.client_id, "client id to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_user_class, global.dhcp.user_class, "user class to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_tftp_server_name, global.dhcp.tftp_server_name, "TFTP server Name returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_oftree_file, global.dhcp.oftree_file, "OF tree returned from DHCP request (option 224)");
BAREBOX_MAGICVAR_NAMED(global_dhcp_retries, global.dhcp.retries, "retry limit");
BAREBOX_MAGICVAR_NAMED(global_dhcp_option224, global.dhcp.option224, "private data to send to the DHCP server (option 224)");
