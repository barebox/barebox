// SPDX-License-Identifier: GPL-2.0+
/*
 * ENETC ethernet controller driver
 * Copyright 2019 NXP
 */

#include <common.h>
#include <net.h>
#include <linux/phy.h>
#include <linux/pci.h>
#include <io.h>
#include <linux/mdio.h>

#include "fsl_enetc.h"

static void enetc_mdio_wait_bsy(struct enetc_mdio_priv *priv)
{
	int to = 10000;

	while ((enetc_read(priv, ENETC_MDIO_CFG) & ENETC_EMDIO_CFG_BSY) &&
	       --to)
		cpu_relax();
}

int enetc_mdio_read_priv(struct enetc_mdio_priv *priv, int addr, int devad,
			 int reg)
{
	if (devad == MDIO_DEVAD_NONE)
		enetc_write(priv, ENETC_MDIO_CFG, ENETC_EMDIO_CFG_C22);
	else
		enetc_write(priv, ENETC_MDIO_CFG, ENETC_EMDIO_CFG_C45);
	enetc_mdio_wait_bsy(priv);

	if (devad == MDIO_DEVAD_NONE) {
		enetc_write(priv, ENETC_MDIO_CTL, ENETC_MDIO_CTL_READ |
			    (addr << 5) | reg);
	} else {
		enetc_write(priv, ENETC_MDIO_CTL, (addr << 5) + devad);
		enetc_mdio_wait_bsy(priv);

		enetc_write(priv, ENETC_MDIO_STAT, reg);
		enetc_mdio_wait_bsy(priv);

		enetc_write(priv, ENETC_MDIO_CTL, ENETC_MDIO_CTL_READ |
			    (addr << 5) | devad);
	}

	enetc_mdio_wait_bsy(priv);
	if (enetc_read(priv, ENETC_MDIO_CFG) & ENETC_EMDIO_CFG_RD_ER)
		return ENETC_MDIO_READ_ERR;

	return enetc_read(priv, ENETC_MDIO_DATA);
}

int enetc_mdio_write_priv(struct enetc_mdio_priv *priv, int addr, int devad,
			  int reg, u16 val)
{
	if (devad == MDIO_DEVAD_NONE)
		enetc_write(priv, ENETC_MDIO_CFG, ENETC_EMDIO_CFG_C22);
	else
		enetc_write(priv, ENETC_MDIO_CFG, ENETC_EMDIO_CFG_C45);
	enetc_mdio_wait_bsy(priv);

	if (devad != MDIO_DEVAD_NONE) {
		enetc_write(priv, ENETC_MDIO_CTL, (addr << 5) + devad);
		enetc_write(priv, ENETC_MDIO_STAT, reg);
	} else {
		enetc_write(priv, ENETC_MDIO_CTL, (addr << 5) + reg);
	}
	enetc_mdio_wait_bsy(priv);

	enetc_write(priv, ENETC_MDIO_DATA, val);
	enetc_mdio_wait_bsy(priv);

	return 0;
}

static int enetc_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct enetc_mdio_priv *priv = bus->priv;

	return enetc_mdio_read_priv(priv, addr, MDIO_DEVAD_NONE, reg);
}

static int enetc_mdio_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct enetc_mdio_priv *priv = bus->priv;

	return enetc_mdio_write_priv(priv, addr, MDIO_DEVAD_NONE, reg, val);
}

static int enetc_mdio_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct enetc_mdio_priv *priv;

	pci_enable_device(pdev);
	pci_set_master(pdev);

	priv = xzalloc(sizeof(*priv));

	priv->regs_base = pci_iomap(pdev, 0);
	if (!priv->regs_base) {
		dev_err(&pdev->dev, "failed to map BAR0\n");
		return -EINVAL;
	}

	priv->regs_base += ENETC_MDIO_BASE;

	priv->bus.read = enetc_mdio_read;
	priv->bus.write = enetc_mdio_write;
	priv->bus.parent = &pdev->dev;
	priv->bus.priv = priv;

	return mdiobus_register(&priv->bus);
}

static DEFINE_PCI_DEVICE_TABLE(enetc_mdio_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_ENETC_MDIO) },
        { },
};

static struct pci_driver enetc_mdio_driver = {
        .name = "fsl_enetc_mdio",
        .id_table = enetc_mdio_pci_tbl,
        .probe = enetc_mdio_probe,
};
device_pci_driver(enetc_mdio_driver);
