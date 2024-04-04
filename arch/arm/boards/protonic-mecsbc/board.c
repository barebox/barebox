// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "MECSBC: " fmt

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>
#include <environment.h>
#include <param.h>
#include <of_device.h>
#include <aiodev.h>
#include <globalvar.h>

struct mecsbc_model {
	const char *name;
	const char *shortname;
};

struct mecsbc_priv {
	int hw_id;
	int hw_rev;
};

static struct mecsbc_priv mecsbc_data;

static int saradc_get_value(const char *chan)
{
	int ret, voltage;

	ret = aiochannel_name_get_value(chan, &voltage);
	if (ret) {
		pr_warn_once("Cannot read ADC %s: %pe\n", chan, ERR_PTR(ret));
		return 0;
	}

	return voltage;
}

static int mecsbc_get_vin_mv(void)
{
	return saradc_get_value("aiodev0.in_value2_mV") * 22;
}

static bool mecsbc_get_usb_boot(void)
{
	return saradc_get_value("aiodev0.in_value0_mV") < 74;
}

static int mecsbc_adc_id_values[] = {
	1800, 1662, 1521, 1354, 1214, 1059, 900, 742, 335, 589, 278, 137, 0
};

static int mecsbc_get_adc_id(const char *chan)
{
	int val;
	unsigned int t;

	val = saradc_get_value(chan) + 74;

	for (t = 0; t < ARRAY_SIZE(mecsbc_adc_id_values); t++) {
		if (val > mecsbc_adc_id_values[t])
			return t;
	}

	return t;
}

static void mecsbc_process_adc(struct device *dev)
{
	mecsbc_data.hw_id = mecsbc_get_adc_id("aiodev0.in_value1_mV");
	mecsbc_data.hw_rev = mecsbc_get_adc_id("aiodev0.in_value3_mV");

	dev_add_param_uint32_ro(dev, "boardrev", &mecsbc_data.hw_rev, "%u");
	dev_add_param_uint32_ro(dev, "boardid", &mecsbc_data.hw_id, "%u");

	/* Check if we need to enable the USB gadget instead of booting */
	if (mecsbc_get_usb_boot()) {
		globalvar_add_simple("boot.default", "net");
		globalvar_add_simple("usbgadget.acm", "1");
		globalvar_add_simple("usbgadget.autostart", "1");
		globalvar_add_simple("system.partitions", "/dev/mmc0(mmc0)");
		pr_info("MECSBC: Enter USB recovery\n");
	} else {
		globalvar_add_simple("boot.default", "bootchooser");
	}

	pr_info("Board id: %d, revision %d\n", mecsbc_data.hw_id, mecsbc_data.hw_rev);
	pr_info("VIN = %d V\n", mecsbc_get_vin_mv() / 1000);
}

static int mecsbc_of_fixup_hwrev(struct device *dev)
{
	const char *compat;
	char *buf;

	compat = of_device_get_match_compatible(dev);

	buf = xasprintf("%s-m%u-r%u", compat, mecsbc_data.hw_id,
			mecsbc_data.hw_rev);
	barebox_set_of_machine_compatible(buf);

	free(buf);

	return 0;
}

static int mecsbc_probe(struct device *dev)
{
	int ret = 0;
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();
	const struct mecsbc_model *model;
	struct device_node *np;

	np = of_find_node_by_name_address(NULL, "saradc@fe720000");
	of_device_ensure_probed(np);

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rk3568_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");
	rk3568_bbu_mmc_register("sd", 0, "/dev/mmc1");

	mecsbc_process_adc(dev);
	mecsbc_of_fixup_hwrev(dev);

	return ret;
}

static const struct mecsbc_model mecsbc = {
	.name = "Protonic MECSBC board",
	.shortname = "mecsbc",
};

static const struct of_device_id mecsbc_of_match[] = {
	{
		.compatible = "prt,mecsbc",
		.data = &mecsbc,
	},
	{ /* sentinel */ },
};

static struct driver mecsbc_board_driver = {
	.name = "board-mecsbc",
	.probe = mecsbc_probe,
	.of_compatible = mecsbc_of_match,
};
coredevice_platform_driver(mecsbc_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(mecsbc_of_match);
