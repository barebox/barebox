/*
 * DNS support driver
 *
 * Copyright (c) 2008 Pieter Voorthuijsen <pieter.voorthuijsen@prodrive.nl>
 * Copyright (c) 2009 Robin Getz <rgetz@blackfin.uclinux.org>
 *
 * This is a simple DNS implementation for U-Boot. It will use the first IP
 * in the DNS response as NetServerIP. This can then be used for any other
 * network related activities.
 *
 * The packet handling is partly based on TADNS, original copyrights
 * follow below.
 *
 */

/*
 * Copyright (c) 2004-2005 Sergey Lyubka <valenok@gmail.com>
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sergey Lyubka wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */

#define pr_fmt(fmt) "dns: " fmt

#include <common.h>
#include <command.h>
#include <net.h>
#include <clock.h>
#include <environment.h>
#include <linux/err.h>

#define DNS_PORT 53

/* http://en.wikipedia.org/wiki/List_of_DNS_record_types */
enum dns_query_type {
	DNS_A_RECORD = 0x01,
	DNS_CNAME_RECORD = 0x05,
	DNS_MX_RECORD = 0x0f,
};

/*
 * DNS network packet
 */
struct header {
	uint16_t	tid;		/* Transaction ID */
	uint16_t	flags;		/* Flags */
	uint16_t	nqueries;	/* Questions */
	uint16_t	nanswers;	/* Answers */
	uint16_t	nauth;		/* Authority PRs */
	uint16_t	nother;		/* Other PRs */
	unsigned char	data[1];	/* Data, variable length */
};

#define STATE_INIT	0
#define STATE_DONE	1

static struct net_connection *dns_con;
static uint64_t dns_timer_start;
static int dns_state;
static IPaddr_t dns_ip;

static int dns_send(const char *name)
{
	int ret;
	struct header *header;
	enum dns_query_type qtype = DNS_A_RECORD;
	unsigned char *packet = net_udp_get_payload(dns_con);
	unsigned char *p, *s, *fullname, *dotptr;
	const unsigned char *domain;

	/* Prepare DNS packet header */
	header           = (struct header *)packet;
	header->tid      = 1;
	header->flags    = htons(0x100);	/* standard query */
	header->nqueries = htons(1);		/* Just one query */
	header->nanswers = 0;
	header->nauth    = 0;
	header->nother   = 0;

	domain = getenv("global.net.domainname");

	if (!strchr(name, '.') && domain && *domain)
		fullname = basprintf(".%s.%s.", name, domain);
	else
		fullname = basprintf(".%s.", name);

	/* replace dots in fullname with chunk len */
	dotptr = fullname;
	do {
		int len;

		s = strchr(dotptr + 1, '.');

		len = s - dotptr - 1;

		*dotptr = len;
		dotptr = s;
	} while (*(dotptr + 1));
	*dotptr = 0;

	strcpy(header->data, fullname);

	p = header->data + strlen(fullname);

	*p++ = 0;			/* Mark end of host name */
	*p++ = 0;			/* Some servers require double null */
	*p++ = (unsigned char)qtype;	/* Query Type */

	*p++ = 0;
	*p++ = 1;				/* Class: inet, 0x0001 */

	ret = net_udp_send(dns_con, p - packet);

	free(fullname);

	return ret;
}

static void dns_recv(struct header *header, unsigned len)
{
	unsigned char *p, *e, *s;
	u16 type;
	int found, stop, dlen;
	short tmp;

	pr_debug("%s\n", __func__);

	/* We sent 1 query. We want to see more that 1 answer. */
	if (ntohs(header->nqueries) != 1)
		return;

	/* Received 0 answers */
	if (header->nanswers == 0) {
		dns_state = STATE_DONE;
		pr_debug("DNS server returned no answers\n");
		return;
	}

	/* Skip host name */
	s = &header->data[0];
	e = ((uint8_t *)header) + len;
	for (p = s; p < e && *p != '\0'; p++)
		continue;

	/* We sent query class 1, query type 1 */
	tmp = p[1] | (p[2] << 8);
	if (&p[5] > e || ntohs(tmp) != DNS_A_RECORD) {
		pr_debug("DNS response was not A record\n");
		return;
	}

	/* Go to the first answer section */
	p += 5;

	/* Loop through the answers, we want A type answer */
	for (found = stop = 0; !stop && &p[12] < e; ) {

		/* Skip possible name in CNAME answer */
		if (*p != 0xc0) {
			while (*p && &p[12] < e)
				p++;
			p--;
		}
		pr_debug("Name (Offset in header): %d\n", p[1]);

		tmp = p[2] | (p[3] << 8);
		type = ntohs(tmp);
		pr_debug("type = %d\n", type);
		if (type == DNS_CNAME_RECORD) {
			/* CNAME answer. shift to the next section */
			debug("Found canonical name\n");
			tmp = p[10] | (p[11] << 8);
			dlen = ntohs(tmp);
			pr_debug("dlen = %d\n", dlen);
			p += 12 + dlen;
		} else if (type == DNS_A_RECORD) {
			pr_debug("Found A-record\n");
			found = stop = 1;
		} else {
			pr_debug("Unknown type\n");
			stop = 1;
		}
	}

	if (found && &p[12] < e) {

		tmp = p[10] | (p[11] << 8);
		dlen = ntohs(tmp);
		p += 12;
		dns_ip = net_read_ip(p);
		dns_state = STATE_DONE;
	}
}

static void dns_handler(void *ctx, char *packet, unsigned len)
{
	(void)ctx;
	dns_recv((struct header *)net_eth_to_udp_payload(packet),
		net_eth_to_udplen(packet));
}

int resolv(const char *host, IPaddr_t *ip)
{
	IPaddr_t nameserver;

	if (!string_to_ip(host, ip))
		return 0;

	dns_ip = 0;
	*ip = 0;

	dns_state = STATE_INIT;

	nameserver = net_get_nameserver();
	if (!nameserver) {
		pr_err("no nameserver specified in $net.nameserver\n");
		return 0;
	}

	pr_debug("resolving host %s via nameserver %pI4\n", host, &nameserver);

	dns_con = net_udp_new(nameserver, DNS_PORT, dns_handler, NULL);
	if (IS_ERR(dns_con))
		return PTR_ERR(dns_con);
	dns_timer_start = get_time_ns();
	dns_send(host);

	while (dns_state != STATE_DONE) {
		if (ctrlc()) {
			break;
		}
		net_poll();
		if (is_timeout(dns_timer_start, SECOND)) {
			dns_timer_start = get_time_ns();
			printf("T ");
			dns_send(host);
		}
	}

	net_unregister(dns_con);

	if (dns_ip) {
		pr_debug("host %s is at %pI4\n", host, &dns_ip);
	} else {
		pr_debug("host %s not found\n", host);
		return -ENOENT;
	}

	*ip = dns_ip;

	return 0;
}

#ifdef CONFIG_CMD_HOST
static int do_host(int argc, char *argv[])
{
	IPaddr_t ip;
	int ret;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	ret = resolv(argv[1], &ip);
	if (ret)
		printf("unknown host %s\n", argv[1]);
	else
		printf("%s is at %pI4\n", argv[1], &ip);

	return 0;
}

BAREBOX_CMD_START(host)
	.cmd		= do_host,
	BAREBOX_CMD_DESC("resolve a hostname")
	BAREBOX_CMD_OPTS("HOSTNAME")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
#endif
