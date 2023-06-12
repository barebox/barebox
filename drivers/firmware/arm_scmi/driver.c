// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Message Protocol driver
 *
 * SCMI Message Protocol is used between the System Control Processor(SCP)
 * and the Application Processors(AP). The Message Handling Unit(MHU)
 * provides a mechanism for inter-processor communication between SCP's
 * Cortex M3 and AP.
 *
 * SCP offers control and management of the core/cluster power states,
 * various power domain DVFS including the core/cluster, certain system
 * clocks configuration, thermal sensors and many others.
 *
 * Copyright (C) 2018-2021 ARM Ltd.
 */

#define pr_fmt(fmt) "SCMI DRIVER - " fmt

#include <common.h>
#include <linux/bitmap.h>
#include <driver.h>
#include <linux/export.h>
#include <io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <of_address.h>
#include <of_device.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/processor.h>

#include "common.h"

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
	SCMI_ERR_MAX
};

/* List of all SCMI devices active in system */
static LIST_HEAD(scmi_list);
/* Protection for the entire list */
/* Track the unique id for the transfers for debug & profiling purpose */
static unsigned transfer_last_id;

static DEFINE_IDR(scmi_requested_devices);

struct scmi_requested_dev {
	const struct scmi_device_id *id_table;
	struct list_head node;
};

/**
 * struct scmi_xfers_info - Structure to manage transfer information
 *
 * @xfer_block: Preallocated Message array
 * @xfer_alloc_table: Bitmap table for allocated messages.
 *	Index of this bitmap table is also used for message
 *	sequence identifier.
 */
struct scmi_xfers_info {
	struct scmi_xfer *xfer_block;
	unsigned long *xfer_alloc_table;
};

/**
 * struct scmi_protocol_instance  - Describe an initialized protocol instance.
 * @handle: Reference to the SCMI handle associated to this protocol instance.
 * @proto: A reference to the protocol descriptor.
 * @users: A refcount to track effective users of this protocol.
 * @priv: Reference for optional protocol private data.
 * @ph: An embedded protocol handle that will be passed down to protocol
 *	initialization code to identify this instance.
 *
 * Each protocol is initialized independently once for each SCMI platform in
 * which is defined by DT and implemented by the SCMI server fw.
 */
struct scmi_protocol_instance {
	const struct scmi_handle	*handle;
	const struct scmi_protocol	*proto;
	int				users;
	void				*priv;
	struct scmi_protocol_handle	ph;
};

#define ph_to_pi(h)	container_of(h, struct scmi_protocol_instance, ph)

/**
 * struct scmi_info - Structure representing a SCMI instance
 *
 * @dev: Device pointer
 * @desc: SoC description for this instance
 * @version: SCMI revision information containing protocol version,
 *	implementation version and (sub-)vendor identification.
 * @handle: Instance of SCMI handle to send to clients
 * @tx_minfo: Universal Transmit Message management info
 * @rx_minfo: Universal Receive Message management info
 * @tx_idr: IDR object to map protocol id to Tx channel info pointer
 * @rx_idr: IDR object to map protocol id to Rx channel info pointer
 * @protocols: IDR for protocols' instance descriptors initialized for
 *	       this SCMI instance: populated on protocol's first attempted
 *	       usage.
 * @protocols_imp: List of protocols implemented, currently maximum of
 *	MAX_PROTOCOLS_IMP elements allocated by the base protocol
 * @active_protocols: IDR storing device_nodes for protocols actually defined
 *		      in the DT and confirmed as implemented by fw.
 * @node: List head
 * @users: Number of users of this instance
 */
struct scmi_info {
	struct device *dev;
	const struct scmi_desc *desc;
	struct scmi_revision_info version;
	struct scmi_handle handle;
	struct scmi_xfers_info tx_minfo;
	struct scmi_xfers_info rx_minfo;
	struct idr tx_idr;
	struct idr rx_idr;
	struct idr protocols;
	/* Ensure mutual exclusive access to protocols instance array */
	u8 *protocols_imp;
	struct idr active_protocols;
	struct list_head node;
	int users;
};

#define handle_to_scmi_info(h)	container_of(h, struct scmi_info, handle)

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
	if (errno < SCMI_SUCCESS && errno > SCMI_ERR_MAX)
		return scmi_linux_errmap[-errno];
	return -EIO;
}

/**
 * scmi_dump_header_dbg() - Helper to dump a message header.
 *
 * @dev: Device pointer corresponding to the SCMI entity
 * @hdr: pointer to header.
 */
static inline void scmi_dump_header_dbg(struct device *dev,
					struct scmi_msg_hdr *hdr)
{
	dev_dbg(dev, "Message ID: %x Sequence ID: %x Protocol: %x\n",
		hdr->id, hdr->seq, hdr->protocol_id);
}

