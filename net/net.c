/*
 *	Copied from Linux Monitor (LiMon) - Networking.
 *
 *	Copyright 1994 - 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000 Roland Borde
 *	Copyright 2000 Paolo Scaffardi
 *	Copyright 2000-2002 Wolfgang Denk, wd@denx.de
 */

/*
 * General Desription:
 *
 * The user interface supports commands for BOOTP, RARP, and TFTP.
 * Also, we support ARP internally. Depending on available data,
 * these interact as follows:
 *
 * BOOTP:
 *
 *	Prerequisites:	- own ethernet address
 *	We want:	- own IP address
 *			- TFTP server IP address
 *			- name of bootfile
 *	Next step:	ARP
 *
 * RARP:
 *
 *	Prerequisites:	- own ethernet address
 *	We want:	- own IP address
 *			- TFTP server IP address
 *	Next step:	ARP
 *
 * ARP:
 *
 *	Prerequisites:	- own ethernet address
 *			- own IP address
 *			- TFTP server IP address
 *	We want:	- TFTP server ethernet address
 *	Next step:	TFTP
 *
 * DHCP:
 *
 *     Prerequisites:	- own ethernet address
 *     We want:		- IP, Netmask, ServerIP, Gateway IP
 *			- bootfilename, lease time
 *     Next step:	- TFTP
 *
 * TFTP:
 *
 *	Prerequisites:	- own ethernet address
 *			- own IP address
 *			- TFTP server IP address
 *			- TFTP server ethernet address
 *			- name of bootfile (if unknown, we use a default name
 *			  derived from our own IP address)
 *	We want:	- load the boot file
 *	Next step:	none
 *
 * NFS:
 *
 *	Prerequisites:	- own ethernet address
 *			- own IP address
 *			- name of bootfile (if unknown, we use a default name
 *			  derived from our own IP address)
 *	We want:	- load the boot file
 *	Next step:	none
 *
 */


#include <common.h>
#include <clock.h>
#include <watchdog.h>
#include <command.h>
#include <environment.h>
#include <param.h>
#include <net.h>
#include <driver.h>
#include <errno.h>
#include <linux/ctype.h>
#include "tftp.h"
#include "rarp.h"
#include "nfs.h"

#define ARP_TIMEOUT		(5 * SECOND)	/* Seconds before trying ARP again */
#ifndef	CONFIG_NET_RETRY_COUNT
# define ARP_TIMEOUT_COUNT	5		/* # of timeouts before giving up  */
#else
# define ARP_TIMEOUT_COUNT  (CONFIG_NET_RETRY_COUNT)
#endif


/** BOOTP EXTENTIONS **/

IPaddr_t	NetOurSubnetMask=0;		/* Our subnet mask (0=unknown)	*/
IPaddr_t	NetOurGatewayIP=0;		/* Our gateways IP address	*/
IPaddr_t	NetOurDNSIP=0;			/* Our DNS IP address		*/
#ifdef CONFIG_BOOTP_DNS2
IPaddr_t	NetOurDNS2IP=0;			/* Our 2nd DNS IP address	*/
#endif
char		NetOurNISDomain[32]={0,};	/* Our NIS domain		*/
char		NetOurHostName[32]={0,};	/* Our hostname			*/
char		NetOurRootPath[64]={0,};	/* Our bootpath			*/

/** END OF BOOTP EXTENTIONS **/

ulong		NetBootFileXferSize;	/* The actual transferred size of the bootfile (in bytes) */
uchar		NetOurEther[6];		/* Our ethernet address			*/
uchar		NetServerEther[6] =	/* Boot server enet address		*/
			{ 0, 0, 0, 0, 0, 0 };
