// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix
// SPDX-FileCopyrightText: Leica Geosystems AG

#include <block.h>
#include <boot.h>
#include <bootm.h>
#include <bootsource.h>
#include <command.h>
#include <common.h>
#include <envfs.h>
#include <environment.h>
#include <fastboot.h>
#include <fcntl.h>
#include <file-list.h>
#include <globalvar.h>
#include <gpio.h>
#include <hab.h>
#include <init.h>
#include <pinctrl.h>
#include <string.h>
#include <system-partitions.h>
#include <watchdog.h>

#include <linux/string.h>
#include <linux/usb/gadget-multi.h>

#include <mach/imx/bbu.h>
#include <mach/imx/generic.h>
#include <mach/imx/ocotp.h>
#include <mach/imx/ocotp-fusemap.h>

#include <boards/hgs/common.h>

static bool hgs_first_boot;
static bool hgs_enable_usbgadget;
static struct hgs_machine *priv;

static unsigned int hgs_get_key_idx(void)
{
	return CONFIG_HABV4_SRK_INDEX;
}

static void hgs_setup_default_machine_options(struct hgs_machine *machine)
{
	if (isempty(machine->mmc_alias))
		machine->mmc_alias = "mmc2";

	if (!barebox_hostname_is_valid(machine->hostname)) {
		const char *fallback_host, *soc_uid;

		switch (machine->type) {
		case HGS_HW_GS05:
			fallback_host = "GS05";
			break;
		default:
			fallback_host = "unknown-hgs-host";
			break;
		}

		free(machine->hostname);
		soc_uid = getenv("soc0.serial_number");
		machine->hostname = xasprintf("%s-%s", fallback_host, soc_uid);
	}
}

static int hgs_mmc_ensure_probed(struct hgs_machine *machine)
{
	struct device_node *np;

	/*
	 * We can't use bootsource_*() helpers since the correct MMC device
	 * needs to be available for USB-Serial-Download boots as well e.g.
	 * during first setup which performs the eMMC setup.
	 *
	 * The barebox /dev/mmcX enumeration matches the OF aliases
	 */
	np = of_find_node_by_alias(NULL, machine->mmc_alias);
	if (IS_ERR(np)) {
		dev_err(machine->dev, "Failed to find /aliases/%s OF node\n",
			machine->mmc_alias);
		return PTR_ERR(np);
	}

	/* Ensure that the MMC and the below partitions are available */
	machine->mmc_cdev = of_cdev_find(np);
	if (IS_ERR(machine->mmc_cdev)) {
		dev_err(machine->dev, "Failed to find /dev/%s\n",
			machine->mmc_alias);
		return PTR_ERR(np);
	}

	return 0;
}

static int hgs_fixup_names(struct hgs_machine *machine)
{
	const struct device *dev = machine->dev;
	int err;

	err = of_prepend_machine_compatible(NULL, machine->dts_compatible);
	if (err) {
		dev_err(dev, "Failed to fixup compatible\n");
		return -EINVAL;
	}

	barebox_set_hostname(machine->hostname);
	barebox_set_model(machine->model);

	return 0;
}

static int hgs_setup_emmc(struct hgs_machine *machine)
{
	static const char * const mmc_commands[] = { "enh_area", "write_reliability" };
	int partitioning_completed;
	unsigned int i;
	char *tmp;
	int error;

	tmp = xasprintf("%s.partitioning_completed", machine->mmc_alias);
	getenv_bool(tmp, &partitioning_completed);
	free(tmp);
	if (partitioning_completed)
		return 0;

	for (i = 0; i < ARRAY_SIZE(mmc_commands); i++) {
		int error;

		error = run_command("mmc %s /dev/%s", mmc_commands[i],
				    machine->mmc_alias);
		if (error) {
			dev_err(machine->dev, "skip eMMC partition-complete\n");
			return -EINVAL;
		}
	}

	/* Make the final complete not part of the loop */
	error = run_command("mmc partition_complete /dev/%s", machine->mmc_alias);
	if (error)
		return -EINVAL;

	dev_info(machine->dev, "Initial eMMC setup completed successfully\n");
	return 0;
}

static int
hgs_fastboot_cmd_flash(struct fastboot *fb, struct file_list_entry *entry,
			 const char *filename, size_t len)
{
	/*
	 * Ensure to trigger a data partition wipe if the the eMMC is going to
	 * be flashed.
	 */
	if (!strncmp(entry->name, "eMMC", 4))
		globalvar_add_simple("linux.bootargs.hgs_datapart_handling",
				     "format_data_partition");

	/* We don't handle the flash, we just want to get notified */
	return FASTBOOT_CMD_FALLTHROUGH;
}

