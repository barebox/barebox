// SPDX-License-Identifier: GPL-2.0-only
/*
 * SiFive L2 cache controller Driver
 *
 * Copyright (C) 2018-2019 SiFive, Inc.
 *
 */

#define pr_fmt(fmt) "sifive-l2: " fmt

#include <io.h>
#include <io-64-nonatomic-lo-hi.h>
#include <linux/printk.h>
#include <stdio.h>
#include <driver.h>
#include <init.h>
#include <soc/sifive/l2_cache.h>
#include <asm/barrier.h>
#include <linux/align.h>
#include <linux/bitops.h>
#include <linux/bug.h>

#define SIFIVE_L2_DIRECCFIX_LOW 0x100
#define SIFIVE_L2_DIRECCFIX_HIGH 0x104
#define SIFIVE_L2_DIRECCFIX_COUNT 0x108

#define SIFIVE_L2_DIRECCFAIL_LOW 0x120
#define SIFIVE_L2_DIRECCFAIL_HIGH 0x124
#define SIFIVE_L2_DIRECCFAIL_COUNT 0x128

#define SIFIVE_L2_DATECCFIX_LOW 0x140
#define SIFIVE_L2_DATECCFIX_HIGH 0x144
#define SIFIVE_L2_DATECCFIX_COUNT 0x148

#define SIFIVE_L2_DATECCFAIL_LOW 0x160
#define SIFIVE_L2_DATECCFAIL_HIGH 0x164
#define SIFIVE_L2_DATECCFAIL_COUNT 0x168

#define SIFIVE_L2_FLUSH64 0x200

#define SIFIVE_L2_CONFIG 0x00
#define SIFIVE_L2_WAYENABLE 0x08
#define SIFIVE_L2_ECCINJECTERR 0x40

#define SIFIVE_L2_MAX_ECCINTR 4

#define MASK_NUM_WAYS   GENMASK(15, 8)
#define NUM_WAYS_SHIFT  8

#define SIFIVE_L2_FLUSH64_LINE_LEN 64

static void __iomem *l2_base = NULL;

static void sifive_l2_config_read(struct device *dev)
{
	u32 regval, val;

	printf("Cache configuration:\n");

	regval = readl(l2_base + SIFIVE_L2_CONFIG);
	val = regval & 0xFF;
	printf("  #Banks: %d\n", val);
	val = (regval & 0xFF00) >> 8;
	printf("  #Ways per bank: %d\n", val);
	val = (regval & 0xFF0000) >> 16;
	printf("  #Sets per bank: %llu\n", 1llu << val);
	val = (regval & 0xFF000000) >> 24;
	printf("  #Bytes per cache block: %llu\n", 1llu << val);

	regval = readl(l2_base + SIFIVE_L2_WAYENABLE);
	printf("  #Index of the largest way enabled: %d\n", regval);
}

void sifive_l2_flush64_range(dma_addr_t start, dma_addr_t end)
{
	unsigned long line;

	start = ALIGN_DOWN(start, 64);
	end = ALIGN(end, 64);

	if (WARN_ON(!l2_base))
		return;

	if (start == end)
		return;

	mb();
	for (line = start; line < end; line += SIFIVE_L2_FLUSH64_LINE_LEN) {
		writeq(line, l2_base + SIFIVE_L2_FLUSH64);
		mb();
	}
}

static void sifive_l2_enable_ways(void)
{
	u32 config;
	u32 ways;

	config = readl(l2_base + SIFIVE_L2_CONFIG);
	ways = (config & MASK_NUM_WAYS) >> NUM_WAYS_SHIFT;

	mb();
	writel(ways - 1, l2_base + SIFIVE_L2_WAYENABLE);
	mb();
}

static int sifive_l2_probe(struct device *dev)
{
	struct resource *iores;

	if (l2_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	l2_base = IOMEM(iores->start);

	sifive_l2_enable_ways();

	dev->info = sifive_l2_config_read;

	return 0;
}

static const struct of_device_id sifive_l2_ids[] = {
	{ .compatible = "sifive,fu540-c000-ccache" },
	{ .compatible = "sifive,fu740-c000-ccache" },
	{ .compatible = "starfive,ccache0" },
	{ /* end of table */ },
};
MODULE_DEVICE_TABLE(of, sifive_l2_ids);

static struct driver sifive_l2_driver = {
	.name = "sfive-l2cache",
	.probe = sifive_l2_probe,
	.of_compatible = sifive_l2_ids,
};
postcore_platform_driver(sifive_l2_driver);
