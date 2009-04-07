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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <common.h>
#include <asm/io.h>
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
	if (!(readl(view) && ULPIVW_SS)) {
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
	if (!(readl(view) && ULPIVW_SS)) {
		writel(ULPIVW_WU, view);
		/* wait for wakeup */
		ret = ulpi_poll(view, ULPIVW_WU);
		if (ret < 0)
			return ret;
	}

	writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_SET) << ULPIVW_ADDR_SHIFT) |
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
		      ((reg + ISP1504_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	/* wait for completion */
	ret = ulpi_poll(view, ULPIVW_RUN);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL(ulpi_clear);