static void hgs_register_bbu_handlers(struct hgs_machine *machine)
{
	char *dev;
	int ret;

	dev = xasprintf("/dev/%s", machine->mmc_alias);
	ret = imx8m_bbu_internal_mmcboot_register_handler("eMMC", dev,
						      BBU_HANDLER_FLAG_DEFAULT);
	if (ret) {
		dev_warn(machine->dev, "Failed to register bbu-eMMC\n");
		free(dev);
	}
}

static int hgs_console_open_fixup(struct device_node *root, void *context)
{
	struct hgs_machine *machine = context;
	struct device_node *console_np;
	struct property *property;

	console_np = of_find_node_by_alias(root, machine->console_alias);
	if (!console_np)
		return -EINVAL;

	property = of_rename_property(console_np, "pinctrl-1", "pinctrl-0");
	if (!property)
		return -EINVAL; /* pinctrl-1 not found */

	return 0;
}

static int hgs_open_console(struct hgs_machine *machine)
{
	struct console_device *console;
	struct pinctrl *pinctrl;
	int err;

	err = of_device_ensure_probed_by_alias(machine->console_alias);
	if (err)
		return err;

	console = of_console_get_by_alias(machine->console_alias);
	if (!console)
		return -EINVAL;

	pinctrl = pinctrl_get_select(console->dev, "uart");
	if (IS_ERR(pinctrl))
		return PTR_ERR(pinctrl);

	err = console_set_active(console, CONSOLE_STDIOE);
	if (err)
		return err;

	console_set_stdoutpath(console, CONFIG_BAUDRATE);

	of_register_fixup(hgs_console_open_fixup, machine);

	return 0;
}

static bool
hgs_data_partition_available(struct hgs_machine *machine)
{
	struct cdev *data_cdev;

	data_cdev = cdev_find_partition(machine->mmc_cdev, "data");
	/*
	 * MMC needs to be formated if the data partition is not available.
	 * This is a valid use-case.
	 */
	if (!data_cdev) {
		dev_info(machine->dev, "MMC data partition not found, trigger (re-)format\n");
		return false;
	}

	if (!cdev_is_block_partition(data_cdev)) {
		dev_warn(machine->dev, "%s is not a block device, trigger (re-)format\n",
			 cdev_name(data_cdev));
		return false;
	}

	return true;
}

static void hgs_set_common_boot_options(struct hgs_machine *machine)
{
	boot_set_default("bootchooser rescue");

	/*
	 * Check if the data partition does exist and if not inform the
	 * userspace to (re-)format the data partition.
	 */
	if (!hgs_data_partition_available(machine))
		globalvar_add_simple("linux.bootargs.hgs_datapart_handling",
				     "format_data_partition");
}

static bool hgs_field_return_mode(struct hgs_machine *machine)
{
	const struct device *dev = machine->dev;
	unsigned int field_return = 0;
	int ret;

	ret = imx_ocotp_read_field(MX8M_OCOTP_FIELD_RETURN, &field_return);
	if (ret) {
		dev_err(dev, "Failed to query the field-return status\n");
		return false;
	}

	return field_return;
}

static int hgs_development_boot(struct hgs_machine *machine)
{
	const char *run_mode;

	/* Allow barebox rw environment */
	of_device_enable_path("/chosen/environment-emmc");

	if (hgs_field_return_mode(machine))
		run_mode = "field_return_mode";
	else
		run_mode = "developer_mode";

	/* Inform user-space about the run mode */
	globalvar_add_simple("linux.bootargs.hgs_runmode", run_mode);

	/*
	 * Init system.partitions for fastboot default, can be overridden in
	 * dev-mode via nv
	 */
	globalvar_add_simple("system.partitions", "/dev/mmc2(eMMC)");

	hgs_enable_usbgadget = true;

	/* Allow shell and mux stdin and stdout correctly */
	return hgs_open_console(machine);
}

static int hgs_release_boot(struct hgs_machine *machine)
{
	set_fastboot_bbu(0);
	usbgadget_autostart(0);

	/* Only signed fit images are allowed */
	bootm_force_signed_images();

	/*
	 * Explicit disable the console else the kernel may grap any console
	 * which is available. This can cause issues like BT issues, because
	 * the kernel may grap the BT serdev.
	 */
	globalvar_add_simple("linux.bootargs.console", "console=\"\"");

	return 0;
}

static int hgs_run_first_boot(struct hgs_machine *machine)
{
	hgs_first_boot = true;

	hgs_open_console(machine);

	hgs_enable_usbgadget = true;

	/*
	 * Prohibit booting for the first boot and instead wait for explicit
	 * fastboot reset command.
	 */
	boot_set_default("does-not-exist");
	set_autoboot_state(AUTOBOOT_ABORT);

	/*
	 * Now we need to wait for the fastboot to reset the device to finish
	 * the eMMC setup and to boot from eMMC boot partition next time.
	 */
	return 0;
}

