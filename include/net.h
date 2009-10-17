/*
 *	LiMon Monitor (LiMon) - Network.
 *
 *	Copyright 1994 - 2000 Neil Russell.
 *	(See License)
 *
 *
 * History
 *	9/16/00	  bor  adapted to TQM823L/STK8xxL board, RARP/TFTP boot added
 */

#ifndef __NET_H__
#define __NET_H__

#include <driver.h>
#include <linux/types.h>
#include <param.h>
#include <asm/byteorder.h>	/* for nton* / ntoh* stuff */


/*
 *	The number of receive packet buffers, and the required packet buffer
 *	alignment in memory.
 *
 */

#ifdef CFG_RX_ETH_BUFFER
# define PKTBUFSRX	CFG_RX_ETH_BUFFER
#else
# define PKTBUFSRX	4
#endif

#define PKTALIGN	32

/*
 * The current receive packet handler.  Called with a pointer to the
 * application packet, and a protocol type (PORT_BOOTPC or PORT_TFTP).
 * All other packets are dealt with without calling the handler.
 */
typedef void	rxhand_f(uchar *, unsigned, unsigned, unsigned);

/*
 *	A timeout handler.  Called after time interval has expired.
 */
typedef void	thand_f(void);

#define NAMESIZE 16

enum eth_state_t {
	ETH_STATE_INIT,
	ETH_STATE_PASSIVE,
	ETH_STATE_ACTIVE
};

struct device_d;

struct eth_device {
	int iobase;
	int state;

	int  (*init) (struct eth_device*);

	int  (*open) (struct eth_device*);
	int  (*send) (struct eth_device*, void *packet, int length);
	int  (*recv) (struct eth_device*);
	void (*halt) (struct eth_device*);
	int  (*get_ethaddr) (struct eth_device*, unsigned char *adr);
	int  (*set_ethaddr) (struct eth_device*, unsigned char *adr);

	struct eth_device *next;
	void *priv;

	struct param_d param_ip;
	struct param_d param_netmask;
	struct param_d param_gateway;
	struct param_d param_serverip;
	struct param_d param_ethaddr;

	struct device_d dev;

	struct list_head list;
};

int eth_register(struct eth_device* dev);    /* Register network device		*/
void eth_unregister(struct eth_device* dev); /* Unregister network device	*/

int eth_open(void);			/* open the device		*/
int eth_send(void *packet, int length);	   /* Send a packet		*/
int eth_rx(void);			/* Check for received packets	*/
void eth_halt(void);			/* stop SCC			*/
char *eth_get_name(void);		/* get name of current device	*/


/**********************************************************************/
/*
 *	Protocol headers.
 */

/*
 *	Ethernet header
 */
typedef struct {
	uchar		et_dest[6];	/* Destination node		*/
	uchar		et_src[6];	/* Source node			*/
	ushort		et_protlen;	/* Protocol or length		*/
	uchar		et_dsap;	/* 802 DSAP			*/
	uchar		et_ssap;	/* 802 SSAP			*/
	uchar		et_ctl;		/* 802 control			*/
	uchar		et_snap1;	/* SNAP				*/
	uchar		et_snap2;
	uchar		et_snap3;
	ushort		et_prot;	/* 802 protocol			*/
} Ethernet_t;

#define ETHER_HDR_SIZE	14		/* Ethernet header size		*/
#define E802_HDR_SIZE	22		/* 802 ethernet header size	*/

/*
 *	Ethernet header
 */
typedef struct {
	uchar		vet_dest[6];	/* Destination node		*/
	uchar		vet_src[6];	/* Source node			*/
	ushort		vet_vlan_type;	/* PROT_VLAN			*/
	ushort		vet_tag;	/* TAG of VLAN			*/
	ushort		vet_type;	/* protocol type		*/
} VLAN_Ethernet_t;

#define VLAN_ETHER_HDR_SIZE	18	/* VLAN Ethernet header size	*/

#define PROT_IP		0x0800		/* IP protocol			*/
#define PROT_ARP	0x0806		/* IP ARP protocol		*/
#define PROT_RARP	0x8035		/* IP ARP protocol		*/
#define PROT_VLAN	0x8100		/* IEEE 802.1q protocol		*/

#define IPPROTO_ICMP	 1	/* Internet Control Message Protocol	*/
#define IPPROTO_UDP	17	/* User Datagram Protocol		*/

/*
 *	Internet Protocol (IP) header.
 */
typedef struct {
	uchar		ip_hl_v;	/* header length and version	*/
	uchar		ip_tos;		/* type of service		*/
	ushort		ip_len;		/* total length			*/
	ushort		ip_id;		/* identification		*/
	ushort		ip_off;		/* fragment offset field	*/
	uchar		ip_ttl;		/* time to live			*/
	uchar		ip_p;		/* protocol			*/
	ushort		ip_sum;		/* checksum			*/
	IPaddr_t	ip_src;		/* Source IP address		*/
	IPaddr_t	ip_dst;		/* Destination IP address	*/
	ushort		udp_src;	/* UDP source port		*/
	ushort		udp_dst;	/* UDP destination port		*/
	ushort		udp_len;	/* Length of UDP packet		*/
	ushort		udp_xsum;	/* Checksum			*/
} IP_t;

