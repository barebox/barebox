// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Freescale Semiconductor, Inc. 2006.
 * Author: Jason Jin<Jason.jin@freescale.com>
 *         Zhang Wei<wei.zhang@freescale.com>
 *
 * with the reference on libata and ahci driver in kernel
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
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
	u32 val = ahci_port_read(ahci_port, PORT_SCR_STAT) & PORT_SCR_STAT_DET;

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
		cpu_to_le32(lower_32_bits(ahci_port->cmd_tbl_dma));
	if (ahci_port->ahci->cap & HOST_CAP_64)
		ahci_port->cmd_slot->tbl_addr_hi =
			cpu_to_le32(upper_32_bits(ahci_port->cmd_tbl_dma));
}

static int ahci_fill_sg(struct ahci_port *ahci_port, dma_addr_t buf_dma, int buf_len)
{
	struct ahci_sg *ahci_sg = ahci_port->cmd_tbl_sg;
	u32 sg_count;

	sg_count = ((buf_len - 1) / AHCI_MAX_DATA_BYTE_COUNT) + 1;
	if (sg_count > AHCI_MAX_SG)
		return -EINVAL;

	while (buf_len) {
		unsigned int now = min(AHCI_MAX_DATA_BYTE_COUNT, buf_len);

		ahci_sg->addr = cpu_to_le32(lower_32_bits(buf_dma));
		if (ahci_port->ahci->cap & HOST_CAP_64)
			ahci_sg->addr_hi = cpu_to_le32(upper_32_bits(buf_dma));
		ahci_sg->flags_size = cpu_to_le32(now - 1);

		buf_len -= now;
		buf_dma += now;
		ahci_sg++;
	}

	return sg_count;
}

static int ahci_io(struct ahci_port *ahci_port, u8 *fis, int fis_len, void *rbuf,
		const void *wbuf, int buf_len)
{
	u32 opts;
	int sg_count;
	int ret;
	void *buf;
	dma_addr_t buf_dma;
	enum dma_data_direction dma_dir;

	if (!ahci_link_ok(ahci_port, 1))
		return -EIO;

	if (wbuf) {
		buf = (void *)wbuf;
		dma_dir = DMA_TO_DEVICE;
	} else {
		buf = rbuf;
		dma_dir = DMA_FROM_DEVICE;
	}

	buf_dma = dma_map_single(ahci_port->ahci->dev, buf, buf_len, dma_dir);

	memcpy(ahci_port->cmd_tbl, fis, fis_len);

	sg_count = ahci_fill_sg(ahci_port, buf_dma, buf_len);
	opts = (fis_len >> 2) | (sg_count << 16);
	if (wbuf)
		opts |= CMD_LIST_OPTS_WRITE;
	ahci_fill_cmd_slot(ahci_port, opts);

	ahci_port_write_f(ahci_port, PORT_CMD_ISSUE, 1);

	ret = wait_on_timeout(WAIT_DATAIO,
			(ahci_port_read(ahci_port, PORT_CMD_ISSUE) & 0x1) == 0);

	dma_unmap_single(ahci_port->ahci->dev, buf_dma, buf_len, dma_dir);

	return ret;
}

/*
 * SCSI INQUIRY command operation.
 */
static int ahci_read_id(struct ata_port *ata, void *buf)
{
	struct ahci_port *ahci = container_of(ata, struct ahci_port, ata);

	u8 fis[20] = {
		0x27,			/* Host to device FIS. */
		1 << 7,			/* Command FIS. */
		ATA_CMD_ID_ATA		/* Command byte. */
	};

	return ahci_io(ahci, fis, sizeof(fis), buf, NULL, SECTOR_SIZE);
}

