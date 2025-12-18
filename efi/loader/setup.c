// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e7a85ec651ed5794eb9a837e1073f6b3146af501/lib/efi_loader/efi_setup.c
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/ac040ae58dba316d86edc788416fae58a5f44e3f/lib/efi_loader/efi_root_node.c
/*
 *  EFI setup code
 *
 *  Copyright (c) 2016-2018 Alexander Graf et al.
 */

#define pr_fmt(fmt) "efi-loader: setup: " fmt

#include <linux/printk.h>
#include <linux/array_size.h>
#include <driver.h>
#include <string.h>
#include <magicvar.h>
#include <efi/loader.h>
#include <efi/mode.h>
#include <efi/guid.h>
#include <efi/devicepath.h>
#include <efi/loader/object.h>
#include <efi/error.h>
#include <xfuncs.h>

struct efi_boot_services *loaderBS;
struct efi_runtime_services *loaderRT;

const efi_guid_t efi_barebox_guid = BAREBOX_GUID;

efi_handle_t efi_root = NULL;

struct efi_root_dp {
	struct efi_device_path_vendor vendor;
	struct efi_device_path end;
} __packed;

/**
 * efi_root_node_register() - create root node
 *
 * Create the root node on which we install all protocols that are
 * not related to a loaded image or a driver.
 *
 * Return:	status code
 */
static efi_status_t efi_root_node_register(void)
{
	efi_status_t ret;
	struct efi_root_dp *dp;

	/* Create device path protocol */
	dp = calloc(1, sizeof(*dp));
	if (!dp)
		return EFI_OUT_OF_RESOURCES;

	/* Fill vendor node */
	dp->vendor.header.type = DEVICE_PATH_TYPE_HARDWARE_DEVICE;
	dp->vendor.header.sub_type = DEVICE_PATH_SUB_TYPE_VENDOR;
	dp->vendor.header.length = sizeof(struct efi_device_path_vendor);
	dp->vendor.Guid = efi_barebox_guid;

	/* Fill end node */
	dp->end.type = DEVICE_PATH_TYPE_END;
	dp->end.sub_type = DEVICE_PATH_SUB_TYPE_END;
	dp->end.length = sizeof(struct efi_device_path);

	/* Create root node and install protocols */
	ret = efi_install_multiple_protocol_interfaces(&efi_root,
		 /* Device path protocol */
		 &efi_device_path_protocol_guid, dp,
		 /* Device path to text protocol */
		 NULL);

	efi_root->type = EFI_OBJECT_TYPE_BAREBOX_FIRMWARE;

	return ret;
}

struct efi_deferred_cb {
	efi_status_t (*cb)(void *);
	void *data;
	struct list_head list;
};

struct efi_deferred_add_protocol_ctx {
	efi_handle_t handle;
	const efi_guid_t *protocol;
	const void *interface;
};

struct efi_deferred_add_protocol {
	struct efi_deferred_cb base;
	struct efi_deferred_add_protocol_ctx ctx;
};

static LIST_HEAD(efi_deferred_cbs);

void efi_register_deferred_init(efi_status_t (*init)(void *), void *data)
{
	struct efi_deferred_cb *deferred = xzalloc(sizeof(*deferred));

	deferred->cb = init;
	deferred->data = data;
	list_add_tail(&deferred->list, &efi_deferred_cbs);
}

static efi_status_t add_protocol(void *_ctx)
{
	struct efi_deferred_add_protocol_ctx *ctx = _ctx;

	return efi_add_protocol(ctx->handle ?: efi_root, ctx->protocol, ctx->interface);
}

void efi_add_root_node_protocol_deferred(const efi_guid_t *protocol, const void *interface)
{
	struct efi_deferred_add_protocol *deferred = xzalloc(sizeof(*deferred));

	deferred->base.cb = add_protocol;
	deferred->base.data = &deferred->ctx;

	list_add_tail(&deferred->base.list, &efi_deferred_cbs);
}

/**
 * efi_init_obj_list() - Initialize and populate EFI object list
 *
 * Return:	status code
 */
efi_status_t efi_init_obj_list(void)
{
	struct efi_deferred_cb *deferred;
	efi_status_t ret;

	/* Initialize once only */
	if (loaderBS)
		return 0;

	efi_loader_set_state(EFI_LOADER_BOOT);

	/* Initialize root node */
	ret = efi_root_node_register();
	if (ret != EFI_SUCCESS)
		goto err;

	ret = efi_alloc_system_table();
	if (ret != EFI_SUCCESS)
		goto err;

	list_for_each_entry(deferred, &efi_deferred_cbs, list)
		deferred->cb(deferred->data);

	/* Initialize system table */
	ret = efi_initialize_system_table();
	if (ret != EFI_SUCCESS)
		goto err;

	/* Indicate supported runtime services */
	ret = efi_init_runtime_supported();
	if (ret != EFI_SUCCESS)
		goto err;

	loaderBS = systab.boottime;
	loaderRT = systab.runtime;

	return 0;

err:
	efi_loader_set_state(EFI_LOADER_INACTIVE);
	return ret;

}

struct device efidev = {
	.name = "efi",
	.id = DEVICE_ID_SINGLE,
};

static int loader_state;

static const char *loader_state_names[] = {
	[EFI_LOADER_INACTIVE] = "inactive",
	[EFI_LOADER_BOOT] = "boot",
	[EFI_LOADER_RUNTIME] = "runtime",
};

static int loader_state_set(struct param_d *param, void *data)
{
	switch (loader_state) {
	case EFI_LOADER_INACTIVE:
		return -EINVAL;
	case EFI_LOADER_BOOT:
		return -efi_errno(efi_init_obj_list());
	case EFI_LOADER_RUNTIME:
		return -EINVAL;
	}
	return 0;
}

static int efi_init_params(void)
{
	int ret;

	if (efi_is_payload())
		return 0;

	ret = platform_device_register(&efidev);
	if (ret)
		return ret;

	dev_add_param_enum(&efidev, "loader",
			   loader_state_set, NULL, &loader_state,
			   loader_state_names, ARRAY_SIZE(loader_state_names),
			   NULL);

	dev_add_param_fixed(&efidev, "payload_default_path",
			    CONFIG_EFI_PAYLOAD_DEFAULT_PATH);

	return 0;
}
late_initcall(efi_init_params);

BAREBOX_MAGICVAR(efi.loader, "EFI loader state (inactive, boot, runtime)");
BAREBOX_MAGICVAR(efi.payload_default_path, "The arch-specific removable media path");

void efi_loader_set_state(enum efi_loader_state state)
{
	loader_state = state;
}

enum efi_loader_state efi_is_loader(void)
{
	return loader_state;
}
