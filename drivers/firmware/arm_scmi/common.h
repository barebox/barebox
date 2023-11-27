/* SPDX-License-Identifier: GPL-2.0 */
/*
 * System Control and Management Interface (SCMI) Message Protocol
 * driver common header file containing some definitions, structures
 * and function prototypes used in all the different SCMI protocols.
 *
 * Copyright (C) 2018-2022 ARM Ltd.
 */
#ifndef _SCMI_COMMON_H
#define _SCMI_COMMON_H

#include <linux/bitfield.h>
#include <driver.h>
#include <errno.h>
#include <linux/kernel.h>
#include <linux/scmi_protocol.h>
#include <linux/types.h>
#include <linux/idr.h>

#include <asm/unaligned.h>

#include "protocols.h"

#define SCMI_MAX_CHANNELS		256

#define SCMI_MAX_RESPONSE_TIMEOUT	(2 * MSEC_PER_SEC)

enum scmi_error_codes {
	SCMI_SUCCESS = 0,	/* Success */
	SCMI_ERR_SUPPORT = -1,	/* Not supported */
	SCMI_ERR_PARAMS = -2,	/* Invalid Parameters */
	SCMI_ERR_ACCESS = -3,	/* Invalid access/permission denied */
	SCMI_ERR_ENTRY = -4,	/* Not found */
	SCMI_ERR_RANGE = -5,	/* Value out of range */
	SCMI_ERR_BUSY = -6,	/* Device busy */
	SCMI_ERR_COMMS = -7,	/* Communication Error */
	SCMI_ERR_GENERIC = -8,	/* Generic Error */
	SCMI_ERR_HARDWARE = -9,	/* Hardware Error */
	SCMI_ERR_PROTOCOL = -10,/* Protocol Error */
};

static const int scmi_linux_errmap[] = {
	/* better than switch case as long as return value is continuous */
	0,			/* SCMI_SUCCESS */
	-EOPNOTSUPP,		/* SCMI_ERR_SUPPORT */
	-EINVAL,		/* SCMI_ERR_PARAM */
	-EACCES,		/* SCMI_ERR_ACCESS */
	-ENOENT,		/* SCMI_ERR_ENTRY */
	-ERANGE,		/* SCMI_ERR_RANGE */
	-EBUSY,			/* SCMI_ERR_BUSY */
	-ECOMM,			/* SCMI_ERR_COMMS */
	-EIO,			/* SCMI_ERR_GENERIC */
	-EREMOTEIO,		/* SCMI_ERR_HARDWARE */
	-EPROTO,		/* SCMI_ERR_PROTOCOL */
};

static inline int scmi_to_linux_errno(int errno)
{
	int err_idx = -errno;

	if (err_idx >= SCMI_SUCCESS && err_idx < ARRAY_SIZE(scmi_linux_errmap))
		return scmi_linux_errmap[err_idx];
	return -EIO;
}

#define MSG_ID_MASK		GENMASK(7, 0)
#define MSG_XTRACT_ID(hdr)	FIELD_GET(MSG_ID_MASK, (hdr))
#define MSG_TYPE_MASK		GENMASK(9, 8)
#define MSG_XTRACT_TYPE(hdr)	FIELD_GET(MSG_TYPE_MASK, (hdr))
#define MSG_TYPE_COMMAND	0
#define MSG_TYPE_DELAYED_RESP	2
#define MSG_TYPE_NOTIFICATION	3
#define MSG_PROTOCOL_ID_MASK	GENMASK(17, 10)
#define MSG_XTRACT_PROT_ID(hdr)	FIELD_GET(MSG_PROTOCOL_ID_MASK, (hdr))
#define MSG_TOKEN_ID_MASK	GENMASK(27, 18)
#define MSG_XTRACT_TOKEN(hdr)	FIELD_GET(MSG_TOKEN_ID_MASK, (hdr))
#define MSG_TOKEN_MAX		(MSG_XTRACT_TOKEN(MSG_TOKEN_ID_MASK) + 1)

