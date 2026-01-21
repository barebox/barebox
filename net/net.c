// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 1994-2000 Neil Russell
// SPDX-FileCopyrightText: 2000 Roland Borde
// SPDX-FileCopyrightText: 2000 Paolo Scaffardi
// SPDX-FileCopyrightText: 2000-2002 Wolfgang Denk <wd@denx.de>

/*
 * net.c - barebox networking support
 *
 * based on U-Boot (LiMon) code
 */

#define pr_fmt(fmt) "net: " fmt

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
#include <globalvar.h>
#include <magicvar.h>
#include <machine_id.h>
#include <linux/ctype.h>
#include <linux/err.h>

static unsigned int net_ip_id;

char *net_server;
IPaddr_t net_gateway;
static IPaddr_t net_nameserver;
static char *net_domainname;

void net_set_nameserver(IPaddr_t nameserver)
{
	net_nameserver = nameserver;
}

IPaddr_t net_get_nameserver(void)
{
	return net_nameserver;
}

void net_set_domainname(const char *name)
{
	free(net_domainname);
	if (name)
		net_domainname = xstrdup(name);
	else
		net_domainname = xstrdup("");
};

const char *net_get_domainname(void)
{
	return net_domainname;
}

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

	resolv(var, &ip);

	return ip;
}

int setenv_ip(const char *name, IPaddr_t ip)
{
	char str[sizeof("255.255.255.255")];

	sprintf(str, "%pI4", &ip);

	setenv(name, str);

	return 0;
}

/**
 * struct pending_arp - Pending ARP state
 * @ip: input IPv4 address whose resolution is being requested 
 * @ether: output MAC addess buffer after receing a response
 */
static struct pending_arp {
	IPaddr_t ip;
	unsigned char *ether;
} pending_arp;

static void arp_handler(struct arprequest *arp)
{
	IPaddr_t tmp;

	/* are we waiting for a reply */
	if (!pending_arp.ip)
		return;

	tmp = net_read_ip(&arp->ar_data[6]);

	/* matched waiting packet's address */
	if (tmp == pending_arp.ip) {
		/* save address for later use */
		memcpy(pending_arp.ether, &arp->ar_data[0], 6);

		/* no arp request pending now */
		pending_arp.ip = 0;
	}
}

struct eth_device *net_route(IPaddr_t dest)
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		if (!edev->ipaddr || !edev->ifup)
			continue;

		if ((dest & edev->netmask) == (edev->ipaddr & edev->netmask)) {
			pr_debug("Route: Using %s (ip=%pI4, nm=%pI4) to reach %pI4\n",
			      dev_name(&edev->dev), &edev->ipaddr, &edev->netmask,
				       &dest);
			return edev;
		}
	}

	pr_debug("Route: No device found for %pI4\n", &dest);

	return NULL;
}

static int arp_request(struct eth_device *edev, IPaddr_t dest, unsigned char *ether)
{
	char *pkt;
	struct arprequest *arp;
	uint64_t arp_start;
	static char *arp_packet;
	struct ethernet *et;
	unsigned retries = 0;
	IPaddr_t arp_wait_ip;
	int ret;

	if (!edev)
		return -EHOSTUNREACH;

	if (!arp_packet) {
		arp_packet = net_alloc_packet();
		if (!arp_packet)
			return -ENOMEM;
	}

	pkt = arp_packet;
	et = (struct ethernet *)arp_packet;

	arp_wait_ip = dest;

	pr_debug("send ARP broadcast for %pI4\n", &dest);

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
		if (!net_gateway)
			arp_wait_ip = dest;
		else
			arp_wait_ip = net_gateway;
	} else {
		arp_wait_ip = dest;
	}

	net_write_ip(arp->ar_data + 16, arp_wait_ip);

	pending_arp.ether = ether;
	pending_arp.ip = arp_wait_ip;

	ret = eth_send(edev, arp_packet, ETHER_HDR_SIZE + ARP_HDR_SIZE);
	if (ret)
		goto out;
	arp_start = get_time_ns();

	while (pending_arp.ip) {
		if (ctrlc()) {
			ret = -EINTR;
			goto out;
		}

		if (is_timeout(arp_start, 3 * SECOND)) {
			printf("T ");
			arp_start = get_time_ns();
			ret = eth_send(edev, arp_packet, ETHER_HDR_SIZE + ARP_HDR_SIZE);
			if (ret)
				goto out;
			retries++;
		}

		if (retries > PKT_NUM_RETRIES) {
			ret = -ETIMEDOUT;
			goto out;
		}

		net_poll();
	}

	pr_debug("Got ARP REPLY for %pI4: %02x:%02x:%02x:%02x:%02x:%02x\n",
		 &dest, ether[0], ether[1], ether[2], ether[3], ether[4],
		 ether[5]);

