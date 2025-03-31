// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2012 Steffen Trumtrar, Pengutronix
// SPDX-FileCopyrightText: 2014 Protonic Holland
// SPDX-FileCopyrightText: 2020 Oleksij Rempel, Pengutronix

#include <bbu.h>
#include <boot.h>
#include <bootm.h>
#include <common.h>
#include <deep-probe.h>
#include <environment.h>
#include <fcntl.h>
#include <globalvar.h>
#include <gpio.h>
#include <i2c/i2c.h>
#include <mach/imx/bbu.h>
#include <mach/imx/imx6.h>
#include <mach/imx/ocotp-fusemap.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <mfd/syscon.h>
#include <net.h>
#include <of_device.h>
#include <linux/regmap.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/usb/usb.h>

#define GPIO_HW_REV_ID  {\
	{IMX_GPIO_NR(2, 8), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "rev_id0"}, \
	{IMX_GPIO_NR(2, 9), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "rev_id1"}, \
	{IMX_GPIO_NR(2, 10), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "rev_id2"} \
}

#define GPIO_HW_TYPE_ID  {\
	{IMX_GPIO_NR(2, 11), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "hw_id0"}, \
	{IMX_GPIO_NR(2, 12), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "hw_id1"}, \
	{IMX_GPIO_NR(2, 13), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "hw_id2"}, \
	{IMX_GPIO_NR(2, 14), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "hw_id3"}, \
	{IMX_GPIO_NR(2, 15), GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "hw_id4"} \
}

enum {
	HW_TYPE_PRTI6Q = 0,
	HW_TYPE_PRTWD2 = 1,
	HW_TYPE_ALTI6S = 2,
	HW_TYPE_VICUT1 = 4,
	HW_TYPE_ALTI6P = 6,
	HW_TYPE_PRTMVT = 8,
	HW_TYPE_PRTI6G = 10,
	HW_TYPE_PRTRVT = 12,
	HW_TYPE_VICUT2 = 16,
	HW_TYPE_PLYM2M = 20,
	HW_TYPE_PRTVT7 = 22,
	HW_TYPE_LANMCU = 23,
	HW_TYPE_PLYBAS = 24,
	HW_TYPE_VICTGO = 28,
	HW_TYPE_JOZACP = 30,
	HW_TYPE_JOZACPP = 31,
};

enum prt_imx6_kvg_pw_mode {
	PW_MODE_KVG_WITH_YACO = 0,
	PW_MODE_KVG_NEW = 1,
	PW_MODE_KUBOTA = 2,
};

/* board specific flags */
#define PRT_IMX6_BOOTCHOOSER		BIT(3)
#define PRT_IMX6_USB_LONG_DELAY		BIT(2)
#define PRT_IMX6_BOOTSRC_EMMC		BIT(1)
#define PRT_IMX6_BOOTSRC_SPI_NOR	BIT(0)

static struct prt_imx6_priv *prt_priv;
struct prt_machine_data {
	unsigned int hw_id;
	unsigned int hw_rev;
	unsigned int i2c_addr;
	unsigned int i2c_adapter;
	unsigned int emmc_usdhc;
	unsigned int sd_usdhc;
	unsigned int flags;
	int (*init)(struct prt_imx6_priv *priv);
};

struct prt_imx6_priv {
	struct device *dev;
	const struct prt_machine_data *dcfg;
	unsigned int hw_id;
	unsigned int hw_rev;
	const char *name;
	unsigned int no_usb_check;
	char *ocotp_serial;
};

struct prti6q_rfid_contents {
	u8 mac[6];
	char serial[10];
	u8 cs;
} __attribute__ ((packed));

#define GPIO_DIP1_FB   IMX_GPIO_NR(4, 18)
#define GPIO_FORCE_ON1 IMX_GPIO_NR(2, 30)
#define GPIO_ON1_CTRL  IMX_GPIO_NR(4, 21)
#define GPIO_ON2_CTRL  IMX_GPIO_NR(4, 22)

