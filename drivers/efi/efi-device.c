/*
 * efi-device.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <bootsource.h>
#include <command.h>
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <linux/sizes.h>
#include <wchar.h>
#include <init.h>
#include <efi.h>
#include <efi/efi.h>
#include <efi/efi-device.h>
#include <linux/err.h>

int efi_locate_handle(enum efi_locate_search_type search_type,
		efi_guid_t *protocol,
		void *search_key,
		unsigned long *no_handles,
		efi_handle_t **buffer)
{
	efi_status_t efiret;
	unsigned long buffer_size = 0;
	efi_handle_t *buf;

	efiret = BS->locate_handle(search_type, protocol, search_key, &buffer_size,
			NULL);
	if (EFI_ERROR(efiret) && efiret != EFI_BUFFER_TOO_SMALL)
		return -efi_errno(efiret);

	buf = malloc(buffer_size);
	if (!buf)
		return -ENOMEM;

	efiret = BS->locate_handle(search_type, protocol, search_key, &buffer_size,
			buf);
	if (EFI_ERROR(efiret)) {
		free(buf);
		return -efi_errno(efiret);
	}

	*no_handles = buffer_size / sizeof(efi_handle_t);
	*buffer = buf;

	return 0;
}

static struct efi_device *efi_find_device(efi_handle_t *handle)
{
	struct device_d *dev;
	struct efi_device *efidev;

	bus_for_each_device(&efi_bus, dev) {
		efidev = container_of(dev, struct efi_device, dev);

		if (efidev->handle == handle)
			return efidev;
	}

	return NULL;
}

static void efi_devinfo(struct device_d *dev)
{
	struct efi_device *efidev = to_efi_device(dev);
	int i;

	printf("Protocols:\n");

	for (i = 0; i < efidev->num_guids; i++)
		printf("  %d: %pUl: %s\n", i, &efidev->guids[i],
					efi_guid_string(&efidev->guids[i]));
}

static efi_handle_t *efi_find_parent(efi_handle_t *handle)
{
	unsigned long handle_count = 0;
        efi_handle_t *handles = NULL, *parent;
	unsigned long num_guids;
	efi_guid_t **guids;
	int ret, i, j, k;
	efi_status_t efiret;
	struct efi_open_protocol_information_entry *entry_buffer;
	unsigned long entry_count;

	ret = efi_locate_handle(by_protocol, &efi_device_path_protocol_guid,
			NULL, &handle_count, &handles);
	if (ret)
		return NULL;

	/*
	 * Normally one would expect a function/pointer to retrieve the parent.
	 * With EFI we have to:
	 * - get all handles
	 * - for each handle get the registered protocols
	 * - for each protocol get the users
	 * - the user which matches the input handle is the parent
	 */
	for (i = 0; i < handle_count; i++) {
		efiret = BS->open_protocol(handles[i], &efi_device_path_protocol_guid,
				NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
		if (EFI_ERROR(efiret))
			continue;

		BS->protocols_per_handle(handles[i], &guids, &num_guids);
		for (j = 0; j < num_guids; j++) {
			efiret = BS->open_protocol_information(handles[i], guids[j],
				&entry_buffer, &entry_count);
			for (k = 0; k < entry_count; k++) {
				if (entry_buffer[k].controller_handle == NULL)
					continue;
				if (entry_buffer[k].controller_handle == handles[i])
					continue;
				if (entry_buffer[k].controller_handle == handle) {
					parent = handles[i];
					goto out;
				}
			}
		}
	}

	parent = NULL;

	free(handles);
out:
	return parent;
}

static struct efi_device *efi_add_device(efi_handle_t *handle, efi_guid_t **guids,
		int num_guids)
{
	struct efi_device *efidev;
	int i;
	efi_guid_t *guidarr;
	efi_status_t efiret;
	void *devpath;

	efidev = efi_find_device(handle);
	if (efidev)
		return ERR_PTR(-EEXIST);

	efiret = BS->open_protocol(handle, &efi_device_path_protocol_guid,
			NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
	if (EFI_ERROR(efiret))
		return ERR_PTR(-EINVAL);

	guidarr = malloc(sizeof(efi_guid_t) * num_guids);

	for (i = 0; i < num_guids; i++)
		memcpy(&guidarr[i], guids[i], sizeof(efi_guid_t));

	efiret = BS->open_protocol(handle, &efi_device_path_protocol_guid,
			&devpath, NULL, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret)) {
		free(guidarr);
		return ERR_PTR(-EINVAL);
	}

	efidev = xzalloc(sizeof(*efidev));

	efidev->guids = guidarr;
	efidev->num_guids = num_guids;
	efidev->handle = handle;
	efidev->dev.bus = &efi_bus;
	efidev->dev.id = DEVICE_ID_SINGLE;
	efidev->dev.info = efi_devinfo;
	efidev->devpath = devpath;

	sprintf(efidev->dev.name, "handle-%p", handle);

	efidev->parent_handle = efi_find_parent(efidev->handle);

	return efidev;
}


