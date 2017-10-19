/*
 * barebox RATP implementation.
 * This is the barebox implementation for the Reliable Asynchronous
 * Transfer Protocol (RATP) as described in RFC916.
 *
 * Copyright (C) 2015 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)  "ratp: " fmt

#include <common.h>
#include <malloc.h>
#include <getopt.h>
#include <ratp.h>
#include <crc.h>
#include <clock.h>
#include <asm/unaligned.h>

/*
 * RATP packet format:
 *
 * Byte No.
 *
 *        +-------------------------------+
 *        |                               |
 *    1   |          Synch Leader         | Hex 01
 *        |                               |
 *        +-------------------------------+
 *        | S | A | F | R | S | A | E | S |
 *    2   | Y | C | I | S | N | N | O | O | Control
 *        | N | K | N | T |   |   | R |   |
 *        +-------------------------------+
 *        |                               |
 *    3   |      Data length (0-255)      |
 *        |                               |
 *        +-------------------------------+
 *        |                               |
 *    4   |        Header Checksum        |
 *        |                               |
 *        +-------------------------------+
 *
 */

struct ratp_header {
	uint8_t	synch;
	uint8_t	control;
	uint8_t	data_length;
	uint8_t	cksum;
};

#define RATP_CONTROL_SO		(1 << 0)
#define RATP_CONTROL_EOR	(1 << 1)
#define RATP_CONTROL_AN		(1 << 2)
#define RATP_CONTROL_SN		(1 << 3)
#define RATP_CONTROL_RST	(1 << 4)
#define RATP_CONTROL_FIN	(1 << 5)
#define RATP_CONTROL_ACK	(1 << 6)
#define RATP_CONTROL_SYN	(1 << 7)

enum ratp_state {
	RATP_STATE_LISTEN,
	RATP_STATE_SYN_SENT,
	RATP_STATE_SYN_RECEIVED,
	RATP_STATE_ESTABLISHED,
	RATP_STATE_FIN_WAIT,
	RATP_STATE_LAST_ACK,
	RATP_STATE_CLOSING,
	RATP_STATE_TIME_WAIT,
	RATP_STATE_CLOSED,
};

struct ratp_message {
	void *buf;
	size_t len;
	struct list_head list;
	void (*complete)(void *ctx, int status);
	void *complete_ctx;
	int eor;
};

static char *ratp_state_str[] = {
	[RATP_STATE_LISTEN]       = "LISTEN",
	[RATP_STATE_SYN_SENT]     = "SYN_SENT",
	[RATP_STATE_SYN_RECEIVED] = "SYN_RECEIVED",
	[RATP_STATE_ESTABLISHED]  = "ESTABLISHED",
	[RATP_STATE_FIN_WAIT]     = "FIN_WAIT",
	[RATP_STATE_LAST_ACK]     = "LAST_ACK",
	[RATP_STATE_CLOSING]      = "CLOSING",
	[RATP_STATE_TIME_WAIT]    = "TIME_WAIT",
	[RATP_STATE_CLOSED]       = "CLOSED",
};

struct ratp_internal {
	struct ratp *ratp;

	enum ratp_state state;
	int sn_sent;
	int sn_received;
	int active;

	void *recvbuf;
	void *sendbuf;
	int sendbuf_len;

	struct list_head recvmsg;
	struct list_head sendmsg;

	struct ratp_message *sendmsg_current;

	uint64_t timewait_timer_start;
	uint64_t retransmission_timer_start;
	int max_retransmission;
	int retransmission_count;
	int srtt;
	int rto;

	int status;

	int in_ratp;
};

static bool ratp_sn(struct ratp_header *hdr)
{
	return hdr->control & RATP_CONTROL_SN ? 1 : 0;
}

static bool ratp_an(struct ratp_header *hdr)
{
	return hdr->control & RATP_CONTROL_AN ? 1 : 0;
}

#define ratp_set_sn(sn) (sn ? RATP_CONTROL_SN : 0)
#define ratp_set_an(an) (an ? RATP_CONTROL_AN : 0)
#define ratp_set_next_sn(sn) (((sn + 1) % 2) ? RATP_CONTROL_SN : 0)
#define ratp_set_next_an(an) (((an + 1) % 2) ? RATP_CONTROL_AN : 0)

static inline int ratp_header_ok(struct ratp_internal *ri, struct ratp_header *h)
{
	uint8_t cksum;
	int ret;

	cksum = h->control;
	cksum += h->data_length;
	cksum += h->cksum;

	ret = cksum == 0xff ? 1 : 0;

	if (ret)
		pr_vdebug("Header ok\n");
	else
		pr_vdebug("Header cksum failed: %02x\n", cksum);

	return ret;
}

static bool ratp_has_data(struct ratp_header *hdr)
{
	if (hdr->control & RATP_CONTROL_SO)
		return 1;
	if (!(hdr->control & (RATP_CONTROL_SYN | RATP_CONTROL_RST | RATP_CONTROL_FIN)) && hdr->data_length)
		return 1;
	return 0;
}

static void ratp_print_header(struct ratp_internal *ri, struct ratp_header *hdr,
		const char *prefix)
{
	uint8_t control = hdr->control;

	pr_debug("%s>%s %s %s %s %s %s %s %s< len: %-3d\n",
		prefix,
		control & RATP_CONTROL_SO  ? "so" : "--",
		control & RATP_CONTROL_EOR ? "eor" : "---",
		control & RATP_CONTROL_AN ? "an" : "--",
		control & RATP_CONTROL_SN ? "sn" : "--",
		control & RATP_CONTROL_RST ? "rst" : "---",
		control & RATP_CONTROL_FIN ? "fin" : "---",
		control & RATP_CONTROL_ACK ? "ack" : "---",
		control & RATP_CONTROL_SYN ? "syn" : "---",
		hdr->data_length);

#ifdef VERBOSE_DEBUG
	if (hdr->data_length)
		memory_display(hdr + 1, 0, hdr->data_length, 1, 0);
#endif
}

static void ratp_create_packet(struct ratp_internal *ri, struct ratp_header *hdr,
		uint8_t control, uint8_t length)
{
	hdr->synch = 0x1;
	hdr->control = control;
	hdr->data_length = length;
	hdr->cksum = (control + length) ^ 0xff;
}