static const struct gpio prt_imx6_kvg_gpios[] = {
	{
		.gpio = GPIO_DIP1_FB,
		.flags = GPIOF_IN,
		.label = "DIP1_FB",
	},
	{
		.gpio = GPIO_FORCE_ON1,
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "FORCE_ON1",
	},
	{
		.gpio = GPIO_ON1_CTRL,
		.flags = GPIOF_IN,
		.label = "ON1_CTRL",
	},
	{
		.gpio = GPIO_ON2_CTRL,
		.flags = GPIOF_IN,
		.label = "ON2_CTRL",
	},
};

static int prt_of_fixup_hwrev(struct prt_imx6_priv *priv)
{
	const char *compat;
	char *buf;

	compat = of_device_get_match_compatible(priv->dev);

	buf = xasprintf("%s-m%u-r%u", compat, priv->hw_id,
			priv->hw_rev);
	barebox_set_of_machine_compatible(buf);

	free(buf);

	return 0;
}

static int prt_imx6_read_rfid(struct prt_imx6_priv *priv, void *buf,
			      size_t size)
{
	const struct prt_machine_data *dcfg = priv->dcfg;
	struct device *dev = priv->dev;
	struct i2c_client cl;
	int ret;

	cl.addr = dcfg->i2c_addr;
	cl.adapter = i2c_get_adapter(dcfg->i2c_adapter);
	if (!cl.adapter) {
		dev_err(dev, "i2c bus not found\n");
		return -ENODEV;
	}

	/* 0x6000 user storage in the RFID tag */
	ret = i2c_read_reg(&cl, 0x6000 | I2C_ADDR_16_BIT, buf, size);
	if (ret < 0) {
		dev_err(dev, "Failed to read the RFID: %pe\n", ERR_PTR(ret));
		return ret;
	}

	return 0;
}

static u8 prt_imx6_calc_rfid_cs(void *buf, size_t size)
{
	unsigned int cs = 0;
	u8 *dat = buf;
	int t;

	for (t = 0; t < size - 1; t++) {
		cs += dat[t];
	}

	cs ^= 0xff;

	return cs & 0xff;

}

static int prt_imx6_set_mac(struct prt_imx6_priv *priv,
			    struct prti6q_rfid_contents *rfid)
{
	struct device *dev = priv->dev;
	struct device_node *node;

	node = of_find_node_by_alias(of_get_root_node(), "ethernet0");
	if (!node) {
		dev_err(dev, "Cannot find FEC!\n");
		return -ENODEV;
	}

	if (!of_device_is_available(node))
		return 0;

	if (!is_valid_ether_addr(&rfid->mac[0])) {
		dev_err(dev, "bad MAC addr: %pM\n", &rfid->mac[0]);

		return -EILSEQ;
	}

	of_eth_register_ethaddr(node, &rfid->mac[0]);

	return 0;
}

static int prt_imx6_set_serial(struct prt_imx6_priv *priv, char *serial)
{
	serial[9] = 0; /* Failsafe */
	dev_info(priv->dev, "Serial number: %s\n", serial);
	barebox_set_serial_number(serial);

	return 0;
}

static int prt_imx6_read_i2c_mac_serial(struct prt_imx6_priv *priv)
{
	struct device *dev = priv->dev;
	struct prti6q_rfid_contents rfid;
	int ret;

	ret = prt_imx6_read_rfid(priv, &rfid, sizeof(rfid));
	if (ret)
		return ret;

	if (rfid.cs != prt_imx6_calc_rfid_cs(&rfid, sizeof(rfid))) {
		dev_err(dev, "RFID: bad checksum!\n");
		return -EBADMSG;
	}

	ret = prt_imx6_set_mac(priv, &rfid);
	if (ret)
		return ret;

	ret = prt_imx6_set_serial(priv, rfid.serial);
	if (ret)
		return ret;

	return 0;
}

#define PRT_IMX6_GP1_FMT_DEC		BIT(31)

static int prt_imx6_read_ocotp_serial(struct prt_imx6_priv *priv)
{
	int ret;
	unsigned val;

	ret = imx_ocotp_read_field(OCOTP_GP1, &val);
	if (ret) {
		dev_err(priv->dev, "Failed to read ocotp serial (%i)\n", ret);
		return ret;
	}

	if (!(val & PRT_IMX6_GP1_FMT_DEC))
		return -EINVAL;
	val &= PRT_IMX6_GP1_FMT_DEC - 1;

	priv->ocotp_serial = xasprintf("%u", val);

	return prt_imx6_set_serial(priv, priv->ocotp_serial);
}