static int efi_register_device(struct efi_device *efidev)
{
	char *dev_path_str;
	struct efi_device *parent;
	int ret;

	if (efi_find_device(efidev->handle))
		return -EEXIST;

	if (efidev->parent_handle) {
		parent = efi_find_device(efidev->parent_handle);
		if (!parent)
			return -EINVAL;

		efidev->dev.parent = &parent->dev;
	}

	ret = register_device(&efidev->dev);
	if (ret)
		return ret;

	dev_path_str = device_path_to_str(efidev->devpath);
	if (dev_path_str) {
		dev_add_param_fixed(&efidev->dev, "devpath", dev_path_str);
		free(dev_path_str);
	}

	debug("registered efi device %s\n", dev_name(&efidev->dev));

	return 0;
}

/**
 * efi_register_devices - iterate over all EFI handles and register
 *                        the devices found
 *
 * in barebox we treat all EFI handles which support the device_path
 * protocol as devices. This function iterates over all handles and
 * registers the corresponding devices. efi_register_devices is safe
 * to call multiple times. Already registered devices will be ignored.
 *
 */
void efi_register_devices(void)
{
	unsigned long handle_count = 0;
        efi_handle_t *handles = NULL;
	unsigned long num_guids;
	efi_guid_t **guids;
	int ret, i;
	struct efi_device **efidevs;
	int registered;

	ret = efi_locate_handle(by_protocol, &efi_device_path_protocol_guid,
			NULL, &handle_count, &handles);
	if (ret)
		return;

	efidevs = xzalloc(handle_count * sizeof(struct efi_device *));

	for (i = 0; i < handle_count; i++) {
		BS->protocols_per_handle(handles[i], &guids, &num_guids);

		efidevs[i] = efi_add_device(handles[i], guids, num_guids);
	}

	/*
	 * We have a list of devices we want to register, but can only
	 * register a device when all parents are registered already.
	 * Do this by continiously iterating over the list until no
	 * further devices are registered.
	 */
	do {
		registered = 0;

		for (i = 0; i < handle_count; i++) {
			if (IS_ERR(efidevs[i]))
				continue;

			ret = efi_register_device(efidevs[i]);
			if (!ret) {
				efidevs[i] = ERR_PTR(-EEXIST);
				registered = 1;
			}
		}
	} while (registered);

	free(efidevs);
	free(handles);
}

int efi_connect_all(void)
{
	efi_status_t  efiret;
	unsigned long handle_count;
	efi_handle_t *handle_buffer;
	int i;

	efiret = BS->locate_handle_buffer(all_handles, NULL, NULL, &handle_count,
			&handle_buffer);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	for (i = 0; i < handle_count; i++)
		efiret = BS->connect_controller(handle_buffer[i], NULL, NULL, true);

	if (handle_buffer)
		BS->free_pool(handle_buffer);

	return 0;
}

static int efi_bus_match(struct device_d *dev, struct driver_d *drv)
{
	struct efi_driver *efidrv = to_efi_driver(drv);
	struct efi_device *efidev = to_efi_device(dev);
	int i;

	for (i = 0; i < efidev->num_guids; i++) {
		if (!memcmp(&efidrv->guid, &efidev->guids[i], sizeof(efi_guid_t))) {
			BS->handle_protocol(efidev->handle, &efidev->guids[i],
					&efidev->protocol);
			return 0;
		}
	}

	return 1;
}

static int efi_bus_probe(struct device_d *dev)
{
	struct efi_driver *efidrv = to_efi_driver(dev->driver);
	struct efi_device *efidev = to_efi_device(dev);

	return efidrv->probe(efidev);
}