#define IP_HDR_SIZE_NO_UDP	(sizeof (IP_t) - 8)
#define IP_HDR_SIZE		(sizeof (IP_t))


/*
 *	Address Resolution Protocol (ARP) header.
 */
typedef struct
{
	ushort		ar_hrd;		/* Format of hardware address	*/
#   define ARP_ETHER	    1		/* Ethernet  hardware address	*/
	ushort		ar_pro;		/* Format of protocol address	*/
	uchar		ar_hln;		/* Length of hardware address	*/
	uchar		ar_pln;		/* Length of protocol address	*/
	ushort		ar_op;		/* Operation			*/
#   define ARPOP_REQUEST    1		/* Request  to resolve  address	*/
#   define ARPOP_REPLY	    2		/* Response to previous request	*/

#   define RARPOP_REQUEST   3		/* Request  to resolve  address	*/
#   define RARPOP_REPLY	    4		/* Response to previous request */

	/*
	 * The remaining fields are variable in size, according to
	 * the sizes above, and are defined as appropriate for
	 * specific hardware/protocol combinations.
	 */
	uchar		ar_data[0];
} ARP_t;

#define ARP_HDR_SIZE	(8+20)		/* Size assuming ethernet	*/

/*
 * ICMP stuff (just enough to handle (host) redirect messages)
 */
#define ICMP_ECHO_REPLY		0	/* Echo reply 			*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO_REQUEST	8	/* Echo request			*/

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET		0	/* Redirect Net			*/
#define ICMP_REDIR_HOST		1	/* Redirect Host		*/

typedef struct icmphdr {
	uchar		type;
	uchar		code;
	ushort		checksum;
	union {
		struct {
			ushort	id;
			ushort	sequence;
		} echo;
		ulong	gateway;
		struct {
			ushort	__unused;
			ushort	mtu;
		} frag;
	} un;
} ICMP_t;


/*
 * Maximum packet size; used to allocate packet storage.
 * TFTP packets can be 524 bytes + IP header + ethernet header.
 * Lets be conservative, and go for 38 * 16.  (Must also be
 * a multiple of 32 bytes).
 */
/*
 * AS.HARNOIS : Better to set PKTSIZE to maximum size because
 * traffic type is not always controlled
 * maximum packet size =  1518
 * maximum packet size and multiple of 32 bytes =  1536
 */
#define PKTSIZE			1518
#define PKTSIZE_ALIGN		1536
/*#define PKTSIZE		608*/

/*
 * Maximum receive ring size; that is, the number of packets
 * we can buffer before overflow happens. Basically, this just
 * needs to be enough to prevent a packet being discarded while
 * we are processing the previous one.
 */
#define RINGSZ		4
#define RINGSZ_LOG2	2

/**********************************************************************/
/*
 *	Globals.
 *
 * Note:
 *
 * All variables of type IPaddr_t are stored in NETWORK byte order
 * (big endian).
 */

/* net.c */
/** BOOTP EXTENTIONS **/
extern IPaddr_t		NetOurGatewayIP;	/* Our gateway IP addresse	*/
extern IPaddr_t		NetOurSubnetMask;	/* Our subnet mask (0 = unknown)*/
extern IPaddr_t		NetOurDNSIP;	 /* Our Domain Name Server (0 = unknown)*/
extern IPaddr_t		NetOurDNS2IP;	 /* Our 2nd Domain Name Server (0 = unknown)*/
extern char		NetOurNISDomain[32];	/* Our NIS domain		*/
extern char		NetOurHostName[32];	/* Our hostname			*/
extern char		NetOurRootPath[64];	/* Our root path		*/
/** END OF BOOTP EXTENTIONS **/
extern ulong		NetBootFileXferSize;	/* size of bootfile in bytes	*/
extern uchar		NetOurEther[6];		/* Our ethernet address		*/
extern uchar		NetServerEther[6];	/* Boot server enet address	*/
extern IPaddr_t		NetOurIP;		/* Our    IP addr (0 = unknown)	*/
extern IPaddr_t		NetServerIP;		/* Server IP addr (0 = unknown)	*/
extern uchar * 		NetTxPacket;		/* THE transmit packet		*/
extern uchar *		NetRxPackets[PKTBUFSRX];/* Receive packets		*/
extern uchar *		NetRxPkt;		/* Current receive packet	*/
extern int		NetRxPktLen;		/* Current rx packet length	*/
extern unsigned		NetIPID;		/* IP ID (counting)		*/
extern uchar		NetBcastAddr[6];	/* Ethernet boardcast address	*/
extern uchar		NetEtherNullAddr[6];