static void ratp_state_change(struct ratp_internal *ri, enum ratp_state state)
{
	pr_debug("state %-10s -> %-10s\n", ratp_state_str[ri->state],
			ratp_state_str[state]);

	ri->state = state;
}

#define RATP_CONTROL_SO		(1 << 0)
#define RATP_CONTROL_EOR	(1 << 1)
#define RATP_CONTROL_AN		(1 << 2)
#define RATP_CONTROL_SN		(1 << 3)
#define RATP_CONTROL_RST	(1 << 4)
#define RATP_CONTROL_FIN	(1 << 5)
#define RATP_CONTROL_ACK	(1 << 6)
#define RATP_CONTROL_SYN	(1 << 7)

static int ratp_send_pkt(struct ratp_internal *ri, void *pkt, int length)
{
	struct ratp_header *hdr = (void *)pkt;

	ratp_print_header(ri, hdr, "send");

	if (ratp_has_data(hdr) ||
	    (hdr->control & (RATP_CONTROL_SYN | RATP_CONTROL_RST | RATP_CONTROL_FIN))) {
		memcpy(ri->sendbuf, pkt, length);
		ri->sn_sent = ratp_sn(hdr);
		ri->sendbuf_len = length;
		ri->retransmission_timer_start = get_time_ns();
		ri->retransmission_count = 0;
	}

	return ri->ratp->send(ri->ratp, pkt, length);
}

static int ratp_send_hdr(struct ratp_internal *ri, uint8_t control)
{
	struct ratp_header hdr = {};

	ratp_create_packet(ri, &hdr, control, 0);

	return ratp_send_pkt(ri, &hdr, sizeof(hdr));
}

static int ratp_recv_char(struct ratp_internal *ri, uint8_t *data, int poll_timeout_ms)
{
	uint64_t start;
	int ret;

	start = get_time_ns();

	while (1) {
		ret = ri->ratp->recv(ri->ratp, data);
		if (ret < 0 && ret != -EAGAIN)
			return ret;

		if (ret == 0)
			return 0;

		if (is_timeout(start, poll_timeout_ms * MSECOND))
			return -EAGAIN;
	}
}

static int ratp_recv_pkt_header(struct ratp_internal *ri, struct ratp_header *hdr,
		int poll_timeout_ms)
{
	int ret;
	uint8_t buf;

	do {
		ret = ratp_recv_char(ri, &buf, 0);
		if (ret < 0)
			return ret;
		hdr->synch = buf;
	} while (hdr->synch != 1);
	ret = ratp_recv_char(ri, &buf, poll_timeout_ms);
	if (ret < 0)
		return ret;

	hdr->control = buf;
	ret = ratp_recv_char(ri, &buf, poll_timeout_ms);
	if (ret < 0)
		return ret;

	hdr->data_length = buf;

	ret = ratp_recv_char(ri, &buf, poll_timeout_ms);
	if (ret < 0)
		return ret;

	hdr->cksum = buf;

	if (!ratp_header_ok(ri, hdr))
		return -EAGAIN;

	return 0;
}

static int ratp_recv_pkt_data(struct ratp_internal *ri, void *data, uint8_t len,
		int poll_timeout_ms)
{
	uint16_t crc_expect, crc_read;
	int ret, i;

	for (i = 0; i < len + 2; i++) {
		ret = ratp_recv_char(ri, data + i, poll_timeout_ms);
		if (ret < 0)
			return ret;
	}

	crc_expect = cyg_crc16(data, len);

	crc_read = get_unaligned_be16(data + len);

	if (crc_expect != crc_read) {
		pr_vdebug("Wrong CRC: expected: 0x%04x, got 0x%04x\n",
				crc_expect, crc_read);
		return -EBADMSG;
	} else {
		pr_vdebug("correct CRC: 0x%04x\n", crc_expect);
	}

	return 0;
}

static int ratp_recv_pkt(struct ratp_internal *ri, void *pkt, int poll_timeout_ms)
{
	struct ratp_header *hdr = pkt;
	void *data = pkt + sizeof(struct ratp_header);
	int ret;

	ret = ratp_recv_pkt_header(ri, hdr, poll_timeout_ms);
	if (ret < 0)
		return ret;

	if (hdr->control & (RATP_CONTROL_SO | RATP_CONTROL_RST | RATP_CONTROL_SYN |
		RATP_CONTROL_FIN))
		return 0;

	if (hdr->data_length) {
		ret = ratp_recv_pkt_data(ri, data, hdr->data_length,
				poll_timeout_ms);
		if (ret)
			return ret;
	}

	return 0;
}

static bool ratp_an_expected(struct ratp_internal *ri, struct ratp_header *hdr)
{
	return ratp_an(hdr) == (ri->sn_sent + 1) % 2;
}

static bool ratp_sn_expected(struct ratp_internal *ri, struct ratp_header *hdr)
{
	return ratp_sn(hdr) == (ri->sn_received + 1) % 2;
}

static int ratp_send_ack(struct ratp_internal *ri, struct ratp_header *hdr)
{
	uint8_t control;
	int ret;

	control = ratp_set_sn(ratp_an(hdr)) |
		ratp_set_next_an(ratp_sn(hdr)) |
		RATP_CONTROL_ACK;

	ret = ratp_send_hdr(ri, control);
	if (ret)
		return ret;

	return 0;
}

static int ratp_send_next_data(struct ratp_internal *ri)
{
	uint16_t crc;
	uint8_t control;
	struct ratp_header *hdr;
	uint8_t *data;
	int pktlen;
	struct ratp_message *msg;
	int len;

	if (ri->sendmsg_current) {
		pr_err("%s: busy\n", __func__);
		return -EBUSY;
	}

	if (list_empty(&ri->sendmsg))
		return 0;

	msg = list_first_entry(&ri->sendmsg, struct ratp_message, list);

	ri->sendmsg_current = msg;

	list_del(&msg->list);

	len = msg->len;

	control = ratp_set_next_sn(ri->sn_sent) |
		ratp_set_next_an(ri->sn_received) |
		RATP_CONTROL_ACK;

	hdr = msg->buf;
	data = (uint8_t *)(hdr + 1);

	if (msg->eor)
		control |= RATP_CONTROL_EOR;

	pktlen = sizeof(struct ratp_header);
	if (len > 1) {
		pktlen += len + 2;
		crc = cyg_crc16(data, len);
		put_unaligned_be16(crc, data + len);
	} else if (len == 1) {
		control |= RATP_CONTROL_SO;
		len = *data;
	}

	ratp_create_packet(ri, hdr, control, len);

	ri->retransmission_count = 0;

	ratp_send_pkt(ri, msg->buf, pktlen);

	return 0;
}