/**
 * scmi_xfer_get() - Allocate one message
 *
 * @handle: Pointer to SCMI entity handle
 * @minfo: Pointer to Tx/Rx Message management info based on channel type
 *
 * Helper function which is used by various message functions that are
 * exposed to clients of this driver for allocating a message traffic event.
 *
 * This function can sleep depending on pending requests already in the system
 * for the SCMI entity.
 *
 * Return: 0 if all went fine, else corresponding error.
 */
static struct scmi_xfer *scmi_xfer_get(const struct scmi_handle *handle,
				       struct scmi_xfers_info *minfo)
{
	u16 xfer_id;
	struct scmi_xfer *xfer;
	unsigned long bit_pos;
	struct scmi_info *info = handle_to_scmi_info(handle);

	bit_pos = find_first_zero_bit(minfo->xfer_alloc_table,
				      info->desc->max_msg);
	if (bit_pos == info->desc->max_msg)
		return ERR_PTR(-ENOMEM);
	set_bit(bit_pos, minfo->xfer_alloc_table);

	xfer_id = bit_pos;

	xfer = &minfo->xfer_block[xfer_id];
	xfer->hdr.seq = xfer_id;
	xfer->done = false;
	xfer->transfer_id = ++transfer_last_id;

	return xfer;
}

/**
 * __scmi_xfer_put() - Release a message
 *
 * @minfo: Pointer to Tx/Rx Message management info based on channel type
 * @xfer: message that was reserved by scmi_xfer_get
 */
static void
__scmi_xfer_put(struct scmi_xfers_info *minfo, struct scmi_xfer *xfer)
{
	clear_bit(xfer->hdr.seq, minfo->xfer_alloc_table);
}

static void scmi_handle_response(struct scmi_chan_info *cinfo,
				 u16 xfer_id, u8 msg_type)
{
	struct scmi_xfer *xfer;
	struct device *dev = cinfo->dev;
	struct scmi_info *info = handle_to_scmi_info(cinfo->handle);
	struct scmi_xfers_info *minfo = &info->tx_minfo;

	/* Are we even expecting this? */
	if (!test_bit(xfer_id, minfo->xfer_alloc_table)) {
		dev_err(dev, "message for %d is not expected!\n", xfer_id);
		info->desc->ops->clear_channel(cinfo);
		return;
	}

	xfer = &minfo->xfer_block[xfer_id];
	/*
	 * Even if a response was indeed expected on this slot at this point,
	 * a buggy platform could wrongly reply feeding us an unexpected
	 * delayed response we're not prepared to handle: bail-out safely
	 * blaming firmware.
	 */
	if (unlikely(msg_type == MSG_TYPE_DELAYED_RESP && !xfer->async_done)) {
		dev_err(dev,
			"Delayed Response for %d not expected! Buggy F/W ?\n",
			xfer_id);
		info->desc->ops->clear_channel(cinfo);
		/* It was unexpected, so nobody will clear the xfer if not us */
		__scmi_xfer_put(minfo, xfer);
		return;
	}

	scmi_dump_header_dbg(dev, &xfer->hdr);

	info->desc->ops->fetch_response(cinfo, xfer);

	if (msg_type == MSG_TYPE_DELAYED_RESP) {
		info->desc->ops->clear_channel(cinfo);
		*xfer->async_done = true;
	} else {
		xfer->done = true;
	}
}

/**
 * scmi_rx_callback() - callback for receiving messages
 *
 * @cinfo: SCMI channel info
 * @msg_hdr: Message header
 *
 * Processes one received message to appropriate transfer information and
 * signals completion of the transfer.
 *
 * NOTE: This function will be invoked in IRQ context, hence should be
 * as optimal as possible.
 */
void scmi_rx_callback(struct scmi_chan_info *cinfo, u32 msg_hdr)
{
	u16 xfer_id = MSG_XTRACT_TOKEN(msg_hdr);
	u8 msg_type = MSG_XTRACT_TYPE(msg_hdr);

	switch (msg_type) {
	case MSG_TYPE_COMMAND:
	case MSG_TYPE_DELAYED_RESP:
		scmi_handle_response(cinfo, xfer_id, msg_type);
		break;
	default:
		WARN_ONCE(1, "received unknown msg_type:%d\n", msg_type);
		break;
	}
}

/**
 * xfer_put() - Release a transmit message
 *
 * @ph: Pointer to SCMI protocol handle
 * @xfer: message that was reserved by scmi_xfer_get
 */
static void xfer_put(const struct scmi_protocol_handle *ph,
		     struct scmi_xfer *xfer)
{
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	struct scmi_info *info = handle_to_scmi_info(pi->handle);

	__scmi_xfer_put(&info->tx_minfo, xfer);
}

/**
 * do_xfer() - Do one transfer
 *
 * @ph: Pointer to SCMI protocol handle
 * @xfer: Transfer to initiate and wait for response
 *
 * Return: -ETIMEDOUT in case of no response, if transmit error,
 *	return corresponding error, else if all goes well,
 *	return 0.
 */
