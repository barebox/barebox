// SPDX-License-Identifier: GPL-2.0-only
/*
 * init.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#ifdef CONFIG_DEBUG_LL
#define DEBUG
#endif

#define pr_fmt(fmt) "efi-init: " fmt

#include <linux/linkage.h>
#include <common.h>
#include <linux/sizes.h>
#include <memory.h>
#include <clock.h>
#include <command.h>
#include <magicvar.h>
#include <init.h>
#include <restart.h>
#include <poweroff.h>
#include <driver.h>
#include <platform_data/serial-ns16550.h>
#include <io.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <linux/err.h>
#include <boot.h>
#include <fs.h>
#include <binfmt.h>
#include <wchar.h>
#include <envfs.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>
#include <libfile.h>
#include <state.h>
#include <bbu.h>
#include <generated/utsrelease.h>

struct efi_runtime_services *RT;
struct efi_boot_services *BS;
struct efi_system_table *efi_sys_table;
efi_handle_t efi_parent_image;
struct efi_device_path *efi_device_path;
struct efi_loaded_image *efi_loaded_image;

void *efi_get_variable(char *name, efi_guid_t *vendor, int *var_size)
{
	efi_status_t efiret;
	void *buf;
	unsigned long size = 0;
	s16 *name16 = xstrdup_char_to_wchar(name);

	efiret = RT->get_variable(name16, vendor, NULL, &size, NULL);

	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL) {
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	buf = malloc(size);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		goto out;
	}

	efiret = RT->get_variable(name16, vendor, NULL, &size, buf);
	if (EFI_ERROR(efiret)) {
		free(buf);
		buf = ERR_PTR(-efi_errno(efiret));
		goto out;
	}

	if (var_size)
		*var_size = size;

out:
	free(name16);

	return buf;
}

int efi_set_variable(char *name, efi_guid_t *vendor, uint32_t attributes,
		     void *buf, unsigned long size)
{
	efi_status_t efiret = EFI_SUCCESS;
	s16 *name16 = xstrdup_char_to_wchar(name);

	efiret = RT->set_variable(name16, vendor, attributes, size, buf);

	free(name16);

	return -efi_errno(efiret);
}

int efi_set_variable_usec(char *name, efi_guid_t *vendor, uint64_t usec)
{
	char buf[20];
	wchar_t buf16[40];

	snprintf(buf, sizeof(buf), "%lld", usec);
	strcpy_char_to_wchar(buf16, buf);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf16,
				(strlen(buf)+1) * sizeof(wchar_t));
}

static int efi_set_variable_printf(char *name, efi_guid_t *vendor, const char *fmt, ...)
{
	va_list args;
	char *buf;
	wchar_t *buf16;

	va_start(args, fmt);
	buf = xvasprintf(fmt, args);
	va_end(args);
	buf16 = xstrdup_char_to_wchar(buf);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf16,
				(strlen(buf)+1) * sizeof(wchar_t));
	free(buf);
	free(buf16);
}

static int efi_set_variable_uint64_le(char *name, efi_guid_t *vendor, uint64_t value)
{
	uint8_t buf[8];

	buf[0] = (uint8_t)(value >> 0U & 0xFF);
	buf[1] = (uint8_t)(value >> 8U & 0xFF);
	buf[2] = (uint8_t)(value >> 16U & 0xFF);
	buf[3] = (uint8_t)(value >> 24U & 0xFF);
	buf[4] = (uint8_t)(value >> 32U & 0xFF);
	buf[5] = (uint8_t)(value >> 40U & 0xFF);
	buf[6] = (uint8_t)(value >> 48U & 0xFF);
	buf[7] = (uint8_t)(value >> 56U & 0xFF);

	return efi_set_variable(name, vendor,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS, buf,
				sizeof(buf));
}

struct efi_boot {
	u32 attributes;
	u16 file_path_len;
	char *description;
	struct efi_device_path *path;
	void *binary;
};

static struct efi_boot *efi_get_boot(int num)
{
	struct efi_boot *boot = xzalloc(sizeof(*boot));
	void *buf, *ptr;
	int size;
	char *name;

	name = xasprintf("Boot%04X", num);

	buf = efi_get_global_var(name, &size);

	free(name);

	if (IS_ERR(buf)) {
		free(boot);
		return NULL;
	}

	ptr = buf;

	boot->attributes = *(u32 *)ptr;

	ptr += sizeof(u32);

	boot->file_path_len = *(u16 *)ptr;

	ptr += sizeof(u16);

	boot->description = xstrdup_wchar_to_char(ptr);

	ptr += (strlen(boot->description) + 1) * 2;

	printf("description: %s\n", boot->description);

	boot->path = memdup(ptr, boot->file_path_len);

	printf("path: %s\n", device_path_to_str(boot->path));

	return boot;
}

static int misc_init(void)
{
	efi_get_boot(1);
	efi_get_boot(2);
	efi_get_boot(3);

	return 0;
}
late_efi_initcall(misc_init);

static struct NS16550_plat ns16550_plat = {
	.clock = 115200 * 16,
};

static int efi_console_init(void)
{
	barebox_set_model("barebox EFI payload");

	add_generic_device("efi-stdio", DEVICE_ID_SINGLE, NULL, 0 , 0, 0, NULL);

	if (IS_ENABLED(CONFIG_X86))
		add_ns16550_device(0, 0x3f8, 0x10, IORESOURCE_IO | IORESOURCE_MEM_8BIT,
					&ns16550_plat);

	return 0;
}
console_efi_initcall(efi_console_init);

static void __noreturn efi_restart_system(struct restart_handler *rst)
{
	RT->reset_system(EFI_RESET_WARM, EFI_SUCCESS, 0, NULL);

	hang();
}

static void __noreturn efi_poweroff_system(struct poweroff_handler *handler)
{
	shutdown_barebox();
	RT->reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn("efi", efi_restart_system);
	poweroff_handler_register_fn(efi_poweroff_system);

	return 0;
}
coredevice_efi_initcall(restart_register_feature);

extern char image_base[];
extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

static int efi_init(void)
{
	char *env;

	defaultenv_append_directory(env_efi);

	env = xasprintf("/efivars/barebox-env-%pUl", &efi_barebox_vendor_guid);
	default_environment_path_set(env);
	free(env);

	return 0;
}
device_efi_initcall(efi_init);

static int efi_core_init(void)
{
	struct device *dev;
	int ret;

	dev = device_alloc("efi-cs", DEVICE_ID_SINGLE);
	ret = platform_device_register(dev);
	if (ret)
		return ret;

	dev = device_alloc("efi-wdt", DEVICE_ID_SINGLE);
	return platform_device_register(dev);
}
core_efi_initcall(efi_core_init);

/* Features of the loader, i.e. systemd-boot, barebox (imported from systemd) */
#define EFI_LOADER_FEATURE_CONFIG_TIMEOUT          (1LL << 0)
#define EFI_LOADER_FEATURE_CONFIG_TIMEOUT_ONE_SHOT (1LL << 1)
#define EFI_LOADER_FEATURE_ENTRY_DEFAULT           (1LL << 2)
#define EFI_LOADER_FEATURE_ENTRY_ONESHOT           (1LL << 3)
#define EFI_LOADER_FEATURE_BOOT_COUNTING           (1LL << 4)
#define EFI_LOADER_FEATURE_XBOOTLDR                (1LL << 5)
#define EFI_LOADER_FEATURE_RANDOM_SEED             (1LL << 6)
#define EFI_LOADER_FEATURE_LOAD_DRIVER             (1LL << 7)
#define EFI_LOADER_FEATURE_SORT_KEY                (1LL << 8)
#define EFI_LOADER_FEATURE_SAVED_ENTRY             (1LL << 9)
#define EFI_LOADER_FEATURE_DEVICETREE              (1LL << 10)
#define EFI_LOADER_FEATURE_SECUREBOOT_ENROLL       (1LL << 11)
#define EFI_LOADER_FEATURE_RETAIN_SHIM             (1LL << 12)


