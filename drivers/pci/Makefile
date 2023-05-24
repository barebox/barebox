# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the PCI bus specific drivers.
#
obj-y		+= pci.o bus.o pci_iomap.o

ccflags-$(CONFIG_PCI_DEBUG) := -DDEBUG

obj-$(CONFIG_PCI_MVEBU)	+= pci-mvebu.o pci-mvebu-phy.o
obj-$(CONFIG_PCI_TEGRA) += pci-tegra.o
obj-$(CONFIG_PCIE_DW) += pcie-designware.o pcie-designware-host.o
obj-$(CONFIG_PCI_IMX6) += pci-imx6.o
obj-$(CONFIG_PCI_LAYERSCAPE) += pci-layerscape.o
obj-$(CONFIG_PCI_EFI) += pci-efi.o
obj-$(CONFIG_PCI_ECAM_GENERIC) += pci-ecam-generic.o
obj-$(CONFIG_PCI_ROCKCHIP) += pcie-dw-rockchip.o
