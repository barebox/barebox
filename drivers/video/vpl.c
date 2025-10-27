// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Video pipeline (VPL) support for barebox
 *
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 */
#define pr_fmt(fmt) "VPL: " fmt

#include <common.h>
#include <driver.h>
#include <of_graph.h>
#include <linux/list.h>
#include <video/vpl.h>

static LIST_HEAD(vpls);

int vpl_register(struct vpl *vpl)
{
	list_add_tail(&vpl->list, &vpls);

	pr_debug("%s: %pOF\n", __func__, vpl->node);

	return 0;
}

static struct vpl *of_find_vpl(struct device_node *node)
{
	struct vpl *vpl;

	of_device_ensure_probed(node);

	list_for_each_entry(vpl, &vpls, list)
		if (vpl->node == node)
			return vpl;

	return NULL;
}

struct vpl *of_vpl_get(struct device_node *node, int port)
{
	node = of_graph_get_port_by_id(node, port);
	if (!node)
		return NULL;

	pr_debug("%s: port: %pOF\n", __func__, node);

	node = of_graph_get_remote_port_parent(node);
	if (!node)
		return NULL;

	pr_debug("%s: remote port parent: %pOF\n", __func__, node);

	return of_find_vpl(node);
}

static int vpl_foreach_endpoint(struct vpl *vpl, unsigned int port, int endpoint_id,
				int (*fn)(struct vpl *, unsigned port, void *data),
				void *data)
{
	struct device_node *node, *endpoint;
	int ret;

	pr_debug("%s: %pOF port %d endpoint %d\n", __func__, vpl->node,
		 port, endpoint_id);

	node = of_graph_get_port_by_id(vpl->node, port);
	if (!node) {
		pr_err("%s: no port %d on %pOF\n", __func__, port, vpl->node);
		return -ENODEV;
	}

	for_each_child_of_node(node, endpoint) {
		struct device_node *remote, *remote_parent;
		struct vpl *remote_vpl;
		u32 remote_port_id = 0;

		if (endpoint_id >= 0) {
			u32 local_endpoint_id = 0;
			of_property_read_u32(endpoint, "reg", &local_endpoint_id);
			if (local_endpoint_id != endpoint_id) {
				pr_debug("%s: skipping endpoint %pOF with id %d\n",
					 __func__, endpoint, local_endpoint_id);
				continue;
			}
		}

		remote = of_graph_get_remote_port(endpoint);
		if (!remote) {
			pr_debug("%s: no remote for endpoint %pOF\n", __func__, endpoint);
			continue;
		}

		of_property_read_u32(remote, "reg", &remote_port_id);

		remote_parent = of_graph_get_remote_port_parent(endpoint);
		if (!remote_parent) {
			pr_debug("%s: no remote_parent\n", __func__);
			return -ENODEV;
		}

		if (!of_device_is_available(remote_parent))
			continue;

		remote_vpl = of_find_vpl(remote_parent);
		if (!remote_vpl) {
			pr_debug("%s: cannot find remote vpl %pOF\n", __func__, remote);
			continue;
		}

		pr_debug("%s: looked up %pOF: %pS\n", __func__, remote, remote_vpl->ioctl);
		ret = fn(remote_vpl, remote_port_id, data);
		if (ret)
			return ret;
	}

	return 0;
}

static const char *ioctl_cmd_str(unsigned cmd)
{
	switch (cmd) {
	case VPL_PREPARE:
		return "PREPARE";
	case VPL_UNPREPARE:
		return "UNPREPARE";
	case VPL_ENABLE:
		return "ENABLE";
	case VPL_DISABLE:
		return "DISABLE";
	case VPL_GET_VIDEOMODES:
		return "GET_VIDEOMODES";
	case VPL_GET_BUS_FORMAT:
		return "GET_BUS_FORMAT";
	case VPL_GET_DISPLAY_INFO:
		return "GET_DISPLAY_INFO";
	default:
		return NULL;
	}
}

static int vpl_do_ioctl(struct vpl *vpl, unsigned port, unsigned cmd,
			 void *data)
{
	char buf[sizeof("0x12345678")];
	const char *cmdstr;

	cmdstr = ioctl_cmd_str(cmd);
	if (!cmdstr) {
		snprintf(buf, sizeof(buf), "0x%08x", cmd);
		cmdstr = buf;
	}

	pr_debug("%s: calling %ps(\"%pOF\", %d, %s, %p)\n", __func__,
		 vpl->ioctl, vpl->node, port, cmdstr, data);

	return vpl->ioctl(vpl, port, cmd, data);
}

struct vpl_ioctl {
	int err;
	unsigned cmd;
	void *ptr;
};

static int vpl_remote_ioctl(struct vpl *vpl, unsigned port, void *_data)
{
	struct vpl_ioctl *data = _data;

	if (!vpl->ioctl) {
		data->err = -EOPNOTSUPP;
		return 0;
	}

	return vpl_do_ioctl(vpl, port, data->cmd, data->ptr);
}

int vpl_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *ptr)
{
	struct vpl_ioctl data = { .cmd = cmd, .ptr = ptr };

	return vpl_foreach_endpoint(vpl, port, -1, vpl_remote_ioctl, &data) ?: data.err;
}

static int vpl_alloc_bridge(struct vpl *vpl, unsigned port, void *_bridge)
{
	struct vpl_bridge **bridge = _bridge;

	(*bridge) = malloc(sizeof(**bridge));
	(*bridge)->vpl = vpl;
	(*bridge)->port = port;

	return 1;
}

struct vpl_bridge *devm_vpl_of_get_bridge(struct device *dev, struct device_node *np,
					  unsigned int port, unsigned int endpoint)
{
	struct vpl_bridge *bridge = NULL;
	struct vpl *vpl;
	int ret;

	vpl = of_find_vpl(np);
	if (!vpl)
		return ERR_PTR(-EPROBE_DEFER);

	ret = vpl_foreach_endpoint(vpl, port, endpoint, vpl_alloc_bridge, &bridge);
	if (ret < 0)
		return ERR_PTR(ret);

	return bridge ?: ERR_PTR(-EINVAL);
}

int vpl_bridge_ioctl(struct vpl_bridge *bridge, unsigned int cmd, void *ptr)
{
	struct vpl *vpl;

	if (IS_ERR_OR_NULL(bridge))
		return PTR_ERR_OR_ZERO(bridge);

	vpl = bridge->vpl;
	if (!vpl->ioctl)
		return -EOPNOTSUPP;

	return vpl_do_ioctl(vpl, bridge->port, cmd, ptr);
}
