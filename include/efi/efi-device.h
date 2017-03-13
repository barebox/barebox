#ifndef __EFI_EFI_DEVICE_H
#define __EFI_EFI_DEVICE_H

struct efi_device {
	struct device_d dev;
	efi_guid_t *guids;
	int num_guids;
	efi_handle_t handle;
	efi_handle_t parent_handle;
	void *protocol;
	struct efi_device_path *devpath;
};

struct efi_driver {
	struct driver_d driver;
	int (*probe)(struct efi_device *efidev);
	void (*remove)(struct efi_device *efidev);
	efi_guid_t guid;
};

extern struct bus_type efi_bus;

static inline struct efi_device *to_efi_device(struct device_d *dev)
{
	return container_of(dev, struct efi_device, dev);
}

static inline struct efi_driver *to_efi_driver(struct driver_d *drv)
{
	return container_of(drv, struct efi_driver, driver);
}

#define device_efi_driver(drv)	\
	register_driver_macro(device, efi, drv)

static inline int efi_driver_register(struct efi_driver *efidrv)
{
	efidrv->driver.bus = &efi_bus;
	return register_driver(&efidrv->driver);
}

int efi_connect_all(void);
void efi_register_devices(void);

#endif /* __EFI_EFI_DEVICE_H */