static int do_xfer(const struct scmi_protocol_handle *ph,
		   struct scmi_xfer *xfer)
{
	int ret;
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	struct scmi_info *info = handle_to_scmi_info(pi->handle);
	struct device *dev = info->dev;
	struct scmi_chan_info *cinfo;
	u64 start;

	/*
	 * Re-instate protocol id here from protocol handle so that cannot be
	 * overridden by mistake (or malice) by the protocol code mangling with
	 * the scmi_xfer structure.
	 */
	xfer->hdr.protocol_id = pi->proto->id;

	cinfo = idr_find(&info->tx_idr, xfer->hdr.protocol_id);
	if (unlikely(!cinfo))
		return -EINVAL;

	ret = info->desc->ops->send_message(cinfo, xfer);
	if (ret < 0) {
		dev_dbg(dev, "Failed to send message %d\n", ret);
		return ret;
	}

	/* And we wait for the response. */
	start = get_time_ns();
	while (!xfer->done) {
		if (is_timeout(start, info->desc->max_rx_timeout_ms * (u64)NSEC_PER_MSEC)) {
			dev_err(dev, "timed out in resp(caller: %pS)\n", (void *)_RET_IP_);
			ret = -ETIMEDOUT;
			break;
		}
	}

	if (!ret && xfer->hdr.status)
		ret = scmi_to_linux_errno(xfer->hdr.status);

	if (info->desc->ops->mark_txdone)
		info->desc->ops->mark_txdone(cinfo, ret);

	return ret;
}

static void reset_rx_to_maxsz(const struct scmi_protocol_handle *ph,
			      struct scmi_xfer *xfer)
{
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	struct scmi_info *info = handle_to_scmi_info(pi->handle);

	xfer->rx.len = info->desc->max_msg_size;
}

#define SCMI_MAX_RESPONSE_TIMEOUT_NS	(2 * NSEC_PER_SEC)

/**
 * do_xfer_with_response() - Do one transfer and wait until the delayed
 *	response is received
 *
 * @ph: Pointer to SCMI protocol handle
 * @xfer: Transfer to initiate and wait for response
 *
 * Return: -ETIMEDOUT in case of no delayed response, if transmit error,
 *	return corresponding error, else if all goes well, return 0.
 */
static int do_xfer_with_response(const struct scmi_protocol_handle *ph,
				 struct scmi_xfer *xfer)
{
	int ret;
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	bool async_response = false;
	u64 start;

	xfer->hdr.protocol_id = pi->proto->id;

	xfer->async_done = &async_response;

	ret = do_xfer(ph, xfer);
	if (ret)
		goto out;

	start = get_time_ns();
	while (!*xfer->async_done) {
		if (is_timeout(start, SCMI_MAX_RESPONSE_TIMEOUT_NS)) {
			ret = -ETIMEDOUT;
			break;
		}
	}

out:
	xfer->async_done = NULL;
	return ret;
}

/**
 * xfer_get_init() - Allocate and initialise one message for transmit
 *
 * @ph: Pointer to SCMI protocol handle
 * @msg_id: Message identifier
 * @tx_size: transmit message size
 * @rx_size: receive message size
 * @p: pointer to the allocated and initialised message
 *
 * This function allocates the message using @scmi_xfer_get and
 * initialise the header.
 *
 * Return: 0 if all went fine with @p pointing to message, else
 *	corresponding error.
 */
static int xfer_get_init(const struct scmi_protocol_handle *ph,
			 u8 msg_id, size_t tx_size, size_t rx_size,
			 struct scmi_xfer **p)
{
	int ret;
	struct scmi_xfer *xfer;
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	struct scmi_info *info = handle_to_scmi_info(pi->handle);
	struct scmi_xfers_info *minfo = &info->tx_minfo;
	struct device *dev = info->dev;

	/* Ensure we have sane transfer sizes */
	if (rx_size > info->desc->max_msg_size ||
	    tx_size > info->desc->max_msg_size)
		return -ERANGE;

	xfer = scmi_xfer_get(pi->handle, minfo);
	if (IS_ERR(xfer)) {
		ret = PTR_ERR(xfer);
		dev_err(dev, "failed to get free message slot(%d)\n", ret);
		return ret;
	}

	xfer->tx.len = tx_size;
	xfer->rx.len = rx_size ? : info->desc->max_msg_size;
	xfer->hdr.id = msg_id;
	xfer->hdr.protocol_id = pi->proto->id;
	xfer->hdr.poll_completion = false;

	*p = xfer;

	return 0;
}

/**
 * version_get() - command to get the revision of the SCMI entity
 *
 * @ph: Pointer to SCMI protocol handle
 * @version: Holds returned version of protocol.
 *
 * Updates the SCMI information in the internal data structure.
 *
 * Return: 0 if all went fine, else return appropriate error.
 */
