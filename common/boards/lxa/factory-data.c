/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <tlv/tlv.h>
#include <common.h>

static int lxa_tlv_handle_serial(struct tlv_device *dev, struct tlv_mapping *map,
				 u16 len, const u8 *val)
{
	const char *buf;
	char *period;

	buf = __tlv_format_str(dev, map, len, val);
	if (!buf)
		return -ENOMEM;

	/* Strip /\d+\./ prefix to arrive at hostname without period */
	period = memrchr(buf, '.', len);
	barebox_set_serial_number(period ? period + 1 : buf);

	return 0;
}

static int lxa_tlv_format_calib(struct tlv_device *dev, struct tlv_mapping *map,
				u16 len, const u8 *val)
{
	return tlv_format_blob(dev, map, len, val);
}

static struct tlv_mapping lxa_tlv_baseboard_v1_mappings[] = {
	/* vendor-specific override */
	{ 0x0004, lxa_tlv_handle_serial, "serial-number"},
	/* vendor-specific additions */
	{ 0x8000, lxa_tlv_format_calib, "usb-host-curr" },
	{ 0x8001, lxa_tlv_format_calib, "usb-host1-curr" },
	{ 0x8002, lxa_tlv_format_calib, "usb-host2-curr" },
	{ 0x8003, lxa_tlv_format_calib, "usb-host3-curr" },
	{ 0x8010, lxa_tlv_format_calib, "out0-volt" },
	{ 0x8011, lxa_tlv_format_calib, "out1-volt" },
	{ 0x8020, lxa_tlv_format_calib, "iobus-curr" },
	{ 0x8021, lxa_tlv_format_calib, "iobus-volt" },
	{ /* sentintel */ },
};

static struct tlv_mapping *baseboard_mappings[] = {
	lxa_tlv_baseboard_v1_mappings, barebox_tlv_v1_mappings, NULL
};

static struct of_device_id baseboard_matches[] = {
	{ .compatible = "lxa,tlv-baseboard-v1" },
	{ /* sentinel */ }
};

static struct tlv_decoder lxa_tlv_baseboard_v1 = {
	.magic = TLV_MAGIC_LXA_BASEBOARD_V1,
	.driver.name = "lxa-tlv-baseboard-v1",
	.driver.of_compatible = baseboard_matches,
	.mappings = baseboard_mappings,
};

static struct tlv_mapping lxa_tlv_powerboard_v1_mappings[] = {
	{ 0x0003, tlv_format_timestamp, "factory-timestamp" },
	{ 0x0005, tlv_format_dec, "modification" },
	{ 0x0006, tlv_format_str, "featureset" },
	{ 0x0007, tlv_format_str, "pcba-serial-number"},
	{ 0x0008, tlv_format_str, "pcba-hardware-release"},
	{ 0x9000, lxa_tlv_format_calib, "pwr-volt" },
	{ 0x9001, lxa_tlv_format_calib, "pwr-curr" },
	{ /* sentintel */ },
};

static struct tlv_mapping *powerboard_mappings[] = {
	lxa_tlv_powerboard_v1_mappings, NULL
};

static struct of_device_id powerboard_matches[] = {
	{ .compatible = "lxa,tlv-powerboard-v1" },
	{ /* sentinel */ }
};

static struct tlv_decoder lxa_tlv_powerboard_v1 = {
	.magic = TLV_MAGIC_LXA_POWERBOARD_V1,
	.driver.name = "lxa-tlv-powerboard-v1",
	.driver.of_compatible = powerboard_matches,
	.mappings = powerboard_mappings,
};


static struct tlv_mapping lxa_tlv_ioboard_v1_mappings[] = {
	{ 0x0003, tlv_format_timestamp, "factory-timestamp" },
	{ 0x0005, tlv_format_dec, "modification" },
	{ 0x0006, tlv_format_str, "featureset" },
	{ 0x0007, tlv_format_str, "pcba-serial-number"},
	{ 0x0008, tlv_format_str, "pcba-hardware-release"},
	{ /* sentintel */ },
};

static struct tlv_mapping *ioboard_mappings[] = {
	lxa_tlv_ioboard_v1_mappings, NULL
};

static struct of_device_id ioboard_matches[] = {
	{ .compatible = "lxa,tlv-ioboard-v1" },
	{ /* sentinel */ }
};

static struct tlv_decoder lxa_tlv_ioboard_v1 = {
	.magic = TLV_MAGIC_LXA_IOBOARD_V1,
	.driver.name = "lxa-tlv-ioboard-v1",
	.driver.of_compatible = ioboard_matches,
	.mappings = ioboard_mappings,
};

static int lxa_tlv_v1_register(void)
{
	struct tlv_decoder *const decoders[] = {
		&lxa_tlv_baseboard_v1,
		&lxa_tlv_powerboard_v1,
		&lxa_tlv_ioboard_v1,
		NULL
	};
	int err = 0;

	for (struct tlv_decoder *const *d = decoders; *d; d++) {
		int ret = tlv_register_decoder(*d);
		if (ret)
			err = ret;
	}

	return err;
}
postmmu_initcall(lxa_tlv_v1_register);
