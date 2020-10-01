// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Copyright 2020 Edmund Henniges <eh@emlix.com>
 * Copyright 2020 Daniel Glöckner <dg@emlix.com>
 * Ported from U-Boot to Barebox
 */

#define pr_fmt(fmt) "net fastboot: " fmt

#include <common.h>
#include <net.h>
#include <fastboot.h>
#include <fastboot_net.h>
#include <environment.h>
#include <progress.h>
#include <unistd.h>
#include <init.h>
#include <work.h>
#include <globalvar.h>
#include <magicvar.h>

#define FASTBOOT_PORT 5554
#define MAX_MTU 1500
#define PACKET_SIZE (min(PKTSIZE, MAX_MTU + ETHER_HDR_SIZE) \
		      - (net_eth_to_udp_payload(0) - (char *)0))

enum {
	FASTBOOT_ERROR = 0,
	FASTBOOT_QUERY = 1,
	FASTBOOT_INIT = 2,
	FASTBOOT_FASTBOOT = 3,
};

enum may_send {
	MAY_NOT_SEND,
	MAY_SEND_MESSAGE,
	MAY_SEND_ACK,
};

struct __packed fastboot_header {
	u8 id;
	u8 flags;
	u16 seq;
};

#define FASTBOOT_MAX_MSG_LEN 64

struct fastboot_net {
	struct fastboot fastboot;

	struct net_connection *net_con;
	struct fastboot_header response_header;
	struct poller_struct poller;
	struct work_queue wq;
	u64 host_waits_since;
	u64 last_download_pkt;
	bool sequence_number_seen;
	bool active_download;
	bool reinit;
	bool send_keep_alive;
	enum may_send may_send;

	IPaddr_t host_addr;
	u16 host_port;
	u8 host_mac[ETH_ALEN];
	u16 sequence_number;
	u16 last_payload_len;
	void *last_payload;
};

static const ushort udp_version = 1;

static bool is_current_connection(struct fastboot_net *fbn)
{
	return fbn->host_addr == net_read_ip(&fbn->net_con->ip->daddr) &&
	       fbn->host_port == fbn->net_con->udp->uh_dport;
}

static void fastboot_net_abort(struct fastboot_net *fbn)
{
	fbn->reinit = true;

	fastboot_abort(&fbn->fastboot);

	fbn->active_download = false;

	poller_unregister(&fbn->poller);

	/*
	 * If the host sends a data packet at a time when an empty packet was
	 * expected, fastboot_abort is called and an error message is sent.
	 * We don't want to execute the contents of the bad packet afterwards.
	 * Clearing command also tells our keep-alive poller to stop sending
	 * messages.
	 */
	wq_cancel_work(&fbn->wq);

	free(fbn->last_payload);
	fbn->last_payload = NULL;
}

static void fastboot_net_save_payload(struct fastboot_net *fbn, void *packet,
				      int len)
{
	/* Save packet for retransmitting */

	fbn->last_payload_len = len;
	free(fbn->last_payload);
	fbn->last_payload = memdup(packet, len);
}

static void fastboot_send(struct fastboot_net *fbn,
			  struct fastboot_header header,
			  const char *error_msg)
{
	short tmp;
	uchar *packet = net_udp_get_payload(fbn->net_con);
	uchar *packet_base = packet;
	bool current_session = false;

	if (fbn->sequence_number == ntohs(header.seq) &&
	    is_current_connection(fbn))
		current_session = true;

	if (error_msg)
		header.id = FASTBOOT_ERROR;

	/* send header */
	memcpy(packet, &header, sizeof(header));
	packet += sizeof(header);

	switch (header.id) {
	case FASTBOOT_QUERY:
		/* send sequence number */
		tmp = htons(fbn->sequence_number);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		break;
	case FASTBOOT_INIT:
		/* send udp version and packet size */
		tmp = htons(udp_version);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		tmp = htons(PACKET_SIZE);
		memcpy(packet, &tmp, sizeof(tmp));
		packet += sizeof(tmp);
		break;
	case FASTBOOT_ERROR:
		pr_err("%s\n", error_msg);

		/* send error message */
		tmp = strlen(error_msg);
		memcpy(packet, error_msg, tmp);
		packet += tmp;

		if (current_session)
			fastboot_net_abort(fbn);

		break;
	}

	if (current_session && header.id != FASTBOOT_QUERY) {
		fbn->sequence_number++;
		fbn->sequence_number_seen = false;
		fastboot_net_save_payload(fbn, packet_base, packet - packet_base);
	}
	net_udp_send(fbn->net_con, packet - packet_base);
}