static int version_get(const struct scmi_protocol_handle *ph, u32 *version)
{
	int ret;
	__le32 *rev_info;
	struct scmi_xfer *t;

	ret = xfer_get_init(ph, PROTOCOL_VERSION, 0, sizeof(*version), &t);
	if (ret)
		return ret;

	ret = do_xfer(ph, t);
	if (!ret) {
		rev_info = t->rx.buf;
		*version = le32_to_cpu(*rev_info);
	}

	xfer_put(ph, t);
	return ret;
}

/**
 * scmi_set_protocol_priv  - Set protocol specific data at init time
 *
 * @ph: A reference to the protocol handle.
 * @priv: The private data to set.
 *
 * Return: 0 on Success
 */
static int scmi_set_protocol_priv(const struct scmi_protocol_handle *ph,
				  void *priv)
{
	struct scmi_protocol_instance *pi = ph_to_pi(ph);

	pi->priv = priv;

	return 0;
}

/**
 * scmi_get_protocol_priv  - Set protocol specific data at init time
 *
 * @ph: A reference to the protocol handle.
 *
 * Return: Protocol private data if any was set.
 */
static void *scmi_get_protocol_priv(const struct scmi_protocol_handle *ph)
{
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);

	return pi->priv;
}

static const struct scmi_xfer_ops xfer_ops = {
	.version_get = version_get,
	.xfer_get_init = xfer_get_init,
	.reset_rx_to_maxsz = reset_rx_to_maxsz,
	.do_xfer = do_xfer,
	.do_xfer_with_response = do_xfer_with_response,
	.xfer_put = xfer_put,
};

/**
 * scmi_revision_area_get  - Retrieve version memory area.
 *
 * @ph: A reference to the protocol handle.
 *
 * A helper to grab the version memory area reference during SCMI Base protocol
 * initialization.
 *
 * Return: A reference to the version memory area associated to the SCMI
 *	   instance underlying this protocol handle.
 */
struct scmi_revision_info *
scmi_revision_area_get(const struct scmi_protocol_handle *ph)
{
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);

	return pi->handle->version;
}

/**
 * scmi_alloc_init_protocol_instance  - Allocate and initialize a protocol
 * instance descriptor.
 * @info: The reference to the related SCMI instance.
 * @proto: The protocol descriptor.
 *
 * Allocate a new protocol instance descriptor, using the provided @proto
 * description, against the specified SCMI instance @info, and initialize it;
 *
 * Return: A reference to a freshly allocated and initialized protocol instance
 *	   or ERR_PTR on failure. On failure the @proto reference is at first
 */
static struct scmi_protocol_instance *
scmi_alloc_init_protocol_instance(struct scmi_info *info,
				  const struct scmi_protocol *proto)
{
	int ret = -ENOMEM;
	struct scmi_protocol_instance *pi;
	const struct scmi_handle *handle = &info->handle;

	pi = kzalloc(sizeof(*pi), GFP_KERNEL);
	if (!pi)
		goto clean;

	pi->proto = proto;
	pi->handle = handle;
	pi->ph.dev = handle->dev;
	pi->ph.xops = &xfer_ops;
	pi->ph.set_priv = scmi_set_protocol_priv;
	pi->ph.get_priv = scmi_get_protocol_priv;
	pi->users++;
	/* proto->init is assured NON NULL by scmi_protocol_register */
	ret = pi->proto->instance_init(&pi->ph);
	if (ret)
		goto clean;

	ret = idr_alloc_one(&info->protocols, pi, proto->id);
	if (ret != proto->id)
		goto clean;

	dev_dbg(handle->dev, "Initialized protocol: 0x%X\n", pi->proto->id);

	return pi;

clean:
	return ERR_PTR(ret);
}

/**
 * scmi_get_protocol_instance  - Protocol initialization helper.
 * @handle: A reference to the SCMI platform instance.
 * @protocol_id: The protocol being requested.
 *
 * In case the required protocol has never been requested before for this
 * instance, allocate and initialize all the needed structures.
 *
 * Return: A reference to an initialized protocol instance or error on failure:
 *	   in particular returns -EPROBE_DEFER when the desired protocol could
 *	   NOT be found.
 */
static struct scmi_protocol_instance * __must_check
scmi_get_protocol_instance(const struct scmi_handle *handle, u8 protocol_id)
{
	struct scmi_protocol_instance *pi;
	struct scmi_info *info = handle_to_scmi_info(handle);

	pi = idr_find(&info->protocols, protocol_id);

	if (pi) {
		pi->users++;
	} else {
		const struct scmi_protocol *proto;

		/* Fails if protocol not registered on bus */
		proto = scmi_protocol_get(protocol_id);
		if (proto)
			pi = scmi_alloc_init_protocol_instance(info, proto);
		else
			pi = ERR_PTR(-EPROBE_DEFER);
	}

	return pi;
}