#define VLAN_NONE	4095			/* untagged 			*/
#define VLAN_IDMASK	0x0fff			/* mask of valid vlan id 	*/
extern ushort		NetOurVLAN;		/* Our VLAN 			*/
extern ushort		NetOurNativeVLAN;	/* Our Native VLAN 		*/

extern uchar		NetCDPAddr[6]; 		/* Ethernet CDP address		*/
extern ushort		CDPNativeVLAN;		/* CDP returned native VLAN	*/
extern ushort		CDPApplianceVLAN;	/* CDP returned appliance VLAN	*/

extern int		NetState;		/* Network loop state		*/

/* ---------- Added by sha ------------ */
extern IPaddr_t  NetArpWaitPacketIP;
extern uchar            *NetArpWaitPacketMAC;
extern uchar          *NetArpWaitTxPacket;     /* THE transmit packet                  */
extern int             NetArpWaitTxPacketSize;
extern int             NetArpWaitTry;
extern uint64_t        NetArpWaitTimerStart;
extern void ArpRequest (void);
/* ------------------------------------ */

#define NETLOOP_CONTINUE	1
#define NETLOOP_SUCCESS		2
#define NETLOOP_FAIL		3

typedef enum { BOOTP, RARP, ARP, TFTP, DHCP, PING, DNS, NFS, CDP, NETCONS, SNTP } proto_t;

/* when CDP completes these hold the return values */
extern ushort CDPNativeVLAN;
extern ushort CDPApplianceVLAN;

/* Initialize the network adapter */
int	NetLoopInit(proto_t);

/* Do the work */
int	NetLoop(void);

/* Shutdown adapters and cleanup */
void	NetStop(void);

/* Load failed.	 Start again. */
void	NetStartAgain(void);

/* Get size of the ethernet header when we send */
int 	NetEthHdrSize(void);

/* Set ethernet header; returns the size of the header */
int	NetSetEther(uchar *, uchar *, uint);

/* Set IP header */
void	NetSetIP(uchar *, IPaddr_t, int, int, int);

/* Checksum */
int	NetCksumOk(uchar *, int);	/* Return true if cksum OK	*/
uint	NetCksum(uchar *, int);		/* Calculate the checksum	*/

/* Set callbacks */
void	NetSetHandler(rxhand_f *);	/* Set RX packet handler	*/
void	NetSetTimeout(uint64_t, thand_f *);/* Set timeout handler		*/

/* Transmit "NetTxPacket" */
void	NetSendPacket(uchar *, int);

/* Transmit UDP packet, performing ARP request if needed */
int	NetSendUDPPacket(uchar *ether, IPaddr_t dest, int dport, int sport, int len);

/* Processes a received packet */
void	NetReceive(uchar *, int);

/* Print an IP address on the console */
#ifdef CONFIG_NET
void	print_IPaddr (IPaddr_t);
#else
#define print_IPaddr(IPaddr_t);
#endif

void netboot_update_env(void);

/*
 * The following functions are a bit ugly, but necessary to deal with
 * alignment restrictions on ARM.
 *
 * We're using inline functions, which had the smallest memory
 * footprint in our tests.
 */
/* return IP *in network byteorder* */
static inline IPaddr_t NetReadIP(void *from)
{
	IPaddr_t ip;
	memcpy((void*)&ip, from, sizeof(ip));
	return ip;
}

/* return ulong *in network byteorder* */
static inline ulong NetReadLong(ulong *from)
{
	ulong l;
	memcpy((void*)&l, (void*)from, sizeof(l));
	return l;
}

/* write IP *in network byteorder* */
static inline void NetWriteIP(void *to, IPaddr_t ip)
{
	memcpy(to, (void*)&ip, sizeof(ip));
}

/* copy IP */
static inline void NetCopyIP(void *to, void *from)
{
	memcpy(to, from, sizeof(IPaddr_t));
}

/* copy ulong */
static inline void NetCopyLong(ulong *to, ulong *from)
{
	memcpy((void*)to, (void*)from, sizeof(ulong));
}

/* Convert an IP address to a string */
char *	ip_to_string (IPaddr_t x, char *s);

/* Convert a string to ip address */
int string_to_ip(const char *s, IPaddr_t *ip);

/* Convert a VLAN id to a string */
void	VLAN_to_string (ushort x, char *s);

/* Convert a string to a vlan id */
ushort string_to_VLAN(const char *s);

/* read an IP address from a environment variable */
IPaddr_t getenv_IPaddr (char *);

/* read a VLAN id from an environment variable */
ushort getenv_VLAN(char *);

int string_to_ethaddr(const char *str, char *enetaddr);
void ethaddr_to_string(const unsigned char *enetaddr, char *str);

/**********************************************************************/
/* Network devices                                                    */
/**********************************************************************/

void eth_set_current(struct eth_device *eth);
struct eth_device *eth_get_current(void);
struct eth_device *eth_get_byname(char *name);

#endif /* __NET_H__ */
