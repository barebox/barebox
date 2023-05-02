// SPDX-License-Identifier: GPL-2.0-only
/*
 * init.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#ifdef CONFIG_DEBUG_LL
#define DEBUG
#endif

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

efi_runtime_services_t *RT;
efi_boot_services_t *BS;
efi_system_table_t *efi_sys_table;
efi_handle_t efi_parent_image;
struct efi_device_path *efi_device_path;
efi_loaded_image_t *efi_loaded_image;

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
late_initcall(misc_init);

static struct NS16550_plat ns16550_plat = {
	.clock = 115200 * 16,
};

static int efi_console_init(void)
{
	barebox_set_model("barebox EFI payload");

	add_generic_device("efi-stdio", DEVICE_ID_SINGLE, NULL, 0 , 0, 0, NULL);

	add_ns16550_device(0, 0x3f8, 0x10, IORESOURCE_IO | IORESOURCE_MEM_8BIT,
				&ns16550_plat);

	return 0;
}
console_initcall(efi_console_init);

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
coredevice_initcall(restart_register_feature);

extern char image_base[];
extern initcall_t __barebox_initcalls_start[], __barebox_early_initcalls_end[],
		  __barebox_initcalls_end[];

static int efi_init(void)
{
	char *env;

	defaultenv_append_directory(env_efi);

	env = xasprintf("/efivars/barebox-env-%pUl", &efi_barebox_vendor_guid);
	default_environment_path_set(env);

	return 0;
}
device_initcall(efi_init);

/**
 * efi-main - Entry point for EFI images
 */
void efi_main(efi_handle_t image, efi_system_table_t *sys_table)
{
	efi_physical_addr_t mem;
	size_t memsize;
	efi_status_t efiret;

#ifdef DEBUG
	sys_table->con_out->output_string(sys_table->con_out, L"barebox\n");
#endif

	BS = sys_table->boottime;

	efi_parent_image = image;
	efi_sys_table = sys_table;
	RT = sys_table->runtime;

	efiret = BS->open_protocol(efi_parent_image, &efi_loaded_image_protocol_guid,
			(void **)&efi_loaded_image,
			efi_parent_image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (!EFI_ERROR(efiret))
		BS->handle_protocol(efi_loaded_image->device_handle,
				&efi_device_path_protocol_guid, (void **)&efi_device_path);

	mem = 0x3fffffff;
	for (memsize = SZ_256M; memsize >= SZ_8M; memsize /= 2) {
		efiret  = BS->allocate_pages(EFI_ALLOCATE_MAX_ADDRESS,
					     EFI_LOADER_DATA,
					     memsize/PAGE_SIZE, &mem);
		if (!EFI_ERROR(efiret))
			break;
		if (efiret != EFI_OUT_OF_RESOURCES)
			panic("failed to allocate malloc pool: %s\n",
			      efi_strerror(efiret));
	}
	if (EFI_ERROR(efiret))
		panic("failed to allocate malloc pool: %s\n",
		      efi_strerror(efiret));
	mem_malloc_init((void *)mem, (void *)mem + memsize - 1);

	start_barebox();
}

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
core_initcall(efi_core_init);

static int efi_postcore_init(void)
{
	char *uuid;

	efi_set_variable_usec("LoaderTimeInitUSec", &efi_systemd_vendor_guid,
			      get_time_ns()/1000);

	uuid = device_path_to_partuuid(device_path_from_handle(
				       efi_loaded_image->device_handle));
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

	bbu_register_std_file_update("fat", 0,	"/boot/EFI/BOOT/BOOTx64.EFI",
				     filetype_exe);

	return 0;
}
postcore_initcall(efi_postcore_init);

static int efi_late_init(void)
{
	char *state_desc;
	int ret;

	if (!IS_ENABLED(CONFIG_STATE))
		return 0;

	state_desc = xasprintf("/boot/EFI/barebox/state.dtb");

	if (state_desc) {
		struct device_node *root = NULL;
		struct device_node *np = NULL;
		struct state *state;

		root = of_read_file(state_desc);
		if (IS_ERR(root)) {
			printf("Cannot open %s: %pe\n", state_desc, root);
			return PTR_ERR(root);
		}

		ret = barebox_register_of(root);
		if (ret)
			pr_warn("Failed to register device-tree: %pe\n", ERR_PTR(ret));

		np = of_find_node_by_alias(root, "state");

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
late_initcall(efi_late_init);

static int do_efiexit(int argc, char *argv[])
{
	return BS->exit(efi_parent_image, EFI_SUCCESS, 0, NULL);
}

BAREBOX_CMD_HELP_START(efiexit)
BAREBOX_CMD_HELP_TEXT("Leave barebox and return to the calling EFI process\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(efiexit)
	.cmd = do_efiexit,
	BAREBOX_CMD_DESC("Usage: efiexit")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_efiexit_help)
BAREBOX_CMD_END
