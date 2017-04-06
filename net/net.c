/*
 * net.c - barebox networking support
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on U-Boot (LiMon) code
 *
 * Copyright 1994 - 2000 Neil Russell.
 * Copyright 2000 Roland Borde
 * Copyright 2000 Paolo Scaffardi
 * Copyright 2000-2002 Wolfgang Denk, wd@denx.de
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <clock.h>
#include <command.h>
#include <environment.h>
#include <param.h>
#include <net.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <init.h>
#include <linux/ctype.h>
#include <linux/err.h>

unsigned char *NetRxPackets[PKTBUFSRX]; /* Receive packets		*/
static unsigned int net_ip_id;

int net_checksum_ok(unsigned char *ptr, int len)
{
	return net_checksum(ptr, len) == 0xffff;
}

uint16_t net_checksum(unsigned char *ptr, int len)
{
	uint32_t xsum = 0;
	uint16_t *p = (uint16_t *)ptr;

	if (len & 1)
		ptr[len] = 0;

	len = (len + 1) >> 1;

	while (len-- > 0)
		xsum += *p++;

	xsum = (xsum & 0xffff) + (xsum >> 16);
	xsum = (xsum & 0xffff) + (xsum >> 16);
	return xsum & 0xffff;
}

IPaddr_t getenv_ip(const char *name)
{
	IPaddr_t ip;
	const char *var = getenv(name);

	if (!var)
		return 0;

	if (!string_to_ip(var, &ip))
		return ip;

	return resolv((char*)var);
}

int setenv_ip(const char *name, IPaddr_t ip)
{
	char str[sizeof("255.255.255.255")];

	sprintf(str, "%pI4", &ip);

	setenv(name, str);

	return 0;
}

static unsigned char *arp_ether;
static IPaddr_t arp_wait_ip;

static void arp_handler(struct arprequest *arp)
{
	IPaddr_t tmp;

	/* are we waiting for a reply */
	if (!arp_wait_ip)
		return;

	tmp = net_read_ip(&arp->ar_data[6]);

	/* matched waiting packet's address */
	if (tmp == arp_wait_ip) {
		/* save address for later use */
		memcpy(arp_ether, &arp->ar_data[0], 6);

		/* no arp request pending now */
		arp_wait_ip = 0;
	}
}

static int arp_request(IPaddr_t dest, unsigned char *ether)
{
	struct eth_device *edev = eth_get_current();
	char *pkt;
	struct arprequest *arp;
	uint64_t arp_start;
	static char *arp_packet;
	struct ethernet *et;
	unsigned retries = 0;
	int ret;

	if (!arp_packet) {
		arp_packet = net_alloc_packet();
		if (!arp_packet)
			return -ENOMEM;
	}

	pkt = arp_packet;
	et = (struct ethernet *)arp_packet;

	arp_wait_ip = dest;

	pr_debug("ARP broadcast\n");

	memset(et->et_dest, 0xff, 6);
	memcpy(et->et_src, edev->ethaddr, 6);
	et->et_protlen = htons(PROT_ARP);

	arp = (struct arprequest *)(pkt + ETHER_HDR_SIZE);

	arp->ar_hrd = htons(ARP_ETHER);
	arp->ar_pro = htons(PROT_IP);
	arp->ar_hln = 6;
	arp->ar_pln = 4;
	arp->ar_op = htons(ARPOP_REQUEST);

	memcpy(arp->ar_data, edev->ethaddr, 6);	/* source ET addr	*/
	net_write_ip(arp->ar_data + 6, edev->ipaddr);	/* source IP addr	*/
	memset(arp->ar_data + 10, 0, 6);	/* dest ET addr = 0     */

	if ((dest & edev->netmask) != (edev->ipaddr & edev->netmask)) {
		if (!edev->gateway)
			arp_wait_ip = dest;
		else
			arp_wait_ip = edev->gateway;
	} else {
		arp_wait_ip = dest;
	}

	net_write_ip(arp->ar_data + 16, arp_wait_ip);

	arp_ether = ether;

	ret = eth_send(edev, arp_packet, ETHER_HDR_SIZE + ARP_HDR_SIZE);
	if (ret)
		return ret;
	arp_start = get_time_ns();

	while (arp_wait_ip) {
		if (ctrlc())
			return -EINTR;

		if (is_timeout(arp_start, 3 * SECOND)) {
			printf("T ");
			arp_start = get_time_ns();
			ret = eth_send(edev, arp_packet, ETHER_HDR_SIZE + ARP_HDR_SIZE);
			if (ret)
				return ret;
			retries++;
		}

		if (retries > PKT_NUM_RETRIES)
			return -ETIMEDOUT;

		net_poll();
	}

	pr_debug("Got ARP REPLY, set server/gtwy eth addr (%02x:%02x:%02x:%02x:%02x:%02x)\n",
		ether[0], ether[1],
		ether[2], ether[3],
		ether[4], ether[5]);
	return 0;
}