out:
	pending_arp.ip = 0;
	pending_arp.ether = NULL;
	return ret;
}

void net_poll(void)
{
	static bool in_net_poll;

	if (in_net_poll)
		return;

	in_net_poll = true;

	eth_rx();

	in_net_poll = false;
}

static void __net_poll(struct poller_struct *poller)
{
	static uint64_t last;

	/*
	 * USB network controllers take a long time in the receive path,
	 * so limit the polling rate to once per 10ms. This is due to
	 * deficiencies in the barebox USB stack: We can't queue URBs and
	 * receive a callback when they are done. Instead, we always
	 * synchronously queue an URB and wait for its completion. In case
	 * of USB network adapters the only way to detect if packets have
	 * been received is to queue a RX URB and see if it completes (in
	 * which case we have received data) or if it timeouts (no data
	 * available). The timeout can't be arbitrarily small, 2ms is the
	 * smallest we can do with the 1ms USB frame size.
	 *
	 * Given that we do a mixture of polling-as-fast-as-possible when
	 * we are waiting for network traffic (tftp, nfs and other users
	 * actively calling net_poll()) and doing a low frequency polling
	 * here to still get packets when no user is actively waiting for
	 * incoming packets. This is used to receive incoming ping packets
	 * and to get fastboot over ethernet going.
	 */
	if (!is_timeout(last, 10 * MSECOND))
		return;

	net_poll();

	last = get_time_ns();
}

static struct poller_struct net_poller = {
	.func = __net_poll,
};

static int init_net_poll(void)
{
	return poller_register(&net_poller, "net");
}
device_initcall(init_net_poll);

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
	IPaddr_t ip;
	int ret;

	ret = resolv(net_server, &ip);
	if (ret)
		return 0;

	return ip;
}

void net_set_serverip(IPaddr_t ip)
{
	free(net_server);

	net_server = xasprintf("%pI4", &ip);
}

const char *net_get_server(void)
{
	return net_server && *net_server ? net_server : NULL;
}

void net_set_serverip_empty(IPaddr_t ip)
{
	if (net_get_server())
		return;

	net_set_serverip(ip);
}

void net_set_ip(struct eth_device *edev, IPaddr_t ip)
{
	edev->ipaddr = ip;
}

IPaddr_t net_get_ip(struct eth_device *edev)
{
	return edev->ipaddr;
}
EXPORT_SYMBOL(net_get_ip);

void net_set_netmask(struct eth_device *edev, IPaddr_t nm)
{
	edev->netmask = nm;
}

void net_set_gateway(struct eth_device *edev, IPaddr_t gw)
{
	net_gateway = gw;
}

IPaddr_t net_get_gateway(void)
{
	return net_gateway;
}

static LIST_HEAD(connection_list);

/**
 * generate_ether_addr - Generates stable software assigned Ethernet address
 * @addr: Pointer to a six-byte array to contain the Ethernet address
 * @ethid: index of the Ethernet interface
 *
 * Derives an Ethernet address (MAC) from the machine ID, that's stable
 * per board that is not multicast and has the local assigned bit set.
 *
 * Return 0 if an address could be generated or a negative error code otherwise.
 */
int generate_ether_addr(u8 *ethaddr, int ethid)
{
	const char *hostname;
	uuid_t id;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_ETHADDR_FROM_MACHINE_ID))
		return -ENOSYS;

	hostname = barebox_get_hostname();
	if (!hostname)
		return -EINVAL;

	ret = machine_id_get_app_specific(&id, ARRAY_AND_SIZE("barebox-macaddr:"),
					  hostname, strlen(hostname), NULL);
	if (ret)
		return ret;

	memcpy(ethaddr, &id.b, ETH_ALEN);
	eth_addr_add(ethaddr, ethid);

	ethaddr[0] &= 0xfe;	/* clear multicast bit */
	ethaddr[0] |= 0x02;	/* set local assignment bit (IEEE802) */

	return 0;
}

