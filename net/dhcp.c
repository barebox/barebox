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
static char dhcp_tftpname[256];

static const char* dhcp_get_barebox_global(const char * var)
{
	char * var_global = basprintf("global.dhcp.%s", var);
	const char *val;

	if (!var_global)
		return NULL;

	val = getenv(var_global);
	free(var_global);
	return val;
}

static int dhcp_set_barebox_global(const char * var, char *val)
{
	char * var_global = basprintf("global.dhcp.%s", var);
	int ret;

	if (!var_global)
		return -ENOMEM;

	ret = setenv(var_global, val);
	free(var_global);
	return ret;
}

struct dhcp_opt {
	unsigned char option;
	/* request automatically the option when creating the DHCP request */
	bool optional;
	bool copy_only_if_valid;
	const char *barebox_var_name;
	const char *barebox_dhcp_global;
	void (*handle)(struct dhcp_opt *opt, unsigned char *data, int tlen);
	int (*handle_param)(struct dhcp_opt *dhcp_opt, u8 *e);
	void *data;

	struct bootp *bp;
};

static void netmask_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	IPaddr_t ip;

	ip = net_read_ip(popt);
	net_set_netmask(ip);
}

static void gateway_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	IPaddr_t ip;

	ip = net_read_ip(popt);
	net_set_gateway(ip);
}

static void env_ip_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	IPaddr_t ip;

	ip = net_read_ip(popt);
	if (IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
		setenv_ip(opt->barebox_var_name, ip);
}

static void env_str_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	char str[256];
	char *tmp = str;

	if (opt->data)
		tmp = opt->data;

	memcpy(tmp, popt, optlen);
	tmp[optlen] = 0;

	if (opt->copy_only_if_valid && !strlen(tmp))
		return;
	if (opt->barebox_var_name && IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
		setenv(opt->barebox_var_name, tmp);
	if (opt->barebox_dhcp_global && IS_ENABLED(CONFIG_GLOBALVAR))
		dhcp_set_barebox_global(opt->barebox_dhcp_global, tmp);

}

static void copy_uint32_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	net_copy_uint32(opt->data, (uint32_t *)popt);
};

static void copy_ip_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	net_copy_ip(opt->data, popt);
};

static void bootfile_vendorex_handle(struct dhcp_opt *opt, unsigned char *popt, int optlen)
{
	if (opt->bp->bp_file[0] != '\0')
		return;

	/*
	 * only use vendor boot file if we didn't
	 * receive a boot file in the main non-vendor
	 * part of the packet - god only knows why
	 * some vendors chose not to use this perfectly
	 * good spot to store the boot file (join on
	 * Tru64 Unix) it seems mind bogglingly crazy
	 * to me
	 */
	pr_warn("*** WARNING: using vendor optional boot file\n");

	/*
	 * I can't use dhcp_vendorex_proc here because I need
	 * to write into the bootp packet - even then I had to
	 * pass the bootp packet pointer into here as the
	 * second arg
	 */
	env_str_handle(opt, popt, optlen);
}