/**
 * scmi_protocol_acquire  - Protocol acquire
 * @handle: A reference to the SCMI platform instance.
 * @protocol_id: The protocol being requested.
 *
 * Register a new user for the requested protocol on the specified SCMI
 * platform instance, possibly triggering its initialization on first user.
 *
 * Return: 0 if protocol was acquired successfully.
 */
int scmi_protocol_acquire(const struct scmi_handle *handle, u8 protocol_id)
{
	return PTR_ERR_OR_ZERO(scmi_get_protocol_instance(handle, protocol_id));
}

void scmi_setup_protocol_implemented(const struct scmi_protocol_handle *ph,
				     u8 *prot_imp)
{
	const struct scmi_protocol_instance *pi = ph_to_pi(ph);
	struct scmi_info *info = handle_to_scmi_info(pi->handle);

	info->protocols_imp = prot_imp;
}

static bool
scmi_is_protocol_implemented(const struct scmi_handle *handle, u8 prot_id)
{
	int i;
	struct scmi_info *info = handle_to_scmi_info(handle);

	if (!info->protocols_imp)
		return false;

	for (i = 0; i < MAX_PROTOCOLS_IMP; i++)
		if (info->protocols_imp[i] == prot_id)
			return true;
	return false;
}

/**
 * scmi_dev_protocol_get  - get protocol operations and handle
 * @protocol_id: The protocol being requested.
 * @ph: A pointer reference used to pass back the associated protocol handle.
 *
 * Get hold of a protocol accounting for its usage, eventually triggering its
 * initialization, and returning the protocol specific operations and related
 * protocol handle which will be used as first argument in most of the
 * protocols operations methods.
 * Being a devres based managed method, protocol hold will be automatically
 * released, and possibly de-initialized on last user, once the SCMI driver
 * owning the scmi_device is unbound from it.
 *
 * Return: A reference to the requested protocol operations or error.
 *	   Must be checked for errors by caller.
 */
static const void __must_check *
scmi_dev_protocol_get(struct scmi_device *sdev, u8 protocol_id,
		       struct scmi_protocol_handle **ph)
{
	struct scmi_protocol_instance *pi;
	struct scmi_handle *handle = sdev->handle;

	if (!ph)
		return ERR_PTR(-EINVAL);

	pi = scmi_get_protocol_instance(handle, protocol_id);
	if (IS_ERR(pi))
		return pi;

	*ph = &pi->ph;

	return pi->proto->ops;
}

static inline
struct scmi_handle *scmi_handle_get_from_info_unlocked(struct scmi_info *info)
{
	info->users++;
	return &info->handle;
}

/**
 * scmi_handle_get() - Get the SCMI handle for a device
 *
 * @dev: pointer to device for which we want SCMI handle
 *
 * NOTE: The function does not track individual clients of the framework
 * and is expected to be maintained by caller of SCMI protocol library.
 * scmi_handle_put must be balanced with successful scmi_handle_get
 *
 * Return: pointer to handle if successful, NULL on error
 */
struct scmi_handle *scmi_handle_get(struct device *dev)
{
	struct list_head *p;
	struct scmi_info *info;
	struct scmi_handle *handle = NULL;

	list_for_each(p, &scmi_list) {
		info = list_entry(p, struct scmi_info, node);
		if (dev->parent == info->dev) {
			handle = scmi_handle_get_from_info_unlocked(info);
			break;
		}
	}

	return handle;
}

/**
 * scmi_handle_put() - Release the handle acquired by scmi_handle_get
 *
 * @handle: handle acquired by scmi_handle_get
 *
 * NOTE: The function does not track individual clients of the framework
 * and is expected to be maintained by caller of SCMI protocol library.
 * scmi_handle_put must be balanced with successful scmi_handle_get
 *
 * Return: 0 is successfully released
 *	if null was passed, it returns -EINVAL;
 */
int scmi_handle_put(const struct scmi_handle *handle)
{
	struct scmi_info *info;

	if (!handle)
		return -EINVAL;

	info = handle_to_scmi_info(handle);
	if (!WARN_ON(!info->users))
		info->users--;

	return 0;
}

static int __scmi_xfer_info_init(struct scmi_info *sinfo,
				 struct scmi_xfers_info *info)
{
	int i;
	struct scmi_xfer *xfer;
	struct device *dev = sinfo->dev;
	const struct scmi_desc *desc = sinfo->desc;

	/* Pre-allocated messages, no more than what hdr.seq can support */
	if (WARN_ON(desc->max_msg >= MSG_TOKEN_MAX)) {
		dev_err(dev, "Maximum message of %d exceeds supported %ld\n",
			desc->max_msg, MSG_TOKEN_MAX);
		return -EINVAL;
	}

	info->xfer_block = kcalloc(desc->max_msg, sizeof(*info->xfer_block), GFP_KERNEL);
	if (!info->xfer_block)
		return -ENOMEM;

	info->xfer_alloc_table = kcalloc(BITS_TO_LONGS(desc->max_msg),
					      sizeof(long), GFP_KERNEL);
	if (!info->xfer_alloc_table)
		return -ENOMEM;