static bool hgs_is_first_boot(struct hgs_machine *machine)
{
	/*
	 * Allow machines which are in early development stage to skip the
	 * initial setup
	 */
	if (machine->skip_firstboot_setup) {
		dev_warn(machine->dev, "Skipping first boot procedure on development device!\n");
		return false;
	}

	/* Device is not locked down -> this is the first boot */
	if (!imx_hab_device_locked_down())
		return true;

	return false;
}

static int hgs_verify_key_usage(struct hgs_machine *machine)
{
	unsigned int current_key = hgs_get_key_idx();
	unsigned int revoked_keys_map;
	int ret = 0;
	int key;

	/* Nothing to do for us */
	if (current_key == 0)
		goto out;

	if (bootsource_get() == BOOTSOURCE_SERIAL)
		goto out;

	ret = imx_ocotp_read_field(MX8M_OCOTP_SRK_REVOKE, &revoked_keys_map);
	if (ret)
		goto out;

	/* Revoke all key below the current one */
	for (key = current_key - 1; key >= 0; key--) {
		/* Key is already revoked -> skip */
		if (revoked_keys_map & BIT(key))
			continue;
		dev_info(machine->dev, "Revoking SRK key %i\n", key);
		imx_hab_revoke_key(key, true);
	}

out:
	imx_ocotp_lock_srk_revoke();
	return ret;
}

static int hgs_open_device(void)
{
	/*
	 * Expose barebox fastboot partition and enable fastboot to let the user
	 * flash an development barebox and to reboot the system via fastboot.
	 */
	hgs_enable_usbgadget = true;

	/*
	 * Prohibit booting and instead wait for explicit fastboot reset
	 * command. This is required to let the i.MX8M hardware re-evaluate the
	 * FIELD_RETURN fuse again.
	 */
	boot_set_default("does-not-exist");
	set_autoboot_state(AUTOBOOT_ABORT);

	/* Field return unlocked -> we need to open the device */
	return imx_hab_field_return(true);
}

static bool hgs_open_device_request(void)
{
	if (IS_ENABLED(CONFIG_HABV4_CSF_UNLOCK_FIELD_RETURN)) {
		if (!imx_ocotp_field_return_locked())
			return true;
	}

	return false;
}

int hgs_common_boot(struct hgs_machine *machine)
{
	int ret;

	hgs_setup_default_machine_options(machine);

	ret = hgs_mmc_ensure_probed(machine);
	if (ret)
		return ret;

	ret = hgs_fixup_names(machine);
	if (ret)
		return ret;

	hgs_register_bbu_handlers(machine);

	priv = machine;

	if (hgs_is_first_boot(machine))
		return hgs_run_first_boot(machine);

	if (hgs_open_device_request())
		return hgs_open_device();

	ret = hgs_verify_key_usage(machine);
	if (ret)
		return ret;

	hgs_set_common_boot_options(machine);

	if (hgs_get_built_type() == HGS_DEV_BUILD)
		return hgs_development_boot(machine);
	else
		return hgs_release_boot(machine);
}

/* --------------------- Additional initcalls ---------------------- */
struct hgs_fusemap {
	uint32_t field;
	unsigned int val;
};

#define SIMPLE_FUSE(fuse)	\
{				\
	.field = fuse,		\
	.val = 0x1,		\
}

#define CUSTOM_FUSE(fuse, _val)	\
{				\
	.field = fuse,		\
	.val = _val,		\
}

/*
 * We need to run this after the environment_initcall() is done since we need
 * access to /env.
 */