static void ratp_start_time_wait_timer(struct ratp_internal *ri)
{
	ri->timewait_timer_start = get_time_ns();
}

static void ratp_msg_done(struct ratp_internal *ri, struct ratp_message *msg, int status)
{
	int alpha, beta, rtt;

	if (!status) {
		rtt = (unsigned long)(get_time_ns() - ri->retransmission_timer_start) / MSECOND;

		alpha = 8;
		beta = 15;

		ri->srtt = (alpha * ri->srtt + (10 - alpha) * rtt) / 10;
		ri->rto = max(200, beta * ri->srtt / 10);

		pr_debug("%s: done. SRTT: %dms RTO: %dms status: %d\n",
			__func__, ri->srtt, ri->rto, ri->status);
	}

	if (msg->complete)
		msg->complete(msg->complete_ctx, status);

	free(msg->buf);
	free(msg);
}

/*
 * This procedure details the behavior of the LISTEN state.  First
 * check the packet for the RST flag.  If it is set then packet is
 * discarded and ignored, return and continue the processing
 * associated with this state.
 *
 * We assume now that the RST flag was not set.  Check the packet
 * for the ACK flag.  If it is set we have an illegal condition
 * since no connection has yet been opened.  Send a RST packet
 * with the correct response SN value:
 *
 * <SN=received AN><CTL=RST>
 *
 * Return to the current state without any further processing.
 *
 * We assume now that neither the RST nor the ACK flags were set.
 * Check the packet for a SYN flag.  If it is set then an attempt
 * is being made to open a connection.  Create a TCB for this
 * connection.  The sender has placed its MDL in the LENGTH field,
 * also specified is the sender's initial SN value.  Retrieve and
 * place them into the TCB.  Note that the presence of the SO flag
 * is ignored since it has no meaning when either of the SYN, RST,
 * or FIN flags are set.
 *
 * Send a SYN packet which acknowledges the SYN received.  Choose
 * the initial SN value and the MDL for this end of the
 * connection:
 *
 * <SN=0><AN=received SN+1 modulo 2><CTL=SYN, ACK><LENGTH=MDL>
 *
 * and go to the RATP_STATE_SYN_RECEIVED state without any further
 * processing.
 *
 * Any packet not satisfying the above tests is discarded and
 * ignored.  Return to the current state without any further
 * processing.
 */
static void ratp_behaviour_a(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (hdr->control & RATP_CONTROL_RST)
		return;

	if (hdr->control & RATP_CONTROL_ACK) {
		uint8_t control = RATP_CONTROL_RST;

		if (hdr->control & RATP_CONTROL_AN)
			control |= RATP_CONTROL_SN;

		ratp_send_hdr(ri, control);

		return;
	}

	if (hdr->control & RATP_CONTROL_SYN) {
		struct ratp_header synack = {};
		uint8_t control = RATP_CONTROL_SYN | RATP_CONTROL_ACK;

		if (!(hdr->control & RATP_CONTROL_SN))
			control |= RATP_CONTROL_AN;

		ratp_create_packet(ri, &synack, control, 255);
		ratp_send_pkt(ri, &synack, sizeof(synack));

		ratp_state_change(ri, RATP_STATE_SYN_RECEIVED);
	}
}

/*
 * This procedure represents the behavior of the SYN-SENT state
 * and is entered when this end of the connection decides to
 * execute an active OPEN.
 *
 * First, check the packet for the ACK flag.  If the ACK flag is
 * set then check to see if the AN value was as expected.  If it
 * was continue below.  Otherwise the AN value was unexpected.  If
 * the RST flag was set then discard the packet and return to the
 * current state without any further processing, else send a
 * reset:
 *
 * <SN=received AN><CTL=RST>
 *
 * Discard the packet and return to the current state without any
 * further processing.
 *
 * At this point either the ACK flag was set and the AN value was
 * as expected or ACK was not set.  Second, check the RST flag.
 * If the RST flag is set there are two cases:
 *
 * . If the ACK flag is set then discard the packet, flush the
 * retransmission queue, inform the user "Error: Connection
 * refused", delete the TCB, and go to the CLOSED state without
 * any further processing.
 *
 * 2. If the ACK flag was not set then discard the packet and
 * return to this state without any further processing.
 *
 * At this point we assume the packet contained an ACK which was
 * Ok, or there was no ACK, and there was no RST.  Now check the
 * packet for the SYN flag.  If the ACK flag was set then our SYN
 * has been acknowledged.  Store MDL received in the TCB.  At this
 * point we are technically in the ESTABLISHED state.  Send an
 * acknowledgment packet and any initial data which is queued to
 * send:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK><DATA>
 *
 * Go to the ESTABLISHED state without any further processing.
 *
 * If the SYN flag was set but the ACK was not set then the other
 * end of the connection has executed an active open also.
 * Acknowledge the SYN, choose your MDL, and send:
 *
 * <SN=0><AN=received SN+1 modulo 2><CTL=SYN, ACK><LENGTH=MDL>
 *
 * Go to the SYN-RECEIVED state without any further processing.
 *
 * Any packet not satisfying the above tests is discarded and
 * ignored.  Return to the current state without any further
 * processing.
 */
