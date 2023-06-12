// SPDX-License-Identifier: GPL-2.0-only
/*
 * Zynq UltraScale+ MPSoC Clock Controller
 *
 * Copyright (C) 2019 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 *
 * Based on the Linux driver in drivers/clk/zynqmp/
 *
 * Copyright (C) 2016-2018 Xilinx
 */

#include <common.h>
#include <init.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <mach/zynqmp/firmware-zynqmp.h>

#include "clk-zynqmp.h"

#define MAX_PARENT			100
#define MAX_NODES			6
#define MAX_NAME_LEN			50

#define CLK_TYPE_FIELD_MASK		GENMASK(3, 0)
#define CLK_FLAG_FIELD_MASK		GENMASK(21, 8)
#define CLK_TYPE_FLAG_FIELD_MASK	GENMASK(31, 24)

#define CLK_PARENTS_ID_MASK		GENMASK(15, 0)
#define CLK_PARENTS_TYPE_MASK		GENMASK(31, 16)

#define CLK_GET_ATTR_VALID		BIT(0)
#define CLK_GET_ATTR_TYPE		BIT(2)

#define CLK_NAME_RESERVED		""
#define CLK_NAME_DUMMY			"dummy_name"

#define CLK_GET_NAME_RESP_LEN		16
#define CLK_GET_TOPOLOGY_RESP_WORDS	3
#define CLK_GET_PARENTS_RESP_WORDS	3

#define CLK_GET_PARENTS_NONE		(-1)
#define CLK_GET_PARENTS_DUMMY		(-2)

enum clk_type {
	CLK_TYPE_INVALID,
	CLK_TYPE_OUTPUT,
	CLK_TYPE_EXTERNAL,
};

enum parent_type {
	PARENT_CLK_SELF,
	PARENT_CLK_NODE1,
	PARENT_CLK_NODE2,
	PARENT_CLK_NODE3,
	PARENT_CLK_NODE4,
	PARENT_CLK_EXTERNAL,
};

struct clock_parent {
	char name[MAX_NAME_LEN];
	enum parent_type type;
};

struct zynqmp_clock_info {
	char clk_name[MAX_NAME_LEN];
	bool valid;
	enum clk_type type;
	struct clock_topology node[MAX_NODES];
	unsigned int num_nodes;
	struct clock_parent parent[MAX_PARENT];
	unsigned int num_parents;
};

static const char clk_type_postfix[][10] = {
	[TYPE_INVALID] = "",
	[TYPE_MUX] = "_mux",
	[TYPE_GATE] = "",
	[TYPE_DIV1] = "_div1",
	[TYPE_DIV2] = "_div2",
	[TYPE_FIXEDFACTOR] = "_ff",
	[TYPE_PLL] = ""
};

static struct clk *(*const clk_topology[]) (const char *name,
				            unsigned int clk_id,
					    const char **parents,
					    unsigned int num_parents,
					    const struct clock_topology *nodes)
					= {
	[TYPE_INVALID] = NULL,
	[TYPE_MUX] = zynqmp_clk_register_mux,
	[TYPE_PLL] = zynqmp_clk_register_pll,
	[TYPE_FIXEDFACTOR] = zynqmp_clk_register_fixed_factor,
	[TYPE_DIV1] = zynqmp_clk_register_divider,
	[TYPE_DIV2] = zynqmp_clk_register_divider,
	[TYPE_GATE] = zynqmp_clk_register_gate
};

static struct zynqmp_clock_info *clock_info;
static const struct zynqmp_eemi_ops *eemi_ops;

static inline bool zynqmp_is_valid_clock(unsigned int id)
{
	return clock_info[id].valid;
}

static char *zynqmp_get_clock_name(unsigned int id)
{
	if (!zynqmp_is_valid_clock(id))
		return ERR_PTR(-EINVAL);

	return clock_info[id].clk_name;
}

static enum clk_type zynqmp_get_clock_type(unsigned int id)
{
	if (!zynqmp_is_valid_clock(id))
		return CLK_TYPE_INVALID;

	return clock_info[id].type;
}

