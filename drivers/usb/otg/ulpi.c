/*
 * Copyright 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <io.h>
#include <errno.h>
#include <usb/ulpi.h>

/* ULPIVIEW register bits */
#define ULPIVW_WU		(1 << 31)	/* Wakeup */
#define ULPIVW_RUN		(1 << 30)	/* read/write run */
#define ULPIVW_WRITE		(1 << 29)	/* 0 = read  1 = write */
#define ULPIVW_SS		(1 << 27)	/* SyncState */
#define ULPIVW_PORT_MASK	0x07	/* Port field */
#define ULPIVW_PORT_SHIFT	24
#define ULPIVW_ADDR_MASK	0xFF	/* data address field */
#define ULPIVW_ADDR_SHIFT	16
#define ULPIVW_RDATA_MASK	0xFF	/* read data field */
#define ULPIVW_RDATA_SHIFT	8
#define ULPIVW_WDATA_MASK	0xFF	/* write data field */
#define ULPIVW_WDATA_SHIFT	0

static int ulpi_poll(void __iomem *view, uint32_t bit)
{
	uint32_t data;
	int timeout = 10000;

	data = readl(view);
	while (data & bit) {
		if (!timeout--)
			return -ETIMEDOUT;

		udelay(1);
		data = readl(view);
	}
	return (data >> ULPIVW_RDATA_SHIFT) & ULPIVW_RDATA_MASK;
}

int ulpi_read(int reg, void __iomem *view)
{
	int ret;

	/* make sure interface is running */
	if (!(readl(view) & ULPIVW_SS)) {
		writel(ULPIVW_WU, view);

		/* wait for wakeup */
		ret = ulpi_poll(view, ULPIVW_WU);
		if (ret < 0)
			return ret;
	}

	/* read the register */
	writel((ULPIVW_RUN | (reg << ULPIVW_ADDR_SHIFT)), view);

	/* wait for completion */
	return ulpi_poll(view, ULPIVW_RUN);
}
EXPORT_SYMBOL(ulpi_read);

int ulpi_set(u8 bits, int reg, void __iomem *view)
{
	int ret;

	/* make sure the interface is running */
	if (!(readl(view) & ULPIVW_SS)) {
		writel(ULPIVW_WU, view);
		/* wait for wakeup */
		ret = ulpi_poll(view, ULPIVW_WU);
		if (ret < 0)
			return ret;
	}

	writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ULPI_REG_SET) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	/* wait for completion */
	ret = ulpi_poll(view, ULPIVW_RUN);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL(ulpi_set);

int ulpi_clear(u8 bits, int reg, void __iomem *view)
{
	int ret;

	writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ULPI_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	/* wait for completion */
	ret = ulpi_poll(view, ULPIVW_RUN);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL(ulpi_clear);

int ulpi_write(u8 bits, int reg, void __iomem *view)
{
	int ret;

	writel((ULPIVW_RUN | ULPIVW_WRITE |
		      (reg << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	/* wait for completion */
	ret = ulpi_poll(view, ULPIVW_RUN);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL(ulpi_write);

struct ulpi_info {
	uint32_t	id;
	char		*name;
};

#define ULPI_ID(vendor, product) (((vendor) << 16) | (product))
#define ULPI_INFO(_id, _name)		\
	{				\
		.id	= (_id),	\
		.name	= (_name),	\
	}

/* ULPI hardcoded IDs, used for probing */
static struct ulpi_info ulpi_ids[] = {
	ULPI_INFO(ULPI_ID(0x04cc, 0x1504), "NXP ISP150x"),
	ULPI_INFO(ULPI_ID(0x0424, 0x0006), "SMSC USB331x"),
};

static int ulpi_read_id(void __iomem *view, int *vid, int *pid)
{
	int i, ret;
	uint32_t ulpi_id = 0;

	for (i = 0; i < 4; i++) {
		ret = ulpi_read(ULPI_PID_HIGH - i, view);
		if (ret < 0)
			return ret;
		ulpi_id = (ulpi_id << 8) | ret;
	}

	*vid = ulpi_id & 0xffff;
	*pid = (ulpi_id >> 16) & 0xffff;

	return 0;
}

static int ulpi_probe(void __iomem *view)
{
	int i, j, vid, pid, ret;

	for (i = 0; i < 4; i++) {
		ret = ulpi_read_id(view, &vid, &pid);
		if (ret)
			return ret;

		for (j = 0; j < ARRAY_SIZE(ulpi_ids); j++) {
			if (ulpi_ids[j].id == ULPI_ID(vid, pid)) {
				pr_info("Found %s ULPI transceiver (0x%04x:0x%04x).\n",
				ulpi_ids[j].name, vid, pid);
				return 0;
			}
		}
	}

	pr_err("No ULPI found. vid: 0x%04x pid: 0x%04x\n", vid, pid);

	return -ENODEV;
}

static int ulpi_set_vbus(void __iomem *view, int on)
{
	int ret;

	if (on) {
		ret = ulpi_set(ULPI_OTG_DRV_VBUS_EXT |		/* enable external Vbus */
				ULPI_OTG_DRV_VBUS |		/* enable internal Vbus */
				ULPI_OTG_USE_EXT_VBUS_IND |	/* use external indicator */
				ULPI_OTG_CHRG_VBUS,		/* charge Vbus */
				ULPI_OTGCTL, view);
	} else {
		ret = ulpi_clear(ULPI_OTG_DRV_VBUS_EXT |		/* disable external Vbus */
				ULPI_OTG_DRV_VBUS,		/* disable internal Vbus */
				ULPI_OTGCTL, view);

		ret |= ulpi_set(ULPI_OTG_USE_EXT_VBUS_IND |	/* use external indicator */
				ULPI_OTG_DISCHRG_VBUS,		/* discharge Vbus */
				ULPI_OTGCTL, view);
	}

	return ret;
}

int ulpi_setup(void __iomem *view, int on)
{
	int ret;

	ret = ulpi_probe(view);
	if (ret)
		return ret;

	return ulpi_set_vbus(view, on);
}
EXPORT_SYMBOL(ulpi_setup);
