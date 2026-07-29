// SPDX-License-Identifier: GPL-2.0

#include <dma.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <mci.h>
#include <module.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>

#include "sdhci.h"

struct sdhci_pci_host {
	struct mci_host mci;
	struct sdhci sdhci;
};

static inline struct sdhci_pci_host *to_sdhci_pci_host(struct mci_host *mci)
{
	return container_of(mci, struct sdhci_pci_host, mci);
}

#define PCI_CLASS_SYSTEM_SDHCI_DMA	((PCI_CLASS_SYSTEM_SDHCI << 8) | 0x01)

static const struct pci_device_id pci_ids[] = {
	/* Generic SD host controller */
	{PCI_DEVICE_CLASS(PCI_CLASS_SYSTEM_SDHCI_DMA, PCI_ANY_ID)},
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(pci, pci_ids);

/*****************************************************************************\
 *                                                                           *
 * MCI core callbacks                                                        *
 *                                                                           *
\*****************************************************************************/

static int sdhci_pci_init(struct mci_host *mci, struct device *dev)
{
	struct sdhci_pci_host *host = to_sdhci_pci_host(mci);
	int ret;

	(void)dev;

	ret = sdhci_reset(&host->sdhci, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL,
		     SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE,
		      SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, 0);

	return 0;
}

static void sdhci_pci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct sdhci_pci_host *host = to_sdhci_pci_host(mci);
	u8 val;

	if (ios->clock)
		sdhci_set_clock(&host->sdhci, ios->clock, host->sdhci.max_clk);

	sdhci_set_bus_width(&host->sdhci, ios->bus_width);

	val = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL);
	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;
	sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, val);
}

static int sdhci_pci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd)
{
	struct sdhci_pci_host *host = to_sdhci_pci_host(mci);

	return sdhci_send_command(&host->sdhci, cmd);
}

static int sdhci_pci_card_present(struct mci_host *mci)
{
	struct sdhci_pci_host *host = to_sdhci_pci_host(mci);

	return !!(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) &
		  SDHCI_CARD_PRESENT);
}

static int sdhci_pci_card_write_protected(struct mci_host *mci)
{
	struct sdhci_pci_host *host = to_sdhci_pci_host(mci);

	return !(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) &
		 SDHCI_WRITE_PROTECT);
}

static const struct mci_ops sdhci_pci_ops = {
	.init = sdhci_pci_init,
	.set_ios = sdhci_pci_set_ios,
	.send_cmd = sdhci_pci_send_cmd,
	.card_present = sdhci_pci_card_present,
	.card_write_protected = sdhci_pci_card_write_protected,
};

/*****************************************************************************\
 *                                                                           *
 * Device probing/removal                                                    *
 *                                                                           *
\*****************************************************************************/

static int sdhci_pci_probe(struct pci_dev *pdev,
			   const struct pci_device_id *ent)
{
	struct sdhci_pci_host *host;
	struct mci_host *mci;
	void __iomem *base;
	int ret;

	(void)ent;

	dev_info(&pdev->dev, "SDHCI controller found [%04x:%04x] (rev %x)\n",
		 (int)pdev->vendor, (int)pdev->device, (int)pdev->revision);

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);

	base = pci_iomap(pdev, 0);
	if (!base)
		return dev_err_probe(&pdev->dev, -EBUSY, "failed to map BAR0\n");

	host = xzalloc(sizeof(*host));
	mci = &host->mci;

	host->sdhci.base = base;
	host->sdhci.mci = mci;
	host->sdhci.quirks2 = SDHCI_QUIRK2_BROKEN_HS200;

	mci->hw_dev = &pdev->dev;
	mci->ops = sdhci_pci_ops;
	mci->max_req_size = 0x8000;

	ret = sdhci_setup_host(&host->sdhci);
	if (ret)
		goto err;

	if (host->sdhci.flags & SDHCI_USE_64_BIT_DMA)
		dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	else
		dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));

	ret = sdhci_setup_adma(&host->sdhci);
	if (ret && ret != -ENOTSUPP)
		dev_warn(&pdev->dev, "ADMA setup failed (%pe), falling back to SDMA\n",
			 ERR_PTR(ret));

	mci->f_max = host->sdhci.max_clk;
	if (!mci->f_max)
		mci->f_max = 50000000;
	mci->f_min = mci->f_max / SDHCI_MAX_DIV_SPEC_300;

	pdev->dev.priv = host;

	ret = mci_register(mci);
	if (ret)
		goto err;

	return 0;

err:
	pdev->dev.priv = NULL;
	free(host);
	return ret;
}

static void sdhci_pci_remove(struct pci_dev *pdev)
{
	struct sdhci_pci_host *host = pdev->dev.priv;

	sdhci_release_adma(&host->sdhci);
	pci_clear_master(pdev);
}

static struct pci_driver sdhci_driver = {
	.name =		"sdhci-pci",
	.id_table =	pci_ids,
	.probe =	sdhci_pci_probe,
	.remove =	sdhci_pci_remove,
};

static int __init sdhci_pci_driver_init(void)
{
	return pci_register_driver(&sdhci_driver);
}
module_init(sdhci_pci_driver_init);