static void ratp_behaviour_b(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if ((hdr->control & RATP_CONTROL_ACK) && !ratp_an_expected(ri, hdr)) {
		if (!(hdr->control & RATP_CONTROL_RST)) {
			uint8_t control;

			control = RATP_CONTROL_RST |
				ratp_set_sn(ratp_an(hdr));

			ratp_send_hdr(ri, control);
		}
		return;
	}

	if (hdr->control & RATP_CONTROL_RST) {
		if (hdr->control & RATP_CONTROL_ACK) {
			ri->status = -ECONNREFUSED;

			pr_debug("Connection refused\n");

			ratp_state_change(ri, RATP_STATE_CLOSED);

		}
		return;
	}

	if (hdr->control & RATP_CONTROL_SYN) {
		uint8_t control;

		ri->sn_received = ratp_sn(hdr);

		if (hdr->control & RATP_CONTROL_ACK) {
			ratp_state_change(ri, RATP_STATE_ESTABLISHED);
			if (list_empty(&ri->sendmsg) || ri->sendmsg_current)
				ratp_send_ack(ri, hdr);
			else
				ratp_send_next_data(ri);
		} else {
			struct ratp_header synack = {};

			control = ratp_set_next_an(ratp_sn(hdr)) |
				RATP_CONTROL_SYN |
				RATP_CONTROL_ACK;

			ratp_create_packet(ri, &synack, control, 255);
			ratp_send_pkt(ri, &synack, sizeof(synack));
			ratp_state_change(ri, RATP_STATE_SYN_RECEIVED);
		}
	}
}

/*
 * Examine the received SN field value.  If the SN value was
 * expected then return and continue the processing associated
 * with this state.
 *
 * We now assume the SN value was not what was expected.
 *
 * If either RST or FIN were set discard the packet and return to
 * the current state without any further processing.
 *
 * If neither RST nor FIN flags were set it is assumed that this
 * packet is a duplicate of one already received.  Send an ACK
 * back:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * Discard the duplicate packet and return to the current state
 * without any further processing.
 */
static int ratp_behaviour_c1(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	int ret;

	pr_debug("%s\n", __func__);

	if (ratp_sn_expected(ri, hdr)) {
		pr_vdebug("%s: sn is expected\n", __func__);
		return 0;
	}

	if (!(hdr->control & RATP_CONTROL_RST) &&
			!(hdr->control & RATP_CONTROL_FIN)) {
		ret = ratp_send_ack(ri, hdr);
		if (ret)
			return ret;
	}

	return 1;

}

/*
 * Examine the received SN field value.  If the SN value was
 * expected then return and continue the processing associated
 * with this state.
 *
 * We now assume the SN value was not what was expected.
 *
 * If either RST or FIN were set discard the packet and return to
 * the current state without any further processing.
 *
 * If SYN was set we assume that the other end crashed and has
 * attempted to open a new connection.  We respond by sending a
 * legal reset:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=RST, ACK>
 *
 * This will cause the other end, currently in the SYN-SENT state,
 * to close.  Flush the retransmission queue, inform the user
 * "Error: Connection reset", discard the packet, delete the TCB,
 * and go to the CLOSED state without any further processing.
 *
 * If neither RST, FIN, nor SYN flags were set it is assumed that
 * this packet is a duplicate of one already received.  Send an
 * ACK back:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * Discard the duplicate packet and return to the current state
 * without any further processing.
 */
static int ratp_behaviour_c2(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	int ret;

	pr_debug("%s\n", __func__);

	if (ratp_sn_expected(ri, hdr))
		return 0;

	if ((hdr->control & RATP_CONTROL_RST) ||
			(hdr->control & RATP_CONTROL_FIN))
		return 1;

	if (hdr->control & RATP_CONTROL_SYN) {
		uint8_t control;

		ri->status = -ECONNRESET;
		pr_debug("Error: Connection reset\n");

		control = RATP_CONTROL_RST | RATP_CONTROL_ACK |
			ratp_set_sn(ratp_an(hdr)) | ratp_set_next_an(ratp_sn(hdr));
		ratp_send_hdr(ri, control);

		ratp_state_change(ri, RATP_STATE_CLOSED);
		return 1;
	}

	pr_debug("Sending ack for duplicate message\n");
	ret = ratp_send_ack(ri, hdr);
	if (ret)
		return ret;

	return 1;
}

/*
 * The packet is examined for a RST flag.  If RST is not set then
 * return and continue the processing associated with this state.
 *
 * RST is now assumed to have been set.  If the connection was
 * originally initiated from the LISTEN state (it was passively
 * opened) then flush the retransmission queue, discard the
 * packet, and go to the LISTEN state without any further
 * processing.
 *
 * If instead the connection was initiated actively (came from the
 * SYN-SENT state) then flush the retransmission queue, inform the
 * user "Error: Connection refused", discard the packet, delete
 * the TCB, and go to the CLOSED state without any further
 * processing.
 */
static int ratp_behaviour_d1(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_RST))
		return 0;

	if (!(ri->active)) {
		ratp_state_change(ri, RATP_STATE_LISTEN);
		return 1;
	}

	ri->status = -ECONNREFUSED;

	pr_debug("Error: connection refused\n");

	ratp_state_change(ri, RATP_STATE_CLOSED);

	return 1;
}

/*
 * The packet is examined for a RST flag.  If RST is not set then
 * return and continue the processing associated with this state.
 *
 * RST is now assumed to have been set.  Any data remaining to be
 * sent is flushed.  The retransmission queue is flushed, the user
 * is informed "Error: Connection reset.", discard the packet,
 * delete the TCB, and go to the CLOSED state without any further
 * processing.
 */
static int ratp_behaviour_d2(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_RST))
		return 0;

	ri->status = -ECONNRESET;

	pr_debug("connection reset\n");

	return 0;
}

/*
 * The packet is examined for a RST flag.  If RST is not set then
 * return and continue the processing associated with this state.
 *
 * RST is now assumed to have been set.  Discard the packet,
 * delete the TCB, and go to the CLOSED state without any further
 * processing.
 */
static int ratp_behaviour_d3(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_RST))
		return 0;

	ratp_state_change(ri, RATP_STATE_CLOSED);

	return 1;
}

/*
 * Check the presence of the SYN flag.  If the SYN flag is not set
 * then return and continue the processing associated with this
 * state.
 *
 * We now assume that the SYN flag was set.  The presence of a SYN
 * here is an error.  Flush the retransmission queue, send a legal
 * RST packet.
 *
 * If the ACK flag was set then send:
 *
 * <SN=received AN><CTL=RST>
 *
 * If the ACK flag was not set then send:
 *
 * <SN=0><CTL=RST>
 *
 * The user should receive the message "Error: Connection reset.",
 * then delete the TCB and go to the CLOSED state without any
 * further processing.
 */
