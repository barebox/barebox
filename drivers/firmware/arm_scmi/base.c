// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Base Protocol
 *
 * Copyright (C) 2018-2021 ARM Ltd.
 */

#define pr_fmt(fmt) "SCMI Notifications BASE - " fmt

#include <common.h>
#include <linux/scmi_protocol.h>

#include "common.h"

#define SCMI_BASE_NUM_SOURCES		1
#define SCMI_BASE_MAX_CMD_ERR_COUNT	1024

enum scmi_base_protocol_cmd {
	BASE_DISCOVER_VENDOR = 0x3,
	BASE_DISCOVER_SUB_VENDOR = 0x4,
	BASE_DISCOVER_IMPLEMENT_VERSION = 0x5,
	BASE_DISCOVER_LIST_PROTOCOLS = 0x6,
	BASE_DISCOVER_AGENT = 0x7,
	BASE_NOTIFY_ERRORS = 0x8,
	BASE_SET_DEVICE_PERMISSIONS = 0x9,
	BASE_SET_PROTOCOL_PERMISSIONS = 0xa,
	BASE_RESET_AGENT_CONFIGURATION = 0xb,
};

struct scmi_msg_resp_base_attributes {
	u8 num_protocols;
	u8 num_agents;
	__le16 reserved;
};

struct scmi_msg_resp_base_discover_agent {
	__le32 agent_id;
	u8 name[SCMI_SHORT_NAME_MAX_SIZE];
};


/**
 * scmi_base_attributes_get() - gets the implementation details
 *	that are associated with the base protocol.
 *
 * @ph: SCMI protocol handle
 *
 * Return: 0 on success, else appropriate SCMI error.
 */
static int scmi_base_attributes_get(const struct scmi_protocol_handle *ph)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_msg_resp_base_attributes *attr_info;
	struct scmi_revision_info *rev = ph->get_priv(ph);

	ret = ph->xops->xfer_get_init(ph, PROTOCOL_ATTRIBUTES,
				      0, sizeof(*attr_info), &t);
	if (ret)
		return ret;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		attr_info = t->rx.buf;
		rev->num_protocols = attr_info->num_protocols;
		rev->num_agents = attr_info->num_agents;
	}

	ph->xops->xfer_put(ph, t);

	return ret;
}

/**
 * scmi_base_vendor_id_get() - gets vendor/subvendor identifier ASCII string.
 *
 * @ph: SCMI protocol handle
 * @sub_vendor: specify true if sub-vendor ID is needed
 *
 * Return: 0 on success, else appropriate SCMI error.
 */
static int
scmi_base_vendor_id_get(const struct scmi_protocol_handle *ph, bool sub_vendor)
{
	u8 cmd;
	int ret, size;
	char *vendor_id;
	struct scmi_xfer *t;
	struct scmi_revision_info *rev = ph->get_priv(ph);


	if (sub_vendor) {
		cmd = BASE_DISCOVER_SUB_VENDOR;
		vendor_id = rev->sub_vendor_id;
		size = ARRAY_SIZE(rev->sub_vendor_id);
	} else {
		cmd = BASE_DISCOVER_VENDOR;
		vendor_id = rev->vendor_id;
		size = ARRAY_SIZE(rev->vendor_id);
	}

	ret = ph->xops->xfer_get_init(ph, cmd, 0, size, &t);
	if (ret)
		return ret;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret)
		strscpy(vendor_id, t->rx.buf, size);

	ph->xops->xfer_put(ph, t);

	return ret;
}

/**
 * scmi_base_implementation_version_get() - gets a vendor-specific
 *	implementation 32-bit version. The format of the version number is
 *	vendor-specific
 *
 * @ph: SCMI protocol handle
 *
 * Return: 0 on success, else appropriate SCMI error.
 */
static int
scmi_base_implementation_version_get(const struct scmi_protocol_handle *ph)
{
	int ret;
	__le32 *impl_ver;
	struct scmi_xfer *t;
	struct scmi_revision_info *rev = ph->get_priv(ph);

	ret = ph->xops->xfer_get_init(ph, BASE_DISCOVER_IMPLEMENT_VERSION,
				      0, sizeof(*impl_ver), &t);
	if (ret)
		return ret;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		impl_ver = t->rx.buf;
		rev->impl_ver = le32_to_cpu(*impl_ver);
	}

	ph->xops->xfer_put(ph, t);

	return ret;
}

/**
 * scmi_base_implementation_list_get() - gets the list of protocols it is
 *	OSPM is allowed to access
 *
 * @ph: SCMI protocol handle
 * @protocols_imp: pointer to hold the list of protocol identifiers
 *
 * Return: 0 on success, else appropriate SCMI error.
 */
