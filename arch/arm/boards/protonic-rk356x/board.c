// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "Protonic RK356x: " fmt

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <init.h>
#include <mach/rockchip/bbu.h>
#include <mach/rockchip/rockchip.h>
#include <environment.h>
#include <param.h>
#include <of_device.h>
#include <aiodev.h>
#include <globalvar.h>
#include <net.h>
#include <barebox-info.h>

struct prt_rk356x_adc_chs {
	int usb_boot;
	int hw_id;
	int hw_rev;
};

struct prt_rk356x_model {
	const char *name;
	const char *shortname;
	const struct prt_rk356x_adc_chs adc_channels;
	const int mac_cnt;
	int (*init)(void);
};

struct prt_rk356x_priv {
	int hw_id;
	int hw_rev;
	struct device *dev;
	const struct prt_rk356x_model *model;
};

static struct prt_rk356x_priv prt_priv;

static int saradc_read_mv(int chan)
{
	int error, voltage = 0;
	char *name;

	name = xasprintf("saradc.in_value%d_mV", chan);
	error = aiochannel_name_get_value(name, &voltage);
	if (error)
		pr_warn_once("Cannot read ADC %s: %pe\n", name, ERR_PTR(error));

	free(name);
	return voltage;
}

static bool prt_rk356x_get_usb_boot(int chan)
{
	return saradc_read_mv(chan) < 74;
}

static int prt_rk356x_adc_id_values[] = {
	1800, 1662, 1521, 1354, 1214, 1059, 900, 742, 335, 589, 278, 137, 0
};

static int prt_rk356x_get_adc_id(int chan)
{
	int val;
	unsigned int t;

	val = saradc_read_mv(chan) + 74;

	for (t = 0; t < ARRAY_SIZE(prt_rk356x_adc_id_values); t++) {
		if (val > prt_rk356x_adc_id_values[t])
			return t;
	}

	return t;
}

static void prt_rk356x_process_adc(struct device *dev,
				   const struct prt_rk356x_adc_chs *adc_chs)
{
	prt_priv.hw_id = prt_rk356x_get_adc_id(adc_chs->hw_id);
	prt_priv.hw_rev = prt_rk356x_get_adc_id(adc_chs->hw_rev);

	dev_add_param_uint32_ro(dev, "boardrev", &prt_priv.hw_rev, "%u");
	dev_add_param_uint32_ro(dev, "boardid", &prt_priv.hw_id, "%u");

	/* Check if we need to enable the USB gadget instead of booting */
	if (prt_rk356x_get_usb_boot(adc_chs->usb_boot)) {
		globalvar_add_simple("boot.default", "net");
		globalvar_add_simple("usbgadget.acm", "1");
		globalvar_add_simple("usbgadget.autostart", "1");
		globalvar_add_simple("system.partitions", "/dev/mmc0(mmc0)");
		pr_info("Enter USB recovery\n");
	} else {
		globalvar_add_simple("boot.default", "bootchooser");
	}

	pr_info("Board id: %d, revision %d\n", prt_priv.hw_id, prt_priv.hw_rev);
}

static int prt_rk356x_inc_mac(u8 *mac)
{
	mac[5]++;
	if (mac[5])
		return 0;
	mac[4]++;
	if (mac[4])
		return 0;
	mac[3]++;
	if (mac[3])
		return 0;
	pr_err("MAC address overflow!!\n");
	return -EINVAL;
}

#define RK_GPT_RESERVED_SPACE (7680 * 512)