static int ratp_behaviour_e(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_SYN))
		return 0;

	ri->status = -ECONNRESET;

	control = RATP_CONTROL_RST;

	if (hdr->control & RATP_CONTROL_ACK)
		control |= ratp_set_sn(ratp_an(hdr));

	ratp_send_hdr(ri, control);

	pr_debug("connection reset\n");

	ratp_state_change(ri, RATP_STATE_CLOSED);

	return 1;
}

/*
 * Check the presence of the ACK flag.  If ACK is not set then
 * discard the packet and return without any further processing.
 *
 * We now assume that the ACK flag was set.  If the AN field value
 * was as expected then return and continue the processing
 * associated with this state.
 *
 * We now assume that the ACK flag was set and that the AN field
 * value was unexpected.  If the connection was originally
 * initiated from the LISTEN state (it was passively opened) then
 * flush the retransmission queue, discard the packet, and send a
 * legal RST packet:
 *
 * <SN=received AN><CTL=RST>
 *
 * Then delete the TCB and go to the LISTEN state without any
 * further processing.
 *
 * Otherwise the connection was initiated actively (came from the
 * SYN-SENT state) then inform the user "Error: Connection
 * refused", flush the retransmission queue, discard the packet,
 * and send a legal RST packet:
 *
 * <SN=received AN><CTL=RST>
 *
 * Then delete the TCB and go to the CLOSED state without any
 * further processing.
 */
static int ratp_behaviour_f1(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_ACK))
		return 1;

	if (ratp_an_expected(ri, hdr))
		return 0;

	control = RATP_CONTROL_RST | ratp_set_sn(ratp_an(hdr));
	ratp_send_hdr(ri, control);

	if (ri->active) {
		ratp_state_change(ri, RATP_STATE_CLOSED);
		ri->status = -ECONNREFUSED;

		pr_debug("connection refused\n");
	} else {
		ratp_state_change(ri, RATP_STATE_LISTEN);
	}

	return 1;
}

/*
 * Check the presence of the ACK flag.  If ACK is not set then
 * discard the packet and return without any further processing.
 *
 * We now assume that the ACK flag was set.  If the AN field value
 * was as expected then flush the retransmission queue and inform
 * the user with an "Ok" if a buffer has been entirely
 * acknowledged.  Another packet containing data may now be sent.
 * Return and continue the processing associated with this state.
 *
 * We now assume that the ACK flag was set and that the AN field
 * value was unexpected.  This is assumed to indicate a duplicate
 * acknowledgment.  It is ignored, return and continue the
 * processing associated with this state.
 */
static int ratp_behaviour_f2(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_ACK))
		return 1;

	if (ratp_an_expected(ri, hdr)) {
		pr_debug("Data succesfully sent\n");
		if (ri->sendmsg_current)
			ratp_msg_done(ri, ri->sendmsg_current, 0);
		ri->sendmsg_current = NULL;
		return 0;
	} else {
		pr_vdebug("%s: an not expected\n", __func__);
	}

	return 0;
}

/*
 * Check the presence of the ACK flag.  If ACK is not set then
 * discard the packet and return without any further processing.
 *
 * We now assume that the ACK flag was set.  If the AN field value
 * was as expected then continue the processing associated with
 * this state.
 *
 * We now assume that the ACK flag was set and that the AN field
 * value was unexpected.  This is ignored, return and continue
 * with the processing associated with this state.
 */
static int ratp_behaviour_f3(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_ACK))
		return 1;

	return 0;
}

/*
 * This procedure represents the behavior of the CLOSED state of a
 * connection.  All incoming packets are discarded.  If the packet
 * had the RST flag set take no action.  Otherwise it is necessary
 * to build a RST packet.  Since this end is closed the other end
 * of the connection has incorrect data about the state of the
 * connection and should be so informed.
 *
 * If the ACK flag was set then send:
 *
 * <SN=received AN><CTL=RST>
 *
 * If the ACK flag was not set then send:
 *
 * <SN=0><AN=received SN+1 modulo 2><CTL=RST, ACK>
 *
 * After sending the reset packet return to the current state
 * without any further processing.
 */
static int ratp_behaviour_g(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;

	pr_debug("%s\n", __func__);

	if (hdr->control & RATP_CONTROL_RST)
		return 0;

	control = RATP_CONTROL_RST;

	if (hdr->control & RATP_CONTROL_ACK)
		control |= ratp_set_sn(ratp_an(hdr));
	else
		control |= ratp_set_next_an(ratp_sn(hdr)) | RATP_CONTROL_ACK;

	ratp_send_hdr(ri, control);

	return 0;
}

static int ratp_behaviour_i1(struct ratp_internal *ri, void *pkt);

/*
 * Our SYN has been acknowledged.  At this point we are
 * technically in the ESTABLISHED state.  Send any initial data
 * which is queued to send:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK><DATA>
 *
 * Go to the ESTABLISHED state and execute procedure I1 to process
 * any data which might be in this packet.
 *
 * Any packet not satisfying the above tests is discarded and
 * ignored.  Return to the current state without any further
 * processing.
 */
static int ratp_behaviour_h1(struct ratp_internal *ri, void *pkt)
{
	pr_debug("%s\n", __func__);

	ratp_state_change(ri, RATP_STATE_ESTABLISHED);

	/* If the input message has data (i.e. it is not just an ACK
	 * without data) then we need to send back an ACK ourselves,
	 * or even data if we have it pending. This is the same
	 * procedure done in i1, so just run it. */
	return ratp_behaviour_i1 (ri, pkt);
}

/*
 * Check the presence of the FIN flag.  If FIN is not set then
 * continue the processing associated with this state.
 *
 * We now assume that the FIN flag was set.  This means the other
 * end has decided to close the connection.  Flush the
 * retransmission queue.  If any data remains to be sent then
 * inform the user "Warning: Data left unsent."  The user must
 * also be informed "Connection closing."  An acknowledgment for
 * the FIN must be sent which also indicates this end is closing:
 *
 * <SN=received AN><AN=received SN + 1 modulo 2><CTL=FIN, ACK>
 *
 * Go to the LAST-ACK state without any further processing.
 */
