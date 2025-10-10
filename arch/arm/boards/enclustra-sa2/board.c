// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt)     "enclustra-SA2: " fmt

#include <stdio.h>
#include <barebox-info.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <net.h>
#include <linux/nvmem-consumer.h>

#include "si5338_config.h"

/** Enclustra's MAC address vendor prefix is 20:B0:F7 */
#define ENCLUSTRA_PREFIX            0x20b0f7
#define SERIAL_NUMBER_NUM_BYTES     4

/*
 * Read the MAC address via the atsha204a driver.
 *
 * Set two consecutive MAC addresses, as specified by the manufacturer.
 */
static void set_mac_addr(void)
{
	uint32_t hwaddr_prefix;
	u8 *hwaddr = NULL;
	struct device_node *np;

	np = of_find_node_by_alias(NULL, "ethernet0");
	if (!np) {
		pr_warn("can't find alias ethernet0\n");
		return;
	}

	hwaddr = nvmem_cell_get_and_read(np, "mac-address", ETH_ALEN);
	if (IS_ERR(hwaddr)) {
		pr_warn("can't read NVMEM cell\n");
		return;
	}

	pr_debug("MAC address: %pM\n", hwaddr);

	/* check vendor prefix and set the environment variable */
	hwaddr_prefix = (hwaddr[0] << 16) | (hwaddr[1] << 8) | (hwaddr[2]);
	if (hwaddr_prefix == ENCLUSTRA_PREFIX) {
		eth_register_ethaddr(0, hwaddr);
		eth_addr_inc(hwaddr); /* calculate 2nd, consecutive MAC address */
		eth_register_ethaddr(1, hwaddr);
	} else {
		pr_err("Invalid MAC address vendor prefix\n");
	}

	free(hwaddr);
}

/*
 * Read the SoM serial number via the atsha204a driver.
 */
static void set_ser_num(void)
{
	u8 *data = NULL;
	uint32_t ser_num = 0;
	char ser_num_str[12];
	struct device_node *np;

	np = of_find_node_by_alias(NULL, "som-sernum");
	if (!np) {
		pr_warn("can't find alias som-sernum\n");
		return;
	}

	data = nvmem_cell_get_and_read(np, "serial-number", SERIAL_NUMBER_NUM_BYTES);
	if (IS_ERR(data)) {
		pr_warn("can't read NVMEM cell\n");
		return;
	}

	ser_num = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	sprintf(ser_num_str, "%u", ser_num);
	barebox_set_serial_number(ser_num_str);

	free(data);
}

static int enclustra_sa2_probe(struct device *dev)
{
	of_devices_ensure_probed_by_compatible("atmel,atsha204a");

	set_mac_addr();
	set_ser_num();

	/* configure clock generator on the Enclustra ST1 baseboard: */
	if (IS_ENABLED(CONFIG_MACH_SOCFPGA_ENCLUSTRA_SA2_SI5338))
		si5338_init();

	return 0;
}

static __maybe_unused struct of_device_id enclustra_sa2_ids[] = {
        {
                .compatible = "enclustra,mercury-sa2",
        }, {
                /* sentinel */
        }
};
BAREBOX_DEEP_PROBE_ENABLE(enclustra_sa2_ids);

static struct driver enclustra_sa2_driver = {
        .name = "skov-imx6",
        .probe = enclustra_sa2_probe,
        .of_compatible = DRV_OF_COMPAT(enclustra_sa2_ids),
};
coredevice_platform_driver(enclustra_sa2_driver);
