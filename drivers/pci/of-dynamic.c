// SPDX-License-Identifier: GPL-2.0
/*
 * Synthesize the minimal devicetree nodes Linux needs to attach properties
 * (most notably mac-address) to runtime-enumerated PCI devices.
 *
 * Loosely modelled on Linux's CONFIG_PCI_DYNAMIC_OF_NODES, but trimmed to
 * just node creation + a "reg" property. Linux discovers PCI topology from
 * hardware, so other properties it would otherwise read (compatible,
 * bus-range, ranges, #address-cells/#size-cells, interrupts, ...) are
 * either redundant or recomputed from config space at probe time. Keeping
 * the generator small keeps the bug surface small.
 *
 * The only thing barebox must put in DT for Linux to find the node is:
 *   - a child of the parent bus' DT node, with
 *   - reg cell[0] = (bus << 16) | (devfn << 8) so of_pci_get_devfn() works.
 *
 * Linux uses of_changeset because its live kernel devicetree is treated
 * as immutable; barebox is free to mutate the live tree directly, so the
 * changeset layer is dropped.
 */

#define pr_fmt(fmt) "PCI: OF: " fmt

#include <common.h>
#include <init.h>
#include <of.h>
#include <of_pci.h>
#include <malloc.h>
#include <linux/pci.h>

static int of_pci_fill_node(struct device_node *np, struct pci_dev *pdev)
{
	u32 reg[5] = { 0 };
	int ret;

	/* Config-space tag (space = 0); only bus/devfn matter for matching. */
	reg[0] = (pdev->bus->number << 16) | (pdev->devfn << 8);
	ret = of_property_write_u32_array(np, "reg", reg, ARRAY_SIZE(reg));
	if (ret)
		return ret;

	if (pci_is_bridge(pdev)) {
		ret = of_property_write_u32(np, "#address-cells", 3);
		if (ret)
			return ret;
		ret = of_property_write_u32(np, "#size-cells", 2);
		if (ret)
			return ret;
	}

	return 0;
}

void of_pci_make_dev_node(struct pci_dev *pdev)
{
	struct device_node *parent_np, *np;
	struct device *parent_dev;
	char *name;

	if (pdev->dev.of_node)
		return;

	parent_dev = pdev->bus->self ? &pdev->bus->self->dev
				     : pdev->bus->host->parent;
	if (!parent_dev || !parent_dev->of_node)
		return;
	parent_np = parent_dev->of_node;

	name = xasprintf("%s@%x,%x",
			 pci_is_bridge(pdev) ? "pci" : "dev",
			 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	np = of_new_node(parent_np, name);
	free(name);
	if (!np)
		return;

	if (of_pci_fill_node(np, pdev)) {
		of_delete_node(np);
		return;
	}

	pdev->dev.of_node = np;

	dev_dbg(&pdev->dev, "created OF node %pOF\n", np);
}

static struct device_node *
of_pci_fixup_dev_node(struct device_node *parent_np, struct pci_dev *pdev)
{
	struct device_node *np;
	char *name;
	u32 reg;

	/*
	 * Match by devfn before creating a new node: any existing child
	 * whose reg[0] devfn byte matches refers to the same hardware,
	 * regardless of node name.
	 */
	for_each_child_of_node(parent_np, np) {
		if (of_property_read_u32_array(np, "reg", &reg, 1))
			continue;
		if (((reg >> 8) & 0xff) == pdev->devfn)
			return np;
	}

	name = xasprintf("%s@%x,%x",
			 pci_is_bridge(pdev) ? "pci" : "dev",
			 PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	np = of_new_node(parent_np, name);
	free(name);
	if (!np)
		return NULL;

	if (of_pci_fill_node(np, pdev)) {
		of_delete_node(np);
		return NULL;
	}

	return np;
}

static void of_pci_fixup_bus(struct device_node *parent_np, struct pci_bus *bus)
{
	struct pci_dev *pdev;
	struct device_node *np;

	list_for_each_entry(pdev, &bus->devices, bus_list) {
		np = of_pci_fixup_dev_node(parent_np, pdev);
		if (np && pdev->subordinate)
			of_pci_fixup_bus(np, pdev->subordinate);
	}
}

static int of_pci_fixup(struct device_node *root, void *ctx)
{
	struct pci_bus *bus;

	list_for_each_entry(bus, &pci_root_buses, node) {
		struct device_node *bb_np, *host_np;
		char *name;

		if (!bus->host || !bus->host->parent)
			continue;
		bb_np = bus->host->parent->of_node;
		if (!bb_np)
			continue;

		name = of_get_reproducible_name(bb_np);
		host_np = of_find_node_by_reproducible_name(root, name);
		free(name);
		if (!host_np) {
			pr_debug("no kernel-DT mirror of host bridge %pOF\n", bb_np);
			continue;
		}

		of_pci_fixup_bus(host_np, bus);
	}

	return 0;
}

static int of_pci_register_fixup(void)
{
	return of_register_fixup(of_pci_fixup, NULL);
}

/*
 * The PCI device OF fixup must run before other fixups want to modify the nodes
 * we are creating here. As OF fixups are running in the order they are registered,
 * register this one early. This might be the first sign that we need a dependency
 * tracking or -EPROBE_DEFERRED mechanism for OF fixups. Keep an eye on it.
 */
core_initcall(of_pci_register_fixup);