static void efi_bus_remove(struct device_d *dev)
{
	struct efi_driver *efidrv = to_efi_driver(dev->driver);
	struct efi_device *efidev = to_efi_device(dev);

	if (efidrv->remove)
		efidrv->remove(efidev);
}

struct bus_type efi_bus = {
	.name = "efi",
	.match = efi_bus_match,
	.probe = efi_bus_probe,
	.remove = efi_bus_remove,
};

static void efi_businfo(struct device_d *dev)
{
	int i;

	printf("Tables:\n");
	for (i = 0; i < efi_sys_table->nr_tables; i++) {
		efi_config_table_t *t = &efi_sys_table->tables[i];

		printf("  %d: %pUl: %s\n", i, &t->guid,
					efi_guid_string(&t->guid));
	}
}

static int efi_is_secure_boot(void)
{
	uint8_t *val;
	int ret = 0;

	val = efi_get_variable("SecureBoot", &efi_global_variable_guid, NULL);
	if (!IS_ERR(val)) {
		ret = *val;
		free(val);
	}

	return ret != 1;
}

static int efi_is_setup_mode(void)
{
	uint8_t *val;
	int ret = 0;

	val = efi_get_variable("SetupMode", &efi_global_variable_guid, NULL);
	if (!IS_ERR(val)) {
		ret = *val;
		free(val);
	}

	return ret != 1;
}

static int is_bio_usbdev(struct efi_device *efidev)
{
	int i;

	for (i = 0; i < efidev->num_guids; i++) {
		if (!efi_guidcmp(efidev->guids[i], EFI_USB_IO_PROTOCOL_GUID))
			return 1;
	}

	return 0;
}

static void efi_set_bootsource(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = BOOTSOURCE_INSTANCE_UNKNOWN;

	efi_handle_t *efi_parent;
	struct efi_device *bootdev;

	if (!efi_loaded_image->parent_handle)
		goto out;

	efi_parent = efi_find_parent(efi_loaded_image->device_handle);

	if (!efi_parent)
		goto out;

	bootdev = efi_find_device(efi_parent);

	if (!bootdev)
		goto out;

	if (is_bio_usbdev(bootdev)) {
		src = BOOTSOURCE_USB;
	} else {
		src = BOOTSOURCE_HD;
	}

out:

	bootsource_set(src);
	bootsource_set_instance(instance);
}

static int efi_init_devices(void)
{
	char *fw_vendor = NULL;
	u16 sys_major = efi_sys_table->hdr.revision >> 16;
	u16 sys_minor = efi_sys_table->hdr.revision & 0xffff;
	int secure_boot = efi_is_secure_boot();
	int setup_mode = efi_is_setup_mode();

	fw_vendor = strdup_wchar_to_char((const wchar_t *)efi_sys_table->fw_vendor);

	pr_info("EFI v%u.%.02u by %s v%u\n",
		sys_major, sys_minor,
		fw_vendor, efi_sys_table->fw_revision);

	bus_register(&efi_bus);

	dev_add_param_fixed(efi_bus.dev, "fw_vendor", fw_vendor);
	free(fw_vendor);

	dev_add_param_uint32_fixed(efi_bus.dev, "major", sys_major, "%u");
	dev_add_param_uint32_fixed(efi_bus.dev, "minor", sys_minor, "%u");
	dev_add_param_uint32_fixed(efi_bus.dev, "fw_revision", efi_sys_table->fw_revision, "%u");
	dev_add_param_bool_fixed(efi_bus.dev, "secure_boot", secure_boot);
	dev_add_param_bool_fixed(efi_bus.dev, "secure_mode",
				 secure_boot & setup_mode);

	efi_bus.dev->info = efi_businfo;

	efi_register_devices();

	efi_set_bootsource();

	return 0;
}
core_initcall(efi_init_devices);

static void efi_devpath(efi_handle_t handle)
{
	efi_status_t efiret;
	void *devpath;
	char *dev_path_str;

	efiret = BS->open_protocol(handle, &efi_device_path_protocol_guid,
				   &devpath, NULL, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret))
		return;

	dev_path_str = device_path_to_str(devpath);
	if (dev_path_str) {
		printf("  Devpath: \n  %s\n", dev_path_str);
		free(dev_path_str);
	}
}

