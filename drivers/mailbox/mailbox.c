// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 */

#define LOG_CATEGORY UCLASS_MAILBOX

#include <common.h>
#include <mailbox.h>
#include <deep-probe.h>

int mbox_send(struct mbox_chan *chan, const void *data)
{
	const struct mbox_chan_ops *ops = chan->mbox->ops;

	return ops->send(chan, data);
}

int mbox_recv(struct mbox_chan *chan, void *data, unsigned long timeout_us)
{
	const struct mbox_chan_ops *ops = chan->mbox->ops;
	u64 start;
	int ret;

	start = get_time_ns();

	for (;;) {
		ret = ops->recv(chan, data);
		if (ret != -ENODATA)
			return ret;
		if (is_timeout(start, timeout_us * 1000))
			return -ETIMEDOUT;
	}
}

static LIST_HEAD(mbox_cons);

int mbox_controller_register(struct mbox_controller *mbox)
{
	list_add_tail(&mbox->node, &mbox_cons);

	return 0;
}


struct mbox_chan *mbox_request_channel(struct device *dev, int index)
{
	struct of_phandle_args spec;
	struct mbox_controller *mbox;
	struct mbox_chan *chan = ERR_PTR(-ENODEV);

	if (of_parse_phandle_with_args(dev->of_node, "mboxes",
					"#mbox-cells", index, &spec)) {
		return ERR_PTR(-ENODEV);
	}

	of_device_ensure_probed(spec.np);

	list_for_each_entry(mbox, &mbox_cons, node) {
		if (mbox->dev->of_node != spec.np)
			continue;

		chan = mbox->of_xlate(mbox, &spec);
		if (!IS_ERR(chan))
			break;
	}

	return chan;
}

struct mbox_chan *mbox_request_channel_byname(struct device *dev, const char *name)
{
	struct device_node *np = dev->of_node;
	struct property *prop;
	const char *mbox_name;
	int index = 0;

	if (!np)
		return ERR_PTR(-EINVAL);

	if (!of_get_property(np, "mbox-names", NULL)) {
		return ERR_PTR(-EINVAL);
	}

	of_property_for_each_string(np, "mbox-names", prop, mbox_name) {
		if (!strncmp(name, mbox_name, strlen(name)))
			return mbox_request_channel(dev, index);
		index++;
	}

	return ERR_PTR(-ENODEV);
}