static int ratp_behaviour_h2(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_FIN))
		return 0;

	ri->status = -ENETDOWN;

	control = ratp_set_sn(ratp_an(hdr)) |
		ratp_set_next_an(ratp_sn(hdr)) |
		RATP_CONTROL_FIN |
		RATP_CONTROL_ACK;

	ratp_send_hdr(ri, control);

	ratp_state_change(ri, RATP_STATE_LAST_ACK);

	return 1;
}

/*
 * This state represents the final behavior of the FIN-WAIT state.
 *
 * If the packet did not contain a FIN we assume this packet is a
 * duplicate and that the other end of the connection has not seen
 * the FIN packet we sent earlier.  Rely upon retransmission of
 * our earlier FIN packet to inform the other end of our desire to
 * close.  Discard the packet and return without any further
 * processing.
 *
 * At this point we have a packet which should contain a FIN.  By
 * the rules of this protocol an ACK of a FIN requires a FIN, ACK
 * in response and no data.  If the packet contains data we have
 * detected an illegal condition.  Send a reset:
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=RST, ACK>
 *
 * Discard the packet, flush the retransmission queue, inform the
 * ser "Error: Connection reset.", delete the TCB, and go to the
 * CLOSED state without any further processing.
 *
 * We now assume that the FIN flag was set and no data was
 * contained in the packet.  If the AN field value was expected
 * then this packet acknowledges a previously sent FIN packet.
 * The other end of the connection is then also assumed to be
 * closing and expects an acknowledgment.  Send an acknowledgment
 * of the FIN:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * Start the 2*SRTT timer associated with the TIME-WAIT state,
 * discard the packet, and go to the TIME-WAIT state without any
 * further processing.
 *
 * Otherwise the AN field value was unexpected.  This indicates a
 * simultaneous closing by both sides of the connection.  Send an
 * acknowledgment of the FIN:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * Discard the packet, and go to the CLOSING state without any
 * further processing.
 */
static int ratp_behaviour_h3(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;
	int expected;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_FIN))
		return 1;

	if (ratp_has_data(hdr)) {
		control = ratp_set_sn(ratp_an(hdr)) |
			ratp_set_next_an(ratp_sn(hdr)) |
			RATP_CONTROL_RST |
			RATP_CONTROL_ACK;
		ratp_send_hdr(ri, control);
		ri->status = -ECONNRESET;
		pr_debug("Error: Connection reset\n");
		ratp_state_change(ri, RATP_STATE_CLOSED);
		return 1;
	}

	control = ratp_set_sn(ratp_an(hdr)) |
		ratp_set_next_an(ratp_sn(hdr)) |
		RATP_CONTROL_ACK;

	expected = ratp_an_expected(ri, hdr);

	ratp_send_hdr(ri, control);

	if (expected) {
		ratp_state_change(ri, RATP_STATE_TIME_WAIT);
		ratp_start_time_wait_timer(ri);
	} else {
		ratp_state_change(ri, RATP_STATE_CLOSING);
	}

	return 1;
}

/*
 * This state represents the final behavior of the LAST-ACK state.
 *
 * If the AN field value is expected then this ACK is in response
 * to the FIN, ACK packet recently sent.  This is the final
 * acknowledging message indicating both side's agreement to close
 * the connection.  Discard the packet, flush all queues, delete
 * the TCB, and go to the CLOSED state without any further
 * processing.
 *
 * Otherwise the AN field value was unexpected.  Discard the
 * packet and remain in the current state without any further
 * processing.
 */
static int ratp_behaviour_h4(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (ratp_an_expected(ri, hdr))
		ratp_state_change(ri, RATP_STATE_CLOSED);

	return 1;
}

/*
 * This state represents the final behavior of the CLOSING state.
 *
 * If the AN field value was expected then this packet
 * acknowledges the FIN packet recently sent.  This is the final
 * acknowledging message indicating both side's agreement to close
 * the connection.  Start the 2*SRTT timer associated with the
 * TIME-WAIT state, discard the packet, and go to the TIME-WAIT
 * state without any further processing.
 *
 * Otherwise the AN field value was unexpected.  Discard the
 * packet and remain in the current state without any further
 * processing.
 */
static int ratp_behaviour_h5(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	pr_debug("%s\n", __func__);

	if (ratp_an_expected(ri, hdr)) {
		ratp_state_change(ri, RATP_STATE_TIME_WAIT);
		ratp_start_time_wait_timer(ri);
	}

	return 0;
}

/*
 * This state represents the behavior of the TIME-WAIT state.
 * Check the presence of the ACK flag.  If ACK is not set then
 * discard the packet and return without any further processing.
 *
 * Check the presence of the FIN flag.  If FIN is not set then
 * discard the packet and return without any further processing.
 *
 * We now assume that the FIN flag was set.  This situation
 * indicates that the last acknowledgment of the FIN packet sent
 * by the other end of the connection did not arrive.  Resend the
 * acknowledgment:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * Restart the 2*SRTT timer, discard the packet, and remain in the
 * current state without any further processing.
 */
static int ratp_behaviour_h6(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	uint8_t control;

	pr_debug("%s\n", __func__);

	if (!(hdr->control & RATP_CONTROL_ACK))
		return 1;

	if (!(hdr->control & RATP_CONTROL_FIN))
		return 1;

	control = ratp_set_next_sn(ratp_an(hdr)) | RATP_CONTROL_ACK;

	ratp_send_hdr(ri, control);

	ratp_start_time_wait_timer(ri);

	return 0;
}

static int msg_recv(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	struct ratp_message *msg;

	pr_debug("%s: Put msg in receive queue\n", __func__);

	msg = xzalloc(sizeof(*msg));
	if (hdr->data_length) {
		msg->len = hdr->data_length;
		msg->buf = xzalloc(msg->len);
		memcpy(msg->buf, pkt + sizeof(struct ratp_header), msg->len);
	} else {
		msg->len = 1;
		msg->buf = xzalloc(1);
		*(uint8_t *)msg->buf = hdr->data_length;
	}

	if (hdr->control & RATP_CONTROL_EOR)
		msg->eor = 1;

	list_add_tail(&msg->list, &ri->recvmsg);

	return 0;
}