static int fastboot_net_wait_may_send(struct fastboot_net *fbn)
{
	uint64_t start = get_time_ns();

	while (!is_timeout(start, 2 * SECOND)) {
		if (fbn->may_send != MAY_NOT_SEND)
			return 0;
	}

	return -ETIMEDOUT;
}

static int fastboot_write_net(struct fastboot *fb, const char *buf,
			      unsigned int n)
{
	struct fastboot_net *fbn = container_of(fb, struct fastboot_net,
						fastboot);
	struct fastboot_header response_header;
	uchar *packet;
	uchar *packet_base;
	int ret;

	if (fbn->reinit)
		return 0;

	/*
	 * This function is either called in command context, in which
	 * case we may wait, or from the keepalive poller which explicitly
	 * only calls us when we don't have to wait here.
	 */
	ret = fastboot_net_wait_may_send(fbn);
	if (ret) {
		fastboot_net_abort(fbn);
		return ret;
	}

	if (n && fbn->may_send == MAY_SEND_ACK) {
		fastboot_send(fbn, fbn->response_header,
				"Have message but only ACK allowed");
		return -EPROTO;
	} else if (!n && fbn->may_send == MAY_SEND_MESSAGE) {
		fastboot_send(fbn, fbn->response_header,
				"Want to send ACK but message expected");
		return -EPROTO;
	}

	response_header = fbn->response_header;
	response_header.flags = 0;
	response_header.seq = htons(fbn->sequence_number);
	++fbn->sequence_number;
	fbn->sequence_number_seen = false;

	packet = net_udp_get_payload(fbn->net_con);
	packet_base = packet;

	/* Write headers */
	memcpy(packet, &response_header, sizeof(response_header));
	packet += sizeof(response_header);
	/* Write response */
	memcpy(packet, buf, n);
	packet += n;

	fastboot_net_save_payload(fbn, packet_base, packet - packet_base);

	memcpy(fbn->net_con->et->et_dest, fbn->host_mac, ETH_ALEN);
	net_write_ip(&fbn->net_con->ip->daddr, fbn->host_addr);
	fbn->net_con->udp->uh_dport = fbn->host_port;

	fbn->may_send = MAY_NOT_SEND;

	net_udp_send(fbn->net_con, fbn->last_payload_len);

	return 0;
}

static void fastboot_start_download_net(struct fastboot *fb)
{
	struct fastboot_net *fbn = container_of(fb, struct fastboot_net,
						fastboot);

	fastboot_start_download_generic(fb);
	fbn->active_download = true;
	fbn->last_download_pkt = get_time_ns();
}

/* must send exactly one packet on all code paths */
static void fastboot_data_download(struct fastboot_net *fbn,
				   const void *fastboot_data,
				   unsigned int fastboot_data_len)
{
	int ret;

	if (fastboot_data_len == 0 ||
	    (fbn->fastboot.download_bytes + fastboot_data_len) >
	    fbn->fastboot.download_size) {
		fastboot_send(fbn, fbn->response_header,
			      "Received invalid data length");
		return;
	}

	ret = fastboot_handle_download_data(&fbn->fastboot, fastboot_data,
					    fastboot_data_len);
	if (ret < 0) {
		fastboot_send(fbn, fbn->response_header, strerror(-ret));
		return;
	}

	fastboot_tx_print(&fbn->fastboot, FASTBOOT_MSG_NONE, "");
}

struct fastboot_work {
	struct work_struct work;
	struct fastboot_net *fbn;
	bool download_finished;
	char command[FASTBOOT_MAX_CMD_LEN + 1];
};

