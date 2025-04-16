// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 John Watts <contact@jookia.org>

#include <common.h>
#include <deep-probe.h>
#include <fs.h>
#include <libfile.h>
#include <net.h>

struct novena_eeprom {
	uint8_t signature[6]; /* 'Novena' */
	uint8_t version; /* 1 or 2, not checked */
	uint8_t page_size; /* v2 only: EEPROM read/write page */
	uint32_t serial; /* 32-bit serial number */
	uint8_t mac[6]; /* Gigabit MAC address */
	uint16_t features; /* features */
	/* ... extra fields omitted ... */
} __packed;

static void power_on_audio_codec(void)
{
	int rc = of_devices_ensure_probed_by_name("regulator-audio-codec");

	if (rc < 0)
		pr_err("Unable to power on audio codec: %pe\n", ERR_PTR(rc));
}

static struct novena_eeprom *novena_read_eeprom(void)
{
	size_t read;
	loff_t max = sizeof(struct novena_eeprom);
	void *eeprom;
	int rc;

	/*
	 * When powered off the audio codec pulls down the EEPROM's I2C line.
	 * Power it on so we can actually read data.
	 */
	power_on_audio_codec();

	rc = of_device_ensure_probed_by_alias("eeprom0");
	if (rc < 0) {
		pr_err("Unable to probe eeprom0: %pe\n", ERR_PTR(rc));
		return NULL;
	}

	rc = read_file_2("/dev/eeprom0", &read, &eeprom, max);

	if (rc < 0 && rc != -EFBIG) {
		pr_err("Unable to read Novena EEPROM: %pe\n", ERR_PTR(rc));
		return NULL;
	} else if (read != max) {
		pr_err("Short read from Novena EEPROM?\n");
		free(eeprom);
		return NULL;
	}

	return eeprom;
}

static bool novena_check_eeprom(struct novena_eeprom *eeprom)
{
	char *sig = eeprom->signature;
	size_t size = sizeof(eeprom->signature);

	if (memcmp("Novena", sig, size) != 0) {
		pr_err("Unknown Novena EEPROM signature\n");
		return false;
	}

	return true;
}

static void novena_set_mac(struct novena_eeprom *eeprom)
{
	struct device_node *dnode;

	dnode = of_find_node_by_alias(of_get_root_node(), "ethernet0");
	if (dnode)
		of_eth_register_ethaddr(dnode, eeprom->mac);
	else
		pr_err("Unable to find ethernet node\n");
}

static int novena_probe(struct device *dev)
{
	struct novena_eeprom *eeprom = novena_read_eeprom();

	if (eeprom && novena_check_eeprom(eeprom))
		novena_set_mac(eeprom);

	free(eeprom);

	return 0;
}

static const struct of_device_id novena_of_match[] = {
	{ .compatible = "kosagi,imx6q-novena", },
	{ /* sentinel */ }
};

static struct driver novena_board_driver = {
	.name = "board-novena",
	.probe = novena_probe,
	.of_compatible = novena_of_match,
};
coredevice_platform_driver(novena_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(novena_of_match);
