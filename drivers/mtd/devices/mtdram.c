// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author: Sebastian Block <basti@linux-source.de>
 * Copyright (c) 2014
 *
 * Some parts are based on mtdram.c found in Linux kernel
 * by   Alexander Larsson <alex@cendio.se>
 * and  Joern Engel <joern@wh.fh-wedel.de>.
 */
#include <common.h>
#include <environment.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/mtd.h>
#include <malloc.h>
#include <of.h>

static int ram_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	memset((char *)mtd->priv + instr->addr, 0xff, instr->len);
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

static int mtdram_probe(struct device *dev)
{
	long type;
	struct resource *iores;
	int device_id;
	struct mtd_info *mtd;
	loff_t size;
	int ret = 0;

	mtd = xzalloc(sizeof(struct mtd_info));

	device_id = DEVICE_ID_SINGLE;
	if (dev->of_node) {
		const char *alias = of_alias_get(dev->of_node);
		if (alias)
			mtd->name = xstrdup(alias);
	}

	type = (long)device_get_match_data(dev);

	if (!mtd->name) {
		device_id = DEVICE_ID_DYNAMIC;
		mtd->name = type == MTD_RAM ? "mtdram" : "mtdrom";
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto nobase;
	}

	mtd->priv = IOMEM(iores->start);
	size = (unsigned long) resource_size(iores);

	mtd->type = type;
	mtd->writesize = 1;
	mtd->writebufsize = 64;
	mtd->size = size;

	mtd->_read = ram_read;

	if (type == MTD_RAM) {
		mtd->flags = MTD_CAP_RAM;
		mtd->_write = ram_write;
		mtd->_erase = ram_erase;
		mtd->erasesize = 1;
	}

	mtd->dev.parent = dev;

	ret = add_mtd_device(mtd, mtd->name, device_id);
	return ret;

nobase:
	kfree(mtd);

	return ret;
}

static __maybe_unused struct of_device_id mtdram_dt_ids[] = {
	{
		.compatible	= "mtd-ram",
		.data		= (void *)MTD_RAM
	}, {
		.compatible	= "mtd-rom",
		.data		= (void *)MTD_ROM
	}, {
		/*  sentinel */
	}
};
MODULE_DEVICE_TABLE(of, mtdram_dt_ids);

static struct driver mtdram_driver = {
	.name	= "mtdram",
	.probe	= mtdram_probe,
	.of_compatible	= DRV_OF_COMPAT(mtdram_dt_ids),
};
device_platform_driver(mtdram_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sebastian Block");
MODULE_DESCRIPTION("MTD RAM driver for NVRAMs");
