/*
 * SNTP support driver
 *
 * Masami Komiya <mkomiya@sonare.it> 2005
 *
 */

#include <common.h>
#include <command.h>
#include <clock.h>
#include <net.h>
#include <rtc.h>

#include "sntp.h"

#define SNTP_TIMEOUT 10

static int SntpOurPort;

static void
SntpSend (void)
{
	struct sntp_pkt_t pkt;
	int pktlen = SNTP_PACKET_LEN;
	int sport;

	debug ("%s\n", __FUNCTION__);

	memset (&pkt, 0, sizeof(pkt));

	pkt.li = NTP_LI_NOLEAP;
	pkt.vn = NTP_VERSION;
	pkt.mode = NTP_MODE_CLIENT;

	memcpy ((char *)NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE, (char *)&pkt, pktlen);

	SntpOurPort = 10000 + ((uint32_t)get_time_ns() % 4096);
	sport = NTP_SERVICE_PORT;

	NetSendUDPPacket (NetServerEther, NetNtpServerIP, sport, SntpOurPort, pktlen);
}

static void
SntpTimeout (void)
{
	puts ("Timeout\n");
	NetState = NETLOOP_FAIL;
	return;
}

static void
SntpHandler (uchar *pkt, unsigned dest, unsigned src, unsigned len)
{
	struct sntp_pkt_t *rpktp = (struct sntp_pkt_t *)pkt;
	struct rtc_time tm;
	ulong seconds;

	debug ("%s\n", __FUNCTION__);

	if (dest != SntpOurPort) return;

	/*
	 * As the RTC's used in U-Boot sepport second resolution only
	 * we simply ignore the sub-second field.
	 */
	memcpy (&seconds, &rpktp->transmit_timestamp, sizeof(ulong));

	to_tm(ntohl(seconds) - 2208988800UL + NetTimeOffset, &tm);
#if (CONFIG_COMMANDS & CFG_CMD_DATE)
	rtc_set (&tm);
#endif
	printf ("Date: %4d-%02d-%02d Time: %2d:%02d:%02d\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	NetState = NETLOOP_SUCCESS;
}

void
SntpStart (void)
{
	debug ("%s\n", __FUNCTION__);

	NetSetTimeout (SNTP_TIMEOUT * SECOND, SntpTimeout);
	NetSetHandler(SntpHandler);
	memset (NetServerEther, 0, 6);

	SntpSend ();
}

int do_sntp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *toff;

	if (argc < 2) {
		NetNtpServerIP = getenv_IPaddr ("ntpserverip");
		if (NetNtpServerIP == 0) {
			printf ("ntpserverip not set\n");
			return (1);
		}
	} else {
		NetNtpServerIP = string_to_ip(argv[1]);
		if (NetNtpServerIP == 0) {
			printf ("Bad NTP server IP address\n");
			return (1);
		}
	}

	toff = getenv ("timeoffset");
	if (toff == NULL) NetTimeOffset = 0;
	else NetTimeOffset = simple_strtol (toff, NULL, 10);

	if (NetLoopInit(SNTP) < 0)
		return 1;

	SntpStart();

	if (NetLoop() < 0) {
		printf("SNTP failed: host %s not responding\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	sntp,	2,	1,	do_sntp,
	"sntp\t- synchronize RTC via network\n",
	"[NTP server IP]\n"
);

