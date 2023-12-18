// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Clock Protocol
 *
 * Copyright (C) 2018-2022 ARM Ltd.
 */

#include <common.h>
#include <qsort.h>

#include "protocols.h"

enum scmi_clock_protocol_cmd {
	CLOCK_ATTRIBUTES = 0x3,
	CLOCK_DESCRIBE_RATES = 0x4,
	CLOCK_RATE_SET = 0x5,
	CLOCK_RATE_GET = 0x6,
	CLOCK_CONFIG_SET = 0x7,
	CLOCK_NAME_GET = 0x8,
};

struct scmi_msg_resp_clock_protocol_attributes {
	__le16 num_clocks;
	u8 max_async_req;
	u8 reserved;
};

struct scmi_msg_resp_clock_attributes {
	__le32 attributes;
#define	CLOCK_ENABLE	BIT(0)
#define SUPPORTS_EXTENDED_NAMES(x)		((x) & BIT(29))
	u8 name[SCMI_SHORT_NAME_MAX_SIZE];
	__le32 clock_enable_latency;
};

struct scmi_clock_set_config {
	__le32 id;
	__le32 attributes;
};

struct scmi_msg_clock_describe_rates {
	__le32 id;
	__le32 rate_index;
};

struct scmi_msg_resp_clock_describe_rates {
	__le32 num_rates_flags;
#define NUM_RETURNED(x)		((x) & 0xfff)
#define RATE_DISCRETE(x)	!((x) & BIT(12))
#define NUM_REMAINING(x)	((x) >> 16)
	struct {
		__le32 value_low;
		__le32 value_high;
	} rate[];
#define RATE_TO_U64(X)		\
({				\
	typeof(X) x = (X);	\
	le32_to_cpu((x).value_low) | (u64)le32_to_cpu((x).value_high) << 32; \
})
};

struct scmi_clock_set_rate {
	__le32 flags;
#define CLOCK_SET_ASYNC		BIT(0)
#define CLOCK_SET_IGNORE_RESP	BIT(1)
#define CLOCK_SET_ROUND_UP	BIT(2)
#define CLOCK_SET_ROUND_AUTO	BIT(3)
	__le32 id;
	__le32 value_low;
	__le32 value_high;
};

struct scmi_msg_resp_set_rate_complete {
	__le32 id;
	__le32 rate_low;
	__le32 rate_high;
};

struct clock_info {
	u32 version;
	int num_clocks;
	struct scmi_clock_info *clk;
};

static int
scmi_clock_protocol_attributes_get(const struct scmi_protocol_handle *ph,
				   struct clock_info *ci)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_msg_resp_clock_protocol_attributes *attr;

	ret = ph->xops->xfer_get_init(ph, PROTOCOL_ATTRIBUTES,
				      0, sizeof(*attr), &t);
	if (ret)
		return ret;

	attr = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret)
		ci->num_clocks = le16_to_cpu(attr->num_clocks);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_clock_attributes_get(const struct scmi_protocol_handle *ph,
				     u32 clk_id, struct scmi_clock_info *clk,
				     u32 version)
{
	int ret;
	u32 attributes;
	struct scmi_xfer *t;
	struct scmi_msg_resp_clock_attributes *attr;

	ret = ph->xops->xfer_get_init(ph, CLOCK_ATTRIBUTES,
				      sizeof(clk_id), sizeof(*attr), &t);
	if (ret)
		return ret;

	put_unaligned_le32(clk_id, t->tx.buf);
	attr = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		u32 latency = 0;
		attributes = le32_to_cpu(attr->attributes);
		strscpy(clk->name, attr->name, SCMI_SHORT_NAME_MAX_SIZE);
		/* clock_enable_latency field is present only since SCMI v3.1 */
		if (PROTOCOL_REV_MAJOR(version) >= 0x2)
			latency = le32_to_cpu(attr->clock_enable_latency);
		clk->enable_latency = latency ? : U32_MAX;
	}

	ph->xops->xfer_put(ph, t);

	/*
	 * If supported overwrite short name with the extended one;
	 * on error just carry on and use already provided short name.
	 */
	if (!ret && PROTOCOL_REV_MAJOR(version) >= 0x2) {
		if (SUPPORTS_EXTENDED_NAMES(attributes))
			ph->hops->extended_name_get(ph, CLOCK_NAME_GET, clk_id,
						    clk->name,
						    SCMI_MAX_STR_SIZE);

	}

	return ret;
}

static int rate_cmp_func(const void *_r1, const void *_r2)
{
	const u64 *r1 = _r1, *r2 = _r2;

	if (*r1 < *r2)
		return -1;
	else if (*r1 == *r2)
		return 0;
	else
		return 1;
}

struct scmi_clk_ipriv {
	struct device *dev;
	u32 clk_id;
	struct scmi_clock_info *clk;
};