static int prt_imx6_set_ocotp_serial(struct param_d *param, void *driver_priv)
{
	struct prt_imx6_priv *priv = driver_priv;
	int ret;
	unsigned val;

	ret = kstrtouint(priv->ocotp_serial, 10, &val);
	if (ret)
		return ret;

	if (val & PRT_IMX6_GP1_FMT_DEC)
		return -ERANGE;
	val |= PRT_IMX6_GP1_FMT_DEC;

	ret = imx_ocotp_write_field(OCOTP_GP1, val);
	if (ret)
		return ret;

	barebox_set_serial_number(priv->ocotp_serial);
	return 0;
}

static int prt_imx6_usb_mount(struct prt_imx6_priv *priv)
{
	struct device *dev = priv->dev;
	const char *path;
	struct stat s;
	int ret;

	ret = mkdir("/usb", 0);
	if (ret) {
		dev_err(dev, "Cannot mkdir /usb\n");
		return ret;
	}

	path = "/dev/disk0.0";
	ret = stat(path, &s);
	if (!ret) {
		ret = mount(path, NULL, "usb", NULL);
		if (ret)
			goto exit_usb_mount;
		return 0;
	}

	path = "/dev/disk0";
	ret = stat(path, &s);
	if (!ret) {
		ret = mount(path, NULL, "usb", NULL);
		if (ret)
			goto exit_usb_mount;
		return 0;
	}

exit_usb_mount:
	dev_err(dev, "Failed to mount %s with error (%i)\n", path, ret);
	return ret;
}

#define OTG_PORTSC1 (MX6_OTG_BASE_ADDR+0x184)

static int prt_imx6_usb_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct prt_imx6_priv *priv = prt_priv;
	struct device *dev = priv->dev;
	char *second_word;
	char buf[sizeof("vicut1q recovery")] = {};
	struct bootm_data bootm_data = {};
	ssize_t size;
	int fd, ret;

	usb_rescan();

	ret = prt_imx6_usb_mount(priv);
	if (ret)
		return ret;

	fd = open("/usb/boot_target", O_RDONLY);
	if (fd < 0) {
		dev_warn(dev, "Can't open /usb/boot_target file, continue with normal boot\n");
		ret = fd;
		goto exit_usb_boot;
	}

	size = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (size < 0) {
		ret = size;
		goto exit_usb_boot;
	}

	/* Length of "vicut1 usb", the shortest possible target. */
	if (size < strlen("vicut1 usb")) {
		dev_err(dev, "Invalid boot target file!\n");
		ret = -EINVAL;
		goto exit_usb_boot;
	}

	second_word = strchr(buf, ' ');
	if (!second_word) {
		dev_err(dev, "Cant find boot target in the boot target file!\n");
		ret = -ENODEV;
		goto exit_usb_boot;
	}

	*second_word = 0;

	if (strcmp(buf, priv->name) && strcmp(buf, "generic-imx6")) {
		dev_err(dev, "Boot target for a different board! (got: %s expected: %s)\n",
			buf, priv->name);
		ret = -EINVAL;
		goto exit_usb_boot;
	}

	bootm_data_init_defaults(&bootm_data);

	second_word++;
	if (strncmp(second_word, "usb", 3) == 0) {
		dev_info(dev, "Booting from USB drive\n");
		bootm_data.os_file = "/usb/linuximage.fit";
	} else if (strncmp(second_word, "recovery", 8) == 0) {
		dev_info(dev, "Booting internal recovery OS\n");
		bootm_data.os_file = "/dev/mmc2.5";
	} else {
		dev_err(dev, "Unknown boot target!\n");
		ret = -ENODEV;
		goto exit_usb_boot;
	}

	ret = globalvar_add_simple("linux.bootargs.root",
		"root=/dev/ram rw rootwait ramdisk_size=196608");
	if (ret)
		goto exit_usb_boot;

	if (verbose)
		bootm_data.verbose = verbose;
	if (dryrun)
		bootm_data.dryrun = dryrun;

	ret = bootm_boot(&bootm_data);
	if (ret)
		goto exit_usb_boot;

	return 0;

exit_usb_boot:
	dev_err(dev, "Failed to run usb boot: %s\n", strerror(-ret));

	return ret;
}