static void fastboot_handle_type_fastboot(struct fastboot_net *fbn,
					  struct fastboot_header header,
					  char *fastboot_data,
					  unsigned int fastboot_data_len)
{
	struct fastboot_work *w;

	fbn->response_header = header;
	fbn->host_waits_since = get_time_ns();
	fbn->may_send = fastboot_data_len ? MAY_SEND_ACK : MAY_SEND_MESSAGE;

	if (fbn->active_download) {
		fbn->last_download_pkt = get_time_ns();

		if (!fastboot_data_len && fbn->fastboot.download_bytes
					   == fbn->fastboot.download_size) {

			fbn->active_download = false;

			w = xzalloc(sizeof(*w));
			w->fbn = fbn;
			w->download_finished = true;

			wq_queue_work(&fbn->wq, &w->work);
		} else {
			fastboot_data_download(fbn, fastboot_data,
					       fastboot_data_len);
		}
		return;
	}

	if (fastboot_data_len > FASTBOOT_MAX_CMD_LEN) {
		fastboot_send(fbn, header, "command too long");
		return;
	}

	if (!list_empty(&fbn->wq.work))
		return;

	if (fastboot_data_len) {
		w = xzalloc(sizeof(*w));
		w->fbn = fbn;
		memcpy(w->command, fastboot_data, fastboot_data_len);
		w->command[fastboot_data_len] = 0;

		wq_queue_work(&fbn->wq, &w->work);
	}
}

static void fastboot_check_retransmit(struct fastboot_net *fbn,
				      struct fastboot_header header)
{
	if (ntohs(header.seq) == fbn->sequence_number - 1 &&
	    is_current_connection(fbn)) {
		/* Retransmit last sent packet */
		memcpy(net_udp_get_payload(fbn->net_con),
		       fbn->last_payload, fbn->last_payload_len);
		net_udp_send(fbn->net_con, fbn->last_payload_len);
	}
}

static void fastboot_handler(void *ctx, char *packet, unsigned int raw_len)
{
	unsigned int len = net_eth_to_udplen(packet);
	struct ethernet *eth_header = (struct ethernet *)packet;
	struct iphdr *ip_header = net_eth_to_iphdr(packet);
	struct udphdr *udp_header = net_eth_to_udphdr(packet);
	char *payload = net_eth_to_udp_payload(packet);
	struct fastboot_net *fbn = ctx;
	struct fastboot_header header;
	char *fastboot_data = payload + sizeof(header);
	u16 tot_len = ntohs(ip_header->tot_len);
	int ret;

	/* catch bogus tot_len values */
	if ((char *)ip_header - packet + tot_len > raw_len)
		return;

	/* catch packets split into fragments that are too small to reply */
	if (fastboot_data - (char *)ip_header > tot_len)
		return;

	/* catch packets too small to be valid */
	if (len < sizeof(struct fastboot_header))
		return;

	memcpy(&header, payload, sizeof(header));
	header.flags = 0;
	len -= sizeof(header);

	/* catch remaining fragmented packets */
	if (fastboot_data - (char *)ip_header + len > tot_len) {
		fastboot_send(fbn, header,
			      "can't reassemble fragmented frames");
		return;
	}
	/* catch too large packets */
	if (len > PACKET_SIZE) {
		fastboot_send(fbn, header, "packet too large");
		return;
	}

	memcpy(fbn->net_con->et->et_dest, eth_header->et_src, ETH_ALEN);
	net_copy_ip(&fbn->net_con->ip->daddr, &ip_header->saddr);
	fbn->net_con->udp->uh_dport = udp_header->uh_sport;

	switch (header.id) {
	case FASTBOOT_QUERY:
		fastboot_send(fbn, header, NULL);
		break;
	case FASTBOOT_INIT:
		if (ntohs(header.seq) != fbn->sequence_number) {
			fastboot_check_retransmit(fbn, header);
			break;
		}
		fbn->host_addr = net_read_ip(&ip_header->saddr);
		fbn->host_port = udp_header->uh_sport;
		memcpy(fbn->host_mac, eth_header->et_src, ETH_ALEN);
		fastboot_net_abort(fbn);
		/* poller just unregistered in fastboot_net_abort() */
		ret = poller_register(&fbn->poller, "fastboot");
		if (ret) {
			pr_err("Cannot register poller: %s\n", strerror(-ret));
			return;
		}
		fastboot_send(fbn, header, NULL);
		break;
	case FASTBOOT_FASTBOOT:
		if (!is_current_connection(fbn))
			break;
		memcpy(fbn->host_mac, eth_header->et_src, ETH_ALEN);

		if (ntohs(header.seq) != fbn->sequence_number) {
			fastboot_check_retransmit(fbn, header);
		} else if (!fbn->sequence_number_seen) {
			fbn->sequence_number_seen = true;
			fastboot_handle_type_fastboot(fbn, header,
						      fastboot_data, len);
		}
		break;
	default:
		fastboot_send(fbn, header, "unknown packet type");
		break;
	}
}

