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

#include <command.h>
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <sizes.h>
#include <wchar.h>
#include <init.h>
#include <efi.h>
#include <mach/efi.h>
#include <mach/efi-device.h>
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

	ret = efi_locate_handle(all_handles, NULL, NULL, &handle_count, &handles);
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
	if (EFI_ERROR(efiret))
		return ERR_PTR(-EINVAL);

	efidev = xzalloc(sizeof(*efidev));

	efidev->guids = guidarr;
	efidev->num_guids = num_guids;
	efidev->handle = handle;
	efidev->dev.bus = &efi_bus;
	efidev->dev.id = DEVICE_ID_SINGLE;
	efidev->dev.info = efi_devinfo;
	efidev->devpath = devpath;

	BS->handle_protocol(handle, &guidarr[0], &efidev->protocol);

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

	ret = efi_locate_handle(all_handles, NULL, NULL, &handle_count, &handles);
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
		if (!memcmp(&efidrv->guid, &efidev->guids[i], sizeof(efi_guid_t)))
			return 0;
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

	return efidrv->remove(efidev);
}

struct bus_type efi_bus = {
	.name = "efi",
	.match = efi_bus_match,
	.probe = efi_bus_probe,
	.remove = efi_bus_remove,
};

static int efi_init_devices(void)
{
	bus_register(&efi_bus);

	efi_register_devices();

	return 0;
}
core_initcall(efi_init_devices);