static int dhcp_set_string_options(struct dhcp_opt *param, u8 *e)
{
	int str_len;
	const char *str = param->data;

	if (!str && param->barebox_var_name && IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
		str = getenv(param->barebox_var_name);

	if (!str && param->barebox_dhcp_global && IS_ENABLED(CONFIG_GLOBALVAR))
		str = dhcp_get_barebox_global(param->barebox_dhcp_global);

	if (!str)
		return 0;

	str_len = strlen(str);
	if (!str_len)
		return 0;

	*e++ = param->option;
	*e++ = str_len;
	memcpy(e, str, str_len);

	return str_len + 2;
}

#define DHCP_HOSTNAME		12
#define DHCP_VENDOR_ID		60
#define DHCP_CLIENT_ID		61
#define DHCP_USER_CLASS		77
#define DHCP_CLIENT_UUID	97

struct dhcp_opt dhcp_options[] = {
	{
		.option = 1,
		.handle = netmask_handle,
	}, {
		.option = 3,
		.handle = gateway_handle,
	}, {
		.option = 6,
		.handle = env_ip_handle,
		.barebox_var_name = "net.nameserver",
	}, {
		.option = DHCP_HOSTNAME,
		.copy_only_if_valid = 1,
		.handle = env_str_handle,
		.handle_param = dhcp_set_string_options,
		.barebox_var_name = "global.hostname",
	}, {
		.option = 15,
		.handle = env_str_handle,
		.barebox_var_name = "net.domainname",
	}, {
		.option = 17,
		.handle = env_str_handle,
		.barebox_dhcp_global = "rootpath",
	}, {
		.option = 51,
		.handle = copy_uint32_handle,
		.data = &dhcp_leasetime,
	}, {
		.option = 54,
		.handle = copy_ip_handle,
		.data = &net_dhcp_server_ip,
		.optional = true,
	}, {
		.option = DHCP_VENDOR_ID,
		.handle_param = dhcp_set_string_options,
		.barebox_dhcp_global = "vendor_id",
	},{
		.option = 66,
		.handle = env_str_handle,
		.barebox_dhcp_global = "tftp_server_name",
		.data = dhcp_tftpname,
	}, {
		.option = 67,
		.handle = bootfile_vendorex_handle,
		.barebox_dhcp_global = "bootfile",
	}, {
		.option = DHCP_CLIENT_ID,
		.handle_param = dhcp_set_string_options,
		.barebox_dhcp_global = "client_id",
	}, {
		.option = DHCP_USER_CLASS,
		.handle_param = dhcp_set_string_options,
		.barebox_dhcp_global = "user_class",
	}, {
		.option = DHCP_CLIENT_UUID,
		.handle_param = dhcp_set_string_options,
		.barebox_dhcp_global = "client_uuid",
	}, {
		.option = 224,
		.handle = env_str_handle,
		.barebox_dhcp_global = "oftree_file",
	},
};

static void dhcp_set_param_data(int option, void* data)
{
	struct dhcp_opt *opt;
	int i;

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		opt = &dhcp_options[i];

		if (opt->option == option) {
			opt->data = data;
			return;
		}
	}
}

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
	IPaddr_t tmp_ip;

	tmp_ip = net_read_ip(&bp->bp_yiaddr);
	net_set_ip(tmp_ip);

	tmp_ip = net_read_ip(&bp->bp_siaddr);
	if (tmp_ip != 0)
		net_set_serverip(tmp_ip);

	if (strlen(bp->bp_file) > 0) {
		if (IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
			setenv("bootfile", bp->bp_file);
		if (IS_ENABLED(CONFIG_GLOBALVAR))
			dhcp_set_barebox_global("bootfile", bp->bp_file);
	}

	debug("bootfile: %s\n", bp->bp_file);
}

/*
 * Initialize BOOTP extension fields in the request.
 */
static int dhcp_extended (u8 *e, int message_type, IPaddr_t ServerID,
			  IPaddr_t RequestedIP)
{
	struct dhcp_opt *opt;
	int i;
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


	e += dhcp_set_ip_options(50, e, RequestedIP);
	e += dhcp_set_ip_options(54, e, ServerID);

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		opt = &dhcp_options[i];
		if (opt->handle_param)
			e += opt->handle_param(opt, e);
	}

	*e++ = 55;		/* Parameter Request List */
	 cnt = e++;		/* Pointer to count of requested items */
	*cnt = 0;

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		if (dhcp_options[i].optional)
			continue;
		*e++  = dhcp_options[i].option;
		*cnt += 1;
	}
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

static void dhcp_options_handle(unsigned char option, unsigned char *popt,
			       int optlen, struct bootp *bp)
{
	int i;
	struct dhcp_opt *opt;

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		opt = &dhcp_options[i];
		if (opt->option == option) {
			opt->bp = bp;
			if (opt->handle)
				opt->handle(opt, popt, optlen);
			return;
		}
	}

	debug("*** Unhandled DHCP Option in OFFER/ACK: %d\n", option);
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
	net_copy_ip(&OfferedIP, &bp_offer->bp_yiaddr);
	extlen = dhcp_extended((u8 *)bp->bp_vend, DHCP_REQUEST, net_dhcp_server_ip,
				OfferedIP);

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
			IPaddr_t ip;
			if (net_read_uint32((uint32_t *)&bp->bp_vend[0]) == htonl(BOOTP_VENDOR_MAGIC))
				dhcp_options_process((u8 *)&bp->bp_vend[4], bp);
			bootp_copy_net_params(bp); /* Store net params from reply */
			dhcp_state = BOUND;
			ip = net_get_ip();
			printf("DHCP client bound to address %pI4\n", &ip);
			return;
		}
		break;
	default:
		debug("%s: INVALID STATE\n", __func__);
		break;
	}
}

