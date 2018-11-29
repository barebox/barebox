/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <asm-generic/div64.h>
#include <command.h>
#include <clock.h>
#include <net.h>
#include <sntp.h>
#include <errno.h>
#include <environment.h>
#include <linux/err.h>

#define SNTP_PORT       123
#define TIMEOUT         1

#define VERSION         4	/* version number */

#define M_RSVD          0	/* reserved */
#define M_SACT          1	/* symmetric active */
#define M_PASV          2	/* symmetric passive */
#define M_CLNT          3	/* client */
#define M_SERV          4	/* server */
#define M_BCST          5	/* broadcast server */
#define M_BCLN          6	/* broadcast client */

typedef uint64_t tstamp;	/* NTP timestamp format */
typedef uint32_t tdist;		/* NTP short format */

struct ntp_packet {
#ifdef __LITTLE_ENDIAN		/* reversed */
	unsigned int mode:3;	/* mode */
	unsigned int version:3;	/* version number */
	unsigned int leap:2;	/* leap indicator */
#else				/* forward */
	unsigned int leap:2;	/* leap indicator */
	unsigned int version:3;	/* version number */
	unsigned int mode:3;	/* mode */
#endif
	uint8_t stratum;	/* stratum */
	uint8_t poll;		/* poll interval */
	int8_t precision;	/* precision */
	tdist rootdelay;	/* root delay */
	tdist rootdisp;		/* root dispersion */
	uint32_t refid;		/* reference ID */
	tstamp reftime;		/* reference time */
	tstamp org;		/* origin timestamp */
	tstamp rec;		/* receive timestamp */
	tstamp xmt;		/* transmit timestamp */
};

static IPaddr_t net_sntp_ip;

#define SNTP_STATE_INIT		0
#define SNTP_STATE_SUCCESS	1

static int sntp_state;

static struct net_connection *sntp_con;

static s64 curr_timestamp;

static int sntp_send(void)
{
	struct ntp_packet *ntp = net_udp_get_payload(sntp_con);

	memset(ntp, 0, sizeof(struct ntp_packet));

	ntp->version = VERSION;
	ntp->mode = M_CLNT;

	return net_udp_send(sntp_con, sizeof(struct ntp_packet));
}

static void sntp_handler(void *ctx, char *pkt, unsigned len)
{
	IPaddr_t ip_addr;
	struct iphdr *ip = net_eth_to_iphdr(pkt);
	struct ntp_packet *ntp =
	    (struct ntp_packet *)net_eth_to_udp_payload(pkt);

	ip_addr = net_read_ip((void *)&ip->saddr);
	if (ip_addr != net_sntp_ip)
		return;

	len = net_eth_to_udplen(pkt);
	if (len < sizeof(struct ntp_packet))
		return;

	pr_debug("received SNTP response\n");

	if (ntp->version != VERSION)
		return;

	if (ntp->mode != M_SERV)
		return;

	curr_timestamp = (get_unaligned_be64(&ntp->xmt) >> 32) - 2208988800UL;

	sntp_state = SNTP_STATE_SUCCESS;
}

s64 sntp(const char *server)
{
	int ret, repeat = 5;
	u64 sntp_start;

	if (!server)
		server = getenv("global.dhcp.ntpserver");
	if (!server)
		return -EINVAL;

	ret = resolv(server, &net_sntp_ip);
	if (ret) {
		printf("Cannot resolve \"%s\": %s\n", server, strerror(-ret));;
		return 1;
	}

	sntp_con = net_udp_new(net_sntp_ip, SNTP_PORT, sntp_handler, NULL);
	if (IS_ERR(sntp_con)) {
		ret = PTR_ERR(sntp_con);
		goto out;
	}

	sntp_start = get_time_ns();
	ret = sntp_send();
	if (ret)
		goto out_unreg;

	sntp_state = SNTP_STATE_INIT;

	while (sntp_state == SNTP_STATE_INIT) {
		if (ctrlc()) {
			ret = -EINTR;
			break;
		}

		net_poll();

		if (is_timeout(sntp_start, 1 * SECOND)) {
			sntp_start = get_time_ns();
			ret = sntp_send();
			if (ret)
				goto out_unreg;
			repeat--;
			if (!repeat) {
				ret = -ETIMEDOUT;
				goto out_unreg;
			}
		}
	}

	net_unregister(sntp_con);

	return curr_timestamp;

out_unreg:
	net_unregister(sntp_con);
out:
	return ret;
}
