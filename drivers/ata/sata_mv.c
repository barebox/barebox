// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <clock.h>
#include <driver.h>
#include <init.h>
#include <ata_drive.h>

#include <asm/io.h>

/* This can/should be moved to a more generic place */
static void ata_ioports_init(struct ata_ioports *io,
			     void *base_data, void *base_reg, void *base_alt,
			     unsigned stride)
{
	/* io->cmd_addr is unused */;
	io->data_addr = base_data;
	io->error_addr = base_reg + 1 * stride;
	/* io->feature_addr is unused */
	io->nsect_addr = base_reg + 2 * stride;
	io->lbal_addr = base_reg + 3 * stride;
	io->lbam_addr = base_reg + 4 * stride;
	io->lbah_addr = base_reg + 5 * stride;
	io->device_addr = base_reg + 6 * stride;
	io->status_addr = base_reg + 7 * stride;
	io->command_addr = base_reg + 7 * stride;
	/* io->altstatus_addr is unused */

	if (base_alt)
		io->ctl_addr = base_alt;
	else
		io->ctl_addr = io->status_addr;

	/* io->alt_dev_addr is unused */
}

#define REG_WINDOW_CONTROL(n)		((n) * 0x10 + 0x30)
#define REG_WINDOW_BASE(n)		((n) * 0x10 + 0x34)

#define REG_EDMA_COMMAND(n)		((n) * 0x2000 + 0x2028)
#define EDMA_EN				(1 << 0)	/* enable EDMA */
#define EDMA_DS				(1 << 1)	/* disable EDMA; self-negated */
#define REG_EDMA_COMMAND__EATARST	0x00000004
#define REG_EDMA_IORDY_TMOUT(n)		((n) * 0x2000 + 0x2034)
#define REG_SATA_IFCFG(n)		((n) * 0x2000 + 0x2050)
#define REG_SATA_IFCFG_GEN2EN		(1 << 7)

#define REG_ATA_BASE			0x2100
#define REG_SSTATUS(n)			((n) * 0x2000 + 0x2300)
#define REG_SERROR(n)			((n) * 0x2000 + 0x2304)
#define REG_SERROR_MASK			0x03fe0000
#define REG_SCONTROL(n)			((n) * 0x2000 + 0x2308)
#define REG_SCONTROL__DET		0x0000000f
#define REG_SCONTROL__DET__INIT		0x00000001
#define REG_SCONTROL__DET__PHYOK	0x00000002
#define REG_SCONTROL__IPM		0x00000f00
#define REG_SCONTROL__IPM__PARTIAL	0x00000100
#define REG_SCONTROL__IPM__SLUMBER	0x00000200

#define PHY_MODE3			0x310
#define	PHY_MODE4			0x314	/* requires read-after-write */
#define PHY_MODE9_GEN2			0x398
#define	PHY_MODE9_GEN1			0x39c

static void mv_soc_65n_phy_errata(void __iomem *base)
{
	u32 reg;

	reg = readl(base + PHY_MODE3);
	reg &= ~(0x3 << 27);	/* SELMUPF (bits 28:27) to 1 */
	reg |= (0x1 << 27);
	reg &= ~(0x3 << 29);	/* SELMUPI (bits 30:29) to 1 */
	reg |= (0x1 << 29);
	writel(reg, base + PHY_MODE3);

	reg = readl(base + PHY_MODE4);
	reg &= ~0x1;	/* SATU_OD8 (bit 0) to 0, reserved bit 16 must be set */
	reg |= (0x1 << 16);
	writel(reg, base + PHY_MODE4);

	reg = readl(base + PHY_MODE9_GEN2);
	reg &= ~0xf;	/* TXAMP[3:0] (bits 3:0) to 8 */
	reg |= 0x8;
	reg &= ~(0x1 << 14);	/* TXAMP[4] (bit 14) to 0 */
	writel(reg, base + PHY_MODE9_GEN2);

	reg = readl(base + PHY_MODE9_GEN1);
	reg &= ~0xf;	/* TXAMP[3:0] (bits 3:0) to 8 */
	reg |= 0x8;
	reg &= ~(0x1 << 14);	/* TXAMP[4] (bit 14) to 0 */
	writel(reg, base + PHY_MODE9_GEN1);
}