static void dhcp_reset_env(void)
{
	struct dhcp_opt *opt;
	int i;

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		opt = &dhcp_options[i];
		if (!opt->barebox_var_name || opt->copy_only_if_valid)
			continue;

		if (IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES))
			setenv(opt->barebox_var_name, "");
		if (opt->barebox_dhcp_global && IS_ENABLED(CONFIG_GLOBALVAR))
			dhcp_set_barebox_global(opt->barebox_dhcp_global, "");
	}
}

int dhcp(int retries, struct dhcp_req_param *param)
{
	int ret = 0;

	dhcp_reset_env();

	dhcp_set_param_data(DHCP_HOSTNAME, param->hostname);
	dhcp_set_param_data(DHCP_VENDOR_ID, param->vendor_id);
	dhcp_set_param_data(DHCP_CLIENT_ID, param->client_id);
	dhcp_set_param_data(DHCP_USER_CLASS, param->user_class);
	dhcp_set_param_data(DHCP_CLIENT_UUID, param->client_uuid);

	if (!retries)
		retries = DHCP_DEFAULT_RETRY;

	dhcp_con = net_udp_new(0xffffffff, PORT_BOOTPS, dhcp_handler, NULL);
	if (IS_ERR(dhcp_con)) {
		ret = PTR_ERR(dhcp_con);
		goto out;
	}

	ret = net_udp_bind(dhcp_con, PORT_BOOTPC);
	if (ret)
		goto out1;

	net_set_ip(0);

	dhcp_start = get_time_ns();
	ret = bootp_request(); /* Basically same as BOOTP */
	if (ret)
		goto out1;

	while (dhcp_state != BOUND) {
		if (ctrlc()) {
			ret = -EINTR;
			goto out1;
		}
		if (!retries) {
			ret = -ETIMEDOUT;
			goto out1;
		}
		net_poll();
		if (is_timeout(dhcp_start, 3 * SECOND)) {
			dhcp_start = get_time_ns();
			printf("T ");
			ret = bootp_request();
			/* no need to check if retries > 0 as we check if != 0 */
			retries--;
			if (ret)
				goto out1;
		}
	}

	if (dhcp_tftpname[0] != 0) {
		IPaddr_t tftpserver = resolv(dhcp_tftpname);
		if (tftpserver)
			net_set_serverip(tftpserver);
	}

out1:
	net_unregister(dhcp_con);
out:
	if (ret)
		debug("dhcp failed: %s\n", strerror(-ret));

	return ret;
}

#ifdef CONFIG_GLOBALVAR
static void dhcp_global_add(const char *var)
{
	char *var_global = basprintf("dhcp.%s", var);

	if (!var_global)
		return;

	globalvar_add_simple(var_global, NULL);
	free(var_global);
}

static int dhcp_global_init(void)
{
	struct dhcp_opt *opt;
	int i;

	for (i = 0; i < ARRAY_SIZE(dhcp_options); i++) {
		opt = &dhcp_options[i];

		if (!opt->barebox_dhcp_global)
			continue;

		dhcp_global_add(opt->barebox_dhcp_global);
	}

	return 0;
}
late_initcall(dhcp_global_init);

BAREBOX_MAGICVAR_NAMED(global_dhcp_bootfile, global.dhcp.bootfile, "bootfile returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_rootpath, global.dhcp.rootpath, "rootpath returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_vendor_id, global.dhcp.vendor_id, "vendor id to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_client_uuid, global.dhcp.client_uuid, "client uuid to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_client_id, global.dhcp.client_id, "client id to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_user_class, global.dhcp.user_class, "user class to send to the DHCP server");
BAREBOX_MAGICVAR_NAMED(global_dhcp_tftp_server_name, global.dhcp.tftp_server_name, "TFTP server Name returned from DHCP request");
BAREBOX_MAGICVAR_NAMED(global_dhcp_oftree_file, global.dhcp.oftree_file, "OF tree returned from DHCP request (option 224)");
BAREBOX_MAGICVAR_NAMED(global_dhcp_retries, global.dhcp.retries, "retry limit");
#endif