void net_poll(void)
{
	eth_rx();
}

static uint16_t net_udp_new_localport(void)
{
	static uint16_t localport;

	localport++;

	if (localport < 1024)
		localport = 1024;

	return localport;
}

IPaddr_t net_get_serverip(void)
{
	struct eth_device *edev = eth_get_current();

	return edev->serverip;
}

void net_set_serverip(IPaddr_t ip)
{
	struct eth_device *edev = eth_get_current();

	edev->serverip = ip;
}

void net_set_ip(IPaddr_t ip)
{
	struct eth_device *edev = eth_get_current();

	edev->ipaddr = ip;
}

IPaddr_t net_get_ip(void)
{
	struct eth_device *edev = eth_get_current();

	return edev->ipaddr;
}

void net_set_netmask(IPaddr_t nm)
{
	struct eth_device *edev = eth_get_current();

	edev->netmask = nm;
}

void net_set_gateway(IPaddr_t gw)
{
	struct eth_device *edev = eth_get_current();

	edev->gateway = gw;
}

static LIST_HEAD(connection_list);

static struct net_connection *net_new(IPaddr_t dest, rx_handler_f *handler,
		void *ctx)
{
	struct eth_device *edev = eth_get_current();
	struct net_connection *con;
	int ret;

	if (!edev)
		return ERR_PTR(-ENETDOWN);

	if (!is_valid_ether_addr(edev->ethaddr)) {
		char str[sizeof("xx:xx:xx:xx:xx:xx")];
		random_ether_addr(edev->ethaddr);
		ethaddr_to_string(edev->ethaddr, str);
		printf("warning: No MAC address set. Using random address %s\n", str);
		eth_set_ethaddr(edev, edev->ethaddr);
	}

	/* If we don't have an ip only broadcast is allowed */
	if (!edev->ipaddr && dest != 0xffffffff)
		return ERR_PTR(-ENETDOWN);

	con = xzalloc(sizeof(*con));
	con->packet = net_alloc_packet();
	con->priv = ctx;
	con->edev = edev;
	memset(con->packet, 0, PKTSIZE);

	con->et = (struct ethernet *)con->packet;
	con->ip = (struct iphdr *)(con->packet + ETHER_HDR_SIZE);
	con->udp = (struct udphdr *)(con->packet + ETHER_HDR_SIZE + sizeof(struct iphdr));
	con->icmp = (struct icmphdr *)(con->packet + ETHER_HDR_SIZE + sizeof(struct iphdr));
	con->handler = handler;

	if (dest == 0xffffffff) {
		memset(con->et->et_dest, 0xff, 6);
	} else {
		ret = arp_request(dest, con->et->et_dest);
		if (ret)
			goto out;
	}

	con->et->et_protlen = htons(PROT_IP);
	memcpy(con->et->et_src, edev->ethaddr, 6);

	con->ip->hl_v = 0x45;
	con->ip->tos = 0;
	con->ip->frag_off = htons(0x4000);	/* No fragmentation */;
	con->ip->ttl = 255;
	net_copy_ip(&con->ip->daddr, &dest);
	net_copy_ip(&con->ip->saddr, &edev->ipaddr);

	list_add_tail(&con->list, &connection_list);

	return con;
out:
	free(con->packet);
	free(con);
	return ERR_PTR(ret);
}