static void efi_dump(efi_handle_t *handles, unsigned long handle_count)
{
	int i, j;
	unsigned long num_guids;
	efi_guid_t **guids;

	if (!handles || !handle_count)
		return;

	for (i = 0; i < handle_count; i++) {
		printf("handle-%p\n", handles[i]);

		BS->protocols_per_handle(handles[i], &guids, &num_guids);
		printf("  Protocols:\n");
		for (j = 0; j < num_guids; j++)
			printf("  %d: %pUl: %s\n", j, guids[j],
					efi_guid_string(guids[j]));
		efi_devpath(handles[i]);
	}
	printf("\n");
}

static unsigned char to_digit(unsigned char c)
{
	if (c >= '0' && c <= '9')
		c -= '0';
	else if (c >= 'A' && c <= 'F')
		c -= 'A' - 10;
	else
		c -= 'a' - 10;

	return c;
}

#define read_xbit(src, dest, bit) 					\
	do {								\
		int __i;						\
		for (__i = (bit - 4); __i >= 0; __i -= 4, src++)	\
			dest |= to_digit(*src) << __i;			\
	} while (0)

static int do_efi_protocol_dump(int argc, char **argv)
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	int ret;
	efi_guid_t guid;
	u32 a = 0;
	u16 b = 0;
	u16 c = 0;
	u8 d0 = 0;
	u8 d1 = 0;
	u8 d2 = 0;
	u8 d3 = 0;
	u8 d4 = 0;
	u8 d5 = 0;
	u8 d6 = 0;
	u8 d7 = 0;

	/* Format 220e73b6-6bdb-4413-8405-b974b108619a */
	if (argc == 1) {
		char *s = argv[0];
		int len = strlen(s);

		if (len != 36)
			return -EINVAL;
		
		read_xbit(s, a, 32);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, b, 16);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, c, 16);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, d0, 8);
		read_xbit(s, d1, 8);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, d2, 8);
		read_xbit(s, d3, 8);
		read_xbit(s, d4, 8);
		read_xbit(s, d5, 8);
		read_xbit(s, d6, 8);
		read_xbit(s, d7, 8);
	} else if (argc == 11) {
		/* Format :
		 *	220e73b6 6bdb 4413 84 05 b9 74 b1 08 61 9a
		 *   or
		 *	0x220e73b6 0x6bdb 0x14413 0x84 0x05 0xb9 0x74 0xb1 0x08 0x61 0x9a
		 */
		a = simple_strtoul(argv[0], NULL, 16);
		b = simple_strtoul(argv[1], NULL, 16);
		c = simple_strtoul(argv[2], NULL, 16);
		d0 = simple_strtoul(argv[3], NULL, 16);
		d1 = simple_strtoul(argv[4], NULL, 16);
		d2 = simple_strtoul(argv[5], NULL, 16);
		d3 = simple_strtoul(argv[6], NULL, 16);
		d4 = simple_strtoul(argv[7], NULL, 16);
		d5 = simple_strtoul(argv[8], NULL, 16);
		d6 = simple_strtoul(argv[9], NULL, 16);
		d7 = simple_strtoul(argv[10], NULL, 16);
	} else {
		return -EINVAL;
	}

	guid = EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7);

	printf("Searching for:\n");
	printf("  %pUl: %s\n", &guid, efi_guid_string(&guid));

	ret = efi_locate_handle(by_protocol, &guid, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(handles, handle_count);

	return 0;
}

static int do_efi_handle_dump(int argc, char *argv[])
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	int ret;

	if (argc > 1)
		return do_efi_protocol_dump(--argc, ++argv);

	ret = efi_locate_handle(all_handles, NULL, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(handles, handle_count);

	return 0;
}

BAREBOX_CMD_HELP_START(efi_handle_dump)
BAREBOX_CMD_HELP_TEXT("Dump all the efi handle with protocol and devpath\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(efi_handle_dump)
	.cmd = do_efi_handle_dump,
	BAREBOX_CMD_DESC("Usage: efi_handle_dump")
	BAREBOX_CMD_OPTS("[a-b-c-d0d1-d3d4d5d6d7] or [a b c d0 d1 d2 d3 d4 d5 d6 d7]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_efi_handle_dump_help)
BAREBOX_CMD_END