static struct net_connection *net_new(struct eth_device *edev, IPaddr_t dest,
				      rx_handler_f *handler, void *ctx)
{
	struct net_connection *con;
	int ret;

	if (!edev) {
		edev = net_route(dest);
		if (!edev && net_gateway)
			edev = net_route(net_gateway);
		if (!edev)
			return ERR_PTR(-EHOSTUNREACH);
	}

	if (!is_valid_ether_addr(edev->ethaddr)) {
		ret = generate_ether_addr(edev->ethaddr, edev->dev.id);
		if (ret)
			random_ether_addr(edev->ethaddr);

		dev_warn(&edev->dev, "No MAC address set. Using %s %pM\n",
			 ret == 1 ? "address computed from unique ID" : "random address",
			 edev->ethaddr);
		eth_set_ethaddr(edev, edev->ethaddr);
	}

	/* If we don't have an ip only broadcast is allowed */
	if (!edev->ipaddr && dest != IP_BROADCAST)
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

	if (dest == IP_BROADCAST) {
		memset(con->et->et_dest, 0xff, 6);
	} else {
		ret = arp_request(edev, dest, con->et->et_dest);
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
	net_free_packet(con->packet);
	free(con);
	return ERR_PTR(ret);
}

struct net_connection *net_udp_eth_new(struct eth_device *edev, IPaddr_t dest,
				       uint16_t dport, rx_handler_f *handler,
				       void *ctx)
{
	struct net_connection *con = net_new(edev, dest, handler, ctx);

	if (IS_ERR(con))
		return con;

	con->proto = IPPROTO_UDP;
	con->udp->uh_dport = htons(dport);
	con->udp->uh_sport = htons(net_udp_new_localport());
	con->ip->protocol = IPPROTO_UDP;

	return con;
}

struct net_connection *net_udp_new(IPaddr_t dest, uint16_t dport,
		rx_handler_f *handler, void *ctx)
{
	return net_udp_eth_new(NULL, dest, dport, handler, ctx);
}

struct net_connection *net_icmp_new(IPaddr_t dest, rx_handler_f *handler,
		void *ctx)
{
	struct net_connection *con = net_new(NULL, dest, handler, ctx);

	if (IS_ERR(con))
		return con;

	con->proto = IPPROTO_ICMP;
	con->ip->protocol = IPPROTO_ICMP;

	return con;
}

void net_unregister(struct net_connection *con)
{
	list_del(&con->list);
	net_free_packet(con->packet);
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

	pr_debug("%s\n", __func__);

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
	net_free_packet(packet);

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

	pr_debug("%s: got arp\n", __func__);

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

static struct iphdr *ip_verify_size(unsigned char *pkt, int *total_len_nic)
{
	struct iphdr *ip = (struct iphdr *)(pkt + ETHER_HDR_SIZE);
	int total_len_pkt;

	total_len_pkt = ntohs(ip->tot_len) + ETHER_HDR_SIZE;

	/* Only trust the packet's size if it's within bounds */
	if (*total_len_nic < sizeof(struct ethernet) + sizeof(struct iphdr) ||
	    *total_len_nic < total_len_pkt) {
		net_bad_packet(pkt, *total_len_nic);
		return NULL;
	}

#ifdef DEBUG
	/* Hitting this warning means we have trailing bytes after the IP
	 * payload that are not needed for padding.
	 *
	 * This may be an indication that the NIC driver is doing funny
	 * offloading stuff it shouldn't, but can also mean that some sender
	 * in the network likes to waste bit time for nought.
	 *
	 * We can't differentiate between the two, so we just print the
	 * warning when DEBUG is defined, so developers may investigate the
	 * reason without annoying users about something that might not even
	 * be barebox's fault.
	 */
	if (WARN_ON_ONCE(*total_len_nic > total_len_pkt &&
			 *total_len_nic > 64)) {
		net_bad_packet(pkt, *total_len_nic);
	}
#endif

	*total_len_nic = total_len_pkt;
	return ip;
}

static int ping_reply(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct ethernet *et = (struct ethernet *)pkt;
	struct icmphdr *icmp;
	struct iphdr *ip;
	unsigned char *packet;
	int ret;

	memcpy(et->et_dest, et->et_src, 6);
	memcpy(et->et_src, edev->ethaddr, 6);
	et->et_protlen = htons(PROT_IP);

	icmp = net_eth_to_icmphdr(pkt);

	ip = ip_verify_size(pkt, &len);
	if (!ip)
		return -EILSEQ;

	icmp->type = ICMP_ECHO_REPLY;
	icmp->checksum = 0;
	icmp->checksum = ~net_checksum((unsigned char *)icmp,
				       len - sizeof(struct iphdr) - ETHER_HDR_SIZE);
	ip->check = 0;
	ip->frag_off = htons(0x4000);
	ip->ttl = 255;
	net_copy_ip((void *)&ip->daddr, &ip->saddr);
	net_copy_ip((void *)&ip->saddr, &edev->ipaddr);
	ip->check = ~net_checksum((unsigned char *)ip, sizeof(struct iphdr));

	packet = net_alloc_packet();
	if (!packet)
		return 0;

	memcpy(packet, pkt, len);

	ret = eth_send(edev, packet, len);

	net_free_packet(packet);

	return ret;
}

static int net_handle_icmp(struct eth_device *edev, unsigned char *pkt, int len)
{
	struct net_connection *con;
	struct icmphdr *icmp;

	pr_debug("%s\n", __func__);

	icmp = net_eth_to_icmphdr(pkt);
	if (icmp->type == ICMP_ECHO_REQUEST)
		ping_reply(edev, pkt, len);

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

	pr_debug("%s\n", __func__);

	ip = ip_verify_size(pkt, &len);
	if (!ip) {
		pr_debug("%s: bad len\n", __func__);
		return 0;
	}

	if ((ip->hl_v & 0xf0) != 0x40)
		goto bad;

	/* Can't deal w/ fragments.
	 * Either a fragment offset (13 bits), or
	 * MF (More Fragments) from fragment flags (3 bits).
	 * MF - because first fragment has fragment offset 0
	 */
	if (ip->frag_off & htons(0x3fff))
		goto bad;
	if (!net_checksum_ok((unsigned char *)ip, sizeof(struct iphdr)))
		goto bad;

	tmp = net_read_ip(&ip->daddr);
	if (edev->ipaddr && tmp != edev->ipaddr && tmp != IP_BROADCAST)
		return 0;

	switch (ip->protocol) {
	case IPPROTO_ICMP:
		return net_handle_icmp(edev, pkt, len);
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

	if (edev->rx_monitor)
		edev->rx_monitor(edev, pkt, len);

	if (edev->rx_preprocessor) {
		ret = edev->rx_preprocessor(edev, &pkt, &len);
		if (ret == -ENOMSG)
			return 0;
		if (ret) {
			pr_debug("%s: rx_preprocessor failed %pe\n", __func__,
				 ERR_PTR(ret));
			return ret;
		}
	}

	switch (et_protlen) {
	case PROT_ARP:
		ret = net_handle_arp(edev, pkt, len);
		break;
	case PROT_IP:
		ret = net_handle_ip(edev, pkt, len);
		break;
	default:
		pr_debug("%s: got unknown protocol type: %d\n", __func__, et_protlen);
		ret = 1;
		break;
	}
out:
	return ret;
}

void net_free_packets(void **packets, unsigned count)
{
	while (count-- > 0)
		net_free_packet(packets[count]);
}

int net_alloc_packets(void **packets, int count)
{
	void *packet;
	int i;

	for (i = 0; i < count; i++) {
		packet = net_alloc_packet();
		if (!packet)
			goto free;
		packets[i] = packet;
	}

	return 0;
free:
	net_free_packets(packets, i);
	return -ENOMEM;
}

static int net_init(void)
{
	globalvar_add_simple_ip("net.nameserver", &net_nameserver);
	globalvar_add_simple_string("net.domainname", &net_domainname);
	globalvar_add_simple_string("net.server", &net_server);
	globalvar_add_simple_ip("net.gateway", &net_gateway);

	return 0;
}

postcore_initcall(net_init);

BAREBOX_MAGICVAR(global.net.nameserver, "The DNS server used for resolving host names");
BAREBOX_MAGICVAR(global.net.domainname, "Domain name used for DNS requests");
BAREBOX_MAGICVAR(global.net.server, "Standard server used for NFS/TFTP");