static int zynqmp_get_parent_names(struct device_node *np,
				   unsigned int clk_id,
				   const char **parent_names)
{
	int i;
	struct clock_topology *clk_nodes;
	struct clock_parent *parents;
	int ret;

	clk_nodes = clock_info[clk_id].node;
	parents = clock_info[clk_id].parent;

	for (i = 0; i < clock_info[clk_id].num_parents; i++) {
		switch (parents[i].type) {
		case PARENT_CLK_SELF:
			break;
		case PARENT_CLK_EXTERNAL:
			ret = of_property_match_string(np, "clock-names",
						       parents[i].name);
			if (ret < 0)
				strcpy(parents[i].name, CLK_NAME_DUMMY);
			break;
		default:
			strcat(parents[i].name,
			       clk_type_postfix[clk_nodes[parents[i].type - 1].type]);
			break;
		}
		parent_names[i] = parents[i].name;
	}

	return clock_info[clk_id].num_parents;
}

static int zynqmp_pm_clock_query_num_clocks(void)
{
	struct zynqmp_pm_query_data qdata = {0};
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	qdata.qid = PM_QID_CLOCK_GET_NUM_CLOCKS;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	return ret_payload[1];
}

static int zynqmp_pm_clock_query_name(unsigned int id, char *response)
{
	struct zynqmp_pm_query_data qdata = {0};
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	qdata.qid = PM_QID_CLOCK_GET_NAME;
	qdata.arg1 = id;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	memcpy(response, ret_payload, CLK_GET_NAME_RESP_LEN);

	return 0;
}

struct zynqmp_pm_query_fixed_factor {
	unsigned int mul;
	unsigned int div;
};

static int zynqmp_pm_clock_query_fixed_factor(unsigned int id,
		struct zynqmp_pm_query_fixed_factor *response)
{
	struct zynqmp_pm_query_data qdata = {0};
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	qdata.qid = PM_QID_CLOCK_GET_FIXEDFACTOR_PARAMS;
	qdata.arg1 = id;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	response->mul = ret_payload[1];
	response->div = ret_payload[2];

	return 0;
}

struct zynqmp_pm_query_topology {
	unsigned int node[CLK_GET_TOPOLOGY_RESP_WORDS];
};

static int zynqmp_pm_clock_query_topology(unsigned int id, unsigned int index,
		struct zynqmp_pm_query_topology *response)
{
	struct zynqmp_pm_query_data qdata;
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	memset(&qdata, 0, sizeof(qdata));
	qdata.qid = PM_QID_CLOCK_GET_TOPOLOGY;
	qdata.arg1 = id;
	qdata.arg2 = index;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	memcpy(response, &ret_payload[1], sizeof(*response));

	return 0;
}

struct zynqmp_pm_query_parents {
	unsigned int parent[CLK_GET_PARENTS_RESP_WORDS];
};

static int zynqmp_pm_clock_query_parents(unsigned int id, unsigned int index,
		struct zynqmp_pm_query_parents *response)
{
	struct zynqmp_pm_query_data qdata;
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	memset(&qdata, 0, sizeof(qdata));
	qdata.qid = PM_QID_CLOCK_GET_PARENTS;
	qdata.arg1 = id;
	qdata.arg2 = index;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	memcpy(response, &ret_payload[1], sizeof(*response));

	return 0;
}

static int zynqmp_pm_clock_query_attributes(unsigned int id, unsigned int *attr)
{
	struct zynqmp_pm_query_data qdata;
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	memset(&qdata, 0, sizeof(qdata));
	qdata.qid = PM_QID_CLOCK_GET_ATTRIBUTES;
	qdata.arg1 = id;

	ret = eemi_ops->query_data(qdata, ret_payload);
	if (ret)
		return ret;

	*attr = ret_payload[1];

	return 0;
}

static int zynqmp_clock_parse_topology(struct clock_topology *topology,
				       struct zynqmp_pm_query_topology *response,
				       size_t max_nodes)
{
	int i;
	enum topology_type type;

	for (i = 0; i < max_nodes && i < ARRAY_SIZE(response->node); i++) {
		type = FIELD_GET(CLK_TYPE_FIELD_MASK, response->node[i]);
		if (type == TYPE_INVALID)
			break;

		topology[i].type = type;
		topology[i].flag =
			FIELD_GET(CLK_FLAG_FIELD_MASK, response->node[i]);
		topology[i].type_flag =
			FIELD_GET(CLK_TYPE_FLAG_FIELD_MASK, response->node[i]);
	}

	return i;
}

