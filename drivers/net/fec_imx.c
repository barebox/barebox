/*
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
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
 */

#include <common.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <driver.h>
#include <fec.h>
#include <io.h>
#include <clock.h>
#include <xfuncs.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <of_net.h>

#include <asm/mmu.h>

#include <mach/generic.h>
#include <mach/clock.h>
#ifndef CONFIG_ARCH_MXS
# include <mach/iim.h>
#endif

#include "fec_imx.h"

struct fec_frame {
	uint8_t data[1500];	/* actual data */
	int length;		/* actual length */
	int used;		/* buffer in use or not */
	uint8_t head[16];	/* MAC header(6 + 6 + 2) + 2(aligned) */
};

/*
 * MII-interface related functions
 */
static int fec_miibus_read(struct mii_bus *bus, int phyAddr, int regAddr)
{
	struct fec_priv *fec = (struct fec_priv *)bus->priv;

	uint32_t reg;		/* convenient holder for the PHY register */
	uint32_t phy;		/* convenient holder for the PHY */
	uint64_t start;

	writel(((clk_get_rate(fec->clk) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);
	/*
	 * reading from any PHY's register is done by properly
	 * programming the FEC's MII data register.
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);
	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA | phy | reg, fec->regs + FEC_MII_DATA);

	/*
	 * wait for the related interrupt
	 */
	start = get_time_ns();
	while (!(readl(fec->regs + FEC_IEVENT) & FEC_IEVENT_MII)) {
		if (is_timeout(start, MSECOND)) {
			dev_err(&fec->edev.dev, "Read MDIO failed...\n");
			return -1;
		}
	}

	/*
	 * clear mii interrupt bit
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);

	/*
	 * it's now safe to read the PHY's register
	 */
	return readl(fec->regs + FEC_MII_DATA);
}

static int fec_miibus_write(struct mii_bus *bus, int phyAddr,
	int regAddr, u16 data)
{
	struct fec_priv *fec = (struct fec_priv *)bus->priv;

	uint32_t reg;		/* convenient holder for the PHY register */
	uint32_t phy;		/* convenient holder for the PHY */
	uint64_t start;

	writel(((clk_get_rate(fec->clk) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);

	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR |
		FEC_MII_DATA_TA | phy | reg | data, fec->regs + FEC_MII_DATA);

	/*
	 * wait for the MII interrupt
	 */
	start = get_time_ns();
	while (!(readl(fec->regs + FEC_IEVENT) & FEC_IEVENT_MII)) {
		if (is_timeout(start, MSECOND)) {
			dev_err(&fec->edev.dev, "Write MDIO failed...\n");
			return -1;
		}
	}

	/*
	 * clear MII interrupt bit
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);

	return 0;
}

static int fec_rx_task_enable(struct fec_priv *fec)
{
	writel(1 << 24, fec->regs + FEC_R_DES_ACTIVE);
	return 0;
}

static int fec_rx_task_disable(struct fec_priv *fec)
{
	return 0;
}

static int fec_tx_task_enable(struct fec_priv *fec)
{
	writel(1 << 24, fec->regs + FEC_X_DES_ACTIVE);
	return 0;
}

static int fec_tx_task_disable(struct fec_priv *fec)
{
	return 0;
}

/**
 * Swap endianess to send data on an i.MX28 based platform
 * @param buf Pointer to little endian data
 * @param len Size in words (max. 1500 bytes)
 * @return Pointer to the big endian data
 */
static void *imx28_fix_endianess_wr(uint32_t *buf, unsigned wlen)
{
	unsigned u;
	static uint32_t data[376];	/* = 1500 bytes + 4 bytes */

	for (u = 0; u < wlen; u++, buf++)
		data[u] = __swab32(*buf);

	return data;
}

/**
 * Swap endianess to read data on an i.MX28 based platform
 * @param buf Pointer to little endian data
 * @param len Size in words (max. 1500 bytes)
 *
 * TODO: Check for the risk of destroying some other data behind the buffer
 * if its size is not a multiple of 4.
 */
static void imx28_fix_endianess_rd(uint32_t *buf, unsigned wlen)
{
	unsigned u;

	for (u = 0; u < wlen; u++, buf++)
		*buf = __swab32(*buf);
}

/**
 * Initialize receive task's buffer descriptors
 * @param[in] fec all we know about the device yet
 * @param[in] count receive buffer count to be allocated
 * @param[in] size size of each receive buffer
 * @return 0 on success
 *
 * For this task we need additional memory for the data buffers. And each
 * data buffer requires some alignment. Thy must be aligned to a specific
 * boundary each (DB_DATA_ALIGNMENT).
 */
static int fec_rbd_init(struct fec_priv *fec, int count, int size)
{
	int ix;

	for (ix = 0; ix < count; ix++) {
		writew(FEC_RBD_EMPTY, &fec->rbd_base[ix].status);
		writew(0, &fec->rbd_base[ix].data_length);
	}
	/*
	 * mark the last RBD to close the ring
	 */
	writew(FEC_RBD_WRAP | FEC_RBD_EMPTY, &fec->rbd_base[ix - 1].status);
	fec->rbd_index = 0;

	return 0;
}

/**
 * Initialize transmit task's buffer descriptors
 * @param[in] fec all we know about the device yet
 *
 * Transmit buffers are created externally. We only have to init the BDs here.\n
 * Note: There is a race condition in the hardware. When only one BD is in
 * use it must be marked with the WRAP bit to use it for every transmit.
 * This bit in combination with the READY bit results into double transmit
 * of each data buffer. It seems the state machine checks READY earlier then
 * resetting it after the first transfer.
 * Using two BDs solves this issue.
 */
static void fec_tbd_init(struct fec_priv *fec)
{
	writew(0x0000, &fec->tbd_base[0].status);
	writew(FEC_TBD_WRAP, &fec->tbd_base[1].status);
	fec->tbd_index = 0;
}

/**
 * Mark the given read buffer descriptor as free
 * @param[in] last 1 if this is the last buffer descriptor in the chain, else 0
 * @param[in] pRbd buffer descriptor to mark free again
 */
static void fec_rbd_clean(int last, struct buffer_descriptor __iomem *pRbd)
{
	/*
	 * Reset buffer descriptor as empty
	 */
	if (last)
		writew(FEC_RBD_WRAP | FEC_RBD_EMPTY, &pRbd->status);
	else
		writew(FEC_RBD_EMPTY, &pRbd->status);
	/*
	 * no data in it
	 */
	writew(0, &pRbd->data_length);
}

static int fec_get_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	return -1;
}

static int fec_set_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;

	/*
	 * Set physical address
	 */
	writel((mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3], fec->regs + FEC_PADDR1);
	writel((mac[4] << 24) + (mac[5] << 16) + 0x8808, fec->regs + FEC_PADDR2);

        return 0;
}

static int fec_init(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	u32 rcntl, xwmrk;

	/*
	 * Clear FEC-Lite interrupt event register(IEVENT)
	 */
	writel(0xffffffff, fec->regs + FEC_IEVENT);

	/*
	 * Set interrupt mask register
	 */
	writel(0x00000000, fec->regs + FEC_IMASK);

	/*
	 * Set FEC-Lite receive control register(R_CNTRL):
	 */
	rcntl = FEC_R_CNTRL_MAX_FL(1518);

	rcntl |= FEC_R_CNTRL_MII_MODE;
	/*
	 * Set MII_SPEED = (1/(mii_speed * 2)) * System Clock
	 * and do not drop the Preamble.
	 */
	writel(((clk_get_rate(fec->clk) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);

	if (fec->interface == PHY_INTERFACE_MODE_RMII) {
		if (fec_is_imx28(fec) || fec_is_imx6(fec)) {
			rcntl |= FEC_R_CNTRL_RMII_MODE | FEC_R_CNTRL_FCE |
				FEC_R_CNTRL_NO_LGTH_CHECK;
		} else {
			/* disable the gasket and wait */
			writel(0, fec->regs + FEC_MIIGSK_ENR);
			while (readl(fec->regs + FEC_MIIGSK_ENR) & FEC_MIIGSK_ENR_READY)
				udelay(1);

			/* configure the gasket for RMII, 50 MHz, no loopback, no echo */
			writel(FEC_MIIGSK_CFGR_IF_MODE_RMII, fec->regs + FEC_MIIGSK_CFGR);

			/* re-enable the gasket */
			writel(FEC_MIIGSK_ENR_EN, fec->regs + FEC_MIIGSK_ENR);
		}
	}

	if (fec->interface == PHY_INTERFACE_MODE_RGMII)
		rcntl |= 1 << 6;

	writel(rcntl, fec->regs + FEC_R_CNTRL);

	/*
	 * Set Opcode/Pause Duration Register
	 */
	writel(0x00010020, fec->regs + FEC_OP_PAUSE);

	xwmrk = 0x2;

	/* set ENET tx at store and forward mode */
	if (fec_is_imx6(fec))
		xwmrk |= 1 << 8;

	writel(xwmrk, fec->regs + FEC_X_WMRK);

	/*
	 * Set multicast address filter
	 */
	writel(0, fec->regs + FEC_IADDR1);
	writel(0, fec->regs + FEC_IADDR2);
	writel(0, fec->regs + FEC_GADDR1);
	writel(0, fec->regs + FEC_GADDR2);

	/* size of each buffer */
	writel(FEC_MAX_PKT_SIZE, fec->regs + FEC_EMRBR);

	return 0;
}

static void fec_update_linkspeed(struct eth_device *edev)
{
	struct fec_priv *fec = (struct fec_priv *)edev->priv;
	int speed = edev->phydev->speed;
	u32 rcntl = readl(fec->regs + FEC_R_CNTRL) & ~FEC_R_CNTRL_RMII_10T;
	u32 ecntl = readl(fec->regs + FEC_ECNTRL) & ~FEC_ECNTRL_SPEED;

	if (speed == SPEED_10)
		rcntl |= FEC_R_CNTRL_RMII_10T;

	if (speed == SPEED_1000)
		ecntl |= FEC_ECNTRL_SPEED;

	writel(rcntl, fec->regs + FEC_R_CNTRL);
	writel(ecntl, fec->regs + FEC_ECNTRL);
}

/**
 * Start the FEC engine
 * @param[in] edev Our device to handle
 */
static int fec_open(struct eth_device *edev)
{
	struct fec_priv *fec = (struct fec_priv *)edev->priv;
	int ret;
	u32 ecr;

	ret = phy_device_connect(edev, &fec->miibus, fec->phy_addr,
				 fec_update_linkspeed, fec->phy_flags,
				 fec->interface);
	if (ret)
		return ret;

	if (fec->phy_init)
		fec->phy_init(edev->phydev);

	/*
	 * Initialize RxBD/TxBD rings
	 */
	fec_rbd_init(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);
	fec_tbd_init(fec);

	/* full-duplex, heartbeat disabled */
	writel(1 << 2, fec->regs + FEC_X_CNTRL);
	fec->rbd_index = 0;

	/*
	 * Enable FEC-Lite controller
	 */
	ecr = FEC_ECNTRL_ETHER_EN;

	/* Enable Swap to support little-endian device */
	if (fec_is_imx6(fec))
		ecr |= 0x100;

	writel(ecr, fec->regs + FEC_ECNTRL);
	/*
	 * Enable SmartDMA receive task
	 */
	fec_rx_task_enable(fec);

	return 0;
}

/**
 * Halt the FEC engine
 * @param[in] dev Our device to handle
 */
static void fec_halt(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	uint64_t tmo;

	/* issue graceful stop command to the FEC transmitter if necessary */
	writel(readl(fec->regs + FEC_X_CNTRL) | FEC_ECNTRL_RESET,
			fec->regs + FEC_X_CNTRL);

	/* wait for graceful stop to register */
	tmo = get_time_ns();
	while (!(readl(fec->regs + FEC_IEVENT) & FEC_IEVENT_GRA)) {
		if (is_timeout(tmo, 1 * SECOND)) {
			dev_err(&dev->dev, "graceful stop timeout\n");
			break;
		}
	}

	/* Disable SmartDMA tasks */
	fec_tx_task_disable(fec);
	fec_rx_task_disable(fec);

	/*
	 * Disable the Ethernet Controller
	 * Note: this will also reset the BD index counter!
	 */
	writel(0, fec->regs + FEC_ECNTRL);
	fec->rbd_index = 0;
	fec->tbd_index = 0;
}

/**
 * Transmit one frame
 * @param[in] dev Our ethernet device to handle
 * @param[in] eth_data Pointer to the data to be transmitted
 * @param[in] data_length Data count in bytes
 * @return 0 on success
 */
static int fec_send(struct eth_device *dev, void *eth_data, int data_length)
{
	unsigned int status;
	uint64_t tmo;

	/*
	 * This routine transmits one frame.  This routine only accepts
	 * 6-byte Ethernet addresses.
	 */
	struct fec_priv *fec = (struct fec_priv *)dev->priv;

	/* Check for valid length of data. */
	if ((data_length > 1500) || (data_length <= 0)) {
		dev_err(&dev->dev, "Payload (%d) to large!\n", data_length);
		return -1;
	}

	if ((uint32_t)eth_data & (DB_DATA_ALIGNMENT-1))
		dev_warn(&dev->dev, "Transmit data not aligned: %p!\n", eth_data);

	/*
	 * Setup the transmit buffer
	 * Note: We are always using the first buffer for transmission,
	 * the second will be empty and only used to stop the DMA engine
	 */
	if (fec_is_imx28(fec))
		eth_data = imx28_fix_endianess_wr(eth_data, (data_length + 3) >> 2);

	writew(data_length, &fec->tbd_base[fec->tbd_index].data_length);

	writel((uint32_t)(eth_data), &fec->tbd_base[fec->tbd_index].data_pointer);
	dma_flush_range((unsigned long)eth_data,
			(unsigned long)(eth_data + data_length));
	/*
	 * update BD's status now
	 * This block:
	 * - is always the last in a chain (means no chain)
	 * - should transmitt the CRC
	 * - might be the last BD in the list, so the address counter should
	 *   wrap (-> keep the WRAP flag)
	 */
	status = readw(&fec->tbd_base[fec->tbd_index].status) & FEC_TBD_WRAP;
	status |= FEC_TBD_LAST | FEC_TBD_TC | FEC_TBD_READY;
	writew(status, &fec->tbd_base[fec->tbd_index].status);
	/* Enable SmartDMA transmit task */
	fec_tx_task_enable(fec);

	/* wait until frame is sent */
	tmo = get_time_ns();
	while (readw(&fec->tbd_base[fec->tbd_index].status) & FEC_TBD_READY) {
		if (is_timeout(tmo, 1 * SECOND)) {
			dev_err(&dev->dev, "transmission timeout\n");
			break;
		}
	}

	/* for next transmission use the other buffer */
	if (fec->tbd_index)
		fec->tbd_index = 0;
	else
		fec->tbd_index = 1;

	return 0;
}

/**
 * Pull one frame from the card
 * @param[in] dev Our ethernet device to handle
 * @return Length of packet read
 */
static int fec_recv(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	struct buffer_descriptor __iomem *rbd = &fec->rbd_base[fec->rbd_index];
	uint32_t ievent;
	int frame_length, len = 0;
	struct fec_frame *frame;
	uint16_t bd_status;

	/*
	 * Check if any critical events have happened
	 */
	ievent = readl(fec->regs + FEC_IEVENT);
	writel(ievent, fec->regs + FEC_IEVENT);

	if (ievent & FEC_IEVENT_BABT) {
		/* BABT, Rx/Tx FIFO errors */
		fec_halt(dev);
		fec_init(dev);
		dev_err(&dev->dev, "some error: 0x%08x\n", ievent);
		return 0;
	}
	if (!fec_is_imx28(fec)) {
		if (ievent & FEC_IEVENT_HBERR) {
			/* Heartbeat error */
			writel(readl(fec->regs + FEC_X_CNTRL) | 0x1,
					fec->regs + FEC_X_CNTRL);
		}
	}
	if (ievent & FEC_IEVENT_GRA) {
		/* Graceful stop complete */
		if (readl(fec->regs + FEC_X_CNTRL) & 0x00000001) {
			fec_halt(dev);
			writel(readl(fec->regs + FEC_X_CNTRL) & ~0x00000001,
					fec->regs + FEC_X_CNTRL);
			fec_init(dev);
		}
	}

	/*
	 * ensure reading the right buffer status
	 */
	bd_status = readw(&rbd->status);

	if (!(bd_status & FEC_RBD_EMPTY)) {
		if ((bd_status & FEC_RBD_LAST) && !(bd_status & FEC_RBD_ERR) &&
			((readw(&rbd->data_length) - 4) > 14)) {

			if (fec_is_imx28(fec))
				imx28_fix_endianess_rd(
					phys_to_virt(readl(&rbd->data_pointer)),
					(readw(&rbd->data_length) + 3) >> 2);

			/*
			 * Get buffer address and size
			 */
			frame = phys_to_virt(readl(&rbd->data_pointer));
			frame_length = readw(&rbd->data_length) - 4;
			net_receive(frame->data, frame_length);
			len = frame_length;
		} else {
			if (bd_status & FEC_RBD_ERR) {
				dev_warn(&dev->dev, "error frame: 0x%p 0x%08x\n", rbd, bd_status);
			}
		}
		/*
		 * free the current buffer, restart the engine
		 * and move forward to the next buffer
		 */
		fec_rbd_clean(fec->rbd_index == (FEC_RBD_NUM - 1) ? 1 : 0, rbd);
		fec_rx_task_enable(fec);
		fec->rbd_index = (fec->rbd_index + 1) % FEC_RBD_NUM;
	}

	return len;
}

static int fec_alloc_receive_packets(struct fec_priv *fec, int count, int size)
{
	void *p;
	int i;

	/* reserve data memory and consider alignment */
	p = dma_alloc_coherent(size * count);
	if (!p)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		writel(virt_to_phys(p), &fec->rbd_base[i].data_pointer);
		p += size;
	}

	return 0;
}

#ifdef CONFIG_OFDEVICE
static int fec_probe_dt(struct device_d *dev, struct fec_priv *fec)
{
	int ret;

	ret = of_get_phy_mode(dev->device_node);
	if (ret < 0)
		fec->interface = PHY_INTERFACE_MODE_MII;
	else
		fec->interface = ret;

	return 0;
}
#else
static int fec_probe_dt(struct device_d *dev, struct fec_priv *fec)
{
	return -ENODEV;
}
#endif
static int fec_probe(struct device_d *dev)
{
        struct fec_platform_data *pdata = (struct fec_platform_data *)dev->platform_data;
        struct eth_device *edev;
	struct fec_priv *fec;
	void *base;
	int ret;
	enum fec_type type;

	ret = dev_get_drvdata(dev, (unsigned long *)&type);
	if (ret)
		return ret;

	fec = xzalloc(sizeof(*fec));
	fec->type = type;
	edev = &fec->edev;
	dev->priv = fec;
	edev->priv = fec;
	edev->open = fec_open;
	edev->send = fec_send;
	edev->recv = fec_recv;
	edev->halt = fec_halt;
	edev->get_ethaddr = fec_get_hwaddr;
	edev->set_ethaddr = fec_set_hwaddr;
	edev->parent = dev;

	fec->clk = clk_get(dev, NULL);
	if (IS_ERR(fec->clk)) {
		ret = PTR_ERR(fec->clk);
		goto err_free;
	}

	clk_enable(fec->clk);

	fec->regs = dev_request_mem_region(dev, 0);

	/* Reset chip. */
	writel(FEC_ECNTRL_RESET, fec->regs + FEC_ECNTRL);
	while(readl(fec->regs + FEC_ECNTRL) & 1) {
		udelay(10);
	}

	/*
	 * reserve memory for both buffer descriptor chains at once
	 * Datasheet forces the startaddress of each chain is 16 byte aligned
	 */
	base = dma_alloc_coherent((2 + FEC_RBD_NUM) *
			sizeof(struct buffer_descriptor));
	fec->rbd_base = base;
	base += FEC_RBD_NUM * sizeof(struct buffer_descriptor);
	fec->tbd_base = base;

	writel(virt_to_phys(fec->tbd_base), fec->regs + FEC_ETDSR);
	writel(virt_to_phys(fec->rbd_base), fec->regs + FEC_ERDSR);

	fec_alloc_receive_packets(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);

	if (dev->device_node) {
		ret = fec_probe_dt(dev, fec);
		fec->phy_addr = -1;
	} else if (pdata) {
		fec->interface = pdata->xcv_type;
		fec->phy_init = pdata->phy_init;
		fec->phy_addr = pdata->phy_addr;
	} else {
		fec->interface = PHY_INTERFACE_MODE_MII;
		fec->phy_addr = -1;
	}

	if (ret)
		goto err_free;

	fec_init(edev);

	fec->miibus.read = fec_miibus_read;
	fec->miibus.write = fec_miibus_write;

	fec->miibus.priv = fec;
	fec->miibus.parent = dev;

	mdiobus_register(&fec->miibus);

	eth_register(edev);
	return 0;

err_free:
	free(fec);
	return ret;
}

static void fec_remove(struct device_d *dev)
{
	struct fec_priv *fec = dev->priv;

	fec_halt(&fec->edev);
}

static __maybe_unused struct of_device_id imx_fec_dt_ids[] = {
	{
		.compatible = "fsl,imx25-fec",
		.data = FEC_TYPE_IMX27,
	}, {
		.compatible = "fsl,imx27-fec",
		.data = FEC_TYPE_IMX27,
	}, {
		.compatible = "fsl,imx28-fec",
		.data = FEC_TYPE_IMX28,
	}, {
		.compatible = "fsl,imx6q-fec",
		.data = FEC_TYPE_IMX6,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_fec_ids[] = {
	{
		.name = "imx27-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX27,
	}, {
		.name = "imx28-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX28,
	}, {
		.name = "imx6-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX6,
	}, {
		/* sentinel */
	},
};

/**
 * Driver description for registering
 */
static struct driver_d fec_driver = {
	.name   = "fec_imx",
	.probe  = fec_probe,
	.remove = fec_remove,
	.of_compatible = DRV_OF_COMPAT(imx_fec_dt_ids),
	.id_table = imx_fec_ids,
};
device_platform_driver(fec_driver);

/**
 * @file
 * @brief Network driver for FreeScale's FEC implementation.
 * This type of hardware can be found on i.MX27 CPUs
 */
