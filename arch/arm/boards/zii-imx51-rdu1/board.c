// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2017 Zodiac Inflight Innovation
 *
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * based on previous iterations of this code
 *
 *     Copyright (C) 2015 Nikita Yushchenko, CogentEmbedded, Inc
 *     Copyright (C) 2015 Andrey Gusakov, CogentEmbedded, Inc
 *
 * based on similar i.MX51 EVK (Babbage) board support code
 *
 *     Copyright (C) 2007 Sascha Hauer, Pengutronix
 */

#include <common.h>
#include <envfs.h>
#include <init.h>
#include <environment.h>
#include <mach/bbu.h>
#include <libfile.h>
#include <mach/imx5.h>
#include <net.h>
#include <linux/crc8.h>
#include <linux/sizes.h>
#include <linux/nvmem-consumer.h>

#include <envfs.h>

static int zii_rdu1_init(void)
{
	const char *hostname;

	if (!of_machine_is_compatible("zii,imx51-rdu1") &&
	    !of_machine_is_compatible("zii,imx51-scu2-mezz") &&
	    !of_machine_is_compatible("zii,imx51-scu3-esb"))
		return 0;

	hostname = of_get_machine_compatible() + strlen("imx51-");

	imx51_babbage_power_init();

	barebox_set_hostname(hostname);

	imx51_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc0", 0);
	imx51_bbu_internal_spi_i2c_register_handler("SPI",
		"/dev/dataflash0.barebox",
		BBU_HANDLER_FLAG_DEFAULT |
		IMX_BBU_FLAG_PARTITION_STARTS_AT_HEADER);

	defaultenv_append_directory(defaultenv_zii_common);
	defaultenv_append_directory(defaultenv_rdu1);

	return 0;
}
coredevice_initcall(zii_rdu1_init);

#define KEY		0
#define VALUE		1
#define STRINGS_NUM	2

static int zii_rdu1_load_config(void)
{
	struct device_node *np, *root;
	size_t len, remaining_space;
	const uint8_t crc8_polynomial = 0x8c;
	DECLARE_CRC8_TABLE(crc8_table);
	const char *cursor, *end;
	const char *file = "/dev/dataflash0.config";
	uint8_t *config;
	int ret = 0;
	enum {
		BLOB_SPINOR,
		BLOB_RAVE_SP_EEPROM,
		BLOB_MICROWIRE,
	} blob;

	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	crc8_populate_lsb(crc8_table, crc8_polynomial);

	for (blob = BLOB_SPINOR; blob <= BLOB_MICROWIRE; blob++) {
		switch (blob) {
		case BLOB_MICROWIRE:
			file = "/dev/microwire-eeprom";
			/* FALLTHROUGH */
		case BLOB_SPINOR:
			config = read_file(file, &remaining_space);
			if (!config) {
				pr_err("Failed to read %s\n", file);
				return -EIO;
			}
			break;
		case BLOB_RAVE_SP_EEPROM:
			/* Needed for error logging below */
			file = "shadow copy in RAVE SP EEPROM";

			root = of_get_root_node();
			np   = of_find_node_by_name(root, "eeprom@a4");
			if (!np)
				return -ENODEV;

			pr_info("Loading %s, this may take a while\n", file);

			remaining_space = SZ_1K;
			config = nvmem_cell_get_and_read(np, "shadow-config",
							 remaining_space);
			if (IS_ERR(config))
				return PTR_ERR(config);

			break;
		}

		/*
		 * The environment blob has its CRC8 stored as the
		 * last byte of the blob, so calculating CRC8 over the
		 * whole things should return 0
		 */
		if (crc8(crc8_table, config, remaining_space, 0)) {
			pr_err("CRC mismatch for %s\n", file);
			free(config);
			config = NULL;
		} else {
			/*
			 * We are done if there's a blob with a valid
			 * CRC8
			 */
			break;
		}
	}

	if (!config) {
		pr_err("No valid config blobs were found\n");
		ret = -EINVAL;
		goto free_config;
	}

	/*
	 * Last byte is CRC8, so it is of no use for our parsing
	 * algorithm
	 */
	remaining_space--;

	cursor = config;
	end = cursor + remaining_space;

	/*
	 * The environemnt is stored a a bunch of zero-terminated
	 * ASCII strings in "key":"value" pairs
	 */
	while (cursor < end) {
		const char *strings[STRINGS_NUM] = { NULL, NULL };
		char *key;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(strings); i++) {
			if (!*cursor) {
				/* We assume that last key:value pair
				 * will be terminated by an extra '\0'
				 * at the end */
				goto free_config;
			}

			len = strnlen(cursor, remaining_space);
			if (len >= remaining_space) {
				ret = -EOVERFLOW;
				goto free_config;
			}

			strings[i] = cursor;

			len++;	/* Account for '\0' at the end of the string */
			cursor += len;
			remaining_space -= len;

			if (cursor > end) {
				ret = -EOVERFLOW;
				goto free_config;
			}
		}

		key = basprintf("config_%s", strings[KEY]);
		ret = setenv(key, strings[VALUE]);
		free(key);

		if (ret)
			goto free_config;
	}

free_config:
	free(config);
	return ret;
}
late_initcall(zii_rdu1_load_config);

static int zii_rdu1_ethernet_init(void)
{
	const char *mac_string;
	struct device_node *np, *root;
	uint8_t mac[ETH_ALEN];
	int ret;

	if (!of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	root = of_get_root_node();

	np = of_find_node_by_alias(root, "ethernet0");
	if (!np) {
		pr_warn("Failed to find ethernet0\n");
		return -ENOENT;
	}

	mac_string = getenv("config_mac");
	if (!mac_string)
		return -ENOENT;

	ret = string_to_ethaddr(mac_string, mac);
	if (ret < 0)
		return ret;

	of_eth_register_ethaddr(np, mac);
	return 0;
}
/* This needs to happen only after zii_rdu1_load_config was
 * executed */
environment_initcall(zii_rdu1_ethernet_init);