	/* Pre-initialize the buffer pointer to pre-allocated buffers */
	for (i = 0, xfer = info->xfer_block; i < desc->max_msg; i++, xfer++) {
		xfer->rx.buf = kcalloc(sizeof(u8), desc->max_msg_size,
					    GFP_KERNEL);
		if (!xfer->rx.buf)
			return -ENOMEM;

		xfer->tx.buf = xfer->rx.buf;
		xfer->done = false;
	}

	return 0;
}

static int scmi_xfer_info_init(struct scmi_info *sinfo)
{
	int ret = __scmi_xfer_info_init(sinfo, &sinfo->tx_minfo);

	if (!ret && idr_find(&sinfo->rx_idr, SCMI_PROTOCOL_BASE))
		ret = __scmi_xfer_info_init(sinfo, &sinfo->rx_minfo);

	return ret;
}

static int scmi_chan_setup(struct scmi_info *info, struct device *dev,
			   int prot_id, bool tx)
{
	int ret, idx;
	struct scmi_chan_info *cinfo;
	struct idr *idr;

	/* Transmit channel is first entry i.e. index 0 */
	idx = tx ? 0 : 1;
	idr = tx ? &info->tx_idr : &info->rx_idr;

	/* check if already allocated, used for multiple device per protocol */
	cinfo = idr_find(idr, prot_id);
	if (cinfo)
		return 0;

	if (!info->desc->ops->chan_available(dev, idx)) {
		cinfo = idr_find(idr, SCMI_PROTOCOL_BASE);
		if (unlikely(!cinfo)) /* Possible only if platform has no Rx */
			return -EINVAL;
		goto idr_alloc;
	}

	cinfo = kzalloc(sizeof(*cinfo), GFP_KERNEL);
	if (!cinfo)
		return -ENOMEM;

	cinfo->dev = dev;

	ret = info->desc->ops->chan_setup(cinfo, info->dev, tx);
	if (ret)
		return ret;

idr_alloc:
	ret = idr_alloc_one(idr, cinfo, prot_id);
	if (ret != prot_id) {
		dev_err(dev, "unable to allocate SCMI idr slot err %d\n", ret);
		return ret;
	}

	cinfo->handle = &info->handle;
	return 0;
}

static inline int
scmi_txrx_setup(struct scmi_info *info, struct device *dev, int prot_id)
{
	int ret = scmi_chan_setup(info, dev, prot_id, true);

	if (!ret) /* Rx is optional, hence no error check */
		scmi_chan_setup(info, dev, prot_id, false);

	return ret;
}

/**
 * scmi_get_protocol_device  - Helper to get/create an SCMI device.
 *
 * @np: A device node representing a valid active protocols for the referred
 * SCMI instance.
 * @info: The referred SCMI instance for which we are getting/creating this
 * device.
 * @prot_id: The protocol ID.
 * @name: The device name.
 *
 * Referring to the specific SCMI instance identified by @info, this helper
 * takes care to return a properly initialized device matching the requested
 * @proto_id and @name: if device was still not existent it is created as a
 * child of the specified SCMI instance @info and its transport properly
 * initialized as usual.
 */
static inline struct scmi_device *
scmi_get_protocol_device(struct device_node *np, struct scmi_info *info,
			 int prot_id, const char *name)
{
	struct scmi_device *sdev;

	/* Already created for this parent SCMI instance ? */
	sdev = scmi_child_dev_find(info->dev, prot_id, name);
	if (sdev)
		return sdev;

	pr_debug("Creating SCMI device (%s) for protocol %x\n", name, prot_id);

	sdev = scmi_device_alloc(np, info->dev, prot_id, name);
	if (!sdev) {
		dev_err(info->dev, "failed to create %d protocol device\n",
			prot_id);
		return NULL;
	}

	if (scmi_txrx_setup(info, &sdev->dev, prot_id)) {
		dev_err(&sdev->dev, "failed to setup transport\n");
		scmi_device_destroy(sdev);
		return NULL;
	}

	return sdev;
}

static inline void
scmi_create_protocol_device(struct device_node *np, struct scmi_info *info,
			    int prot_id, const char *name)
{
	struct scmi_device *sdev;

	sdev = scmi_get_protocol_device(np, info, prot_id, name);
	if (!sdev)
		return;

	/* setup handle now as the transport is ready */
	scmi_set_handle(sdev);

	/* Register if not done yet */
	if (sdev->dev.id == DEVICE_ID_DYNAMIC)
		register_device(&sdev->dev);
}

/**
 * scmi_create_protocol_devices  - Create devices for all pending requests for
 * this SCMI instance.
 *
 * @np: The device node describing the protocol
 * @info: The SCMI instance descriptor
 * @prot_id: The protocol ID
 *
 * All devices previously requested for this instance (if any) are found and
 * created by scanning the proper @&scmi_requested_devices entry.
 */