static void fastboot_do_work(struct work_struct *w)
{
	struct fastboot_work *fw = container_of(w, struct fastboot_work, work);
	struct fastboot_net *fbn = fw->fbn;

	if (fw->download_finished) {
		fastboot_download_finished(&fbn->fastboot);
		goto out;
	}

	fbn->reinit = false;
	fastboot_tx_print(&fbn->fastboot, FASTBOOT_MSG_NONE, "");

	fbn->send_keep_alive = true;

	fastboot_exec_cmd(&fbn->fastboot, fw->command);
	fbn->send_keep_alive = false;
out:
	free(fw);
}

static void fastboot_work_cancel(struct work_struct *w)
{
	struct fastboot_work *fw = container_of(w, struct fastboot_work, work);

	free(fw);
}

static void fastboot_poll(struct poller_struct *poller)
{
	struct fastboot_net *fbn = container_of(poller, struct fastboot_net,
					       poller);

	if (fbn->active_download) {
		net_poll();
		if (is_timeout(fbn->last_download_pkt, 5 * SECOND)) {
			pr_err("No progress for 5s, aborting\n");
			fastboot_net_abort(fbn);
			return;
		}
	}

	if (!fbn->send_keep_alive)
		return;

	if (!is_timeout(fbn->host_waits_since, 30ULL * SECOND))
		return;

	if (fbn->may_send != MAY_SEND_MESSAGE)
		return;

	fastboot_tx_print(&fbn->fastboot, FASTBOOT_MSG_INFO, "still busy");
}

void fastboot_net_free(struct fastboot_net *fbn)
{
	fastboot_generic_close(&fbn->fastboot);
	net_unregister(fbn->net_con);
	fastboot_generic_free(&fbn->fastboot);
	wq_unregister(&fbn->wq);
	free(fbn->last_payload);
	free(fbn);
}

struct fastboot_net *fastboot_net_init(struct fastboot_opts *opts)
{
	struct fastboot_net *fbn;
	const char *partitions = get_fastboot_partitions();
	bool bbu = get_fastboot_bbu();
	int ret;

	fbn = xzalloc(sizeof(*fbn));
	fbn->fastboot.write = fastboot_write_net;
	fbn->fastboot.start_download = fastboot_start_download_net;

	if (opts) {
		fbn->fastboot.files = opts->files;
		fbn->fastboot.cmd_exec = opts->cmd_exec;
		fbn->fastboot.cmd_flash = opts->cmd_flash;
		ret = fastboot_generic_init(&fbn->fastboot, opts->export_bbu);
	} else {
		fbn->fastboot.files = file_list_parse(partitions ?
						      partitions : "");
		ret = fastboot_generic_init(&fbn->fastboot, bbu);
	}
	if (ret)
		goto fail_generic_init;

	fbn->net_con = net_udp_new(IP_BROADCAST, FASTBOOT_PORT,
				   fastboot_handler, fbn);
	if (IS_ERR(fbn->net_con)) {
		ret = PTR_ERR(fbn->net_con);
		goto fail_net_con;
	}
	net_udp_bind(fbn->net_con, FASTBOOT_PORT);

	eth_open(fbn->net_con->edev);

	fbn->poller.func = fastboot_poll;

	fbn->wq.fn = fastboot_do_work;
	fbn->wq.cancel = fastboot_work_cancel;

	wq_register(&fbn->wq);

	return fbn;

fail_net_con:
	fastboot_generic_free(&fbn->fastboot);
fail_generic_init:
	free(fbn);
	return ERR_PTR(ret);
}

static struct fastboot_net *fastboot_net_obj;
static int fastboot_net_autostart;

static int fastboot_on_boot(void)
{
	struct fastboot_net *fbn;

	globalvar_add_simple_bool("fastboot.net.autostart",
				  &fastboot_net_autostart);

	if (!fastboot_net_autostart)
		return 0;

	ifup_all(0);
	fbn = fastboot_net_init(NULL);

	if (IS_ERR(fbn))
		return PTR_ERR(fbn);

	fastboot_net_obj = fbn;
	return 0;
}

static void fastboot_net_exit(void)
{
	if (fastboot_net_obj)
		fastboot_net_free(fastboot_net_obj);
}

postenvironment_initcall(fastboot_on_boot);
predevshutdown_exitcall(fastboot_net_exit);

BAREBOX_MAGICVAR(global.fastboot.net.autostart,
		 "If true, automatically start fastboot over UDP during startup");
