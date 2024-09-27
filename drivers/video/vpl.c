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

int vpl_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *ptr)
{
	struct device_node *node, *endpoint;
	int ret;

	pr_debug("%s: %pOF port %d\n", __func__, vpl->node, port);

	node = of_graph_get_port_by_id(vpl->node, port);
	if (!node) {
		pr_err("%s: no port %d on %pOF\n", __func__, port, vpl->node);
		return -ENODEV;
	}

	for_each_child_of_node(node, endpoint) {
		struct device_node *remote, *remote_parent;
		struct vpl *remote_vpl;
		u32 remote_port_id;

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
		ret = remote_vpl->ioctl(remote_vpl, remote_port_id, cmd, ptr);
		if (ret)
			return ret;
	}

	return 0;
}