static int zynqmp_clock_parse_parents(struct clock_parent *parents,
				      struct zynqmp_pm_query_parents *response,
				      size_t max_parents)
{
	int i;
	struct clock_parent *parent;
	char *parent_name;
	int parent_id;

	for (i = 0; i < max_parents && i < ARRAY_SIZE(response->parent); i++) {
		if (response->parent[i] == CLK_GET_PARENTS_NONE)
			break;

		parent = &parents[i];
		if (response->parent[i] == CLK_GET_PARENTS_DUMMY) {
			parent->type = PARENT_CLK_SELF;
			strncpy(parent->name, CLK_NAME_DUMMY, MAX_NAME_LEN);
			continue;
		}

		parent_id = FIELD_GET(CLK_PARENTS_ID_MASK, response->parent[i]);
		parent_name = zynqmp_get_clock_name(parent_id);
		if (IS_ERR(parent_name))
			continue;

		strncpy(parent->name, parent_name, MAX_NAME_LEN);
		parent->type = FIELD_GET(CLK_PARENTS_TYPE_MASK, response->parent[i]);
	}

	return i;
}

static int zynqmp_clock_get_topology(unsigned int id,
				     struct clock_topology *topology,
				     size_t max_nodes)
{
	int ret;
	int i;
	struct zynqmp_pm_query_topology response;

	for (i = 0; i < max_nodes; i += ret) {
		ret = zynqmp_pm_clock_query_topology(id, i, &response);
		if (ret)
			return ret;

		ret = zynqmp_clock_parse_topology(&topology[i], &response,
						  max_nodes - i);
		if (ret == 0)
			break;
	}

	return i;
}

static int zynqmp_clock_get_parents(unsigned int clk_id,
				    struct clock_parent *parents,
				    size_t max_parents)
{
	int ret;
	int i;
	struct zynqmp_pm_query_parents response;

	for (i = 0; i < max_parents; i += ret) {
		ret = zynqmp_pm_clock_query_parents(clk_id, i, &response);
		if (ret)
			return ret;

		ret = zynqmp_clock_parse_parents(&parents[i], &response,
						 max_parents - i);
		if (ret == 0)
			break;
	}

	return i;
}

struct clk *zynqmp_clk_register_fixed_factor(const char *name,
					     unsigned int clk_id,
					     const char **parents,
					     unsigned int num_parents,
					     const struct clock_topology *topology)
{
	unsigned int err;
	struct zynqmp_pm_query_fixed_factor response = {0};
	unsigned flags;

	err = zynqmp_pm_clock_query_fixed_factor(clk_id, &response);
	if (err)
		return ERR_PTR(err);

	flags = topology->flag;

	return clk_fixed_factor(strdup(name), strdup(parents[0]),
			        response.mul, response.div, flags);
}

static struct clk *zynqmp_register_clk_topology(char *clk_name,
					        int clk_id,
						int num_parents,
						const char **parent_names)
{
	int i;
	unsigned int num_nodes;
	char tmp_name[MAX_NAME_LEN];
	char parent_name[MAX_NAME_LEN];
	struct clock_topology *nodes;
	struct clk *clk = NULL;

	nodes = clock_info[clk_id].node;
	num_nodes = clock_info[clk_id].num_nodes;

	for (i = 0; i < num_nodes; i++) {
		if (!clk_topology[nodes[i].type])
			continue;

		/*
		 * Register only the last sub-node in the chain with the name of the
		 * original clock, but postfix other sub-node inside the chain with
		 * their type.
		 */
		snprintf(tmp_name, MAX_NAME_LEN, "%s%s", clk_name,
			(i == num_nodes - 1) ? "" : clk_type_postfix[nodes[i].type]);

		clk = (*clk_topology[nodes[i].type])(tmp_name, clk_id,
						     parent_names,
						     num_parents,
						     &nodes[i]);
		if (IS_ERR(clk))
			pr_warn("failed to register node %s of clock %s\n",
				tmp_name, clk_name);

		/*
		 * Only link the first sub-node to the original (first)
		 * parent, but link every other sub-node with their preceeding
		 * sub-clock via the first parent.
		 */
		parent_names[0] = parent_name;
		strncpy(parent_name, tmp_name, MAX_NAME_LEN);
	}

	return clk;
}

