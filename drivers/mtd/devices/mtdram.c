/*
 * Author: Sebastian Block <basti@linux-source.de>
 * Copyright (c) 2014
 *
 * Some parts are based on mtdram.c found in Linux kernel
 * by   Alexander Larsson <alex@cendio.se>
 * and  Joern Engel <joern@wh.fh-wedel.de>.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <common.h>
#include <environment.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/mtd.h>
#include <malloc.h>
#include <of.h>

struct mtdram_priv_data {
	struct mtd_info mtd;
	void *base;
};

static int ram_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	memset((char *)mtd->priv + instr->addr, 0xff, instr->len);
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return 0;
}

static int ram_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	memcpy((char*)mtd->priv + to, buf, len);
	*retlen = len;
	return 0;
}

static int ram_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	memcpy(buf, mtd->priv + from, len);
	*retlen = len;
	return 0;
}

static int mtdram_probe(struct device_d *dev)
{
	void __iomem *base;
	int device_id;
	struct mtd_info *mtd;
	struct resource *res;
	loff_t size;
	int ret = 0;

	mtd = xzalloc(sizeof(struct mtd_info));

	device_id = DEVICE_ID_SINGLE;
	if (dev->device_node) {
		const char *alias = of_alias_get(dev->device_node);
		if (alias)
			mtd->name = xstrdup(alias);
	}

	if (!mtd->name) {
		device_id = DEVICE_ID_DYNAMIC;
		mtd->name = "mtdram";
	}

	base = dev_request_mem_region(dev, 0);
	if (!base) {
		ret = -EBUSY;
		goto nobase;
	}

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	size = (unsigned long) resource_size(res);
	mtd->priv = base;

	mtd->type = MTD_RAM;
	mtd->writesize = 1;
	mtd->writebufsize = 64;
	mtd->flags = MTD_CAP_RAM;
	mtd->size = size;

	mtd->read = ram_read;
	mtd->write = ram_write;
	mtd->erase = ram_erase;
	mtd->erasesize = 1;

	mtd->parent = dev;

	ret = add_mtd_device(mtd, mtd->name, device_id);
	return ret;

nobase:
	kfree(mtd);

	return ret;
}

static __maybe_unused struct of_device_id mtdram_dt_ids[] = {
	{
		.compatible	= "mtd-ram",
	}, {
		/*  sentinel */
	}
};

static struct driver_d mtdram_driver = {
	.name	= "mtdram",
	.probe	= mtdram_probe,
	.of_compatible	= DRV_OF_COMPAT(mtdram_dt_ids),
};
device_platform_driver(mtdram_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sebastian Block");
MODULE_DESCRIPTION("MTD RAM driver for NVRAMs");