static int efi_postcore_init(void)
{
	const struct efi_device_path *parent_image_dp, *loaded_image_dp;
	char *bbu_path = "/boot/" CONFIG_EFI_PAYLOAD_DEFAULT_PATH;

	char *filepath, *uuid;
	static const uint64_t loader_features =
		EFI_LOADER_FEATURE_DEVICETREE;

	efi_set_variable_usec("LoaderTimeInitUSec", &efi_systemd_vendor_guid,
			      get_time_ns()/1000);

	efi_set_variable_printf("LoaderInfo", &efi_systemd_vendor_guid,
			"barebox-" UTS_RELEASE);

	efi_set_variable_printf("LoaderFirmwareInfo", &efi_systemd_vendor_guid,
				"%ls %u.%02u", efi_sys_table->fw_vendor,
				efi_sys_table->fw_revision >> 16,
				efi_sys_table->fw_revision & 0xffff);

	efi_set_variable_printf("LoaderFirmwareType", &efi_systemd_vendor_guid,
				"UEFI %u.%02u", efi_sys_table->hdr.revision >> 16,
				efi_sys_table->hdr.revision & 0xffff);

	efi_set_variable_uint64_le("LoaderFeatures", &efi_systemd_vendor_guid,
				   loader_features);

	loaded_image_dp = device_path_from_handle(efi_loaded_image->device_handle);
	pr_debug("loaded-image: %pD\n", loaded_image_dp);

	if (!loaded_image_dp)
		return -EINVAL;

	uuid = device_path_to_partuuid(loaded_image_dp);
	if (uuid) {
		wchar_t *uuid16 = xstrdup_char_to_wchar(uuid);
		efi_set_variable("LoaderDevicePartUUID",
				 &efi_systemd_vendor_guid,
				 EFI_VARIABLE_BOOTSERVICE_ACCESS |
				 EFI_VARIABLE_RUNTIME_ACCESS,
				 uuid16, (strlen(uuid)+1) * sizeof(wchar_t));
		free(uuid);
		free(uuid16);
	}

	parent_image_dp = device_path_from_handle(efi_parent_image);
	pr_debug("parent-image: %pD\n", parent_image_dp);

	filepath = device_path_to_filepath(parent_image_dp);
	if (filepath) {
		bbu_path = basprintf("/boot/%s", filepath);
		free(filepath);
	}

	bbu_register_std_file_update("fat", 0, bbu_path, filetype_exe);

	return 0;
}
postcore_efi_initcall(efi_postcore_init);