static void iter_clk_describe_prepare_message(void *message,
					      const unsigned int desc_index,
					      const void *priv)
{
	struct scmi_msg_clock_describe_rates *msg = message;
	const struct scmi_clk_ipriv *p = priv;

	msg->id = cpu_to_le32(p->clk_id);
	/* Set the number of rates to be skipped/already read */
	msg->rate_index = cpu_to_le32(desc_index);
}

static int
iter_clk_describe_update_state(struct scmi_iterator_state *st,
			       const void *response, void *priv)
{
	u32 flags;
	struct scmi_clk_ipriv *p = priv;
	const struct scmi_msg_resp_clock_describe_rates *r = response;

	flags = le32_to_cpu(r->num_rates_flags);
	st->num_remaining = NUM_REMAINING(flags);
	st->num_returned = NUM_RETURNED(flags);
	p->clk->rate_discrete = RATE_DISCRETE(flags);

	/* Warn about out of spec replies ... */
	if (!p->clk->rate_discrete &&
	    (st->num_returned != 3 || st->num_remaining != 0)) {
		dev_warn(p->dev,
			 "Out-of-spec CLOCK_DESCRIBE_RATES reply for %s - returned:%d remaining:%d rx_len:%zd\n",
			 p->clk->name, st->num_returned, st->num_remaining,
			 st->rx_len);

		/*
		 * A known quirk: a triplet is returned but num_returned != 3
		 * Check for a safe payload size and fix.
		 */
		if (st->num_returned != 3 && st->num_remaining == 0 &&
		    st->rx_len == sizeof(*r) + sizeof(__le32) * 2 * 3) {
			st->num_returned = 3;
			st->num_remaining = 0;
		} else {
			dev_err(p->dev,
				"Cannot fix out-of-spec reply !\n");
			return -EPROTO;
		}
	}

	return 0;
}

static int
iter_clk_describe_process_response(const struct scmi_protocol_handle *ph,
				   const void *response,
				   struct scmi_iterator_state *st, void *priv)
{
	int ret = 0;
	struct scmi_clk_ipriv *p = priv;
	const struct scmi_msg_resp_clock_describe_rates *r = response;

	if (!p->clk->rate_discrete) {
		switch (st->desc_index + st->loop_idx) {
		case 0:
			p->clk->range.min_rate = RATE_TO_U64(r->rate[0]);
			break;
		case 1:
			p->clk->range.max_rate = RATE_TO_U64(r->rate[1]);
			break;
		case 2:
			p->clk->range.step_size = RATE_TO_U64(r->rate[2]);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	} else {
		u64 *rate = &p->clk->list.rates[st->desc_index + st->loop_idx];

		*rate = RATE_TO_U64(r->rate[st->loop_idx]);
		p->clk->list.num_rates++;
	}

	return ret;
}

static int
scmi_clock_describe_rates_get(const struct scmi_protocol_handle *ph, u32 clk_id,
			      struct scmi_clock_info *clk)
{
	int ret;
	void *iter;
	struct scmi_iterator_ops ops = {
		.prepare_message = iter_clk_describe_prepare_message,
		.update_state = iter_clk_describe_update_state,
		.process_response = iter_clk_describe_process_response,
	};
	struct scmi_clk_ipriv cpriv = {
		.clk_id = clk_id,
		.clk = clk,
		.dev = ph->dev,
	};

	iter = ph->hops->iter_response_init(ph, &ops, SCMI_MAX_NUM_RATES,
					    CLOCK_DESCRIBE_RATES,
					    sizeof(struct scmi_msg_clock_describe_rates),
					    &cpriv);
	if (IS_ERR(iter))
		return PTR_ERR(iter);

	ret = ph->hops->iter_response_run(iter);
	if (ret)
		return ret;

	if (!clk->rate_discrete) {
		dev_dbg(ph->dev, "Min %llu Max %llu Step %llu Hz\n",
			clk->range.min_rate, clk->range.max_rate,
			clk->range.step_size);
	} else if (clk->list.num_rates) {
		qsort(clk->list.rates, clk->list.num_rates,
		     sizeof(clk->list.rates[0]), rate_cmp_func);
	}

