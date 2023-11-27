// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Reset Protocol
 *
 * Copyright (C) 2019-2022 ARM Ltd.
 */

#define pr_fmt(fmt) "SCMI Notifications RESET - " fmt

#include <common.h>
#include <linux/scmi_protocol.h>

#include "protocols.h"

enum scmi_reset_protocol_cmd {
	RESET_DOMAIN_ATTRIBUTES = 0x3,
	RESET = 0x4,
	RESET_DOMAIN_NAME_GET = 0x6,
};

#define NUM_RESET_DOMAIN_MASK	0xffff

struct scmi_msg_resp_reset_domain_attributes {
	__le32 attributes;
#define SUPPORTS_EXTENDED_NAMES(x)	((x) & BIT(29))
	__le32 latency;
	u8 name[SCMI_SHORT_NAME_MAX_SIZE];
};

struct scmi_msg_reset_domain_reset {
	__le32 domain_id;
	__le32 flags;
#define AUTONOMOUS_RESET	BIT(0)
#define EXPLICIT_RESET_ASSERT	BIT(1)
	__le32 reset_state;
#define ARCH_COLD_RESET		0
};

struct reset_dom_info {
	u32 latency_us;
	char name[SCMI_MAX_STR_SIZE];
};

struct scmi_reset_info {
	u32 version;
	int num_domains;
	struct reset_dom_info *dom_info;
};

static int scmi_reset_attributes_get(const struct scmi_protocol_handle *ph,
				     struct scmi_reset_info *pi)
{
	int ret;
	struct scmi_xfer *t;
	u32 attr;

	ret = ph->xops->xfer_get_init(ph, PROTOCOL_ATTRIBUTES,
				      0, sizeof(attr), &t);
	if (ret)
		return ret;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		attr = get_unaligned_le32(t->rx.buf);
		pi->num_domains = attr & NUM_RESET_DOMAIN_MASK;
	}

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int
scmi_reset_domain_attributes_get(const struct scmi_protocol_handle *ph,
				 u32 domain, struct reset_dom_info *dom_info,
				 u32 version)
{
	int ret;
	u32 attributes;
	struct scmi_xfer *t;
	struct scmi_msg_resp_reset_domain_attributes *attr;

	ret = ph->xops->xfer_get_init(ph, RESET_DOMAIN_ATTRIBUTES,
				      sizeof(domain), sizeof(*attr), &t);
	if (ret)
		return ret;

	put_unaligned_le32(domain, t->tx.buf);
	attr = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		attributes = le32_to_cpu(attr->attributes);

		dom_info->latency_us = le32_to_cpu(attr->latency);
		if (dom_info->latency_us == U32_MAX)
			dom_info->latency_us = 0;
		strscpy(dom_info->name, attr->name, SCMI_SHORT_NAME_MAX_SIZE);
	}

	ph->xops->xfer_put(ph, t);

	/*
	 * If supported overwrite short name with the extended one;
	 * on error just carry on and use already provided short name.
	 */
	if (!ret && PROTOCOL_REV_MAJOR(version) >= 0x3 &&
	    SUPPORTS_EXTENDED_NAMES(attributes))
		ph->hops->extended_name_get(ph, RESET_DOMAIN_NAME_GET, domain,
					    dom_info->name, SCMI_MAX_STR_SIZE);

	return ret;
}

static int scmi_reset_num_domains_get(const struct scmi_protocol_handle *ph)
{
	struct scmi_reset_info *pi = ph->get_priv(ph);

	return pi->num_domains;
}

static const char *
scmi_reset_name_get(const struct scmi_protocol_handle *ph, u32 domain)
{
	struct scmi_reset_info *pi = ph->get_priv(ph);

	struct reset_dom_info *dom = pi->dom_info + domain;

	return dom->name;
}

static int scmi_reset_latency_get(const struct scmi_protocol_handle *ph,
				  u32 domain)
{
	struct scmi_reset_info *pi = ph->get_priv(ph);
	struct reset_dom_info *dom = pi->dom_info + domain;

	return dom->latency_us;
}

static int scmi_domain_reset(const struct scmi_protocol_handle *ph, u32 domain,
			     u32 flags, u32 state)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_msg_reset_domain_reset *dom;
	struct scmi_reset_info *pi = ph->get_priv(ph);
	struct reset_dom_info *rdom;

	if (domain >= pi->num_domains)
		return -EINVAL;

	rdom = pi->dom_info + domain;

	ret = ph->xops->xfer_get_init(ph, RESET, sizeof(*dom), 0, &t);
	if (ret)
		return ret;

	dom = t->tx.buf;
	dom->domain_id = cpu_to_le32(domain);
	dom->flags = cpu_to_le32(flags);
	dom->reset_state = cpu_to_le32(state);

	ret = ph->xops->do_xfer(ph, t);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_reset_domain_reset(const struct scmi_protocol_handle *ph,
				   u32 domain)
{
	return scmi_domain_reset(ph, domain, AUTONOMOUS_RESET,
				 ARCH_COLD_RESET);
}

static int
scmi_reset_domain_assert(const struct scmi_protocol_handle *ph, u32 domain)
{
	return scmi_domain_reset(ph, domain, EXPLICIT_RESET_ASSERT,
				 ARCH_COLD_RESET);
}

static int
scmi_reset_domain_deassert(const struct scmi_protocol_handle *ph, u32 domain)
{
	return scmi_domain_reset(ph, domain, 0, ARCH_COLD_RESET);
}

static const struct scmi_reset_proto_ops reset_proto_ops = {
	.num_domains_get = scmi_reset_num_domains_get,
	.name_get = scmi_reset_name_get,
	.latency_get = scmi_reset_latency_get,
	.reset = scmi_reset_domain_reset,
	.assert = scmi_reset_domain_assert,
	.deassert = scmi_reset_domain_deassert,
};

static int scmi_reset_protocol_init(const struct scmi_protocol_handle *ph)
{
	int domain, ret;
	u32 version;
	struct scmi_reset_info *pinfo;

	ret = ph->xops->version_get(ph, &version);
	if (ret)
		return ret;

	dev_dbg(ph->dev, "Reset Version %d.%d\n",
		PROTOCOL_REV_MAJOR(version), PROTOCOL_REV_MINOR(version));

	pinfo = devm_kzalloc(ph->dev, sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo)
		return -ENOMEM;

	ret = scmi_reset_attributes_get(ph, pinfo);
	if (ret)
		return ret;

	pinfo->dom_info = devm_kcalloc(ph->dev, pinfo->num_domains,
				       sizeof(*pinfo->dom_info), GFP_KERNEL);
	if (!pinfo->dom_info)
		return -ENOMEM;

	for (domain = 0; domain < pinfo->num_domains; domain++) {
		struct reset_dom_info *dom = pinfo->dom_info + domain;

		scmi_reset_domain_attributes_get(ph, domain, dom, version);
	}

	pinfo->version = version;
	return ph->set_priv(ph, pinfo);
}

static const struct scmi_protocol scmi_reset = {
	.id = SCMI_PROTOCOL_RESET,
	.instance_init = &scmi_reset_protocol_init,
	.ops = &reset_proto_ops,
};

DEFINE_SCMI_PROTOCOL_REGISTER(reset, scmi_reset)