static int hgs_run_first_boot_setup(void)
{
	unsigned int flags = IMX_SRK_HASH_WRITE_PERMANENT |
			     IMX_SRK_HASH_WRITE_LOCK;
	const struct hgs_fusemap common_fusemap[] = {
		/* Security */
		/* MX8M_OCOTP_SEC_CONFIG[1] is done during imx_hab_lockdown_device() */
		/* MX8M_OCOTP_SRK_HASH[255:0] is done during imx_hab_write_srk_hash_file() */
		SIMPLE_FUSE(MX8M_OCOTP_TZASC_EN),
		/* Debug */
		SIMPLE_FUSE(MX8M_OCOTP_KTE),
		SIMPLE_FUSE(MX8M_OCOTP_SJC_DISABLE),
		CUSTOM_FUSE(MX8M_OCOTP_JTAG_SMODE, 0x3),
		SIMPLE_FUSE(MX8M_OCOTP_JTAG_HEO),
		/* Boot */
		SIMPLE_FUSE(MX8M_OCOTP_FORCE_COLD_BOOT),
		SIMPLE_FUSE(MX8M_OCOTP_RECOVERY_SDMMC_BOOT_DIS),
		/* eFuse locks */
		CUSTOM_FUSE(MX8M_OCOTP_USB_ID_LOCK, 0x3),
		SIMPLE_FUSE(MX8M_OCOTP_SJC_RESP_LOCK),
		/* MX8M_OCOTP_SRK_LOCK is done during imx_hab_write_srk_hash_file() */
	};
	const struct hgs_fusemap imx8mp_fusemap[] = {
		/* Boot */
		SIMPLE_FUSE(MX8MP_OCOTP_ROM_NO_LOG),
	};
	const struct hgs_fusemap *iter;
	struct device *dev = priv->dev;
	struct watchdog *wdg;
	unsigned int i;
	int err;

	if (!hgs_first_boot)
		return 0;

	/* Before doing anything inform the EFI to not power-cycle us */
	wdg = watchdog_get_by_name("efiwdt");
	if (wdg) {
		err = watchdog_ping(wdg);
		if (err)
			dev_warn(dev, "Failed to ping the EFI watchdog\n");
	} else {
		dev_warn(dev, "Failed to find and ping EFI watchdog\n");
	}

	/* eMMC setup must always run first! */
	err = hgs_setup_emmc(priv);
	if (err)
		return err;

	err = imx_hab_write_srk_hash_file("/env/imx-srk-fuse.bin", flags);
	if (err && err != -EEXIST) {
		dev_err(dev, "Failed to burn SRK fuses\n");
		return err;
	}
	dev_info(dev, "SRK fuses burnt successfully\n");

	err = imx_ocotp_permanent_write(1);
	if (err) {
		dev_err(dev, "Failed to enable permanent write\n");
		return err;
	}

	for (i = 0; i < ARRAY_SIZE(common_fusemap); i++) {
		iter = &common_fusemap[i];
		err = imx_ocotp_write_field(iter->field, iter->val);
	}

	if (cpu_is_mx8mp()) {
		for (i = 0; i < ARRAY_SIZE(imx8mp_fusemap); i++) {
			iter = &imx8mp_fusemap[i];
			err = imx_ocotp_write_field(iter->field, iter->val);
		}
	}

	imx_ocotp_permanent_write(0);

	if (err) {
		dev_err(dev, "Failed to burn individual fuses\n");
		return err;
	}
	dev_info(dev, "Burning of individual fuses succeeded\n");

	/*
	 * Lockdown the device at the end since this signals barebox that the
	 * initial (first-boot) is done.
	 */
	err = imx_hab_lockdown_device(flags);
	if (err) {
		dev_err(dev, "Failed to lockdown the device\n");
		return err;
	}
	dev_info(dev, "Lockdown of the device succeeded\n");
	return 0;
}

/*
 * Notify the PP4 at the latest point to ensure that barebox was started
 * properly. If this function is not reached the PP4 timeout of 10sec is
 * triggered which puts us into serial-downloader mode.
 */
static int hgs_notify_pp4(void)
{
	struct device *efi;
	int ret;

	efi = get_device_by_name("efi");
	ret = dev_set_param(efi, "cpu_rdy", "1");
	if (ret)
		return ret;

	return 0;
}

/*
 * Custom USB gadget handling since we need to get notified upon the flash of
 * the data partition. The setup is done very late like the
 * usbgadget_autostart_init() since we need to ensure that the NV is loaded
 * properly for development case.
 */
static int hgs_usbgadget_autostart(void)
{
	struct usbgadget_funcs funcs = {};
	struct f_multi_opts *opts;
	int ret;

	if (!hgs_enable_usbgadget)
		return 0;

	/*
	 * Some HW is missing a HW serial connecotr. Fastboot is used for file
	 * download.
	 */
	funcs.flags |= USBGADGET_EXPORT_BBU | USBGADGET_FASTBOOT | USBGADGET_ACM;

	opts = usbgadget_prepare(&funcs);
	if (IS_ERR(opts))
		return PTR_ERR(opts);

	/* Custom hook to get notified once the user want to flash something */
	opts->fastboot_opts.cmd_flash = hgs_fastboot_cmd_flash;

	ret = usbgadget_register(opts);
	if (ret)
		usb_multi_opts_release(opts);

	return ret;
}

static int hgs_postenvironment_initcall(void)
{
	int ret;

	/*
	 * Guard this initcall via 'priv' which is set
	 * during hgs_common_boot().
	 */
	if (!priv)
		return 0;

	ret = hgs_notify_pp4();
	if (ret)
		return ret;

	ret = hgs_run_first_boot_setup();
	if (ret)
		return ret;

	ret = hgs_usbgadget_autostart();
	if (ret)
		return ret;

	return 0;
}
postenvironment_initcall(hgs_postenvironment_initcall);