static void scmi_create_protocol_devices(struct device_node *np,
					 struct scmi_info *info, int prot_id)
{
	struct list_head *phead;

	phead = idr_find(&scmi_requested_devices, prot_id);
	if (phead) {
		struct scmi_requested_dev *rdev;

		list_for_each_entry(rdev, phead, node)
			scmi_create_protocol_device(np, info, prot_id,
						    rdev->id_table->name);
	}
}

/**
 * scmi_protocol_device_request  - Helper to request a device
 *
 * @id_table: A protocol/name pair descriptor for the device to be created.
 *
 * This helper let an SCMI driver request specific devices identified by the
 * @id_table to be created for each active SCMI instance.
 *
 * The requested device name MUST NOT be already existent for any protocol;
 * at first the freshly requested @id_table is annotated in the IDR table
 * @scmi_requested_devices, then a matching device is created for each already
 * active SCMI instance. (if any)
 *
 * This way the requested device is created straight-away for all the already
 * initialized(probed) SCMI instances (handles) and it remains also annotated
 * as pending creation if the requesting SCMI driver was loaded before some
 * SCMI instance and related transports were available: when such late instance
 * is probed, its probe will take care to scan the list of pending requested
 * devices and create those on its own (see @scmi_create_protocol_devices and
 * its enclosing loop)
 *
 * Return: 0 on Success
 */
int scmi_protocol_device_request(const struct scmi_device_id *id_table)
{
	int ret = 0;
	struct list_head *phead = NULL;
	struct scmi_requested_dev *rdev;
	struct scmi_info *info;
	struct idr *idr;

	pr_debug("Requesting SCMI device (%s) for protocol 0x%x\n",
		 id_table->name, id_table->protocol_id);

	/*
	 * Search for the matching protocol rdev list and then search
	 * of any existent equally named device...fails if any duplicate found.
	 */
	__idr_for_each_entry(&scmi_requested_devices, idr) {
		struct list_head *head = idr->ptr;
		if (!phead) {
			/* A list found registered in the IDR is never empty */
			rdev = list_first_entry(head, struct scmi_requested_dev,
						node);
			if (rdev->id_table->protocol_id ==
			    id_table->protocol_id)
				phead = head;
		}
		list_for_each_entry(rdev, head, node) {
			if (!strcmp(rdev->id_table->name, id_table->name)) {
				pr_err("Ignoring duplicate request [%d] %s\n",
				       rdev->id_table->protocol_id,
				       rdev->id_table->name);
				ret = -EINVAL;
				goto out;
			}
		}
	}

	/*
	 * No duplicate found for requested id_table, so let's create a new
	 * requested device entry for this new valid request.
	 */
	rdev = kzalloc(sizeof(*rdev), GFP_KERNEL);
	if (!rdev) {
		ret = -ENOMEM;
		goto out;
	}
	rdev->id_table = id_table;

	/*
	 * Append the new requested device table descriptor to the head of the
	 * related protocol list, eventually creating such head if not already
	 * there.
	 */
	if (!phead) {
		phead = kzalloc(sizeof(*phead), GFP_KERNEL);
		if (!phead) {
			kfree(rdev);
			ret = -ENOMEM;
			goto out;
		}
		INIT_LIST_HEAD(phead);

		ret = idr_alloc_one(&scmi_requested_devices, (void *)phead,
				id_table->protocol_id);
		if (ret != id_table->protocol_id) {
			pr_err("Failed to save SCMI device - ret:%d\n", ret);
			kfree(rdev);
			kfree(phead);
			ret = -EINVAL;
			goto out;
		}
		ret = 0;
	}
	list_add(&rdev->node, phead);

	/*
	 * Now effectively create and initialize the requested device for every
	 * already initialized SCMI instance which has registered the requested
	 * protocol as a valid active one: i.e. defined in DT and supported by
	 * current platform FW.
	 */
	list_for_each_entry(info, &scmi_list, node) {
		struct device_node *child;

		child = idr_find(&info->active_protocols,
				 id_table->protocol_id);
		if (child) {
			struct scmi_device *sdev;

			sdev = scmi_get_protocol_device(child, info,
							id_table->protocol_id,
							id_table->name);
			/* Set handle if not already set: device existed */
			if (sdev && !sdev->handle)
				sdev->handle =
					scmi_handle_get_from_info_unlocked(info);
		} else {
			dev_err(info->dev,
				"Failed. SCMI protocol %d not active.\n",
				id_table->protocol_id);
		}
	}

out:
	return ret;
}

/**
 * scmi_protocol_device_unrequest  - Helper to unrequest a device
 *
 * @id_table: A protocol/name pair descriptor for the device to be unrequested.
 *
 * An helper to let an SCMI driver release its request about devices; note that
 * devices are created and initialized once the first SCMI driver request them
 * but they destroyed only on SCMI core unloading/unbinding.
 *
 * The current SCMI transport layer uses such devices as internal references and
 * as such they could be shared as same transport between multiple drivers so
 * that cannot be safely destroyed till the whole SCMI stack is removed.
 * (unless adding further burden of refcounting.)
 */
