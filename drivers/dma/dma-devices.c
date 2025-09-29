// SPDX-License-Identifier: GPL-2.0+
/*
 * Direct Memory Access U-Class driver
 *
 * Copyright (C) 2018 Álvaro Fernández Rojas <noltari@gmail.com>
 * Copyright (C) 2015 - 2018 Texas Instruments Incorporated <www.ti.com>
 * Written by Mugunthan V N <mugunthanvnm@ti.com>
 *
 * Author: Mugunthan V N <mugunthanvnm@ti.com>
 */

#include <driver.h>
#include <dma.h>
#include <dma-devices.h>

static LIST_HEAD(dma_devices);

static int dma_of_xlate_default(struct dma *dma,
				struct of_phandle_args *args)
{
	dev_dbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (args->args_count > 1) {
		pr_err("Invalid args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	if (args->args_count)
		dma->id = args->args[0];
	else
		dma->id = 0;

	return 0;
}

static int dma_request(struct device *dev, struct dma *dma)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_dbg(dma->dev, "%s(dev=%p, dma=%p)\n", __func__, dev, dma);

	if (!ops->request)
		return 0;

	return ops->request(dma);
}

static struct dma_device *dma_find_by_node(struct device_node *np)
{
	struct dma_device *dmad;

	list_for_each_entry(dmad, &dma_devices, list) {
		if (dmad->dev->of_node == np)
			return dmad;
	}

	return NULL;
}

struct dma *dma_get_by_index(struct device *dev, int index)
{
	int ret;
	struct of_phandle_args args;
	const struct dma_device_ops *ops;
	struct dma *dma;
	struct dma_device *dmad;

	dev_dbg(dev, "%s index=%d)\n", __func__, index);

	ret = of_parse_phandle_with_args(dev->of_node, "dmas", "#dma-cells", index,
					 &args);
	if (ret) {
		dev_err(dev, "%s: dev_read_phandle_with_args failed: %pe\n",
		       __func__, ERR_PTR(ret));
		return ERR_PTR(ret);
	}

	dmad = dma_find_by_node(args.np);
	if (!dmad)
		return ERR_PTR(-ENODEV);

	dma = xzalloc(sizeof(*dma));

	dma->dmad = dmad;
	dma->dev = dmad->dev;

	ops = dmad->ops;

	if (ops->of_xlate)
		ret = ops->of_xlate(dma, &args);
	else
		ret = dma_of_xlate_default(dma, &args);
	if (ret) {
		dev_err(dma->dev, "of_xlate() failed: %pe\n", ERR_PTR(ret));
		return ERR_PTR(ret);
	}

	ret = dma_request(dmad->dev, dma);
	if (ret)
		return ERR_PTR(ret);

	return dma;
}

struct dma *dma_get_by_name(struct device *dev, const char *name)
{
	int index;

	dev_dbg(dev, "%s(dev=%p, name=%s)\n", __func__, dev, name);

	index = of_property_match_string(dev->of_node, "dma-names", name);
	if (index < 0) {
		dev_err(dev, "dev_read_stringlist_search() failed: %pe\n",
			ERR_PTR(index));
		return ERR_PTR(index);
	}

	return dma_get_by_index(dev, index);
}

int dma_release(struct dma *dma)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_dbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->rfree)
		return 0;

	return ops->rfree(dma);
}

int dma_enable(struct dma *dma)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_dbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->enable)
		return -ENOSYS;

	return ops->enable(dma);
}

int dma_disable(struct dma *dma)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_dbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->disable)
		return -ENOSYS;

	return ops->disable(dma);
}

int dma_prepare_rcv_buf(struct dma *dma, dma_addr_t dst, size_t size)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_vdbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->prepare_rcv_buf)
		return -1;

	return ops->prepare_rcv_buf(dma, dst, size);
}

int dma_receive(struct dma *dma, dma_addr_t *dst, void *metadata)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_vdbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->receive)
		return -ENOSYS;

	return ops->receive(dma, dst, metadata);
}

int dma_send(struct dma *dma, dma_addr_t src, size_t len, void *metadata)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_vdbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->send)
		return -ENOSYS;

	return ops->send(dma, src, len, metadata);
}

int dma_get_cfg(struct dma *dma, u32 cfg_id, void **cfg_data)
{
	const struct dma_device_ops *ops = dma->dmad->ops;

	dev_dbg(dma->dev, "%s(dma=%p)\n", __func__, dma);

	if (!ops->get_cfg)
		return -ENOSYS;

	return ops->get_cfg(dma, cfg_id, cfg_data);
}

int dma_device_register(struct dma_device *dmad)
{
	list_add_tail(&dmad->list, &dma_devices);

	return 0;
}