/**
 * pack_scmi_header() - packs and returns 32-bit header
 *
 * @hdr: pointer to header containing all the information on message id,
 *	protocol id, sequence id and type.
 *
 * Return: 32-bit packed message header to be sent to the platform.
 */
static inline u32 pack_scmi_header(struct scmi_msg_hdr *hdr)
{
	return FIELD_PREP(MSG_ID_MASK, hdr->id) |
		FIELD_PREP(MSG_TYPE_MASK, hdr->type) |
		FIELD_PREP(MSG_TOKEN_ID_MASK, hdr->seq) |
		FIELD_PREP(MSG_PROTOCOL_ID_MASK, hdr->protocol_id);
}

/**
 * unpack_scmi_header() - unpacks and records message and protocol id
 *
 * @msg_hdr: 32-bit packed message header sent from the platform
 * @hdr: pointer to header to fetch message and protocol id.
 */
static inline void unpack_scmi_header(u32 msg_hdr, struct scmi_msg_hdr *hdr)
{
	hdr->id = MSG_XTRACT_ID(msg_hdr);
	hdr->protocol_id = MSG_XTRACT_PROT_ID(msg_hdr);
	hdr->type = MSG_XTRACT_TYPE(msg_hdr);
}

struct scmi_revision_info *
scmi_revision_area_get(const struct scmi_protocol_handle *ph);
void scmi_setup_protocol_implemented(const struct scmi_protocol_handle *ph,
				     u8 *prot_imp);

extern struct bus_type scmi_bus_type;

int __init scmi_bus_init(void);

struct scmi_handle *scmi_handle_get(struct device *dev);

#define SCMI_BUS_NOTIFY_DEVICE_REQUEST		0
#define SCMI_BUS_NOTIFY_DEVICE_UNREQUEST	1
extern struct blocking_notifier_head scmi_requested_devices_nh;

struct scmi_device *scmi_device_create(struct device_node *np,
				       struct device *parent, int protocol,
				       const char *name);
void scmi_device_destroy(struct device *parent, int protocol, const char *name);

int scmi_protocol_acquire(const struct scmi_handle *handle, u8 protocol_id);
void scmi_protocol_release(const struct scmi_handle *handle, u8 protocol_id);

/* SCMI Transport */
/**
 * struct scmi_chan_info - Structure representing a SCMI channel information
 *
 * @id: An identifier for this channel: this matches the protocol number
 *      used to initialize this channel
 * @dev: Reference to device in the SCMI hierarchy corresponding to this
 *	 channel
 * @rx_timeout_ms: The configured RX timeout in milliseconds.
 * @handle: Pointer to SCMI entity handle
 * @transport_info: Transport layer related information
 */
struct scmi_chan_info {
	int id;
	struct device *dev;
	unsigned int rx_timeout_ms;
	struct scmi_handle *handle;
	void *transport_info;
};

/**
 * struct scmi_transport_ops - Structure representing a SCMI transport ops
 *
 * @link_supplier: Optional callback to add link to a supplier device
 * @chan_available: Callback to check if channel is available or not
 * @chan_setup: Callback to allocate and setup a channel
 * @chan_free: Callback to free a channel
 * @get_max_msg: Optional callback to provide max_msg dynamically
 *		 Returns the maximum number of messages for the channel type
 *		 (tx or rx) that can be pending simultaneously in the system
 * @send_message: Callback to send a message
 * @mark_txdone: Callback to mark tx as done
 * @fetch_response: Callback to fetch response
 * @clear_channel: Callback to clear a channel
 */
struct scmi_transport_ops {
	int (*link_supplier)(struct device *dev);
	bool (*chan_available)(struct device_node *of_node, int idx);
	int (*chan_setup)(struct scmi_chan_info *cinfo, struct device *dev,
			  bool tx);
	int (*chan_free)(int id, void *p, void *data);
	unsigned int (*get_max_msg)(struct scmi_chan_info *base_cinfo);
	int (*send_message)(struct scmi_chan_info *cinfo,
			    struct scmi_xfer *xfer);
	void (*mark_txdone)(struct scmi_chan_info *cinfo, int ret,
			    struct scmi_xfer *xfer);
	void (*fetch_response)(struct scmi_chan_info *cinfo,
			       struct scmi_xfer *xfer);
	void (*clear_channel)(struct scmi_chan_info *cinfo);
};