IPaddr_t	NetOurIP;		/* Our IP addr (0 = unknown)		*/
IPaddr_t	NetServerIP;		/* Our IP addr (0 = unknown)		*/
uchar *NetRxPkt;			/* Current receive packet		*/
int		NetRxPktLen;		/* Current rx packet length		*/
unsigned	NetIPID;		/* IP packet ID				*/
uchar		NetBcastAddr[6] =	/* Ethernet bcast address		*/
			{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uchar		NetEtherNullAddr[6] =
			{ 0, 0, 0, 0, 0, 0 };
int		NetState;		/* Network loop state			*/

/* XXX in both little & big endian machines 0xFFFF == ntohs(-1) */
ushort		NetOurVLAN = 0xFFFF;		/* default is without VLAN	*/
ushort		NetOurNativeVLAN = 0xFFFF;	/* ditto			*/

char		BootFile[128];		/* Boot File name			*/

uchar	PktBuf[(PKTBUFSRX+1) * PKTSIZE_ALIGN + PKTALIGN];

uchar *NetRxPackets[PKTBUFSRX]; /* Receive packets			*/

static rxhand_f *packetHandler;		/* Current RX packet handler		*/
static thand_f *timeHandler;		/* Current timeout handler		*/
static uint64_t	timeStart;		/* Time base value			*/
static uint64_t	timeDelta;		/* Current timeout value		*/
uchar *NetTxPacket = 0;			/* THE transmit packet			*/

static int net_check_prereq (proto_t protocol);

/**********************************************************************/

IPaddr_t	NetArpWaitPacketIP;
IPaddr_t	NetArpWaitReplyIP;
uchar	       *NetArpWaitPacketMAC;	/* MAC address of waiting packet's destination	*/
uchar	       *NetArpWaitTxPacket;	/* THE transmit packet			*/
int		NetArpWaitTxPacketSize;
uchar 		NetArpWaitPacketBuf[PKTSIZE_ALIGN + PKTALIGN];
uint64_t	NetArpWaitTimerStart;

void ArpRequest (void)
{
	int i;
	uchar *pkt;
	ARP_t *arp;

#ifdef ET_DEBUG
	printf ("ARP broadcast\n");
#endif
	pkt = NetTxPacket;

	pkt += NetSetEther (pkt, NetBcastAddr, PROT_ARP);

	arp = (ARP_t *) pkt;

	arp->ar_hrd = htons (ARP_ETHER);
	arp->ar_pro = htons (PROT_IP);
	arp->ar_hln = 6;
	arp->ar_pln = 4;
	arp->ar_op = htons (ARPOP_REQUEST);

	memcpy (&arp->ar_data[0], NetOurEther, 6);		/* source ET addr	*/
	NetWriteIP ((uchar *) & arp->ar_data[6], NetOurIP);	/* source IP addr	*/
	for (i = 10; i < 16; ++i) {
		arp->ar_data[i] = 0;				/* dest ET addr = 0     */
	}

	if ((NetArpWaitPacketIP & NetOurSubnetMask) !=
	    (NetOurIP & NetOurSubnetMask)) {
		if (NetOurGatewayIP == 0) {
			puts ("## Warning: gatewayip needed but not set\n");
			NetArpWaitReplyIP = NetArpWaitPacketIP;
		} else {
			NetArpWaitReplyIP = NetOurGatewayIP;
		}
	} else {
		NetArpWaitReplyIP = NetArpWaitPacketIP;
	}

	NetWriteIP ((uchar *) & arp->ar_data[16], NetArpWaitReplyIP);
	(void) eth_send (NetTxPacket, (pkt - NetTxPacket) + ARP_HDR_SIZE);
}

static void ArpTimeoutCheck(void)
{
	if (!NetArpWaitPacketIP)
		return;

	/* check for arp timeout */
	if (is_timeout(NetArpWaitTimerStart, ARP_TIMEOUT)) {
		NetArpWaitTimerStart = get_time_ns();
		ArpRequest();
	}
}

/**********************************************************************/
/*
 *	Main network processing loop.
 */

int NetLoopInit(proto_t protocol)
{
	struct eth_device *eth_current = eth_get_current();
	IPaddr_t ip;
	int ret;
	int	i;

	if (!eth_current) {
		printf("Current ethernet device not set!\n");
		return -1;
	}

	ip = dev_get_param_ip(&eth_current->dev, "ipaddr");
	NetCopyIP(&NetOurIP, &ip);

	/* XXX problem with bss workaround */
	NetArpWaitPacketMAC = NULL;
	NetArpWaitTxPacket = NULL;
	NetArpWaitPacketIP = 0;
	NetArpWaitReplyIP = 0;

	/*
	 *	Setup packet buffers, aligned correctly.
	 */
	NetTxPacket = &PktBuf[0] + (PKTALIGN - 1);
	NetTxPacket -= (ulong)NetTxPacket % PKTALIGN;
	for (i = 0; i < PKTBUFSRX; i++) {
		NetRxPackets[i] = NetTxPacket + (i+1)*PKTSIZE_ALIGN;
	}

	NetArpWaitTxPacket = &NetArpWaitPacketBuf[0] + (PKTALIGN - 1);
	NetArpWaitTxPacket -= (ulong)NetArpWaitTxPacket % PKTALIGN;
	NetArpWaitTxPacketSize = 0;

	if (eth_open() < 0)
		return -1;

	string_to_ethaddr(dev_get_param(&eth_get_current()->dev, "ethaddr"),
			NetOurEther);

	NetState = NETLOOP_CONTINUE;

	NetOurGatewayIP = dev_get_param_ip(&eth_current->dev, "gateway");
	NetOurSubnetMask = dev_get_param_ip(&eth_current->dev, "netmask");
	NetOurVLAN = getenv_VLAN("vlan");
	NetOurNativeVLAN = getenv_VLAN("nvlan");
	NetServerIP = dev_get_param_ip(&eth_current->dev, "serverip");

	ret = net_check_prereq(protocol);
	if (ret)
		eth_halt();

	return ret;
}

int NetLoop(void)
{
	/*
	 *	Start the ball rolling with the given start function.  From
	 *	here on, this code is a state machine driven by received
	 *	packets and timer events.
	 */

	NetBootFileXferSize = 0;

	/*
	 *	Main packet reception loop.  Loop receiving packets until
	 *	someone sets `NetState' to a state that terminates.
	 */
	for (;;) {
		WATCHDOG_RESET();
#ifdef CONFIG_SHOW_ACTIVITY
		{
			extern void show_activity(int arg);
			show_activity(1);
		}
#endif
		/*
		 *	Check the ethernet for a new packet.  The ethernet
		 *	receive routine will process it.
		 */
		eth_rx();

		/*
		 *	Abort if ctrl-c was pressed.
		 */
		if (ctrlc()) {
			eth_halt();
			puts ("\nAbort\n");
			return -1;
		}

		ArpTimeoutCheck();

		/*
		 *	Check for a timeout, and run the timeout handler
		 *	if we have one.
		 */
		if (timeHandler && is_timeout(timeStart, timeDelta)) {
			thand_f *x;
			x = timeHandler;
			timeHandler = (thand_f *)0;
			(*x)();
		}


		switch (NetState) {
		case NETLOOP_SUCCESS:
			if (NetBootFileXferSize > 0) {
				char buf[10];
				printf("Bytes transferred = %ld (%lx hex)\n",
					NetBootFileXferSize,
					NetBootFileXferSize);
				sprintf(buf, "0x%lx", NetBootFileXferSize);
				setenv("filesize", buf);
			}
			eth_halt();
			return NetBootFileXferSize;

		case NETLOOP_FAIL:
			eth_halt();
			return -1;
		}
	}
}

/**********************************************************************/
/*
 *	Miscelaneous bits.
 */

void
NetSetHandler(rxhand_f * f)
{
	packetHandler = f;
}


void
NetSetTimeout(uint64_t iv, thand_f * f)
{
	if (iv == 0) {
		timeHandler = (thand_f *)0;
	} else {
		timeHandler = f;
		timeStart = get_time_ns();
		timeDelta = iv;
	}
}


void
NetSendPacket(uchar * pkt, int len)
{
	(void) eth_send(pkt, len);
}

int
NetSendUDPPacket(uchar *ether, IPaddr_t dest, int dport, int sport, int len)
{
	uchar *pkt;

	/* convert to new style broadcast */
	if (dest == 0)
		dest = 0xFFFFFFFF;

	/* if broadcast, make the ether address a broadcast and don't do ARP */
	if (dest == 0xFFFFFFFF)
		ether = NetBcastAddr;

	/* if MAC address was not discovered yet, save the packet and do an ARP request */
	if (memcmp(ether, NetEtherNullAddr, 6) == 0) {

#ifdef ET_DEBUG
		printf("sending ARP for %08lx\n", dest);
#endif
		NetArpWaitPacketIP = dest;
		NetArpWaitPacketMAC = ether;

		pkt = NetArpWaitTxPacket;
		pkt += NetSetEther (pkt, NetArpWaitPacketMAC, PROT_IP);

		NetSetIP (pkt, dest, dport, sport, len);
		memcpy(pkt + IP_HDR_SIZE, (uchar *)NetTxPacket + (pkt - (uchar *)NetArpWaitTxPacket) + IP_HDR_SIZE, len);

		/* size of the waiting packet */
		NetArpWaitTxPacketSize = (pkt - NetArpWaitTxPacket) + IP_HDR_SIZE + len;

		/* and do the ARP request */
		NetArpWaitTimerStart = get_time_ns();
		ArpRequest();
		return 1;	/* waiting */
	}

#ifdef ET_DEBUG
	printf("sending UDP to %08lx/%02x:%02x:%02x:%02x:%02x:%02x\n",
		dest, ether[0], ether[1], ether[2], ether[3], ether[4], ether[5]);
#endif

	pkt = (uchar *)NetTxPacket;
	pkt += NetSetEther (pkt, ether, PROT_IP);
	NetSetIP (pkt, dest, dport, sport, len);
	(void) eth_send(NetTxPacket, (pkt - NetTxPacket) + IP_HDR_SIZE + len);

	return 0;	/* transmitted */
}

void
NetReceive(uchar * inpkt, int len)
{
	Ethernet_t *et;
	IP_t	*ip;
	ARP_t	*arp;
	IPaddr_t tmp;
	int	x;
	uchar *pkt;
	ushort cti = 0, vlanid = VLAN_NONE, myvlanid, mynvlanid;

#ifdef ET_DEBUG
	printf("packet received\n");
#endif

	NetRxPkt = inpkt;
	NetRxPktLen = len;
	et = (Ethernet_t *)inpkt;

	/* too small packet? */
	if (len < ETHER_HDR_SIZE)
		return;

	myvlanid = ntohs(NetOurVLAN);
	if (myvlanid == (ushort)-1)
		myvlanid = VLAN_NONE;
	mynvlanid = ntohs(NetOurNativeVLAN);
	if (mynvlanid == (ushort)-1)
		mynvlanid = VLAN_NONE;

	x = ntohs(et->et_protlen);

#ifdef ET_DEBUG
	printf("packet received\n");
#endif

	if (x < 1514) {
		/*
		 *	Got a 802 packet.  Check the other protocol field.
		 */
		x = ntohs(et->et_prot);

		ip = (IP_t *)(inpkt + E802_HDR_SIZE);
		len -= E802_HDR_SIZE;

	} else if (x != PROT_VLAN) {	/* normal packet */
		ip = (IP_t *)(inpkt + ETHER_HDR_SIZE);
		len -= ETHER_HDR_SIZE;

	} else {			/* VLAN packet */
		VLAN_Ethernet_t *vet = (VLAN_Ethernet_t *)et;

#ifdef ET_DEBUG
		printf("VLAN packet received\n");
#endif
		/* too small packet? */
		if (len < VLAN_ETHER_HDR_SIZE)
			return;

		/* if no VLAN active */
		if ((ntohs(NetOurVLAN) & VLAN_IDMASK) == VLAN_NONE
				)
			return;

		cti = ntohs(vet->vet_tag);
		vlanid = cti & VLAN_IDMASK;
		x = ntohs(vet->vet_type);

		ip = (IP_t *)(inpkt + VLAN_ETHER_HDR_SIZE);
		len -= VLAN_ETHER_HDR_SIZE;
	}

#ifdef ET_DEBUG
	printf("Receive from protocol 0x%x\n", x);
#endif

	if ((myvlanid & VLAN_IDMASK) != VLAN_NONE) {
		if (vlanid == VLAN_NONE)
			vlanid = (mynvlanid & VLAN_IDMASK);
		/* not matched? */
		if (vlanid != (myvlanid & VLAN_IDMASK))
			return;
	}

	switch (x) {

	case PROT_ARP:
		/*
		 * We have to deal with two types of ARP packets:
		 * - REQUEST packets will be answered by sending  our
		 *   IP address - if we know it.
		 * - REPLY packets are expected only after we asked
		 *   for the TFTP server's or the gateway's ethernet
		 *   address; so if we receive such a packet, we set
		 *   the server ethernet address
		 */
#ifdef ET_DEBUG
		puts ("Got ARP\n");
#endif
		arp = (ARP_t *)ip;
		if (len < ARP_HDR_SIZE) {
			printf("bad length %d < %d\n", len, ARP_HDR_SIZE);
			return;
		}
		if (ntohs(arp->ar_hrd) != ARP_ETHER) {
			return;
		}
		if (ntohs(arp->ar_pro) != PROT_IP) {
			return;
		}
		if (arp->ar_hln != 6) {
			return;
		}
		if (arp->ar_pln != 4) {
			return;
		}

		if (NetOurIP == 0) {
			return;
		}

		if (NetReadIP(&arp->ar_data[16]) != NetOurIP) {
			return;
		}

		switch (ntohs(arp->ar_op)) {
		case ARPOP_REQUEST:		/* reply with our IP address	*/
#ifdef ET_DEBUG
			puts ("Got ARP REQUEST, return our IP\n");
#endif
			pkt = (uchar *)et;
			pkt += NetSetEther(pkt, et->et_src, PROT_ARP);
			arp->ar_op = htons(ARPOP_REPLY);
			memcpy   (&arp->ar_data[10], &arp->ar_data[0], 6);
			NetCopyIP(&arp->ar_data[16], &arp->ar_data[6]);
			memcpy   (&arp->ar_data[ 0], NetOurEther, 6);
			NetCopyIP(&arp->ar_data[ 6], &NetOurIP);
			memcpy(NetTxPacket, et, (pkt - (uchar *)et) + ARP_HDR_SIZE);
			eth_send((uchar *)NetTxPacket, (pkt - (uchar *)et) + ARP_HDR_SIZE);
			return;

		case ARPOP_REPLY:		/* arp reply */
			/* are we waiting for a reply */
			if (!NetArpWaitPacketIP || !NetArpWaitPacketMAC)
				break;
#ifdef ET_DEBUG
			printf("Got ARP REPLY, set server/gtwy eth addr (%02x:%02x:%02x:%02x:%02x:%02x)\n",
				arp->ar_data[0], arp->ar_data[1],
				arp->ar_data[2], arp->ar_data[3],
				arp->ar_data[4], arp->ar_data[5]);
#endif

			tmp = NetReadIP(&arp->ar_data[6]);

			/* matched waiting packet's address */
			if (tmp == NetArpWaitReplyIP) {
#ifdef ET_DEBUG
				puts ("Got it\n");
#endif
				/* save address for later use */
				memcpy(NetArpWaitPacketMAC, &arp->ar_data[0], 6);

#ifdef CONFIG_NETCONSOLE
				(*packetHandler)(0,0,0,0);
#endif
				/* modify header, and transmit it */
				memcpy(((Ethernet_t *)NetArpWaitTxPacket)->et_dest, NetArpWaitPacketMAC, 6);
				(void) eth_send(NetArpWaitTxPacket, NetArpWaitTxPacketSize);

				/* no arp request pending now */
				NetArpWaitPacketIP = 0;
				NetArpWaitTxPacketSize = 0;
				NetArpWaitPacketMAC = NULL;

			}
			return;
		default:
#ifdef ET_DEBUG
			printf("Unexpected ARP opcode 0x%x\n", ntohs(arp->ar_op));
#endif
			return;
		}
		break;

	case PROT_RARP:
#ifdef ET_DEBUG
		puts ("Got RARP\n");
#endif
		arp = (ARP_t *)ip;
		if (len < ARP_HDR_SIZE) {
			printf("bad length %d < %d\n", len, ARP_HDR_SIZE);
			return;
		}

		if ((ntohs(arp->ar_op) != RARPOP_REPLY) ||
			(ntohs(arp->ar_hrd) != ARP_ETHER)   ||
			(ntohs(arp->ar_pro) != PROT_IP)     ||
			(arp->ar_hln != 6) || (arp->ar_pln != 4)) {

			puts ("invalid RARP header\n");
		} else {
			NetCopyIP(&NetOurIP,    &arp->ar_data[16]);
			if (NetServerIP == 0)
				NetCopyIP(&NetServerIP, &arp->ar_data[ 6]);
			memcpy (NetServerEther, &arp->ar_data[ 0], 6);

			(*packetHandler)(0,0,0,0);
		}
		break;

	case PROT_IP:
#ifdef ET_DEBUG
		puts ("Got IP\n");
#endif
		if (len < IP_HDR_SIZE) {
			debug ("len bad %d < %d\n", len, IP_HDR_SIZE);
			return;
		}
		if (len < ntohs(ip->ip_len)) {
			printf("len bad %d < %d\n", len, ntohs(ip->ip_len));
			return;
		}
		len = ntohs(ip->ip_len);
#ifdef ET_DEBUG
		printf("len=%d, v=%02x\n", len, ip->ip_hl_v & 0xff);
#endif
		if ((ip->ip_hl_v & 0xf0) != 0x40) {
			return;
		}
		if (ip->ip_off & htons(0x1fff)) { /* Can't deal w/ fragments */
			return;
		}
		if (!NetCksumOk((uchar *)ip, IP_HDR_SIZE_NO_UDP / 2)) {
			puts ("checksum bad\n");
			return;
		}
		tmp = NetReadIP(&ip->ip_dst);
		if (NetOurIP && tmp != NetOurIP && tmp != 0xFFFFFFFF) {
			return;
		}
		/*
		 * watch for ICMP host redirects
		 *
		 * There is no real handler code (yet). We just watch
		 * for ICMP host redirect messages. In case anybody
		 * sees these messages: please contact me
		 * (wd@denx.de), or - even better - send me the
		 * necessary fixes :-)
		 *
		 * Note: in all cases where I have seen this so far
		 * it was a problem with the router configuration,
		 * for instance when a router was configured in the
		 * BOOTP reply, but the TFTP server was on the same
		 * subnet. So this is probably a warning that your
		 * configuration might be wrong. But I'm not really
		 * sure if there aren't any other situations.
		 */
		if (ip->ip_p == IPPROTO_ICMP) {
			ICMP_t *icmph = (ICMP_t *)&(ip->udp_src);

			switch (icmph->type) {
			case ICMP_REDIRECT:
				if (icmph->code != ICMP_REDIR_HOST)
					return;
				puts (" ICMP Host Redirect to ");
				print_IPaddr(icmph->un.gateway);
				putchar(' ');
				return;
#ifdef CONFIG_NET_PING
			case ICMP_ECHO_REPLY:
				/*
				 *	IP header OK.  Pass the packet to the current handler.
				 */
				/* XXX point to ip packet */
				(*packetHandler)((uchar *)ip, 0, 0, 0);
				return;
#endif
			default:
				return;
			}
		} else if (ip->ip_p != IPPROTO_UDP) {	/* Only UDP packets */
			return;
		}

#ifdef CONFIG_UDP_CHECKSUM
		if (ip->udp_xsum != 0) {
			ulong   xsum;
			ushort *sumptr;
			ushort  sumlen;

			xsum  = ip->ip_p;
			xsum += (ntohs(ip->udp_len));
			xsum += (ntohl(ip->ip_src) >> 16) & 0x0000ffff;
			xsum += (ntohl(ip->ip_src) >>  0) & 0x0000ffff;
			xsum += (ntohl(ip->ip_dst) >> 16) & 0x0000ffff;
			xsum += (ntohl(ip->ip_dst) >>  0) & 0x0000ffff;

			sumlen = ntohs(ip->udp_len);
			sumptr = (ushort *) &(ip->udp_src);

			while (sumlen > 1) {
				ushort sumdata;

				sumdata = *sumptr++;
				xsum += ntohs(sumdata);
				sumlen -= 2;
			}
			if (sumlen > 0) {
				ushort sumdata;

				sumdata = *(unsigned char *) sumptr;
				sumdata = (sumdata << 8) & 0xff00;
				xsum += sumdata;
			}
			while ((xsum >> 16) != 0) {
				xsum = (xsum & 0x0000ffff) + ((xsum >> 16) & 0x0000ffff);
			}
			if ((xsum != 0x00000000) && (xsum != 0x0000ffff)) {
				printf(" UDP wrong checksum %08x %08x\n", xsum, ntohs(ip->udp_xsum));
				return;
			}
		}
#endif

#ifdef CONFIG_NETCONSOLE
		nc_input_packet((uchar *)ip +IP_HDR_SIZE,
						ntohs(ip->udp_dst),
						ntohs(ip->udp_src),
						ntohs(ip->udp_len) - 8);
#endif
		/*
		 *	IP header OK.  Pass the packet to the current handler.
		 */
		(*packetHandler)((uchar *)ip +IP_HDR_SIZE,
						ntohs(ip->udp_dst),
						ntohs(ip->udp_src),
						ntohs(ip->udp_len) - 8);
		break;
	}
}


/**********************************************************************/

static int net_check_prereq (proto_t protocol)
{
	struct eth_device *edev = eth_get_current();

	switch (protocol) {
		/* Fall through */
#ifdef CONFIG_NET_NFS
	case NFS:
#endif
	case NETCONS:
	case TFTP:
		if (NetServerIP == 0) {
			printf("*** ERROR: `%s.serverip' not set\n", dev_id(&edev->dev));
			return -1;
		}

		if (NetOurIP == 0) {
			printf("*** ERROR: `%s.ipaddr' not set\n", dev_id(&edev->dev));
			return -1;
		}
		/* Fall through */

	case DHCP:
	case RARP:
	case BOOTP:
		if (memcmp (NetOurEther, "\0\0\0\0\0\0", 6) == 0) {
			printf("*** ERROR: `%s.ethaddr' not set\n", dev_id(&edev->dev));
			return -1;
		}
		/* Fall through */
	default:
		return 0;
	}

	return -1; /* not reached */
}
/**********************************************************************/

int
NetCksumOk(uchar * ptr, int len)
{
	return !((NetCksum(ptr, len) + 1) & 0xfffe);
}


unsigned
NetCksum(uchar * ptr, int len)
{
	ulong	xsum;
	ushort *p = (ushort *)ptr;

	xsum = 0;
	while (len-- > 0)
		xsum += *p++;
	xsum = (xsum & 0xffff) + (xsum >> 16);
	xsum = (xsum & 0xffff) + (xsum >> 16);
	return xsum & 0xffff;
}

int
NetEthHdrSize(void)
{
	ushort myvlanid;

	myvlanid = ntohs(NetOurVLAN);
	if (myvlanid == (ushort)-1)
		myvlanid = VLAN_NONE;

	return ((myvlanid & VLAN_IDMASK) == VLAN_NONE) ? ETHER_HDR_SIZE : VLAN_ETHER_HDR_SIZE;
}

int
NetSetEther(uchar * xet, uchar * addr, uint prot)
{
	Ethernet_t *et = (Ethernet_t *)xet;
	ushort myvlanid;

	myvlanid = ntohs(NetOurVLAN);
	if (myvlanid == (ushort)-1)
		myvlanid = VLAN_NONE;

	memcpy (et->et_dest, addr, 6);
	memcpy (et->et_src, NetOurEther, 6);
	if ((myvlanid & VLAN_IDMASK) == VLAN_NONE) {
	et->et_protlen = htons(prot);
		return ETHER_HDR_SIZE;
	} else {
		VLAN_Ethernet_t *vet = (VLAN_Ethernet_t *)xet;

		vet->vet_vlan_type = htons(PROT_VLAN);
		vet->vet_tag = htons((0 << 5) | (myvlanid & VLAN_IDMASK));
		vet->vet_type = htons(prot);
		return VLAN_ETHER_HDR_SIZE;
	}
}

void
NetSetIP(uchar * xip, IPaddr_t dest, int dport, int sport, int len)
{
	IP_t *ip = (IP_t *)xip;

	/*
	 *	If the data is an odd number of bytes, zero the
	 *	byte after the last byte so that the checksum
	 *	will work.
	 */
	if (len & 1)
		xip[IP_HDR_SIZE + len] = 0;

	/*
	 *	Construct an IP and UDP header.
	 *	(need to set no fragment bit - XXX)
	 */
	ip->ip_hl_v  = 0x45;		/* IP_HDR_SIZE / 4 (not including UDP) */
	ip->ip_tos   = 0;
	ip->ip_len   = htons(IP_HDR_SIZE + len);
	ip->ip_id    = htons(NetIPID++);
	ip->ip_off   = htons(0x4000);	/* No fragmentation */
	ip->ip_ttl   = 255;
	ip->ip_p     = 17;		/* UDP */
	ip->ip_sum   = 0;
	NetCopyIP((void*)&ip->ip_src, &NetOurIP); /* already in network byte order */
	NetCopyIP((void*)&ip->ip_dst, &dest);	   /* - "" - */
	ip->udp_src  = htons(sport);
	ip->udp_dst  = htons(dport);
	ip->udp_len  = htons(8 + len);
	ip->udp_xsum = 0;
	ip->ip_sum   = ~NetCksum((uchar *)ip, IP_HDR_SIZE_NO_UDP / 2);
}

char *ip_to_string (IPaddr_t x, char *s)
{
	x = ntohl (x);
	sprintf (s, "%d.%d.%d.%d",
		 (int) ((x >> 24) & 0xff),
		 (int) ((x >> 16) & 0xff),
		 (int) ((x >> 8) & 0xff), (int) ((x >> 0) & 0xff)
	);
	return s;
}

int string_to_ip(const char *s, IPaddr_t *ip)
{
	IPaddr_t addr = 0;
	char *e;
	int i;

	if (!s)
		return -EINVAL;

	for (i = 0; i < 4; i++) {
		ulong val;

		if (!isdigit(*s))
			return -EINVAL;

		val = simple_strtoul(s, &e, 10);
		addr <<= 8;
		addr |= (val & 0xFF);

		if (*e != '.' && i != 3)
			return -EINVAL;

		s = e + 1;
	}

	*ip = htonl(addr);
	return 0;
}

void VLAN_to_string(ushort x, char *s)
{
	x = ntohs(x);

	if (x == (ushort)-1)
		x = VLAN_NONE;

	if (x == VLAN_NONE)
		strcpy(s, "none");
	else
		sprintf(s, "%d", x & VLAN_IDMASK);
}

ushort string_to_VLAN(const char *s)
{
	ushort id;

	if (s == NULL)
		return htons(VLAN_NONE);

	if (*s < '0' || *s > '9')
		id = VLAN_NONE;
	else
		id = (ushort)simple_strtoul(s, NULL, 10);

	return htons(id);
}

void print_IPaddr (IPaddr_t x)
{
	char tmp[16];

	ip_to_string (x, tmp);

	puts (tmp);
}

ushort getenv_VLAN(char *var)
{
	return string_to_VLAN(getenv(var));
}

int string_to_ethaddr(const char *str, char *enetaddr)
{
	ulong reg;
	char *e;

        if (!str || strlen(str) != 17)
                return -1;

        if (str[2] != ':' || str[5] != ':' || str[8] != ':' ||
                        str[11] != ':' || str[14] != ':')
                return -1;

	for (reg = 0; reg < 6; ++reg) {
		enetaddr[reg] = simple_strtoul (str, &e, 16);
			str = e + 1;
	}

	return 0;
}

void ethaddr_to_string(const unsigned char *enetaddr, char *str)
{
	sprintf (str, "%02X:%02X:%02X:%02X:%02X:%02X",
		 enetaddr[0], enetaddr[1], enetaddr[2], enetaddr[3],
		 enetaddr[4], enetaddr[5]);
}