static int mv_sata_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;
	struct ide_port *ide;
	u32 try_again = 0;
	u32 scontrol;
	int ret, i;
	u32 tmp;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "Failed to request mem resources\n");
		return PTR_ERR(iores);
	}
	base = IOMEM(iores->start);

	/* disable MBus windows */
	for (i = 0; i < 4; ++i) {
		writel(0, base + REG_WINDOW_CONTROL(i));
		writel(0, base + REG_WINDOW_BASE(i));
	}

	/* enable first window */
	writel(0x7fff0e01, base + REG_WINDOW_CONTROL(0));
	writel(0, base + REG_WINDOW_BASE(0));

again:
	/* Clear SError */
	writel(0x0, base + REG_SERROR(0));
	/* disable EDMA */
	writel(EDMA_DS, base + REG_EDMA_COMMAND(0));
	/* Wait for the chip to confirm eDMA is off. */
	ret = wait_on_timeout(10 * MSECOND,
				(readl(base + REG_EDMA_COMMAND(0)) & EDMA_EN) == 0);
	if (ret) {
		dev_err(dev, "Failed to wait for eDMA off (sstatus=0x%08x)\n",
			readl(base + REG_SSTATUS(0)));
		return ret;
	}

	/* increase IORdy signal timeout */
	writel(0x800, base + REG_EDMA_IORDY_TMOUT(0));
	/* set GEN2i Speed */
	tmp = readl(base + REG_SATA_IFCFG(0));
	tmp |= REG_SATA_IFCFG_GEN2EN;
	writel(tmp, base + REG_SATA_IFCFG(0));

	mv_soc_65n_phy_errata(base);

	/* strobe for hard-reset */
	writel(REG_EDMA_COMMAND__EATARST, base + REG_EDMA_COMMAND(0));
	writel(REG_EDMA_COMMAND__EATARST, base + REG_EDMA_COMMAND(0));
	udelay(25);
	writel(0x0, base + REG_EDMA_COMMAND(0));

	scontrol = readl(base + REG_SCONTROL(0));
	scontrol &= ~(REG_SCONTROL__DET | REG_SCONTROL__IPM);
	/* disable power management */
	scontrol |= REG_SCONTROL__IPM__PARTIAL | REG_SCONTROL__IPM__SLUMBER;

	/* perform interface communication initialization */
	writel(scontrol | REG_SCONTROL__DET__INIT, base + REG_SCONTROL(0));
	writel(scontrol, base + REG_SCONTROL(0));

	ret = wait_on_timeout(10 * MSECOND,
			      (readl(base + REG_SSTATUS(0)) & REG_SCONTROL__DET) == (REG_SCONTROL__DET__INIT | REG_SCONTROL__DET__PHYOK));
	if (ret) {
		dev_err(dev, "Failed to wait for phy (sstatus=0x%08x)\n",
			readl(base + REG_SSTATUS(0)));
		return ret;
	}

	ide = xzalloc(sizeof(*ide));

	ide->port.dev = dev;

	ata_ioports_init(&ide->io, base + REG_ATA_BASE, base + REG_ATA_BASE,
			 NULL, 4);

	dev->priv = ide;

	/* enable EDMA */
	writel(EDMA_EN, base + REG_EDMA_COMMAND(0));

	ret = ide_port_register(ide);
	if (ret)
		free(ide);

	/*
	 * Under most conditions the above is enough and works as expected.
	 * With some specific hardware combinations, the setup fails however
	 * leading to an unusable SATA drive. From the error status bits it
	 * was not obvious what exactly went wrong.
	 * The ARMADA-XP datasheet advices to hard-reset the SATA core and
	 * drive and try again.
	 * When this happens, just try again multiple times, to give the drive
	 * some time to reach a stable state. If after 5 (randomly chosen) tries,
	 * the drive still doesn't work, just give up on it.
	 */
	tmp = readl(base + REG_SERROR(0));
	if (tmp & REG_SERROR_MASK) {
		try_again++;
		if (try_again > 5)
			return -ENODEV;
		dev_dbg(dev, "PHY layer error. Try again. (serror=0x%08x)\n", tmp);
		if (ide->port.initialized) {
			blockdevice_unregister(&ide->port.blk);
			unregister_device(&ide->port.class_dev);
		}

		mdelay(100);
		goto again;
	}

	return ret;
}

static const struct of_device_id mv_sata_dt_ids[] = {
	{
		.compatible = "marvell,armada-370-sata",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, mv_sata_dt_ids);

static struct driver mv_sata_driver = {
	.name = "mv_sata",
	.probe = mv_sata_probe,
	.of_compatible = mv_sata_dt_ids,
};
device_platform_driver(mv_sata_driver);