	return ret;
}

static int
scmi_clock_rate_get(const struct scmi_protocol_handle *ph,
		    u32 clk_id, u64 *value)
{
	int ret;
	struct scmi_xfer *t;

	ret = ph->xops->xfer_get_init(ph, CLOCK_RATE_GET,
				      sizeof(__le32), sizeof(u64), &t);
	if (ret)
		return ret;

	put_unaligned_le32(clk_id, t->tx.buf);

	ret = ph->xops->do_xfer(ph, t);
	if (!ret)
		*value = get_unaligned_le64(t->rx.buf);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_clock_rate_set(const struct scmi_protocol_handle *ph,
			       u32 clk_id, u64 rate)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_clock_set_rate *cfg;

	ret = ph->xops->xfer_get_init(ph, CLOCK_RATE_SET, sizeof(*cfg), 0, &t);
	if (ret)
		return ret;

	cfg = t->tx.buf;
	cfg->flags = cpu_to_le32(0);
	cfg->id = cpu_to_le32(clk_id);
	cfg->value_low = cpu_to_le32(rate & 0xffffffff);
	cfg->value_high = cpu_to_le32(rate >> 32);

	ret = ph->xops->do_xfer(ph, t);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int
scmi_clock_config_set(const struct scmi_protocol_handle *ph, u32 clk_id,
		      u32 config, bool atomic)
{
	int ret;
	struct scmi_xfer *t;
	struct scmi_clock_set_config *cfg;

	ret = ph->xops->xfer_get_init(ph, CLOCK_CONFIG_SET,
				      sizeof(*cfg), 0, &t);
	if (ret)
		return ret;

	cfg = t->tx.buf;
	cfg->id = cpu_to_le32(clk_id);
	cfg->attributes = cpu_to_le32(config);

	ret = ph->xops->do_xfer(ph, t);

	ph->xops->xfer_put(ph, t);
	return ret;
}

static int scmi_clock_enable(const struct scmi_protocol_handle *ph, u32 clk_id)
{
	return scmi_clock_config_set(ph, clk_id, CLOCK_ENABLE, false);
}

static int scmi_clock_disable(const struct scmi_protocol_handle *ph, u32 clk_id)
{
	return scmi_clock_config_set(ph, clk_id, 0, false);
}

static int scmi_clock_enable_atomic(const struct scmi_protocol_handle *ph,
				    u32 clk_id)
{
	return scmi_clock_config_set(ph, clk_id, CLOCK_ENABLE, true);
}

static int scmi_clock_disable_atomic(const struct scmi_protocol_handle *ph,
				     u32 clk_id)
{
	return scmi_clock_config_set(ph, clk_id, 0, true);
}

static int scmi_clock_count_get(const struct scmi_protocol_handle *ph)
{
	struct clock_info *ci = ph->get_priv(ph);

	return ci->num_clocks;
}

static const struct scmi_clock_info *
scmi_clock_info_get(const struct scmi_protocol_handle *ph, u32 clk_id)
{
	struct scmi_clock_info *clk;
	struct clock_info *ci = ph->get_priv(ph);

	if (clk_id >= ci->num_clocks)
		return NULL;

	clk = ci->clk + clk_id;
	if (!clk->name[0])
		return NULL;

	return clk;
}

static const struct scmi_clk_proto_ops clk_proto_ops = {
	.count_get = scmi_clock_count_get,
	.info_get = scmi_clock_info_get,
	.rate_get = scmi_clock_rate_get,
	.rate_set = scmi_clock_rate_set,
	.enable = scmi_clock_enable,
	.disable = scmi_clock_disable,
	.enable_atomic = scmi_clock_enable_atomic,
	.disable_atomic = scmi_clock_disable_atomic,
};

static int scmi_clock_protocol_init(const struct scmi_protocol_handle *ph)
{
	u32 version;
	int clkid, ret;
	struct clock_info *cinfo;

	ret = ph->xops->version_get(ph, &version);
	if (ret)
		return ret;

	dev_dbg(ph->dev, "Clock Version %d.%d\n",
		PROTOCOL_REV_MAJOR(version), PROTOCOL_REV_MINOR(version));

	cinfo = devm_kzalloc(ph->dev, sizeof(*cinfo), GFP_KERNEL);
	if (!cinfo)
		return -ENOMEM;

	ret = scmi_clock_protocol_attributes_get(ph, cinfo);
	if (ret)
		return ret;

	cinfo->clk = devm_kcalloc(ph->dev, cinfo->num_clocks,
				  sizeof(*cinfo->clk), GFP_KERNEL);
	if (!cinfo->clk)
		return -ENOMEM;

	for (clkid = 0; clkid < cinfo->num_clocks; clkid++) {
		struct scmi_clock_info *clk = cinfo->clk + clkid;

		ret = scmi_clock_attributes_get(ph, clkid, clk, version);
		if (!ret)
			scmi_clock_describe_rates_get(ph, clkid, clk);
	}

	cinfo->version = version;
	return ph->set_priv(ph, cinfo);
}

static const struct scmi_protocol scmi_clock = {
	.id = SCMI_PROTOCOL_CLOCK,
	.instance_init = &scmi_clock_protocol_init,
	.ops = &clk_proto_ops,
};

DEFINE_SCMI_PROTOCOL_REGISTER(clock, scmi_clock)
