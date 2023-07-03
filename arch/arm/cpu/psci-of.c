// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "psci-of: " fmt

#include <common.h>
#include <of.h>
#include <asm/psci.h>
#include <linux/arm-smccc.h>

int of_psci_fixup(struct device_node *root, unsigned long psci_version,
		  const char *method)
{
	struct device_node *psci;
	int ret;
	const char *compat;
	struct device_node *cpus, *cpu;

	psci = of_create_node(root, "/psci");
	if (!psci)
		return -EINVAL;

	if (psci_version >= ARM_PSCI_VER_1_0) {
		compat = "arm,psci-1.0";
	} else if (psci_version >= ARM_PSCI_VER_0_2) {
		compat = "arm,psci-0.2";
	} else {
		pr_err("Unsupported PSCI version %ld\n", psci_version);
		return -EINVAL;
	}

	cpus = of_find_node_by_path_from(root, "/cpus");
	if (!cpus) {
		pr_err("Cannot find /cpus node, PSCI fixup aborting\n");
		return -EINVAL;
	}

	cpu = cpus;
	while (1) {
		cpu = of_find_node_by_type(cpu, "cpu");
		if (!cpu)
			break;
		of_property_write_string(cpu, "enable-method", "psci");
		pr_debug("Fixed %pOF\n", cpu);
	}

	ret = of_property_write_string(psci, "compatible", compat);
	if (ret)
		return ret;

	ret = of_property_write_string(psci, "method", method);
	if (ret)
		return ret;

	return 0;
}