static int ahci_rw(struct ata_port *ata, void *rbuf, const void *wbuf,
		sector_t block, blkcnt_t num_blocks)
{
	struct ahci_port *ahci = container_of(ata, struct ahci_port, ata);
	u8 fis[20] = {
		0x27,			/* Host to device FIS. */
		1 << 7			/* Command FIS. */
	};
	int ret;
	int lba48 = ata_id_has_lba48(ata->id);

	/* Command byte. */
	if (lba48)
		fis[2] = wbuf ? ATA_CMD_WRITE_EXT : ATA_CMD_READ_EXT;
	else
		fis[2] = wbuf ? ATA_CMD_WRITE : ATA_CMD_READ;

	while (num_blocks) {
		int now;

		now = min_t(blkcnt_t, MAX_SATA_BLOCKS_READ_WRITE, num_blocks);

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

static int ahci_read(struct ata_port *ata, void *buf, sector_t block,
		blkcnt_t num_blocks)
{
	return ahci_rw(ata, buf, NULL, block, num_blocks);
}

static int ahci_write(struct ata_port *ata, const void *buf, sector_t block,
		blkcnt_t num_blocks)
{
	return ahci_rw(ata, NULL, buf, block, num_blocks);
}

static int ahci_init_port(struct ahci_port *ahci_port)
{
	u32 val, cmd;
	void *mem;
	dma_addr_t mem_dma;
	int ret;

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

	mem = dma_alloc_coherent(AHCI_PORT_PRIV_DMA_SZ, &mem_dma);
	if (!mem) {
		return -ENOMEM;
	}

	/*
	 * First item in chunk of DMA memory: 32-slot command list,
	 * 32 bytes each in size
	 */
	ahci_port->cmd_slot = mem;
	ahci_port->cmd_slot_dma = mem_dma;

	ahci_port_debug(ahci_port, "cmd_slot = 0x%p (0x%pad)\n",
			ahci_port->cmd_slot, &ahci_port->cmd_slot_dma);

	/*
	 * Second item: Received-FIS area
	 */
	ahci_port->rx_fis = mem + AHCI_CMD_LIST_SZ;
	ahci_port->rx_fis_dma = mem_dma + AHCI_CMD_LIST_SZ;

	/*
	 * Third item: data area for storing a single command
	 * and its scatter-gather table
	 */
	ahci_port->cmd_tbl = mem + AHCI_CMD_LIST_SZ + AHCI_RX_FIS_SZ;
	ahci_port->cmd_tbl_dma = mem_dma + AHCI_CMD_LIST_SZ + AHCI_RX_FIS_SZ;

	ahci_port_debug(ahci_port, "cmd_tbl = 0x%p (0x%pad)\n",
			ahci_port->cmd_tbl, &ahci_port->cmd_tbl_dma);

	ahci_port->cmd_tbl_sg = ahci_port->cmd_tbl + AHCI_CMD_TBL_HDR_SZ;

	ahci_port_write_f(ahci_port, PORT_LST_ADDR, lower_32_bits(ahci_port->cmd_slot_dma));
	if (ahci_port->ahci->cap & HOST_CAP_64)
		ahci_port_write_f(ahci_port, PORT_LST_ADDR_HI, upper_32_bits(ahci_port->cmd_slot_dma));
	ahci_port_write_f(ahci_port, PORT_FIS_ADDR, lower_32_bits(ahci_port->rx_fis_dma));
	if (ahci_port->ahci->cap & HOST_CAP_64)
		ahci_port_write_f(ahci_port, PORT_FIS_ADDR_HI, upper_32_bits(ahci_port->rx_fis_dma));

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
			(ahci_port_read(ahci_port, PORT_SCR_STAT) & PORT_SCR_STAT_DET) == 0x3);
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
			((ahci_port_read(ahci_port, PORT_TFDATA) &
			 (ATA_STATUS_BUSY | ATA_STATUS_DRQ)) == 0) ||
			((ahci_port_read(ahci_port, PORT_SCR_STAT) &
			 PORT_SCR_STAT_DET) == 1));
	if (ret) {
		ahci_port_info(ahci_port, "timeout.\n");
		ret = -ENODEV;
		goto err_init;
	}

	if ((ahci_port_read(ahci_port, PORT_SCR_STAT) & PORT_SCR_STAT_DET) == 1) {
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

	if ((val & PORT_SCR_STAT_DET) == 0x3)
		return 0;

	ret = -ENODEV;

err_init:
	dma_free_coherent(mem, mem_dma, AHCI_PORT_PRIV_DMA_SZ);
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

	speed = (cap & HOST_CAP_ISS) >> 20;
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
	       ((cap & HOST_CAP_NCS) >> 8) + 1,
	       (cap & HOST_CAP_NP) + 1, speed_s, impl, scc_s);

	printf("flags: "
	       "%s%s%s%s%s%s%s"
	       "%s%s%s%s%s%s%s"
	       "%s%s%s%s%s%s%s\n",
	       cap & HOST_CAP_64 ? "64bit " : "",
	       cap & HOST_CAP_NCQ ? "ncq " : "",
	       cap & HOST_CAP_SNTF ? "sntf " : "",
	       cap & HOST_CAP_SMPS ? "ilck " : "",
	       cap & HOST_CAP_SSS ? "stag " : "",
	       cap & HOST_CAP_ALPM ? "pm " : "",
	       cap & HOST_CAP_LED ? "led " : "",
	       cap & HOST_CAP_CLO ? "clo " : "",
	       cap & HOST_CAP_RESERVED ? "nz " : "",
	       cap & HOST_CAP_ONLY ? "only " : "",
	       cap & HOST_CAP_SPM ? "pmp " : "",
	       cap & HOST_CAP_FBS ? "fbss " : "",
	       cap & HOST_CAP_PIO_MULTI ? "pio " : "",
	       cap & HOST_CAP_SSC ? "slum " : "",
	       cap & HOST_CAP_PART ? "part " : "",
	       cap & HOST_CAP_CCC ? "ccc " : "",
	       cap & HOST_CAP_EMS ? "ems " : "",
	       cap & HOST_CAP_SXS ? "sxs " : "",
	       cap2 & HOST_CAP2_APST ? "apst " : "",
	       cap2 & HOST_CAP2_NVMHCI ? "nvmp " : "",
	       cap2 & HOST_CAP2_BOH ? "boh " : "");
}