void scmi_protocol_device_unrequest(const struct scmi_device_id *id_table)
{
	struct list_head *phead;

	pr_debug("Unrequesting SCMI device (%s) for protocol %x\n",
		 id_table->name, id_table->protocol_id);

	phead = idr_find(&scmi_requested_devices, id_table->protocol_id);
	if (phead) {
		struct scmi_requested_dev *victim, *tmp;

		list_for_each_entry_safe(victim, tmp, phead, node) {
			if (!strcmp(victim->id_table->name, id_table->name)) {
				list_del(&victim->node);
				kfree(victim);
				break;
			}
		}

		if (list_empty(phead)) {
			idr_remove(&scmi_requested_devices,
				   id_table->protocol_id);
			kfree(phead);
		}
	}
}

static void version_info(struct device *dev)
{
	struct scmi_info *info = dev->priv;

	printf("SCMI information:\n"
	       "  version: %u.%u\n"
	       "  firmware version: 0x%x\n"
	       "  vendor: %s (sub: %s)\n",
	       info->version.minor_ver,
	       info->version.major_ver,
	       info->version.impl_ver,
	       info->version.vendor_id,
	       info->version.sub_vendor_id);
}

static int scmi_probe(struct device *dev)
{
	int ret;
	struct scmi_handle *handle;
	const struct scmi_desc *desc;
	struct scmi_info *info;
	struct device_node *child, *np = dev->of_node;

	desc = of_device_get_match_data(dev);
	if (!desc)
		return -EINVAL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	info->desc = desc;
	INIT_LIST_HEAD(&info->node);
	idr_init(&info->protocols);
	idr_init(&info->active_protocols);

	dev->priv = info;
	idr_init(&info->tx_idr);
	idr_init(&info->rx_idr);

	handle = &info->handle;
	handle->dev = info->dev;
	handle->version = &info->version;
	handle->protocol_get = scmi_dev_protocol_get;

	ret = scmi_txrx_setup(info, dev, SCMI_PROTOCOL_BASE);
	if (ret)
		return ret;

	ret = scmi_xfer_info_init(info);
	if (ret)
		return ret;

	/*
	 * Trigger SCMI Base protocol initialization.
	 * It's mandatory and won't be ever released/deinit until the
	 * SCMI stack is shutdown/unloaded as a whole.
	 */
	ret = scmi_protocol_acquire(handle, SCMI_PROTOCOL_BASE);
	if (ret) {
		dev_err(dev, "unable to communicate with SCMI\n");
		return ret;
	}

	list_add_tail(&info->node, &scmi_list);

	for_each_available_child_of_node(np, child) {
		u32 prot_id;

		if (of_property_read_u32(child, "reg", &prot_id))
			continue;

		if (!FIELD_FIT(MSG_PROTOCOL_ID_MASK, prot_id))
			dev_err(dev, "Out of range protocol %d\n", prot_id);

		if (!scmi_is_protocol_implemented(handle, prot_id)) {
			dev_err(dev, "SCMI protocol %d not implemented\n",
				prot_id);
			continue;
		}

		/*
		 * Save this valid DT protocol descriptor amongst
		 * @active_protocols for this SCMI instance/
		 */
		ret = idr_alloc_one(&info->active_protocols, child, prot_id);
		if (ret != prot_id) {
			dev_err(dev, "SCMI protocol %d already activated. Skip\n",
				prot_id);
			continue;
		}

		scmi_create_protocol_devices(child, info, prot_id);
	}

	dev->info = version_info;

	return 0;
}

void scmi_free_channel(struct scmi_chan_info *cinfo, struct idr *idr, int id)
{
	idr_remove(idr, id);
}

/* Each compatible listed below must have descriptor associated with it */
static const struct of_device_id scmi_of_match[] = {
#ifdef CONFIG_ARM_SMCCC
	{ .compatible = "arm,scmi-smc", .data = &scmi_smc_desc},
#endif
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, scmi_of_match);

static struct driver arm_scmi_driver = {
	.name = "arm-scmi",
	.of_compatible = scmi_of_match,
	.probe = scmi_probe,
};
core_platform_driver(arm_scmi_driver);

static int __init scmi_bus_driver_init(void)
{
	scmi_bus_init();

	scmi_base_register();

	scmi_reset_register();
	scmi_clock_register();
	scmi_voltage_register();

	return 0;
}
pure_initcall(scmi_bus_driver_init);

MODULE_ALIAS("platform: arm-scmi");
MODULE_AUTHOR("Sudeep Holla <sudeep.holla@arm.com>");
MODULE_DESCRIPTION("ARM SCMI protocol driver");
MODULE_LICENSE("GPL v2");