struct net_connection *net_udp_new(IPaddr_t dest, uint16_t dport,
		rx_handler_f *handler, void *ctx)
{
	struct net_connection *con = net_new(dest, handler, ctx);

	if (IS_ERR(con))
		return con;

	con->proto = IPPROTO_UDP;
	con->udp->uh_dport = htons(dport);
	con->udp->uh_sport = htons(net_udp_new_localport());
	con->ip->protocol = IPPROTO_UDP;

	return con;
}

struct net_connection *net_icmp_new(IPaddr_t dest, rx_handler_f *handler,
		void *ctx)
{
	struct net_connection *con = net_new(dest, handler, ctx);

	if (IS_ERR(con))
		return con;

	con->proto = IPPROTO_ICMP;
	con->ip->protocol = IPPROTO_ICMP;

	return con;
}

void net_unregister(struct net_connection *con)
{
	list_del(&con->list);
	free(con->packet);
	free(con);
}

static int net_ip_send(struct net_connection *con, int len)
{
	con->ip->tot_len = htons(sizeof(struct iphdr) + len);
	con->ip->id = htons(net_ip_id++);
	con->ip->check = 0;
	con->ip->check = ~net_checksum((unsigned char *)con->ip, sizeof(struct iphdr));

	return eth_send(con->edev, con->packet, ETHER_HDR_SIZE + sizeof(struct iphdr) + len);
}

int net_udp_send(struct net_connection *con, int len)
{
	con->udp->uh_ulen = htons(len + 8);
	con->udp->uh_sum = 0;

	return net_ip_send(con, sizeof(struct udphdr) + len);
}

int net_icmp_send(struct net_connection *con, int len)
{
	con->icmp->checksum = ~net_checksum((unsigned char *)con->icmp,
			sizeof(struct icmphdr) + len);

	return net_ip_send(con, sizeof(struct icmphdr) + len);
}

static int net_answer_arp(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct arprequest *arp = (struct arprequest *)(pkt + ETHER_HDR_SIZE);
	struct ethernet *et = (struct ethernet *)pkt;
	unsigned char *packet;
	int ret;

	debug("%s\n", __func__);

	memcpy (et->et_dest, et->et_src, 6);
	memcpy (et->et_src, edev->ethaddr, 6);

	et->et_protlen = htons(PROT_ARP);
	arp->ar_op = htons(ARPOP_REPLY);
	memcpy(&arp->ar_data[10], &arp->ar_data[0], 6);
	net_copy_ip(&arp->ar_data[16], &arp->ar_data[6]);
	memcpy(&arp->ar_data[0], edev->ethaddr, 6);
	net_copy_ip(&arp->ar_data[6], &edev->ipaddr);

	packet = net_alloc_packet();
	if (!packet)
		return 0;
	memcpy(packet, pkt, ETHER_HDR_SIZE + ARP_HDR_SIZE);
	ret = eth_send(edev, packet, ETHER_HDR_SIZE + ARP_HDR_SIZE);
	free(packet);

	return ret;
}

static void net_bad_packet(unsigned char *pkt, int len)
{
#ifdef DEBUG
	/*
	 * We received a bad packet. for now just dump it.
	 * We could add more sophisticated debugging here
	 */
	memory_display(pkt, 0, len, 1, 0);
#endif
}

static int net_handle_arp(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct arprequest *arp;

	debug("%s: got arp\n", __func__);

	/*
	 * We have to deal with two types of ARP packets:
	 * - REQUEST packets will be answered by sending  our
	 *   IP address - if we know it.
	 * - REPLY packets are expected only after we asked
	 *   for the TFTP server's or the gateway's ethernet
	 *   address; so if we receive such a packet, we set
	 *   the server ethernet address
	 */
	arp = (struct arprequest *)(pkt + ETHER_HDR_SIZE);
	if (len < ARP_HDR_SIZE)
		goto bad;
	if (ntohs(arp->ar_hrd) != ARP_ETHER)
		goto bad;
	if (ntohs(arp->ar_pro) != PROT_IP)
		goto bad;
	if (arp->ar_hln != 6)
		goto bad;
	if (arp->ar_pln != 4)
		goto bad;
	if (edev->ipaddr == 0)
		return 0;
	if (net_read_ip(&arp->ar_data[16]) != edev->ipaddr)
		return 0;

	switch (ntohs(arp->ar_op)) {
	case ARPOP_REQUEST:
		return net_answer_arp(edev, pkt, len);
	case ARPOP_REPLY:
		arp_handler(arp);
		return 1;
	default:
		pr_debug("Unexpected ARP opcode 0x%x\n", ntohs(arp->ar_op));
		return -EINVAL;
	}

	return 0;

bad:
	net_bad_packet(pkt, len);
	return -EINVAL;
}

