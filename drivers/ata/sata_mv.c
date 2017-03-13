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

#define REG_WINDOW_CONTROL(n)	((n) * 0x10 + 0x30)
#define REG_WINDOW_BASE(n)	((n) * 0x10 + 0x34)

#define REG_EDMA_COMMAND(n)	((n) * 0x2000 + 0x2028)
#define REG_EDMA_COMMAND__EATARST	0x00000004

#define REG_ATA_BASE		0x2100
#define REG_SSTATUS(n)		((n) * 0x2000 + 0x2300)
#define REG_SCONTROL(n)		((n) * 0x2000 + 0x2308)
#define REG_SCONTROL__DET		0x0000000f
#define REG_SCONTROL__DET__INIT		0x00000001
#define REG_SCONTROL__DET__PHYOK	0x00000002
#define REG_SCONTROL__IPM		0x00000f00
#define REG_SCONTROL__IPM__PARTIAL	0x00000100
#define REG_SCONTROL__IPM__SLUMBER	0x00000200

static int mv_sata_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;
	struct ide_port *ide;
	u32 scontrol;
	int ret, i;

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

	ret = ide_port_register(ide);
	if (ret)
		free(ide);

	return ret;
}

static const struct of_device_id mv_sata_dt_ids[] = {
	{
		.compatible = "marvell,armada-370-sata",
	}, {
		/* sentinel */
	}
};

static struct driver_d mv_sata_driver = {
	.name = "mv_sata",
	.probe = mv_sata_probe,
	.of_compatible = mv_sata_dt_ids,
};
device_platform_driver(mv_sata_driver);
