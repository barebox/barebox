#include <common.h>
#include <command.h>
#include <clock.h>
#include <net.h>
#include <errno.h>
#include <linux/err.h>

static uint16_t ping_sequence_number;

static IPaddr_t	net_ping_ip;		/* the ip address to ping 		*/

#define PING_STATE_INIT		0
#define PING_STATE_SUCCESS	1

static int ping_state;

static struct net_connection *ping_con;

static int ping_send(void)
{
	unsigned char *payload;
	struct icmphdr *icmp;
	uint64_t ts;

	icmp = ping_con->icmp;

	icmp->type = ICMP_ECHO_REQUEST;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->un.echo.id = 0;
	icmp->un.echo.sequence = htons(ping_sequence_number);

	ping_sequence_number++;

	payload = (char *)(icmp + 1);
	ts = get_time_ns();
	memcpy(payload, &ts, sizeof(ts));
	payload[8] = 0xab;
	payload[9] = 0xcd;
	return net_icmp_send(ping_con, 9);
}

static void ping_handler(void *ctx, char *pkt, unsigned len)
{
	IPaddr_t tmp;
	struct iphdr *ip = net_eth_to_iphdr(pkt);

	tmp = net_read_ip((void *)&ip->saddr);
	if (tmp != net_ping_ip)
		return;

	ping_state = PING_STATE_SUCCESS;
}

static int do_ping(int argc, char *argv[])
{
	int ret;
	uint64_t ping_start;
	unsigned retries = 0;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	ret = resolv(argv[1], &net_ping_ip);
	if (ret) {
		printf("Cannot resolve \"%s\": %s\n", argv[1], strerror(-ret));
		return 1;
	}

	ping_state = PING_STATE_INIT;
	ping_sequence_number = 0;

	ping_con = net_icmp_new(net_ping_ip, ping_handler, NULL);
	if (IS_ERR(ping_con)) {
		ret = PTR_ERR(ping_con);
		goto out;
	}

	printf("PING %s (%pI4)\n", argv[1], &net_ping_ip);

	ping_start = get_time_ns();
	ret = ping_send();
	if (ret)
		goto out_unreg;

	while (ping_state == PING_STATE_INIT) {
		if (ctrlc()) {
			ret = -EINTR;
			break;
		}

		net_poll();

		if (is_timeout(ping_start, SECOND)) {
			/* No answer, send another packet */
			ping_start = get_time_ns();
			ret = ping_send();
			if (ret)
				goto out_unreg;
			retries++;
		}

		if (retries > PKT_NUM_RETRIES) {
			ret = -ETIMEDOUT;
			goto out_unreg;
		}
	}

	if (!ret)
		printf("host %s is alive\n", argv[1]);

out_unreg:
	net_unregister(ping_con);
out:
	if (ret)
		printf("ping failed: %s\n", strerror(-ret));
	return ping_state == PING_STATE_SUCCESS ? 0 : 1;
}

BAREBOX_CMD_START(ping)
	.cmd		= do_ping,
	BAREBOX_CMD_DESC("send ICMP echo requests")
	BAREBOX_CMD_OPTS("DESTINATION")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