/*
 * This represents that stage of processing in the ESTABLISHED
 * state in which all the flag bits have been processed and only
 * data may remain.  The packet is examined to see if it contains
 * data.  If not the packet is now discarded, return to the
 * current state without any further processing.
 *
 * We assume the packet contained data, that either the SO flag
 * was set or LENGTH is positive.  That data is placed into the
 * user's receive buffers.  As these become full the user should
 * be informed "Receive buffer full."  An acknowledgment is sent:
 *
 * <SN=received AN><AN=received SN+1 modulo 2><CTL=ACK>
 *
 * If data is queued to send then it is most efficient to
 * 'piggyback' this acknowledgment on that data packet.
 *
 * The packet is now discarded, return to the ESTABLISHED state
 * without any further processing.
 */
static int ratp_behaviour_i1(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;

	if (!ratp_has_data (hdr))
		return 1;

	pr_vdebug("%s **received** %d\n", __func__, hdr->data_length);

	ri->sn_received = ratp_sn(hdr);

	msg_recv(ri, pkt);

	if (list_empty(&ri->sendmsg) || ri->sendmsg_current)
		ratp_send_ack(ri, hdr);
	else
		ratp_send_next_data(ri);

	return 0;
}

/*
 * State machine as desribed in RFC916
 *
 * STATE                BEHAVIOR
 * =============+========================
 * LISTEN       |  A
 * -------------+------------------------
 * SYN-SENT     |  B
 * -------------+------------------------
 * SYN-RECEIVED |  C1  D1  E  F1  H1
 * -------------+------------------------
 * ESTABLISHED  |  C2  D2  E  F2  H2  I1
 * -------------+------------------------
 * FIN-WAIT     |  C2  D2  E  F3  H3
 * -------------+------------------------
 * LAST-ACK     |  C2  D3  E  F3  H4
 * -------------+------------------------
 * CLOSING      |  C2  D3  E  F3  H5
 * -------------+------------------------
 * TIME-WAIT    |  D3  E  F3 H6
 * -------------+------------------------
 * CLOSED       |  G
 * -------------+------------------------
 */