static int efi_late_init(void)
{
	const char *state_desc = "/boot/EFI/barebox/state.dtb";
	struct device_node *state_root = NULL;
	size_t size;
	void *fdt;
	int ret;

	if (!IS_ENABLED(CONFIG_STATE))
		return 0;

	if (!get_mounted_path("/boot")) {
		pr_warn("boot device couldn't be determined%s\n",
			IS_ENABLED(CONFIG_FS_EFI) ? " without CONFIG_FS_EFI" : "");
		return 0;
	}

	fdt = read_file(state_desc, &size);
	if (!fdt) {
		pr_info("unable to read %s: %m\n", state_desc);
		return 0;
	}

	state_root = of_unflatten_dtb(fdt, size);
	if (!IS_ERR(state_root)) {
		struct device_node *np = NULL;
		struct state *state;

		ret = barebox_register_of(state_root);
		if (ret)
			pr_warn("Failed to register device-tree: %pe\n", ERR_PTR(ret));

		np = of_find_node_by_alias(state_root, "state");

		state = state_new_from_node(np, false);
		if (IS_ERR(state))
			return PTR_ERR(state);

		ret = state_load(state);
		if (ret != -ENOMEDIUM)
			pr_warn("Failed to load persistent state, continuing with defaults, %d\n",
				ret);

		return 0;
	}

	return 0;
}
late_efi_initcall(efi_late_init);

static int do_efiexit(int argc, char *argv[])
{
	if (!BS)
		return -ENOSYS;

	console_flush();

	if (!streq_ptr(argv[1], "-f"))
		shutdown_barebox();

	return BS->exit(efi_parent_image, EFI_SUCCESS, 0, NULL);
}

BAREBOX_CMD_HELP_START(efiexit)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f",  "force exit, don't call barebox shutdown")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(efiexit)
	.cmd = do_efiexit,
	BAREBOX_CMD_DESC("Leave barebox and return to the calling EFI process")
	BAREBOX_CMD_OPTS("[-flrw]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_efiexit_help)
BAREBOX_CMD_END
