// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Ahmad Fatoum
 */

#include <common.h>
#include <init.h>
#include <acpi.h>

static const char *profiles[] = {
	"Unspecified",
	"Desktop",
	"Mobile",
	"Workstation",
	"Enterprise Server",
	"SOHO Server",
	"Applicance PC",
	"Performance Server",
	"Tablet",
};

static int acpi_test_probe(struct device_d *dev)
{
	const char *profile = "reserved";
	u8 *sdt;
	u8 profileno;

	dev_dbg(dev, "driver initializing...\n");

	sdt = (u8 __force *)dev_request_mem_region_by_name(dev, "SDT");
	if (IS_ERR(sdt)) {
		dev_err(dev, "no SDT resource available: %pe\n", sdt);
		return PTR_ERR(sdt);
	}

	dev_dbg(dev, "SDT is at 0x%p\n", sdt);

	profileno = sdt[45];

	if (profileno < ARRAY_SIZE(profiles))
		profile = profiles[profileno];

	dev_info(dev, "PM profile is for '%s'\n", profile);

	return 0;
}

static void acpi_test_remove(struct device_d *dev)
{
	dev_info(dev, "FADT driver removed\n");
}

static struct acpi_driver acpi_test_driver = {
	.signature = "FACP",
	.driver = {
		.name = "acpi-test",
		.probe = acpi_test_probe,
		.remove = acpi_test_remove,
	}
};
device_acpi_driver(acpi_test_driver);