static void prt_imx6_bootentry_release(struct bootentry *entry)
{
	free(entry);
}

static int prt_imx6_bootentry_create(struct bootentries *bootentries, const char *name)
{
	struct bootentry *entry;

	entry = xzalloc(sizeof(*entry));
	if (!entry)
		return -ENOMEM;

	entry->me.type = MENU_ENTRY_NORMAL;
	entry->release = prt_imx6_bootentry_release;
	entry->boot = prt_imx6_usb_boot;
	entry->title = xstrdup(name);
	entry->description = xstrdup("Boot FIT image of a USB drive");
	bootentries_add_entry(bootentries, entry);

	return 0;
}

static int prt_imx6_bootentry_provider(struct bootentries *bootentries,
				     const char *name)
{
	int found = 0;
	unsigned int v;

	if (strncmp(name, "prt-usb", 7))
		return found;

	v = readl(OTG_PORTSC1);
	if ((v & 0x0c00) == 0)	/* No usb device detected */
		return found;

	if (!prt_imx6_bootentry_create(bootentries, name))
		found = 1;

	return found;
}

static int prt_imx6_env_init(struct prt_imx6_priv *priv)
{
	const struct prt_machine_data *dcfg = priv->dcfg;
	struct device *dev = priv->dev;
	char *delay, *bootsrc, *boot_targets;
	unsigned int autoboot_timeout;
	int ret;

	ret = setenv("global.linux.bootargs.base", "consoleblank=0 vt.color=0x00");
	if (ret)
		goto exit_env_init;

	if (priv->no_usb_check) {
		set_autoboot_state(AUTOBOOT_BOOT);
	} else {
		if (dcfg->flags & PRT_IMX6_USB_LONG_DELAY)
			autoboot_timeout = 4;
		else
			autoboot_timeout = 1;

		/* the usb_delay value is used for poller_call_async() */
		delay = basprintf("%d", autoboot_timeout);
		ret = setenv("global.autoboot_timeout", delay);
		free(delay);
		if (ret)
			goto exit_env_init;
	}

	if (dcfg->flags & PRT_IMX6_BOOTCHOOSER)
		bootsrc = "bootchooser";
	else
		bootsrc = "mmc2";

	if (!priv->no_usb_check)
		boot_targets = xasprintf("prt-usb %s", bootsrc);
	else
		boot_targets = xasprintf("%s prt-usb", bootsrc);

	ret = setenv("global.boot.default", boot_targets);
	free(boot_targets);
	if (ret)
		goto exit_env_init;

	dev_info(dev, "Board specific env init is done\n");
	return 0;

exit_env_init:
	dev_err(dev, "Failed to set env: %pe\n", ERR_PTR(ret));

	return ret;
}

