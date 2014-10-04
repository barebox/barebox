#
# Makefile for the PCI bus specific drivers.
#
obj-y		+= pci.o bus.o pci_iomap.o

ccflags-$(CONFIG_PCI_DEBUG) := -DDEBUG

CPPFLAGS += $(ccflags-y)

obj-$(CONFIG_PCI_MVEBU)	+= pci-mvebu.o pci-mvebu-phy.o
obj-$(CONFIG_PCI_TEGRA) += pci-tegra.o
