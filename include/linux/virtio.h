/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VIRTIO_H
#define _LINUX_VIRTIO_H
/* Everything a virtio driver needs to work with any particular virtio
 * implementation. */
#include <linux/types.h>
#include <driver.h>
#include <linux/slab.h>

struct virtio_device_id {
	__u32 device;
	__u32 vendor;
};
#define VIRTIO_DEV_ANY_ID	0xffffffff

/**
 * virtio scatter-gather struct
 *
 * @addr:		sg buffer address
 * @lengh:		sg buffer length
 */
struct virtio_sg {
	void *addr;
	size_t length;
};

static inline void virtio_sg_init_one(struct virtio_sg *sg,
				      void *addr, size_t length)
{
	sg[0].addr = addr;
	sg[0].length = length;
}

struct virtio_config_ops;

/**
 * virtio_device - representation of a device using virtio
 * @index: unique position on the virtio bus
 * @failed: saved value for VIRTIO_CONFIG_S_FAILED bit (for restore)
 * @config_enabled: configuration change reporting enabled
 * @config_change_pending: configuration change reported while disabled
 * @dev: underlying device.
 * @id: the device type identification (used to match it with a driver).
 * @config: the configuration ops for this device.
 * @vringh_config: configuration ops for host vrings.
 * @vqs: the list of virtqueues for this device.
 * @features: the features supported by both driver and device.
 * @priv: private pointer for the driver's use.
 */
struct virtio_device {
	int index;
	bool failed;
	bool config_enabled;
	bool config_change_pending;
	struct device_d dev;
	struct virtio_device_id id;
	const struct virtio_config_ops *config;
	struct list_head vqs;
	u64 features;
	void *priv;
	u32 status_param;
};

static inline struct virtio_device *dev_to_virtio(struct device_d *_dev)
{
	return container_of(_dev, struct virtio_device, dev);
}

void virtio_add_status(struct virtio_device *dev, unsigned int status);
int register_virtio_device(struct virtio_device *dev);
void unregister_virtio_device(struct virtio_device *dev);
bool is_virtio_device(struct device_d *dev);

void virtio_break_device(struct virtio_device *dev);

void virtio_config_changed(struct virtio_device *dev);
void virtio_config_disable(struct virtio_device *dev);
void virtio_config_enable(struct virtio_device *dev);
int virtio_finalize_features(struct virtio_device *dev);

size_t virtio_max_dma_size(struct virtio_device *vdev);

#define virtio_device_for_each_vq(vdev, vq) \
	list_for_each_entry(vq, &vdev->vqs, list)

/**
 * virtio_driver - operations for a virtio I/O driver
 * @driver: underlying device driver (populate name and owner).
 * @id_table: the ids serviced by this driver.
 * @feature_table: an array of feature numbers supported by this driver.
 * @feature_table_size: number of entries in the feature table array.
 * @feature_table_legacy: same as feature_table but when working in legacy mode.
 * @feature_table_size_legacy: number of entries in feature table legacy array.
 * @probe: the function to call when a device is found.  Returns 0 or -errno.
 * @scan: optional function to call after successful probe; intended
 *    for virtio-scsi to invoke a scan.
 * @remove: the function to call when a device is removed.
 * @config_changed: optional function to call when the device configuration
 *    changes; may be called in interrupt context.
 * @freeze: optional function to call during suspend/hibernation.
 * @restore: optional function to call on resume.
 */
struct virtio_driver {
	struct driver_d driver;
	const struct virtio_device_id *id_table;
	const unsigned int *feature_table;
	unsigned int feature_table_size;
	const unsigned int *feature_table_legacy;
	unsigned int feature_table_size_legacy;
	int (*validate)(struct virtio_device *dev);
	int (*probe)(struct virtio_device *dev);
	void (*scan)(struct virtio_device *dev);
	void (*remove)(struct virtio_device *dev);
	void (*config_changed)(struct virtio_device *dev);
};

static inline struct virtio_driver *drv_to_virtio(struct driver_d *drv)
{
	return container_of(drv, struct virtio_driver, driver);
}

int virtio_driver_register(struct virtio_driver *drv);

/* module_virtio_driver() - Helper macro for drivers that don't do
 * anything special in module init/exit.  This eliminates a lot of
 * boilerplate.  Each module may only use this macro once, and
 * calling it replaces module_init() and module_exit()
 */
#define module_virtio_driver(drv) \
	device_virtio_driver(drv)

#define device_virtio_driver(drv) \
	register_driver_macro(device,virtio,drv)

#endif /* _LINUX_VIRTIO_H */