static void prt_rk356x_board_info(void)
{
	struct cdev *cdev;
	ssize_t size;
	u8 buf[11];
	int i;

	cdev = cdev_by_name("mmc0");
	if (!cdev) {
		pr_warn("No eMMC device\n");
		return;
	}

	/* Read and check TAG */
	size = cdev_read(cdev, buf, 4, RK_GPT_RESERVED_SPACE, 0);
	if (size != 4) {
		pr_warn("Unable to read board data from eMMC\n");
		return;
	}
	if (strncmp(buf, "PRTm", 4)) {
		pr_warn("Board has no serial number and no MAC address\n");
		return;
	}

	/* Read MAC at offset 4 */
	if (prt_priv.model->mac_cnt > 0) {
		size = cdev_read(cdev, buf, 6, RK_GPT_RESERVED_SPACE + 4, 0);
		if (size != 6) {
			pr_warn("Unable to read MAC address from eMMC\n");
			return;
		}
		if (is_valid_ether_addr(buf)) {
			eth_register_ethaddr(0, buf);
			for (i = 1; i < prt_priv.model->mac_cnt; i++) {
				prt_rk356x_inc_mac(buf);
				eth_register_ethaddr(i, buf);
			}
		} else {
			pr_warn("Board has invalid MAC address\n");
		}
	}

	/* Read serial at offset 10 */
	size = cdev_read(cdev, buf, 10, RK_GPT_RESERVED_SPACE + 10, 0);
	if (size != 10) {
		pr_warn("Unable to read serial number from eMMC\n");
		return;
	}
	buf[10] = 0;
	pr_info("Board serial number: %s\n", buf);
	barebox_set_serial_number(buf);
}

static int mecsbc_sd_of_fixup(struct device_node *root, void *context)
{
	struct device *dev = context;
	struct device_node *np;

	dev_info(dev, "Fixing up /regulator-sd\n");

	np = of_find_node_by_path_from(root, "/regulator-sd");
	if (!np) {
		dev_err(dev, "Cannot find /regulator-sd node\n");
		return 0;
	}

	of_property_write_u32(np, "regulator-min-microvolt", 3300000);

	return 0;
}

static int prt_rk356x_mecsbc_init(void)
{
	if (prt_priv.hw_id == 0 && prt_priv.hw_rev == 0)
		of_register_fixup(mecsbc_sd_of_fixup, prt_priv.dev);

	return 0;
}

static void prt_rk356x_bbu(void)
{
	enum bootsource bootsource = bootsource_get();
	int instance = bootsource_get_instance();

	if (bootsource == BOOTSOURCE_MMC && instance == 1)
		of_device_enable_path("/chosen/environment-sd");
	else
		of_device_enable_path("/chosen/environment-emmc");

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");
	rockchip_bbu_mmc_register("sd", 0, "/dev/mmc1");
}

static int prt_rk356x_devices_init(void)
{
	int ret;

	if (!prt_priv.model)
		return 0;

	prt_rk356x_bbu();
	prt_rk356x_board_info();

	if (prt_priv.model->init) {
		ret = prt_priv.model->init();
		if (ret) {
			pr_warn("Model init failed with %d\n", ret);
			return ret;
		}
	}

	return 0;
}
late_initcall(prt_rk356x_devices_init);

static int prt_rk356x_of_fixup_hwrev(struct device *dev)
{
	const char *compat;
	char *buf;

	compat = of_device_get_match_compatible(dev);

	buf = xasprintf("%s-m%u-r%u", compat, prt_priv.hw_id,
			prt_priv.hw_rev);
	barebox_set_of_machine_compatible(buf);

	free(buf);
	return 0;
}

static int prt_rk356x_probe(struct device *dev)
{
	int error;
	const struct prt_rk356x_model *model;

	error = of_device_ensure_probed_by_alias("saradc");
	if (error) {
		pr_err("saradc is not available\n");
		return error;
	}

	model = device_get_match_data(dev);

	barebox_set_model(model->name);
	barebox_set_hostname(model->shortname);

	prt_rk356x_process_adc(dev, &model->adc_channels);
	prt_rk356x_of_fixup_hwrev(dev);

	prt_priv.dev = dev;
	prt_priv.model = model;
	return 0;
}

static const struct prt_rk356x_model mecsbc = {
	.name = "Protonic MECSBC board",
	.shortname = "mecsbc",
	.adc_channels = {0, 1, 3},
	.mac_cnt = 3,
	.init = prt_rk356x_mecsbc_init,
};

static const struct of_device_id prt_rk356x_of_match[] = {
	{
		.compatible = "prt,mecsbc",
		.data = &mecsbc,
	},
	{ /* sentinel */ },
};

static struct driver prt_rk356x_board_driver = {
	.name = "board-protonic-rk356x",
	.probe = prt_rk356x_probe,
	.of_compatible = prt_rk356x_of_match,
};
coredevice_platform_driver(prt_rk356x_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(prt_rk356x_of_match);