/**
 * struct scmi_desc - Description of SoC integration
 *
 * @ops: Pointer to the transport specific ops structure
 * @max_rx_timeout_ms: Timeout for communication with SoC (in Milliseconds)
 * @max_msg: Maximum number of messages for a channel type (tx or rx) that can
 *	be pending simultaneously in the system. May be overridden by the
 *	get_max_msg op.
 * @max_msg_size: Maximum size of data per message that can be handled.
 * @sync_cmds_completed_on_ret: Flag to indicate that the transport assures
 *				synchronous-command messages are atomically
 *				completed on .send_message: no need to poll
 *				actively waiting for a response.
 *				Used by core internally only when polling is
 *				selected as a waiting for reply method: i.e.
 *				if a completion irq was found use that anyway.
 */
struct scmi_desc {
	const struct scmi_transport_ops *ops;
	int max_rx_timeout_ms;
	int max_msg;
	int max_msg_size;
	const bool sync_cmds_completed_on_ret;
};

static inline bool is_polling_required(struct scmi_chan_info *cinfo,
				       const struct scmi_desc *desc)
{
	return true;
}

static inline bool is_transport_polling_capable(const struct scmi_desc *desc)
{
	return desc->sync_cmds_completed_on_ret;
}

static inline bool is_polling_enabled(struct scmi_chan_info *cinfo,
				      const struct scmi_desc *desc)
{
	return is_polling_required(cinfo, desc) &&
		is_transport_polling_capable(desc);
}

void scmi_xfer_raw_put(const struct scmi_handle *handle,
		       struct scmi_xfer *xfer);
struct scmi_xfer *scmi_xfer_raw_get(const struct scmi_handle *handle);
struct scmi_chan_info *
scmi_xfer_raw_channel_get(const struct scmi_handle *handle, u8 protocol_id);

int scmi_xfer_raw_inflight_register(const struct scmi_handle *handle,
				    struct scmi_xfer *xfer);

int scmi_xfer_raw_wait_for_message_response(struct scmi_chan_info *cinfo,
					    struct scmi_xfer *xfer,
					    unsigned int timeout_ms);
#ifdef CONFIG_ARM_SCMI_TRANSPORT_SMC
extern const struct scmi_desc scmi_smc_desc;
#endif
#ifdef CONFIG_ARM_SCMI_TRANSPORT_OPTEE
extern const struct scmi_desc scmi_optee_desc;
#endif

void scmi_rx_callback(struct scmi_chan_info *cinfo, u32 msg_hdr, void *priv);

/* shmem related declarations */
struct scmi_shared_mem;

void shmem_tx_prepare(struct scmi_shared_mem __iomem *shmem,
		      struct scmi_xfer *xfer, struct scmi_chan_info *cinfo);
u32 shmem_read_header(struct scmi_shared_mem __iomem *shmem);
void shmem_fetch_response(struct scmi_shared_mem __iomem *shmem,
			  struct scmi_xfer *xfer);
void shmem_clear_channel(struct scmi_shared_mem __iomem *shmem);

/* declarations for message passing transports */
struct scmi_msg_payld;

/* Maximum overhead of message w.r.t. struct scmi_desc.max_msg_size */
#define SCMI_MSG_MAX_PROT_OVERHEAD (2 * sizeof(__le32))

size_t msg_response_size(struct scmi_xfer *xfer);
size_t msg_command_size(struct scmi_xfer *xfer);
void msg_tx_prepare(struct scmi_msg_payld *msg, struct scmi_xfer *xfer);
u32 msg_read_header(struct scmi_msg_payld *msg);
void msg_fetch_response(struct scmi_msg_payld *msg, size_t len,
			struct scmi_xfer *xfer);
#endif /* _SCMI_COMMON_H */