static int zynqmp_register_clocks(struct device *dev,
				  struct clk **clks, size_t num_clocks)
{
	unsigned int i;
	const char *parent_names[MAX_PARENT];
	char *name;
	struct device_node *node = dev->of_node;
	int num_parents;

	for (i = 0; i < num_clocks; i++) {
		if (zynqmp_get_clock_type(i) != CLK_TYPE_OUTPUT)
			continue;
		name = zynqmp_get_clock_name(i);
		if (IS_ERR(name))
			continue;

		num_parents = zynqmp_get_parent_names(node, i, parent_names);
		if (num_parents < 0) {
			dev_warn(dev, "failed to find parents for %s\n", name);
			continue;
		}

		clks[i] = zynqmp_register_clk_topology(name, i, num_parents,
						       parent_names);
		if (IS_ERR_OR_NULL(clks[i]))
			dev_warn(dev, "failed to register clock %s: %ld\n",
				 name, PTR_ERR(clks[i]));
	}

	return 0;
}

static void zynqmp_fill_clock_info(struct zynqmp_clock_info *clock_info,
				   size_t num_clocks)
{
	int i;
	int ret;
	unsigned int attr;

	for (i = 0; i < num_clocks; i++) {
		zynqmp_pm_clock_query_name(i, clock_info[i].clk_name);
		if (!strcmp(clock_info[i].clk_name, CLK_NAME_RESERVED))
			continue;

		ret = zynqmp_pm_clock_query_attributes(i, &attr);
		if (ret)
			continue;

		clock_info[i].valid = attr & CLK_GET_ATTR_VALID;
		clock_info[i].type = attr & CLK_GET_ATTR_TYPE ?
			CLK_TYPE_EXTERNAL : CLK_TYPE_OUTPUT;
	}

	for (i = 0; i < num_clocks; i++) {
		if (zynqmp_get_clock_type(i) != CLK_TYPE_OUTPUT)
			continue;
		clock_info[i].num_nodes =
			zynqmp_clock_get_topology(i, clock_info[i].node,
						  ARRAY_SIZE(clock_info[i].node));
	}

	for (i = 0; i < num_clocks; i++) {
		if (zynqmp_get_clock_type(i) != CLK_TYPE_OUTPUT)
			continue;
		ret = zynqmp_clock_get_parents(i, clock_info[i].parent,
					       ARRAY_SIZE(clock_info[i].parent));
		if (ret < 0)
			continue;
		clock_info[i].num_parents = ret;
	}
}

static int zynqmp_clock_probe(struct device *dev)
{
	int err;
	u32 api_version;
	int num_clocks;
	struct clk_onecell_data *clk_data;

	eemi_ops = zynqmp_pm_get_eemi_ops();
	if (!eemi_ops)
		return -ENODEV;

	/* Check version to make sure firmware is available */
	err = eemi_ops->get_api_version(&api_version);
	if (err) {
		dev_err(dev, "Firmware not available\n");
		return err;
	}

	num_clocks = zynqmp_pm_clock_query_num_clocks();
	if (num_clocks < 1)
		return num_clocks;

	clock_info = kcalloc(num_clocks, sizeof(*clock_info), GFP_KERNEL);
	if (!clock_info)
		return -ENOMEM;

	zynqmp_fill_clock_info(clock_info, num_clocks);

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return -ENOMEM;

	clk_data->clks = kcalloc(num_clocks, sizeof(*clk_data->clks), GFP_KERNEL);
	if (!clk_data->clks) {
		kfree(clk_data);
		return -ENOMEM;
	}

	zynqmp_register_clocks(dev, clk_data->clks, num_clocks);
	clk_data->clk_num = num_clocks;
	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, clk_data);

	/*
	 * We can free clock_info now, as is only used to store clock info
	 * from firmware for registering the clocks.
	 */
	kfree(clock_info);

	return 0;
}

static struct of_device_id zynqmp_clock_of_match[] = {
	{.compatible = "xlnx,zynqmp-clk"},
	{},
};
MODULE_DEVICE_TABLE(of, zynqmp_clock_of_match);

static struct driver zynqmp_clock_driver = {
	.probe	= zynqmp_clock_probe,
	.name	= "zynqmp_clock",
	.of_compatible = DRV_OF_COMPAT(zynqmp_clock_of_match),
};
postcore_platform_driver(zynqmp_clock_driver);
