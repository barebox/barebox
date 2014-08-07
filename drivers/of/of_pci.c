#include <common.h>
#include <errno.h>
#include <of.h>
#include <of_pci.h>

/**
 * of_pci_get_devfn() - Get device and function numbers for a device node
 * @np: device node
 *
 * Parses a standard 5-cell PCI resource and returns an 8-bit value that can
 * be passed to the PCI_SLOT() and PCI_FUNC() macros to extract the device
 * and function numbers respectively. On error a negative error code is
 * returned.
 */
int of_pci_get_devfn(struct device_node *np)
{
	unsigned int size;
	const __be32 *reg;

	reg = of_get_property(np, "reg", &size);

	if (!reg || size < 5 * sizeof(__be32))
		return -EINVAL;

	return (be32_to_cpup(reg) >> 8) & 0xff;
}
EXPORT_SYMBOL_GPL(of_pci_get_devfn);