static int prt_imx6_bbu(struct prt_imx6_priv *priv)
{
	const struct prt_machine_data *dcfg = priv->dcfg;
	u32 emmc_flags = 0;
	char *devicefile;
	int ret;

	if (dcfg->flags & PRT_IMX6_BOOTSRC_SPI_NOR) {
		ret = imx6_bbu_internal_spi_i2c_register_handler("SPI", "/dev/m25p0.barebox",
							 BBU_HANDLER_FLAG_DEFAULT);
		if (ret)
			goto exit_bbu;
	} else {
		emmc_flags = BBU_HANDLER_FLAG_DEFAULT;
	}

	devicefile = xasprintf("/dev/mmc%d", dcfg->emmc_usdhc);
	ret = imx6_bbu_internal_mmcboot_register_handler("eMMC", devicefile,
						   emmc_flags);
	if (ret)
		goto exit_bbu;

	devicefile = xasprintf("/dev/mmc%d", dcfg->sd_usdhc);

	ret = imx6_bbu_internal_mmc_register_handler("SD", devicefile, 0);
	if (ret)
		goto exit_bbu;

	return 0;
exit_bbu:
	dev_err(priv->dev, "Failed to register bbu: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_imx6_devices_init(void)
{
	struct prt_imx6_priv *priv = prt_priv;
	struct device *ocotp_dev;
	struct param_d *p;

	if (!priv)
		return 0;

	if (priv->dcfg->init)
		priv->dcfg->init(priv);

	prt_imx6_bbu(priv);

	/*
	 * Read serial number from fuses. On success we'll assume the imx_ocotp
	 * driver takes care of providing the mac address if needed. On
	 * failure we'll fallback to reading and setting serial and mac from an
	 * attached RFID eeprom.
	 */
	if (prt_imx6_read_ocotp_serial(priv) != 0)
		prt_imx6_read_i2c_mac_serial(priv);

	bootentry_register_provider(prt_imx6_bootentry_provider);

	prt_imx6_env_init(priv);

	ocotp_dev = get_device_by_name("ocotp0");
	if (ocotp_dev) {
		p = dev_add_param_string(ocotp_dev, "serial_number",
				 prt_imx6_set_ocotp_serial, NULL,
				 &priv->ocotp_serial, priv);
		if (IS_ERR(p))
			return PTR_ERR(p);
	}

	return 0;
}
late_initcall(prt_imx6_devices_init);

static int prt_imx6_init_kvg_set_ctrl(struct prt_imx6_priv *priv, bool val)
{
	int ret;

	ret = gpio_direction_output(GPIO_ON1_CTRL, val);
	if (ret)
		return ret;

	ret = gpio_direction_output(GPIO_ON2_CTRL, val);
	if (ret)
		return ret;

	return 0;
}

static int prt_imx6_yaco_set_kvg_power_mode(struct prt_imx6_priv *priv,
					    const char *serial)
{
	static const char command[] = "{\"command\":\"mode\",\"value\":\"kvg\",\"on2\":true}";
	struct device *dev = priv->dev;
	struct console_device *yccon;
	int ret;

	yccon = of_console_get_by_alias(serial);
	if (!yccon) {
		dev_dbg(dev, "Cant find the %s node, try later\n", serial);
		return -EPROBE_DEFER;
	}

	ret = console_set_baudrate(yccon, 115200);
	if (ret)
		goto exit_yaco_set_kvg_power_mode;

	yccon->puts(yccon, command, sizeof(command));

	dev_info(dev, "Send YaCO power init sequence to %s\n", serial);
	return 0;

exit_yaco_set_kvg_power_mode:
	dev_err(dev, "Failed to set YaCO pw mode: %pe", ERR_PTR(ret));

	return ret;
}

static int prt_imx6_init_kvg_power(struct prt_imx6_priv *priv,
				   enum prt_imx6_kvg_pw_mode pw_mode)
{
	const char *mode;
	int ret;

	ret = gpio_request_array(prt_imx6_kvg_gpios,
				 ARRAY_SIZE(prt_imx6_kvg_gpios));
	if (ret)
		goto exit_init_kvg_vicut;

	mdelay(1);

	if (!gpio_get_value(GPIO_DIP1_FB))
		pw_mode = PW_MODE_KUBOTA;

	switch (pw_mode) {
	case PW_MODE_KVG_WITH_YACO:
		mode = "KVG (with YaCO)";

		/* GPIO_ON1_CTRL and GPIO_ON2_CTRL are N.C. on the SoC for
		 * older revisions */

		/* Inform YaCO of power mode */
		ret = prt_imx6_yaco_set_kvg_power_mode(priv, "serial0");
		break;
	case PW_MODE_KVG_NEW:
		mode = "KVG (new)";

		ret = prt_imx6_init_kvg_set_ctrl(priv, true);
		if (ret)
			goto exit_init_kvg_vicut;
		break;
	case PW_MODE_KUBOTA:
		mode = "Kubota";
		ret = prt_imx6_init_kvg_set_ctrl(priv, false);
		if (ret)
			goto exit_init_kvg_vicut;
		break;
	default:
		ret = -ENODEV;
		goto exit_init_kvg_vicut;
	}

	dev_info(priv->dev, "Power mode: %s\n", mode);

	return 0;

exit_init_kvg_vicut:
	dev_err(priv->dev, "KvG power init failed: %i\n", ret);

	return ret;
}

static int prt_imx6_init_victgo(struct prt_imx6_priv *priv)
{
	int ret = 0;

	/* Bit 1 of HW-REV is pulled low by 2k2, but must be high on some
	 * revisions
	 */
	if (priv->hw_rev & 2) {
		ret = gpio_direction_output(IMX_GPIO_NR(2, 9), 1);
		if (ret) {
			dev_err(priv->dev, "Failed to set gpio up\n");
			return ret;
		}
	}

	return prt_imx6_init_kvg_power(priv, PW_MODE_KVG_NEW);
}

static int prt_imx6_init_prti6g(struct prt_imx6_priv *priv)
{
	struct regmap *gpr;

	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6ul-iomuxc-gpr");
	if (!IS_ERR(gpr)) {
		int ret;

		/* Configure FEC1 to use 50MHz clock provided by the PHY */
		ret = regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6UL_GPR1_ENET1_CLK_DIR | IMX6UL_GPR1_ENET1_CLK_SEL,
			IMX6UL_GPR1_ENET1_CLK_SEL);
		if (ret)
			dev_err(priv->dev, "regmap error\n");
	} else {
		dev_err(priv->dev, "failed to find fsl,imx6ul-iomux-gpr regmap\n");
	}

	return 0;
}

static int prt_imx6_init_kvg_new(struct prt_imx6_priv *priv)
{
	return prt_imx6_init_kvg_power(priv, PW_MODE_KVG_NEW);
}

static int prt_imx6_init_kvg_yaco(struct prt_imx6_priv *priv)
{
	return prt_imx6_init_kvg_power(priv, PW_MODE_KVG_WITH_YACO);
}

#define GPIO_KEY_F6     (0xe0 + 5)
#define GPIO_KEY_CYCLE  (0xe0 + 2)

static int prt_imx6_init_prtvt7(struct prt_imx6_priv *priv)
{
	/* This function relies heavely on the gpio-pca9539 driver */

	gpio_direction_input(GPIO_KEY_F6);
	gpio_direction_input(GPIO_KEY_CYCLE);

	if (gpio_get_value(GPIO_KEY_CYCLE) && gpio_get_value(GPIO_KEY_F6))
		priv->no_usb_check = 1;

	return 0;
}

static int prt_imx6_init_prtwd3(struct prt_imx6_priv *priv)
{
	void __iomem *iomux = (void *)MX6_IOMUXC_BASE_ADDR;
	uint32_t val;

	val = readl(iomux + IOMUXC_GPR1);
	val |= IMX6Q_GPR1_ENET_CLK_SEL_ANATOP;
	writel(val, iomux + IOMUXC_GPR1);

	return 0;
}

static int prt_imx6_rfid_fixup(struct prt_imx6_priv *priv,
			       struct device_node *root)
{
	const struct prt_machine_data *dcfg = priv->dcfg;
	struct device_node *node, *i2c_node;
	char *eeprom_node_name, *alias;
	int na, ns, len = 0;
	int ret;
	u8 *tmp;

	alias = xasprintf("i2c%d", dcfg->i2c_adapter);

	i2c_node = of_find_node_by_alias(root, alias);
	kfree(alias);
	if (!i2c_node) {
		dev_err(priv->dev, "Unsupported i2c adapter\n");
		return -ENODEV;
	}

	eeprom_node_name = xasprintf("/eeprom@%x", dcfg->i2c_addr);

	node = of_create_node(i2c_node, eeprom_node_name);
	if (!node) {
		dev_err(priv->dev, "Failed to create node %s\n",
			eeprom_node_name);
		return -ENOMEM;
	}

	ret = of_property_write_string(node, "compatible", "atmel,24c256");
	if (ret)
		goto free_eeprom;

	na = of_n_addr_cells(node);
	ns = of_n_size_cells(node);
	tmp = xzalloc((na + ns) * 4);

	of_write_number(tmp + len, dcfg->i2c_addr, na);
	len += na * 4;
	of_write_number(tmp + len, 0, ns);
	len += ns * 4;

	ret = of_set_property(node, "reg", tmp, len, 1);
	kfree(tmp);
	if (ret)
		goto free_eeprom;

	return 0;
free_eeprom:
	kfree(eeprom_node_name);
	dev_err(priv->dev, "Failed to apply fixup: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_imx6_of_fixup(struct device_node *root, void *data)
{
	struct prt_imx6_priv *priv = data;
	int ret;

	if (!root) {
		dev_err(priv->dev, "Unable to find the root node\n");
		dump_stack();
		return -ENODEV;
	}

	ret = prt_imx6_rfid_fixup(priv, root);
	if (ret)
		goto exit_of_fixups;

	return 0;
exit_of_fixups:
	dev_err(priv->dev, "Failed to apply OF fixups: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_imx6_get_id(struct prt_imx6_priv *priv)
{
	struct gpio gpios_type[] = GPIO_HW_TYPE_ID;
	struct gpio gpios_rev[] = GPIO_HW_REV_ID;
	struct device_node *gpio_np = NULL;
	int ret;

	gpio_np = of_find_node_by_name_address(NULL, "gpio@20a0000");
	if (!gpio_np)
		return -ENODEV;

	ret = of_device_ensure_probed(gpio_np);
	if (ret)
		return ret;

	ret = gpio_array_to_id(gpios_type, ARRAY_SIZE(gpios_type), &priv->hw_id);
	if (ret)
		goto exit_get_id;

	ret = gpio_array_to_id(gpios_rev, ARRAY_SIZE(gpios_rev), &priv->hw_rev);
	if (ret)
		goto exit_get_id;

	return 0;
exit_get_id:
	dev_err(priv->dev, "Failed to read gpio ID: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_imx6_get_dcfg(struct prt_imx6_priv *priv)
{
	const struct prt_machine_data *dcfg, *found = NULL;
	int ret;

	dcfg = of_device_get_match_data(priv->dev);
	if (!dcfg) {
		ret = -EINVAL;
		goto exit_get_dcfg;
	}

	for (; dcfg->hw_id != UINT_MAX; dcfg++) {
		if (dcfg->hw_id != priv->hw_id)
			continue;
		if (dcfg->hw_rev > priv->hw_rev)
			break;
		found = dcfg;
	}

	if (!found) {
		ret = -ENODEV;
		goto exit_get_dcfg;
	}

	priv->dcfg = found;

	return 0;
exit_get_dcfg:
	dev_err(priv->dev, "Failed to get dcfg: %i\n", ret);
	return ret;
}

static int prt_imx6_probe(struct device *dev)
{
	struct prt_imx6_priv *priv;
	struct param_d *p;
	int ret;

	priv = xzalloc(sizeof(*priv));
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->name = of_get_machine_compatible();

	pr_info("Detected machine type: %s\n", priv->name);

	ret = prt_imx6_get_id(priv);
	if (ret)
		goto free_priv;

	pr_info("  HW type:     %d\n", priv->hw_id);
	pr_info("  HW revision: %d\n", priv->hw_rev);
	prt_of_fixup_hwrev(priv);

	ret = prt_imx6_get_dcfg(priv);
	if (ret)
		goto free_priv;

	p = dev_add_param_uint32_ro(dev, "boardrev", &priv->hw_rev, "%u");
	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto free_priv;
	}

	p = dev_add_param_uint32_ro(dev, "boardid", &priv->hw_id, "%u");
	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto free_priv;
	}

	ret = prt_imx6_of_fixup(of_get_root_node(), priv);
	if (ret)
		goto free_priv;

	prt_priv = priv;

	return 0;
free_priv:
	kfree(priv);
	return ret;
}

static const struct prt_machine_data prt_imx6_cfg_alti6p[] = {
	{
		.hw_id = HW_TYPE_ALTI6P,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_EMMC,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_victgo[] = {
	{
		.hw_id = HW_TYPE_VICTGO,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_victgo,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_vicut1[] = {
	{
		.hw_id = HW_TYPE_VICUT1,
		.hw_rev = 0,
		.i2c_addr = 0x50,
		.i2c_adapter = 1,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_VICUT1,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_yaco,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_VICUT2,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_new,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_vicut1q[] = {
	{
		.hw_id = HW_TYPE_VICUT1,
		.hw_rev = 0,
		.i2c_addr = 0x50,
		.i2c_adapter = 1,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_VICUT1,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_yaco,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_VICUT2,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_yaco,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_VICUT2,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_new,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_vicutp[] = {
	{
		.hw_id = HW_TYPE_VICUT2,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_kvg_new,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_lanmcu[] = {
	{
		.hw_id = HW_TYPE_LANMCU,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_EMMC | PRT_IMX6_BOOTCHOOSER,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_plybas[] = {
	{
		.hw_id = HW_TYPE_PLYBAS,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR | PRT_IMX6_USB_LONG_DELAY,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_plym2m[] = {
	{
		.hw_id = HW_TYPE_PLYM2M,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR | PRT_IMX6_USB_LONG_DELAY,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prti6g[] = {
	{
		.hw_id = HW_TYPE_PRTI6G,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 1,
		.sd_usdhc = 0,
		.init = prt_imx6_init_prti6g,
		.flags = PRT_IMX6_BOOTSRC_EMMC | PRT_IMX6_BOOTCHOOSER,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prti6q[] = {
	{
		.hw_id = HW_TYPE_PRTI6Q,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 2,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = HW_TYPE_PRTI6Q,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prtmvt[] = {
	{
		.hw_id = HW_TYPE_PRTMVT,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prtrvt[] = {
	{
		.hw_id = HW_TYPE_PRTRVT,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_SPI_NOR,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prtvt7[] = {
	{
		.hw_id = HW_TYPE_PRTVT7,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_prtvt7,
		.flags = PRT_IMX6_BOOTSRC_EMMC | PRT_IMX6_BOOTCHOOSER |
			PRT_IMX6_USB_LONG_DELAY,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prtwd2[] = {
	{
		.hw_id = HW_TYPE_PRTWD2,
		.hw_rev = 0,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.flags = PRT_IMX6_BOOTSRC_EMMC,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_prtwd3[] = {
	{
		.hw_id = HW_TYPE_PRTWD2,
		.hw_rev = 2,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 2,
		.sd_usdhc = 0,
		.init = prt_imx6_init_prtwd3,
		.flags = PRT_IMX6_BOOTSRC_EMMC,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct prt_machine_data prt_imx6_cfg_jozacp[] = {
	{
		.hw_id = HW_TYPE_JOZACP,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 0,
		.sd_usdhc = 2,
		.flags = PRT_IMX6_BOOTSRC_EMMC | PRT_IMX6_BOOTCHOOSER,
	}, {
		.hw_id = HW_TYPE_JOZACPP,
		.hw_rev = 1,
		.i2c_addr = 0x51,
		.i2c_adapter = 0,
		.emmc_usdhc = 0,
		.sd_usdhc = 2,
		.flags = PRT_IMX6_BOOTSRC_EMMC | PRT_IMX6_BOOTCHOOSER,
	}, {
		.hw_id = UINT_MAX
	},
};

static const struct of_device_id prt_imx6_of_match[] = {
	{ .compatible = "alt,alti6p", .data = &prt_imx6_cfg_alti6p },
	{ .compatible = "kvg,victgo", .data = &prt_imx6_cfg_victgo },
	{ .compatible = "kvg,vicut1", .data = &prt_imx6_cfg_vicut1 },
	{ .compatible = "kvg,vicut1q", .data = &prt_imx6_cfg_vicut1q },
	{ .compatible = "kvg,vicutp", .data = &prt_imx6_cfg_vicutp },
	{ .compatible = "vdl,lanmcu", .data = &prt_imx6_cfg_lanmcu },
	{ .compatible = "ply,plybas", .data = &prt_imx6_cfg_plybas },
	{ .compatible = "ply,plym2m", .data = &prt_imx6_cfg_plym2m },
	{ .compatible = "prt,prti6g", .data = &prt_imx6_cfg_prti6g },
	{ .compatible = "prt,prti6q", .data = &prt_imx6_cfg_prti6q },
	{ .compatible = "prt,prtmvt", .data = &prt_imx6_cfg_prtmvt },
	{ .compatible = "prt,prtrvt", .data = &prt_imx6_cfg_prtrvt },
	{ .compatible = "prt,prtvt7", .data = &prt_imx6_cfg_prtvt7 },
	{ .compatible = "prt,prtwd2", .data = &prt_imx6_cfg_prtwd2 },
	{ .compatible = "prt,prtwd3", .data = &prt_imx6_cfg_prtwd3 },
	{ .compatible = "joz,jozacp", .data = &prt_imx6_cfg_jozacp },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(prt_imx6_of_match);

static struct driver prt_imx6_board_driver = {
	.name = "board-protonic-imx6",
	.probe = prt_imx6_probe,
	.of_compatible = DRV_OF_COMPAT(prt_imx6_of_match),
};
postcore_platform_driver(prt_imx6_board_driver);