static int
scmi_base_implementation_list_get(const struct scmi_protocol_handle *ph,
				  u8 *protocols_imp)
{
	u8 *list;
	int ret, loop;
	struct scmi_xfer *t;
	__le32 *num_skip, *num_ret;
	u32 tot_num_ret = 0, loop_num_ret;
	struct device *dev = ph->dev;
	struct scmi_revision_info *rev = ph->get_priv(ph);

	ret = ph->xops->xfer_get_init(ph, BASE_DISCOVER_LIST_PROTOCOLS,
				      sizeof(*num_skip), 0, &t);
	if (ret)
		return ret;

	num_skip = t->tx.buf;
	num_ret = t->rx.buf;
	list = t->rx.buf + sizeof(*num_ret);

	do {
		size_t real_list_sz;
		u32 calc_list_sz;

		/* Set the number of protocols to be skipped/already read */
		*num_skip = cpu_to_le32(tot_num_ret);

		ret = ph->xops->do_xfer(ph, t);
		if (ret)
			break;

		loop_num_ret = le32_to_cpu(*num_ret);
		if (!loop_num_ret)
			break;

		if (loop_num_ret > rev->num_protocols - tot_num_ret) {
			dev_err(dev,
				"No. Returned protocols > Total protocols.\n");
			break;
		}

		if (t->rx.len < (sizeof(u32) * 2)) {
			dev_err(dev, "Truncated reply - rx.len:%zd\n",
				t->rx.len);
			ret = -EPROTO;
			break;
		}

		real_list_sz = t->rx.len - sizeof(u32);
		calc_list_sz = (1 + (loop_num_ret - 1) / sizeof(u32)) *
				sizeof(u32);
		if (calc_list_sz != real_list_sz) {
			dev_warn(dev,
				 "Malformed reply - real_sz:%zd  calc_sz:%u  (loop_num_ret:%d)\n",
				 real_list_sz, calc_list_sz, loop_num_ret);
			/*
			 * Bail out if the expected list size is bigger than the
			 * total payload size of the received reply.
			 */
			if (calc_list_sz > real_list_sz) {
				ret = -EPROTO;
				break;
			}
		}

		for (loop = 0; loop < loop_num_ret; loop++)
			protocols_imp[tot_num_ret + loop] = *(list + loop);

		tot_num_ret += loop_num_ret;

		ph->xops->reset_rx_to_maxsz(ph, t);
	} while (tot_num_ret < rev->num_protocols);

	ph->xops->xfer_put(ph, t);

	return ret;
}

/**
 * scmi_base_discover_agent_get() - discover the name of an agent
 *
 * @ph: SCMI protocol handle
 * @id: Agent identifier
 * @name: Agent identifier ASCII string
 *
 * An agent id of 0 is reserved to identify the platform itself.
 * Generally operating system is represented as "OSPM"
 *
 * Return: 0 on success, else appropriate SCMI error.
 */
static int scmi_base_discover_agent_get(const struct scmi_protocol_handle *ph,
					int id, char *name)
{
	int ret;
	struct scmi_msg_resp_base_discover_agent *agent_info;
	struct scmi_xfer *t;

	ret = ph->xops->xfer_get_init(ph, BASE_DISCOVER_AGENT,
				      sizeof(__le32), sizeof(*agent_info), &t);
	if (ret)
		return ret;

	put_unaligned_le32(id, t->tx.buf);

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		agent_info = t->rx.buf;
		strscpy(name, agent_info->name, SCMI_SHORT_NAME_MAX_SIZE);
	}

	ph->xops->xfer_put(ph, t);

	return ret;
}

static int scmi_base_protocol_init(const struct scmi_protocol_handle *ph)
{
	int id, ret;
	u8 *prot_imp;
	u32 version;
	char name[SCMI_SHORT_NAME_MAX_SIZE];
	struct device *dev = ph->dev;
	struct scmi_revision_info *rev = scmi_revision_area_get(ph);

	ret = ph->xops->version_get(ph, &version);
	if (ret)
		return ret;

	rev->major_ver = PROTOCOL_REV_MAJOR(version),
	rev->minor_ver = PROTOCOL_REV_MINOR(version);
	ph->set_priv(ph, rev);

	ret = scmi_base_attributes_get(ph);
	if (ret)
		return ret;

	prot_imp = devm_kcalloc(dev, rev->num_protocols, sizeof(u8),
				GFP_KERNEL);
	if (!prot_imp)
		return -ENOMEM;

	scmi_base_vendor_id_get(ph, false);
	scmi_base_vendor_id_get(ph, true);
	scmi_base_implementation_version_get(ph);
	scmi_base_implementation_list_get(ph, prot_imp);

	scmi_setup_protocol_implemented(ph, prot_imp);

	dev_info(dev, "SCMI Protocol v%d.%d '%s:%s' Firmware version 0x%x\n",
		 rev->major_ver, rev->minor_ver, rev->vendor_id,
		 rev->sub_vendor_id, rev->impl_ver);
	dev_dbg(dev, "Found %d protocol(s) %d agent(s)\n", rev->num_protocols,
		rev->num_agents);

	for (id = 0; id < rev->num_agents; id++) {
		scmi_base_discover_agent_get(ph, id, name);
		dev_dbg(dev, "Agent %d: %s\n", id, name);
	}

	return 0;
}

static const struct scmi_protocol scmi_base = {
	.id = SCMI_PROTOCOL_BASE,
	.instance_init = &scmi_base_protocol_init,
	.ops = NULL,
};

DEFINE_SCMI_PROTOCOL_REGISTER(base, scmi_base)
