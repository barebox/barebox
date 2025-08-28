// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "PRTPUK: " fmt

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

struct prtpuk_model {
	const char *name;
	const char *shortname;
};

struct prtpuk_priv {
	int hw_id;
	int hw_rev;
};

static struct prtpuk_priv prtpuk_data;

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

static int prtpuk_get_vin_mv(void)
{
	return saradc_get_value("aiodev0.in_value4_mV") * 22;
}

static bool prtpuk_get_usb_boot(void)
{
	return saradc_get_value("aiodev0.in_value1_mV") < 74;
}

static int prtpuk_adc_id_values[] = {
	1800, 1662, 1521, 1354, 1214, 1059, 900, 742, 335, 589, 278, 137, 0
};

static int prtpuk_get_adc_id(const char *chan)
{
	int val;
	unsigned int t;

	val = saradc_get_value(chan) + 74;

	for (t = 0; t < ARRAY_SIZE(prtpuk_adc_id_values); t++) {
		if (val > prtpuk_adc_id_values[t])
			return t;
	}

	return t;
}

static void prtpuk_process_adc(struct device *dev)
{
	int vin;

	prtpuk_data.hw_id = prtpuk_get_adc_id("aiodev0.in_value3_mV");
	prtpuk_data.hw_rev = prtpuk_get_adc_id("aiodev0.in_value2_mV");

	dev_add_param_uint32_ro(dev, "boardrev", &prtpuk_data.hw_rev, "%u");
	dev_add_param_uint32_ro(dev, "boardid", &prtpuk_data.hw_id, "%u");

	/* Check if we need to enable the USB gadget instead of booting */
	if (prtpuk_get_usb_boot()) {
		globalvar_add_simple("boot.default", "net");
		globalvar_add_simple("usbgadget.acm", "1");
		globalvar_add_simple("usbgadget.autostart", "1");
		globalvar_add_simple("system.partitions", "/dev/disk0(mmc0)");
		pr_info("PRTPUK: Enter USB recovery\n");
	} else {
		globalvar_add_simple("boot.default", "bootchooser");
	}

	pr_info("Board id: %d, revision %d\n", prtpuk_data.hw_id, prtpuk_data.hw_rev);
	vin = prtpuk_get_vin_mv();
	pr_info("VIN = %d.%d V\n", vin / 1000, (vin % 1000) / 100);
}

static int prtpuk_of_fixup_hwrev(struct device *dev)
{
	const char *compat;
	char *buf;

	compat = of_device_get_match_compatible(dev);

	buf = xasprintf("%s-m%u-r%u", compat, prtpuk_data.hw_id,
			prtpuk_data.hw_rev);
	barebox_set_of_machine_compatible(buf);

	free(buf);

	return 0;
}

static int prtpuk_probe(struct device *dev)
{
	int ret = 0;
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();
	const struct prtpuk_model *model;
	struct device_node *np;

	np = of_find_node_by_name_address(NULL, "adc@2ae00000");
	of_device_ensure_probed(np);

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");

	prtpuk_process_adc(dev);
	prtpuk_of_fixup_hwrev(dev);

	return ret;
}

static const struct prtpuk_model prtpuk = {
	.name = "Protonic PRTPUK board",
	.shortname = "prtpuk",
};

static const struct of_device_id prtpuk_of_match[] = {
	{
		.compatible = "prt,prtpuk",
		.data = &prtpuk,
	},
	{ /* sentinel */ },
};

static struct driver prtpuk_board_driver = {
	.name = "board-prtpuk",
	.probe = prtpuk_probe,
	.of_compatible = prtpuk_of_match,
};
coredevice_platform_driver(prtpuk_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(prtpuk_of_match);