static int net_handle_udp(unsigned char *pkt, int len)
{
	struct iphdr *ip = (struct iphdr *)(pkt + ETHER_HDR_SIZE);
	struct net_connection *con;
	struct udphdr *udp;
	int port;

	udp = (struct udphdr *)(ip + 1);
	port = ntohs(udp->uh_dport);
	list_for_each_entry(con, &connection_list, list) {
		if (con->proto == IPPROTO_UDP && port == ntohs(con->udp->uh_sport)) {
			con->handler(con->priv, pkt, len);
			return 0;
		}
	}
	return -EINVAL;
}

static int net_handle_icmp(unsigned char *pkt, int len)
{
	struct net_connection *con;

	debug("%s\n", __func__);

	list_for_each_entry(con, &connection_list, list) {
		if (con->proto == IPPROTO_ICMP) {
			con->handler(con->priv, pkt, len);
			return 0;
		}
	}
	return 0;
}

static int net_handle_ip(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct iphdr *ip = (struct iphdr *)(pkt + ETHER_HDR_SIZE);
	IPaddr_t tmp;

	debug("%s\n", __func__);

	if (len < sizeof(struct ethernet) + sizeof(struct iphdr) ||
		len < ETHER_HDR_SIZE + ntohs(ip->tot_len)) {
		debug("%s: bad len\n", __func__);
		goto bad;
	}

	if ((ip->hl_v & 0xf0) != 0x40)
		goto bad;

	if (ip->frag_off & htons(0x1fff)) /* Can't deal w/ fragments */
		goto bad;
	if (!net_checksum_ok((unsigned char *)ip, sizeof(struct iphdr)))
		goto bad;

	tmp = net_read_ip(&ip->daddr);
	if (edev->ipaddr && tmp != edev->ipaddr && tmp != 0xffffffff)
		return 0;

	switch (ip->protocol) {
	case IPPROTO_ICMP:
		return net_handle_icmp(pkt, len);
	case IPPROTO_UDP:
		return net_handle_udp(pkt, len);
	}

	return 0;
bad:
	net_bad_packet(pkt, len);
	return 0;
}

int net_receive(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct ethernet *et = (struct ethernet *)pkt;
	int et_protlen = ntohs(et->et_protlen);
	int ret;

	led_trigger_network(LED_TRIGGER_NET_RX);

	if (len < ETHER_HDR_SIZE) {
		ret = 0;
		goto out;
	}

	switch (et_protlen) {
	case PROT_ARP:
		ret = net_handle_arp(edev, pkt, len);
		break;
	case PROT_IP:
		ret = net_handle_ip(edev, pkt, len);
		break;
	default:
		debug("%s: got unknown protocol type: %d\n", __func__, et_protlen);
		ret = 1;
		break;
	}
out:
	return ret;
}

static struct device_d net_device = {
	.name = "net",
	.id = DEVICE_ID_SINGLE,
};

static char *net_nameserver;
static char *net_domainname;

static int net_init(void)
{
	int i;

	for (i = 0; i < PKTBUFSRX; i++)
		NetRxPackets[i] = net_alloc_packet();

	register_device(&net_device);
	net_nameserver = xstrdup("");
	dev_add_param_string(&net_device, "nameserver", NULL, NULL,
			     &net_nameserver, NULL);
	net_domainname = xstrdup("");
	dev_add_param_string(&net_device, "domainname", NULL, NULL,
			     &net_domainname, NULL);

	return 0;
}

postcore_initcall(net_init);
