// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Power Protocol
 *
 * Copyright (C) 2018-2022 ARM Ltd.
 */

#define pr_fmt(fmt) "SCMI Notifications POWER - " fmt

#include <module.h>
#include <linux/scmi_protocol.h>

#include "protocols.h"

enum scmi_power_protocol_cmd {
	POWER_DOMAIN_ATTRIBUTES = 0x3,
	POWER_STATE_SET = 0x4,
	POWER_STATE_GET = 0x5,
	POWER_DOMAIN_NAME_GET = 0x8,
};

struct scmi_msg_resp_power_attributes {
	__le16 num_domains;
	__le16 reserved;
	__le32 stats_addr_low;
	__le32 stats_addr_high;
	__le32 stats_size;
};

struct scmi_msg_resp_power_domain_attributes {
	__le32 flags;
#define SUPPORTS_EXTENDED_NAMES(x)	((x) & BIT(27))
	    u8 name[SCMI_SHORT_NAME_MAX_SIZE];
};

struct scmi_power_set_state {
	__le32 flags;
	__le32 domain;
	__le32 state;
};

struct power_dom_info {
	char name[SCMI_MAX_STR_SIZE];
};

struct scmi_power_info {
	u32 version;
	int num_domains;
	u64 stats_addr;
	u32 stats_size;
	struct power_dom_info *dom_info;
};

static int scmi_power_attributes_get(const struct scmi_protocol_handle *ph,
				     struct scmi_power_info *pi)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_msg_resp_power_attributes *attr;

	ret = ph->xops->xfer_get_init(ph, PROTOCOL_ATTRIBUTES,
				      0, sizeof(*attr), &t);
	if (ret)
		return ret;

	attr = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		pi->num_domains = le16_to_cpu(attr->num_domains);
		pi->stats_addr = le32_to_cpu(attr->stats_addr_low) |
				(u64)le32_to_cpu(attr->stats_addr_high) << 32;
		pi->stats_size = le32_to_cpu(attr->stats_size);
	}

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int
scmi_power_domain_attributes_get(const struct scmi_protocol_handle *ph,
				 u32 domain, struct power_dom_info *dom_info,
				 u32 version)
{
	int ret;
	u32 flags;
	struct scmi_xfer *t;
	struct scmi_msg_resp_power_domain_attributes *attr;

	ret = ph->xops->xfer_get_init(ph, POWER_DOMAIN_ATTRIBUTES,
				      sizeof(domain), sizeof(*attr), &t);
	if (ret)
		return ret;

	put_unaligned_le32(domain, t->tx.buf);
	attr = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		flags = le32_to_cpu(attr->flags);

		strscpy(dom_info->name, attr->name, SCMI_SHORT_NAME_MAX_SIZE);
	}
	ph->xops->xfer_put(ph, t);

	/*
	 * If supported overwrite short name with the extended one;
	 * on error just carry on and use already provided short name.
	 */
	if (!ret && PROTOCOL_REV_MAJOR(version) >= 0x3 &&
	    SUPPORTS_EXTENDED_NAMES(flags)) {
		ph->hops->extended_name_get(ph, POWER_DOMAIN_NAME_GET,
					    domain, dom_info->name,
					    SCMI_MAX_STR_SIZE);
	}

	return ret;
}

static int scmi_power_state_set(const struct scmi_protocol_handle *ph,
				u32 domain, u32 state)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_power_set_state *st;

	ret = ph->xops->xfer_get_init(ph, POWER_STATE_SET, sizeof(*st), 0, &t);
	if (ret)
		return ret;

	st = t->tx.buf;
	st->flags = cpu_to_le32(0);
	st->domain = cpu_to_le32(domain);
	st->state = cpu_to_le32(state);

	ret = ph->xops->do_xfer(ph, t);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_power_state_get(const struct scmi_protocol_handle *ph,
				u32 domain, u32 *state)
{
	int ret;
	struct scmi_xfer *t;

	ret = ph->xops->xfer_get_init(ph, POWER_STATE_GET, sizeof(u32), sizeof(u32), &t);
	if (ret)
		return ret;

	put_unaligned_le32(domain, t->tx.buf);

	ret = ph->xops->do_xfer(ph, t);
	if (!ret)
		*state = get_unaligned_le32(t->rx.buf);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_power_num_domains_get(const struct scmi_protocol_handle *ph)
{
	struct scmi_power_info *pi = ph->get_priv(ph);

	return pi->num_domains;
}

static const char *
scmi_power_name_get(const struct scmi_protocol_handle *ph,
		    u32 domain)
{
	struct scmi_power_info *pi = ph->get_priv(ph);
	struct power_dom_info *dom = pi->dom_info + domain;

	return dom->name;
}

static const struct scmi_power_proto_ops power_proto_ops = {
	.num_domains_get = scmi_power_num_domains_get,
	.name_get = scmi_power_name_get,
	.state_set = scmi_power_state_set,
	.state_get = scmi_power_state_get,
};

static int scmi_power_protocol_init(const struct scmi_protocol_handle *ph)
{
	int domain, ret;
	u32 version;
	struct scmi_power_info *pinfo;

	ret = ph->xops->version_get(ph, &version);
	if (ret)
		return ret;

	dev_dbg(ph->dev, "Power Version %d.%d\n",
		PROTOCOL_REV_MAJOR(version), PROTOCOL_REV_MINOR(version));

	pinfo = devm_kzalloc(ph->dev, sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo)
		return -ENOMEM;

	ret = scmi_power_attributes_get(ph, pinfo);
	if (ret)
		return ret;

	pinfo->dom_info = devm_kcalloc(ph->dev, pinfo->num_domains,
				       sizeof(*pinfo->dom_info), GFP_KERNEL);
	if (!pinfo->dom_info)
		return -ENOMEM;

	for (domain = 0; domain < pinfo->num_domains; domain++) {
		struct power_dom_info *dom = pinfo->dom_info + domain;

		scmi_power_domain_attributes_get(ph, domain, dom, version);
	}

	pinfo->version = version;

	return ph->set_priv(ph, pinfo);
}

static const struct scmi_protocol scmi_power = {
	.id = SCMI_PROTOCOL_POWER,
	.instance_init = &scmi_power_protocol_init,
	.ops = &power_proto_ops,
};

DEFINE_SCMI_PROTOCOL_REGISTER(power, scmi_power)