static int ratp_state_machine(struct ratp_internal *ri, void *pkt)
{
	struct ratp_header *hdr = pkt;
	int ret;

	ratp_print_header(ri, hdr, "                      recv");
	pr_debug(" state %s\n", ratp_state_str[ri->state]);

	switch (ri->state) {
	case RATP_STATE_LISTEN:
		ratp_behaviour_a(ri, pkt);
		break;
	case RATP_STATE_SYN_SENT:
		ratp_behaviour_b(ri, pkt);
		break;
	case RATP_STATE_SYN_RECEIVED:
		ret = ratp_behaviour_c1(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_d1(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f1(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h1(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_ESTABLISHED:
		ret = ratp_behaviour_c2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_d2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_i1(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_FIN_WAIT:
		ret = ratp_behaviour_c2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_d2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h3(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_LAST_ACK:
		ret = ratp_behaviour_c2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_d3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h4(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_CLOSING:
		ret = ratp_behaviour_c2(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_d3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h5(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_TIME_WAIT:
		ret = ratp_behaviour_d3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_e(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_f3(ri, pkt);
		if (ret)
			return ret;
		ret = ratp_behaviour_h6(ri, pkt);
		if (ret)
			return ret;
		break;
	case RATP_STATE_CLOSED:
		ratp_behaviour_g(ri, pkt);
		break;
	};

	return 0;
}

/**
 * ratp_closed() - Check if a connection is closed
 *
 * Return: true if a connection is closed, false otherwise
 */
bool ratp_closed(struct ratp *ratp)
{
	struct ratp_internal *ri = ratp->internal;

	if (!ri)
		return true;

	return ri->state == RATP_STATE_CLOSED;
}

/**
 * ratp_busy() - Check if we are inside the RATP code
 *
 * Needed for RATP debugging. The RATP console uses this to determine
 * if it is called from inside the RATP code.
 *
 * Return: true if we are inside the RATP code, false otherwise
 */
bool ratp_busy(struct ratp *ratp)
{
	struct ratp_internal *ri = ratp->internal;

	if (!ri)
		return false;

	return ri->in_ratp != 0;
}

/**
 * ratp_poll() - Execute RATP state machine
 * @ratp: The RATP link
 *
 * This function should be executed periodically to keep the RATP state
 * machine going.
 *
 * Return: 0 if successful, a negative error code otherwise.
 */
int ratp_poll(struct ratp *ratp)
{
	struct ratp_internal *ri = ratp->internal;
	int ret;

	if (!ri)
		return -ENETDOWN;

	ri->in_ratp++;

	ret = ratp_recv_pkt(ri, ri->recvbuf, 100);
	if (ret == 0) {

		if (ri->state == RATP_STATE_TIME_WAIT &&
		    is_timeout(ri->timewait_timer_start, ri->srtt * 2 * MSECOND)) {
			pr_debug("2*SRTT timer timed out\n");
			ret = -ECONNRESET;
			goto out;
		}

		ret = ratp_state_machine(ri, ri->recvbuf);
		if (ret < 0)
			goto out;

		if (ri->status < 0) {
			ret = ri->status;
			goto out;
		}
	}

	if (ri->sendmsg_current && is_timeout(ri->retransmission_timer_start,
	    ri->rto * MSECOND)) {

		ri->retransmission_count++;
		if (ri->retransmission_count == ri->max_retransmission) {
			ri->status = ret = -ETIMEDOUT;
			ri->state = RATP_STATE_CLOSED;
			goto out;
		}

		pr_debug("%s: retransmit\n", __func__);

		ratp_print_header(ri, ri->sendbuf, "resend");

		ri->retransmission_timer_start = get_time_ns();

		ret = ri->ratp->send(ratp, ri->sendbuf, ri->sendbuf_len);
		if (ret)
			goto out;
	}

	if (!ri->sendmsg_current && !list_empty(&ri->sendmsg))
		ratp_send_next_data(ri);

	ret = 0;
out:
	ri->in_ratp--;

	return ret;
}

/**
 * ratp_establish(): Establish a RATP link
 * @ratp: The RATP link
 * @active: if true actively create a connection
 * @timeout_ms: Timeout in ms to wait until a connection is established. If
 *              0 wait forever.
 *
 * This function establishes a link with the remote end. It expects the
 * send and receive functions to be set, all other struct ratp_internal members can
 * be left uninitialized.
 *
 * Return: 0 if successful, a negative error code otherwise.
 */
int ratp_establish(struct ratp *ratp, bool active, int timeout_ms)
{
	struct ratp_internal *ri;
	int ret;
	uint64_t start;

	ri = xzalloc(sizeof(*ri));
	ri->ratp = ratp;
	ratp->internal = ri;

	ri->recvbuf = xmalloc(512);
	ri->sendbuf = xmalloc(512);
	INIT_LIST_HEAD(&ri->recvmsg);
	INIT_LIST_HEAD(&ri->sendmsg);
	ri->max_retransmission = 100;
	ri->srtt = 100;
	ri->rto = 100;
	ri->active = active;

	ri->in_ratp++;

	if (ri->active) {
		ratp_send_hdr(ri, RATP_CONTROL_SYN);

		ratp_state_change(ri, RATP_STATE_SYN_SENT);
	}

	start = get_time_ns();

	while (1) {
		ret = ratp_poll(ri->ratp);
		if (ret < 0)
			goto out;

		if (ri->state == RATP_STATE_ESTABLISHED) {
			ret = 0;
			goto out;
		}

		if (timeout_ms && is_timeout(start, MSECOND * timeout_ms)) {
			ret = -ETIMEDOUT;
			goto out;
		}
	}

out:
	ri->in_ratp--;

	if (ret) {
		free(ri->recvbuf);
		free(ri->sendbuf);
		free(ri);
		ratp->internal = NULL;
	}

	return ret;
}

void ratp_close(struct ratp *ratp)
{
	struct ratp_internal *ri = ratp->internal;
	struct ratp_message *msg, *tmp;
	struct ratp_header fin = {};

	if (!ri)
		return;

	if (ri->state == RATP_STATE_ESTABLISHED || ri->state == RATP_STATE_SYN_RECEIVED) {
		uint64_t start;
		u8 control;

		pr_debug("Closing...\n");

		ratp_state_change(ri, RATP_STATE_FIN_WAIT);

		control = ratp_set_next_sn(ri->sn_sent) |
			ratp_set_next_an(ri->sn_received) |
			RATP_CONTROL_FIN | RATP_CONTROL_ACK;

		ratp_create_packet(ri, &fin, control, 0);

		ratp_send_pkt(ri, &fin, sizeof(fin));

		start = get_time_ns();

		while (!is_timeout(start, ri->srtt * MSECOND * 2))
			ratp_poll(ratp);
	}

	list_for_each_entry_safe(msg, tmp, &ri->sendmsg, list)
		ratp_msg_done(ri, msg, -ECONNRESET);

	free(ri);
	ratp->internal = NULL;

	pr_debug("Closed\n");
}

/**
 * ratp_send_complete(): Send data over a RATP link
 * @ratp: The RATP link
 * @data: The data buffer
 * @len: The length of the message to send
 * @complete: The completion callback for the message
 * @complete_ctx: context pointer for the completion callback
 *
 * Queue a RATP message for transmission. This only queues the message,
 * ratp_poll has to be called to actually transfer the message.
 * @complete will be called upon completion of the message.
 *
 * Return: 0 if successful, a negative error code otherwise.
 */
int ratp_send_complete(struct ratp *ratp, const void *data, size_t len,
		   void (*complete)(void *ctx, int status), void *complete_ctx)
{
	struct ratp_internal *ri = ratp->internal;
	struct ratp_message *msg;

	if (!ri || ri->state != RATP_STATE_ESTABLISHED)
		return -ENETDOWN;

	if (!len)
		return -EINVAL;

	ri->in_ratp++;

	while (len) {
		int now = min((int)len, 255);

		msg = xzalloc(sizeof(*msg));
		msg->buf = xzalloc(sizeof(struct ratp_header) + now + 2);
		msg->len = now;
		memcpy(msg->buf + sizeof(struct ratp_header), data, now);

		list_add_tail(&msg->list, &ri->sendmsg);

		len -= now;
	}

	msg->eor = 1;
	msg->complete = complete;
	msg->complete_ctx = complete_ctx;

	ri->in_ratp--;

	return 0;
}

/**
 * ratp_send(): Send data over a RATP link
 * @ratp: The RATP link
 * @data: The data buffer
 * @len: The length of the message to send
 *
 * Queue a RATP message for transmission. This only queues the message,
 * ratp_poll has to be called to actually transfer the message.
 *
 * Return: 0 if successful, a negative error code otherwise.
 */
int ratp_send(struct ratp *ratp, const void *data, size_t len)
{
	return ratp_send_complete(ratp, data, len, NULL, NULL);
}

/**
 * ratp_recv() - Receive data from a RATP link
 * @ratp: The RATP link
 * @data: Pointer to data
 * @len: The length of the data in bytes
 *
 * If a message is available it fills @data with a pointer to the data.
 * This function does not wait for new messages. If no data is available
 * -EAGAIN is returned. If data is received @data has to be freed by the
 * caller.
 *
 * Return: 0 if successful, a negative error code otherwise.
 */
int ratp_recv(struct ratp *ratp, void **data, size_t *len)
{
	struct ratp_internal *ri = ratp->internal;
	struct ratp_message *msg, *tmp;
	void *pos;
	int num = 0;

	*len = 0;

	if (!ri || ri->state != RATP_STATE_ESTABLISHED)
		return -ENETDOWN;

	if (list_empty(&ri->recvmsg))
		return -EAGAIN;

	list_for_each_entry(msg, &ri->recvmsg, list) {
		*len += msg->len;
		num++;
		if (msg->eor)
			goto eor;
	}

	return -EAGAIN;

eor:
	*data = malloc(*len);
	if (!*data)
		return -ENOMEM;

	pos = *data;

	list_for_each_entry_safe(msg, tmp, &ri->recvmsg, list) {
		memcpy(pos, msg->buf, msg->len);
		pos += msg->len;

		list_del(&msg->list);

		free(msg->buf);
		free(msg);
	}

	return 0;
}
