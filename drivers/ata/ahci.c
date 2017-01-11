/*
 * Copyright (C) Freescale Semiconductor, Inc. 2006.
 * Author: Jason Jin<Jason.jin@freescale.com>
 *         Zhang Wei<wei.zhang@freescale.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * with the reference on libata and ahci drvier in kernel
 *
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <scsi.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <disks.h>
#include <ata_drive.h>
#include <linux/sizes.h>
#include <clock.h>

#include "ahci.h"

#define AHCI_MAX_DATA_BYTE_COUNT  SZ_4M

/*
 * Some controllers limit number of blocks they can read/write at once.
 * Contemporary SSD devices work much faster if the read/write size is aligned
 * to a power of 2.
 */
#define MAX_SATA_BLOCKS_READ_WRITE	0x80

/* Maximum timeouts for each event */
#define WAIT_SPINUP	(10 * SECOND)
#define WAIT_DATAIO	(5 * SECOND)
#define WAIT_FLUSH	(5 * SECOND)
#define WAIT_LINKUP	(4 * MSECOND)

#define ahci_port_debug(port, fmt, arg...) \
	dev_dbg(port->ahci->dev, "port %d: " fmt, port->num, ##arg)

#define ahci_port_info(port, fmt, arg...) \
	dev_info(port->ahci->dev, "port %d: " fmt, port->num, ##arg)

#define ahci_debug(ahci, fmt, arg...) \
	dev_dbg(ahci->dev, fmt, ##arg)

struct ahci_cmd_hdr {
	u32	opts;
	u32	status;
	u32	tbl_addr;
	u32	tbl_addr_hi;
	u32	reserved[4];
};

struct ahci_sg {
	u32	addr;
	u32	addr_hi;
	u32	reserved;
	u32	flags_size;
};

static inline void ahci_iowrite(struct ahci_device *ahci, int ofs, u32 val)
{
	writel(val, ahci->mmio_base + ofs);
}

static inline u32 ahci_ioread(struct ahci_device *ahci, int ofs)
{
	return readl(ahci->mmio_base + ofs);
}

static inline void ahci_iowrite_f(struct ahci_device *ahci, int ofs, u32 val)
{
	writel(val, ahci->mmio_base + ofs);
	readl(ahci->mmio_base);
}

static inline void ahci_port_write(struct ahci_port *port, int ofs, u32 val)
{
	writel(val, port->port_mmio + ofs);
}

static inline void ahci_port_write_f(struct ahci_port *port, int ofs, u32 val)
{
	writel(val, port->port_mmio + ofs);
	readl(port->port_mmio + ofs);
}

static inline u32 ahci_port_read(struct ahci_port *port, int ofs)
{
	return readl(port->port_mmio + ofs);
}

static inline void __iomem *ahci_port_base(void __iomem *base, int port)
{
	return base + 0x100 + (port * 0x80);
}

static int ahci_link_ok(struct ahci_port *ahci_port, int verbose)
{
	u32 val = ahci_port_read(ahci_port, PORT_SCR_STAT) & 0xf;

	if (val == 0x3)
		return true;

	if (verbose)
		dev_err(ahci_port->ahci->dev, "port %d: no link\n", ahci_port->num);

	return false;
}

static void ahci_fill_cmd_slot(struct ahci_port *ahci_port, u32 opts)
{
	ahci_port->cmd_slot->opts = cpu_to_le32(opts);
	ahci_port->cmd_slot->status = 0;
	ahci_port->cmd_slot->tbl_addr =
		cpu_to_le32((unsigned long)ahci_port->cmd_tbl & 0xffffffff);
	ahci_port->cmd_slot->tbl_addr_hi = 0;
}

static int ahci_fill_sg(struct ahci_port *ahci_port, const void *buf, int buf_len)
{
	struct ahci_sg *ahci_sg = ahci_port->cmd_tbl_sg;
	u32 sg_count;

	sg_count = ((buf_len - 1) / AHCI_MAX_DATA_BYTE_COUNT) + 1;
	if (sg_count > AHCI_MAX_SG)
		return -EINVAL;

	while (buf_len) {
		unsigned int now = min(AHCI_MAX_DATA_BYTE_COUNT, buf_len);

		ahci_sg->addr = cpu_to_le32((u32)buf);
		ahci_sg->addr_hi = 0;
		ahci_sg->flags_size = cpu_to_le32(now - 1);

		buf_len -= now;
		buf += now;
	}

	return sg_count;
}

static int ahci_io(struct ahci_port *ahci_port, u8 *fis, int fis_len, void *rbuf,
		const void *wbuf, int buf_len)
{
	u32 opts;
	int sg_count;
	int ret;

	if (!ahci_link_ok(ahci_port, 1))
		return -EIO;

	if (wbuf)
		dma_sync_single_for_device((unsigned long)wbuf, buf_len,
					   DMA_TO_DEVICE);
	if (rbuf)
		dma_sync_single_for_device((unsigned long)rbuf, buf_len,
					   DMA_FROM_DEVICE);

	memcpy((unsigned char *)ahci_port->cmd_tbl, fis, fis_len);

	sg_count = ahci_fill_sg(ahci_port, rbuf ? rbuf : wbuf, buf_len);
	opts = (fis_len >> 2) | (sg_count << 16);
	if (wbuf)
		opts |= 1 << 6;
	ahci_fill_cmd_slot(ahci_port, opts);

	ahci_port_write_f(ahci_port, PORT_CMD_ISSUE, 1);

	ret = wait_on_timeout(WAIT_DATAIO,
			(readl(ahci_port->port_mmio + PORT_CMD_ISSUE) & 0x1) == 0);
	if (ret)
		return -ETIMEDOUT;

	if (wbuf)
		dma_sync_single_for_cpu((unsigned long)wbuf, buf_len,
					DMA_TO_DEVICE);
	if (rbuf)
		dma_sync_single_for_cpu((unsigned long)rbuf, buf_len,
					DMA_FROM_DEVICE);

	return 0;
}

/*
 * SCSI INQUIRY command operation.
 */
static int ahci_read_id(struct ata_port *ata, void *buf)
{
	struct ahci_port *ahci = container_of(ata, struct ahci_port, ata);
	u8 fis[20];

	memset(fis, 0, sizeof(fis));

	/* Construct the FIS */
	fis[0] = 0x27;			/* Host to device FIS. */
	fis[1] = 1 << 7;		/* Command FIS. */
	fis[2] = ATA_CMD_ID_ATA;	/* Command byte. */

	return ahci_io(ahci, fis, sizeof(fis), buf, NULL, SECTOR_SIZE);
}

static int ahci_rw(struct ata_port *ata, void *rbuf, const void *wbuf,
		unsigned int block, int num_blocks)
{
	struct ahci_port *ahci = container_of(ata, struct ahci_port, ata);
	u8 fis[20];
	int ret;
	int lba48 = ata_id_has_lba48(ata->id);

	memset(fis, 0, sizeof(fis));

	/* Construct the FIS */
	fis[0] = 0x27;			/* Host to device FIS. */
	fis[1] = 1 << 7;		/* Command FIS. */

	/* Command byte. */
	if (lba48)
		fis[2] = wbuf ? ATA_CMD_WRITE_EXT : ATA_CMD_READ_EXT;
	else
		fis[2] = wbuf ? ATA_CMD_WRITE : ATA_CMD_READ;

	while (num_blocks) {
		int now;

		now = min(MAX_SATA_BLOCKS_READ_WRITE, num_blocks);

		fis[4] = (block >> 0) & 0xff;
		fis[5] = (block >> 8) & 0xff;
		fis[6] = (block >> 16) & 0xff;

		if (lba48) {
			fis[7] = 1 << 6; /* device reg: set LBA mode */
			fis[8] = ((block >> 24) & 0xff);
			fis[3] = 0xe0; /* features */
		} else {
			fis[7] = ((block >> 24) & 0xf) | 0xe0;
		}

		/* Block (sector) count */
		fis[12] = (now >> 0) & 0xff;
		fis[13] = (now >> 8) & 0xff;

		ret = ahci_io(ahci, fis, sizeof(fis), rbuf, wbuf, now * SECTOR_SIZE);
		if (ret)
			return ret;

		if (rbuf)
			rbuf += now * SECTOR_SIZE;
		if (wbuf)
			wbuf += now * SECTOR_SIZE;
		num_blocks -= now;
		block += now;
	}

	return 0;
}

static int ahci_read(struct ata_port *ata, void *buf, unsigned int block,
		int num_blocks)
{
	return ahci_rw(ata, buf, NULL, block, num_blocks);
}

static int ahci_write(struct ata_port *ata, const void *buf, unsigned int block,
		int num_blocks)
{
	return ahci_rw(ata, NULL, buf, block, num_blocks);
}

static int ahci_init_port(struct ahci_port *ahci_port)
{
	void __iomem *port_mmio;
	u32 val, cmd;
	int ret;

	port_mmio = ahci_port->port_mmio;

	/* make sure port is not active */
	val = ahci_port_read(ahci_port, PORT_CMD);
	if (val & (PORT_CMD_LIST_ON | PORT_CMD_FIS_ON | PORT_CMD_FIS_RX | PORT_CMD_START)) {
		ahci_port_debug(ahci_port, "Port is active. Deactivating.\n");
		val &= ~(PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
			 PORT_CMD_FIS_RX | PORT_CMD_START);
		ahci_port_write(ahci_port, PORT_CMD, val);

		/*
		 * spec says 500 msecs for each bit, so
		 * this is slightly incorrect.
		 */
		mdelay(500);
	}

	/*
	 * First item in chunk of DMA memory: 32-slot command table,
	 * 32 bytes each in size
	 */
	ahci_port->cmd_slot = dma_alloc_coherent(AHCI_CMD_SLOT_SZ * 32,
						 DMA_ADDRESS_BROKEN);
	if (!ahci_port->cmd_slot) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	ahci_port_debug(ahci_port, "cmd_slot = 0x%x\n", (unsigned)ahci_port->cmd_slot);

	/*
	 * Second item: Received-FIS area
	 */
	ahci_port->rx_fis = (unsigned long)dma_alloc_coherent(AHCI_RX_FIS_SZ,
							      DMA_ADDRESS_BROKEN);
	if (!ahci_port->rx_fis) {
		ret = -ENOMEM;
		goto err_alloc1;
	}

	/*
	 * Third item: data area for storing a single command
	 * and its scatter-gather table
	 */
	ahci_port->cmd_tbl = dma_alloc_coherent(AHCI_CMD_TBL_SZ,
						DMA_ADDRESS_BROKEN);
	if (!ahci_port->cmd_tbl) {
		ret = -ENOMEM;
		goto err_alloc2;
	}

	memset(ahci_port->cmd_slot, 0, AHCI_CMD_SLOT_SZ * 32);
	memset((void *)ahci_port->rx_fis, 0, AHCI_RX_FIS_SZ);
	memset(ahci_port->cmd_tbl, 0, AHCI_CMD_TBL_SZ);

	ahci_port_debug(ahci_port, "cmd_tbl_dma = 0x%p\n", ahci_port->cmd_tbl);

	ahci_port->cmd_tbl_sg = ahci_port->cmd_tbl + AHCI_CMD_TBL_HDR_SZ;

	ahci_port_write_f(ahci_port, PORT_LST_ADDR, (u32)ahci_port->cmd_slot);
	ahci_port_write_f(ahci_port, PORT_FIS_ADDR, ahci_port->rx_fis);

	/*
	 * Add the spinup command to whatever mode bits may
	 * already be on in the command register.
	 */
	cmd = ahci_port_read(ahci_port, PORT_CMD);
	cmd |= PORT_CMD_FIS_RX;
	cmd |= PORT_CMD_SPIN_UP;
	cmd |= PORT_CMD_ICC_ACTIVE;
	ahci_port_write_f(ahci_port, PORT_CMD, cmd);

	mdelay(10);

	cmd = ahci_port_read(ahci_port, PORT_CMD);
	cmd |= PORT_CMD_START;
	ahci_port_write_f(ahci_port, PORT_CMD, cmd);

	/*
	 * Bring up SATA link.
	 * SATA link bringup time is usually less than 1 ms; only very
	 * rarely has it taken between 1-2 ms. Never seen it above 2 ms.
	 */
	ret = wait_on_timeout(WAIT_LINKUP,
			(ahci_port_read(ahci_port, PORT_SCR_STAT) & 0xf) == 0x3);
	if (ret) {
		ahci_port_info(ahci_port, "SATA link timeout\n");
		ret = -ETIMEDOUT;
		goto err_init;
	}

	ahci_port_info(ahci_port, "SATA link ok\n");

	/* Clear error status */
	val = ahci_port_read(ahci_port, PORT_SCR_ERR);
	if (val)
		ahci_port_write(ahci_port, PORT_SCR_ERR, val);

	ahci_port_info(ahci_port, "Spinning up device...\n");

	ret = wait_on_timeout(WAIT_SPINUP,
			((readl(port_mmio + PORT_TFDATA) &
			 (ATA_STATUS_BUSY | ATA_STATUS_DRQ)) == 0)
			|| ((readl(port_mmio + PORT_SCR_STAT) & 0xf) == 1));
	if (ret) {
		ahci_port_info(ahci_port, "timeout.\n");
		ret = -ENODEV;
		goto err_init;
	}

	if ((readl(port_mmio + PORT_SCR_STAT) & 0xf) == 1) {
		ahci_port_info(ahci_port, "down.\n");
		ret = -ENODEV;
		goto err_init;
	}

	ahci_port_info(ahci_port, "ok.\n");

	val = ahci_port_read(ahci_port, PORT_SCR_ERR);

	ahci_port_write(ahci_port, PORT_SCR_ERR, val);

	/* ack any pending irq events for this port */
	val = ahci_port_read(ahci_port, PORT_IRQ_STAT);
	if (val)
		ahci_port_write(ahci_port, PORT_IRQ_STAT, val);

	ahci_iowrite(ahci_port->ahci, HOST_IRQ_STAT, 1 << ahci_port->num);

	/* set irq mask (enables interrupts) */
	ahci_port_write(ahci_port, PORT_IRQ_MASK, DEF_PORT_IRQ);

	/* register linkup ports */
	val = ahci_port_read(ahci_port, PORT_SCR_STAT);

	ahci_port_debug(ahci_port, "status: 0x%08x\n", val);

	if ((val & 0xf) == 0x03)
		return 0;

	ret = -ENODEV;

err_init:
	dma_free_coherent(ahci_port->cmd_tbl, 0, AHCI_CMD_TBL_SZ);
err_alloc2:
	dma_free_coherent((void *)ahci_port->rx_fis, 0, AHCI_RX_FIS_SZ);
err_alloc1:
	dma_free_coherent(ahci_port->cmd_slot, 0, AHCI_CMD_SLOT_SZ * 32);
err_alloc:
	return ret;
}

static int ahci_port_start(struct ata_port *ata_port)
{
	struct ahci_port *ahci_port = container_of(ata_port, struct ahci_port, ata);
	int ret;

	ret = ahci_init_port(ahci_port);
	if (ret)
		return ret;

	if (!ahci_link_ok(ahci_port, 1))
		return -EIO;

	ahci_port_write_f(ahci_port, PORT_CMD,
			PORT_CMD_ICC_ACTIVE | PORT_CMD_FIS_RX |
			PORT_CMD_POWER_ON | PORT_CMD_SPIN_UP |
			PORT_CMD_START);

	return 0;
}

static struct ata_port_operations ahci_ops = {
	.init = ahci_port_start,
	.read_id = ahci_read_id,
	.read = ahci_read,
	.write = ahci_write,
};

#if 0
/*
 * In the general case of generic rotating media it makes sense to have a
 * flush capability. It probably even makes sense in the case of SSDs because
 * one cannot always know for sure what kind of internal cache/flush mechanism
 * is embodied therein. At first it was planned to invoke this after the last
 * write to disk and before rebooting. In practice, knowing, a priori, which
 * is the last write is difficult. Because writing to the disk in u-boot is
 * very rare, this flush command will be invoked after every block write.
 */
static int ata_io_flush(u8 port)
{
	u8 fis[20];
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 cmd_fis_len = 5;	/* five dwords */

	/* Preset the FIS */
	memset(fis, 0, 20);
	fis[0] = 0x27;		 /* Host to device FIS. */
	fis[1] = 1 << 7;	 /* Command FIS. */
	fis[2] = ATA_CMD_FLUSH_EXT;

	memcpy((unsigned char *)pp->cmd_tbl, fis, 20);
	ahci_fill_cmd_slot(pp, cmd_fis_len);
	mywritel_with_flush(1, port_mmio + PORT_CMD_ISSUE);

	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE,
			WAIT_MS_FLUSH, 0x1)) {
		debug("scsi_ahci: flush command timeout on port %d.\n", port);
		return -EIO;
	}

	return 0;
}
#endif

void ahci_print_info(struct ahci_device *ahci)
{
	u32 vers, cap, cap2, impl, speed;
	const char *speed_s;
	const char *scc_s;

	vers = ahci_ioread(ahci, HOST_VERSION);
	cap = ahci->cap;
	cap2 = ahci_ioread(ahci, HOST_CAP2);
	impl = ahci->port_map;

	speed = (cap >> 20) & 0xf;
	if (speed == 1)
		speed_s = "1.5";
	else if (speed == 2)
		speed_s = "3";
	else if (speed == 3)
		speed_s = "6";
	else
		speed_s = "?";

	scc_s = "SATA";

	printf("AHCI %02x%02x.%02x%02x "
	       "%u slots %u ports %s Gbps 0x%x impl %s mode\n",
	       (vers >> 24) & 0xff,
	       (vers >> 16) & 0xff,
	       (vers >> 8) & 0xff,
	       vers & 0xff,
	       ((cap >> 8) & 0x1f) + 1, (cap & 0x1f) + 1, speed_s, impl, scc_s);

	printf("flags: "
	       "%s%s%s%s%s%s%s"
	       "%s%s%s%s%s%s%s"
	       "%s%s%s%s%s%s\n",
	       cap & (1 << 31) ? "64bit " : "",
	       cap & (1 << 30) ? "ncq " : "",
	       cap & (1 << 28) ? "ilck " : "",
	       cap & (1 << 27) ? "stag " : "",
	       cap & (1 << 26) ? "pm " : "",
	       cap & (1 << 25) ? "led " : "",
	       cap & (1 << 24) ? "clo " : "",
	       cap & (1 << 19) ? "nz " : "",
	       cap & (1 << 18) ? "only " : "",
	       cap & (1 << 17) ? "pmp " : "",
	       cap & (1 << 16) ? "fbss " : "",
	       cap & (1 << 15) ? "pio " : "",
	       cap & (1 << 14) ? "slum " : "",
	       cap & (1 << 13) ? "part " : "",
	       cap & (1 << 7) ? "ccc " : "",
	       cap & (1 << 6) ? "ems " : "",
	       cap & (1 << 5) ? "sxs " : "",
	       cap2 & (1 << 2) ? "apst " : "",
	       cap2 & (1 << 1) ? "nvmp " : "",
	       cap2 & (1 << 0) ? "boh " : "");
}

void ahci_info(struct device_d *dev)
{
	struct ahci_device *ahci = dev->priv;

	ahci_print_info(ahci);
}

static int ahci_detect(struct device_d *dev)
{
	struct ahci_device *ahci = dev->priv;
	int i;

	for (i = 0; i < ahci->n_ports; i++) {
		struct ahci_port *ahci_port = &ahci->ports[i];

		ata_port_detect(&ahci_port->ata);
	}

	return 0;
}

int ahci_add_host(struct ahci_device *ahci)
{
	u8 *mmio = (u8 *)ahci->mmio_base;
	u32 tmp, cap_save;
	int i, ret;

	ahci->host_flags = ATA_FLAG_SATA
				| ATA_FLAG_NO_LEGACY
				| ATA_FLAG_MMIO
				| ATA_FLAG_PIO_DMA
				| ATA_FLAG_NO_ATAPI;
	ahci->pio_mask = 0x1f;
	ahci->udma_mask = 0x7f;	/* FIXME: assume to support UDMA6 */

	ahci_debug(ahci, "ahci_host_init: start\n");

	cap_save = readl(mmio + HOST_CAP);
	cap_save &= ((1 << 28) | (1 << 17));
	cap_save |= (1 << 27);  /* Staggered Spin-up. Not needed. */

	/* global controller reset */
	tmp = ahci_ioread(ahci, HOST_CTL);
	if ((tmp & HOST_RESET) == 0)
		ahci_iowrite_f(ahci, HOST_CTL, tmp | HOST_RESET);

	/*
	 * reset must complete within 1 second, or
	 * the hardware should be considered fried.
	 */
	ret = wait_on_timeout(SECOND, (readl(mmio + HOST_CTL) & HOST_RESET) == 0);
	if (ret) {
		ahci_debug(ahci,"controller reset failed (0x%x)\n", tmp);
		return -ENODEV;
	}

	ahci_iowrite_f(ahci, HOST_CTL, HOST_AHCI_EN);
	ahci_iowrite(ahci, HOST_CAP, cap_save);
	ahci_iowrite_f(ahci, HOST_PORTS_IMPL, 0xf);

	ahci->cap = ahci_ioread(ahci, HOST_CAP);
	ahci->port_map = ahci_ioread(ahci, HOST_PORTS_IMPL);
	ahci->n_ports = (ahci->cap & 0x1f) + 1;

	ahci_debug(ahci, "cap 0x%x  port_map 0x%x  n_ports %d\n",
	      ahci->cap, ahci->port_map, ahci->n_ports);

	for (i = 0; i < ahci->n_ports; i++) {
		struct ahci_port *ahci_port = &ahci->ports[i];

		ahci_port->num = i;
		ahci_port->ahci = ahci;
		ahci_port->ata.dev = ahci->dev;
		ahci_port->port_mmio = ahci_port_base(mmio, i);
		ahci_port->ata.ops = &ahci_ops;
		ata_port_register(&ahci_port->ata);
	}

	tmp = ahci_ioread(ahci, HOST_CTL);
	ahci_iowrite(ahci, HOST_CTL, tmp | HOST_IRQ_EN);
	tmp = ahci_ioread(ahci, HOST_CTL);

	ahci->dev->detect = ahci_detect;

	return 0;
}

static int ahci_probe(struct device_d *dev)
{
	struct resource *iores;
	struct ahci_device *ahci;
	void __iomem *regs;
	int ret;

	ahci = xzalloc(sizeof(*ahci));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	regs = IOMEM(iores->start);

	ahci->dev = dev;
	ahci->mmio_base = regs;
	dev->priv = ahci;
	dev->info = ahci_info;

	ret = ahci_add_host(ahci);
	if (ret)
		free(ahci);

	return ret;
}

static __maybe_unused struct of_device_id ahci_dt_ids[] = {
	{
		.compatible = "calxeda,hb-ahci",
	}, {
		/* sentinel */
	}
};

static struct driver_d ahci_driver = {
	.name   = "ahci",
	.probe  = ahci_probe,
	.of_compatible = DRV_OF_COMPAT(ahci_dt_ids),
};
device_platform_driver(ahci_driver);
