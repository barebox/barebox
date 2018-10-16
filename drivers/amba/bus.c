/*
 * Copyright (C) 2003 Deep Blue Solutions Ltd, All Rights Reserved.
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2.
 */
#include <common.h>
#include <driver.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>
#include <io.h>
#include <init.h>

#define to_amba_driver(d)	container_of(d, struct amba_driver, drv)

static const struct amba_id *
amba_lookup(const struct amba_id *table, struct amba_device *dev)
{
	int ret = 0;

	while (table->mask) {
		ret = (dev->periphid & table->mask) == table->id;
		if (ret)
			break;
		table++;
	}

	return ret ? table : NULL;
}

static int amba_match(struct device_d *dev, struct driver_d *drv)
{
	struct amba_device *pcdev = to_amba_device(dev);

	struct amba_driver *pcdrv = to_amba_driver(drv);

	return amba_lookup(pcdrv->id_table, pcdev) == NULL;
}

static int amba_get_enable_pclk(struct amba_device *pcdev)
{
	struct clk *pclk = clk_get(&pcdev->dev, "apb_pclk");
	int ret;

	pcdev->pclk = pclk;

	if (IS_ERR(pclk))
		return PTR_ERR(pclk);

	ret = clk_enable(pclk);
	if (ret) {
		clk_put(pclk);
	}

	return ret;
}

static int amba_probe(struct device_d *dev)
{
	struct amba_device *pcdev = to_amba_device(dev);
	struct amba_driver *pcdrv = to_amba_driver(dev->driver);
	const struct amba_id *id = amba_lookup(pcdrv->id_table, pcdev);

	return pcdrv->probe(pcdev, id);
}

static void amba_remove(struct device_d *dev)
{
	struct amba_device *pcdev = to_amba_device(dev);
	struct amba_driver *drv = to_amba_driver(dev->driver);

	if (drv->remove)
		drv->remove(pcdev);
}

struct bus_type amba_bustype = {
	.name = "amba",
	.match = amba_match,
	.probe = amba_probe,
	.remove = amba_remove,
};

int amba_driver_register(struct amba_driver *drv)
{
	drv->drv.bus = &amba_bustype;

	if (drv->probe)
		drv->drv.probe = amba_probe;
	if (drv->remove)
		drv->drv.remove = amba_remove;

	return register_driver(&drv->drv);
}

/**
 *	amba_device_add - add a previously allocated AMBA device structure
 *	@dev: AMBA device allocated by amba_device_alloc
 *	@parent: resource parent for this devices resources
 *
 *	Claim the resource, and read the device cell ID if not already
 *	initialized.  Register the AMBA device with the Linux device
 *	manager.
 */
int amba_device_add(struct amba_device *dev)
{
	u32 size;
	void __iomem *tmp;
	int ret;
	struct resource *res = NULL;

	dev->dev.bus = &amba_bustype;

	/*
	 * Dynamically calculate the size of the resource
	 * and use this for iomap
	 */
	size = resource_size(&dev->res);
	res = request_iomem_region("amba", dev->res.start, dev->res.end);
	if (IS_ERR(res))
		return PTR_ERR(res);
	dev->base = tmp = (void __force __iomem *)res->start;
	if (!tmp) {
		ret = -ENOMEM;
		goto err_release;
	}

	/* Hard-coded primecell ID instead of plug-n-play */
	if (dev->periphid != 0)
		goto skip_probe;

	ret = amba_get_enable_pclk(dev);
	if (ret == 0) {
		u32 pid, cid;

		/*
		 * Read pid and cid based on size of resource
		 * they are located at end of region
		 */
		pid = amba_device_get_pid(tmp, size);
		cid = amba_device_get_cid(tmp, size);

		if (cid == AMBA_CID)
			dev->periphid = pid;

		if (!dev->periphid)
			ret = -ENODEV;
	}

	if (ret)
		goto err_release;

 skip_probe:
	ret = register_device(&dev->dev);
	if (ret)
		goto err_release;

	dev_add_param_uint32_fixed(&dev->dev, "periphid", dev->periphid, "0x%08x");

	return ret;
 err_release:
	release_region(res);
	return ret;
}

struct amba_device *
amba_aphb_device_add(struct device_d *parent, const char *name, int id,
		     resource_size_t base, size_t size,
		     void *pdata, unsigned int periphid)
{
	struct amba_device *dev;
	int ret;

	dev = amba_device_alloc(name, id, base, size);

	dev->periphid = periphid;
	dev->dev.platform_data = pdata;
	dev->dev.parent = parent;

	ret = amba_device_add(dev);
	if (ret)
		return ERR_PTR(ret);

	return dev;
}

/**
 *	amba_device_alloc - allocate an AMBA device
 *	@name: sysfs name of the AMBA device
 *	@base: base of AMBA device
 *	@size: size of AMBA device
 *
 *	Allocate and initialize an AMBA device structure.  Returns %NULL
 *	on failure.
 */
struct amba_device *amba_device_alloc(const char *name, int id, resource_size_t base,
	size_t size)
{
	struct amba_device *dev;

	dev = xzalloc(sizeof(*dev));

	dev_set_name(&dev->dev, name);
	dev->dev.id = id;
	dev->res.start = base;
	dev->res.end = base + size - 1;
	dev->res.flags = IORESOURCE_MEM;
	dev->dev.resource = &dev->res;

	return dev;
}


static int amba_bus_init(void)
{
	return bus_register(&amba_bustype);
}
pure_initcall(amba_bus_init);