void ahci_info(struct device *dev)
{
	struct ahci_device *ahci = dev->priv;

	ahci_print_info(ahci);
}

static int ahci_detect(struct device *dev)
{
	struct ahci_device *ahci = dev->priv;
	int n_ports = max_t(int, ahci->n_ports, fls(ahci->port_map));
	int i;

	for (i = 0; i < n_ports; i++) {
		struct ahci_port *ahci_port = &ahci->ports[i];

		if (!(ahci->port_map & (1 << i)))
			continue;

		ata_port_detect(&ahci_port->ata);
	}

	return 0;
}

int ahci_add_host(struct ahci_device *ahci)
{
	u32 tmp, cap_save;
	int n_ports, i, ret;

	ahci->host_flags = ATA_FLAG_SATA
				| ATA_FLAG_NO_LEGACY
				| ATA_FLAG_MMIO
				| ATA_FLAG_PIO_DMA
				| ATA_FLAG_NO_ATAPI;
	ahci->pio_mask = 0x1f;
	ahci->udma_mask = 0x7f;	/* FIXME: assume to support UDMA6 */

	ahci_debug(ahci, "ahci_host_init: start\n");

	cap_save = ahci_ioread(ahci, HOST_CAP);
	cap_save &= (HOST_CAP_SMPS | HOST_CAP_SPM);
	cap_save |= HOST_CAP_SSS;  /* Staggered Spin-up. Not needed. */

	/* global controller reset */
	tmp = ahci_ioread(ahci, HOST_CTL);
	if ((tmp & HOST_RESET) == 0)
		ahci_iowrite_f(ahci, HOST_CTL, tmp | HOST_RESET);

	/*
	 * reset must complete within 1 second, or
	 * the hardware should be considered fried.
	 */
	ret = wait_on_timeout(SECOND, (ahci_ioread(ahci, HOST_CTL) & HOST_RESET) == 0);
	if (ret) {
		ahci_debug(ahci, "controller reset failed (0x%x)\n", tmp);
		return -ENODEV;
	}

	ahci_iowrite_f(ahci, HOST_CTL, HOST_AHCI_EN);
	ahci_iowrite(ahci, HOST_CAP, cap_save);
	ahci_iowrite_f(ahci, HOST_PORTS_IMPL, 0xf);

	ahci->cap = ahci_ioread(ahci, HOST_CAP);
	ahci->port_map = ahci_ioread(ahci, HOST_PORTS_IMPL);
	ahci->n_ports = (ahci->cap & HOST_CAP_NP) + 1;

	ahci_debug(ahci, "cap 0x%x  port_map 0x%x  n_ports %d\n",
	      ahci->cap, ahci->port_map, ahci->n_ports);

	n_ports = max_t(int, ahci->n_ports, fls(ahci->port_map));

	for (i = 0; i < n_ports; i++) {
		struct ahci_port *ahci_port = &ahci->ports[i];

		if (!(ahci->port_map & (1 << i)))
			continue;

		ahci_port->num = i;
		ahci_port->ahci = ahci;
		ahci_port->ata.dev = ahci->dev;
		ahci_port->port_mmio = ahci_port_base(ahci->mmio_base, i);
		ahci_port->ata.ops = &ahci_ops;
		ata_port_register(&ahci_port->ata);
	}

	tmp = ahci_ioread(ahci, HOST_CTL);
	ahci_iowrite(ahci, HOST_CTL, tmp | HOST_IRQ_EN);
	tmp = ahci_ioread(ahci, HOST_CTL);

	ahci->dev->detect = ahci_detect;

	return 0;
}

static int ahci_probe(struct device *dev)
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
MODULE_DEVICE_TABLE(of, ahci_dt_ids);

static struct driver ahci_driver = {
	.name   = "ahci",
	.probe  = ahci_probe,
	.of_compatible = DRV_OF_COMPAT(ahci_dt_ids),
};
device_platform_driver(ahci_driver);
