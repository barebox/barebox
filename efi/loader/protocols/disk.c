// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/2fdfb802e30a1fbd65a830a283d7bd87631f08c0/lib/efi_loader/efi_disk.c
/*
 *  EFI application disk support
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define pr_fmt(fmt) "efi-loader: disk: " fmt

#include <efi/loader.h>
#include <efi/devicepath.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <efi/protocol/block.h>
#include <efi/loader/object.h>
#include <efi/loader/devicepath.h>
#include <efi/loader/file.h>
#include <efi/loader/trace.h>
#include <efi/partition.h>
#include <malloc.h>
#include <bootsource.h>
#include <block.h>
#include <fs.h>

const efi_guid_t efi_system_partition_guid = PARTITION_SYSTEM_GUID;

static struct cdev *chosen_esp_cdev;
static int chosen_esp_dirfd = -1;
static int max_esp_score;

int efiloader_esp_mount_dir(void)
{
	return chosen_esp_dirfd;
}

/**
 * struct efi_disk_obj - EFI disk object
 *
 * @header:	EFI object header
 * @ops:	EFI disk I/O protocol interface
 * @dp:		device path to the block device
 * @bdev:	internal block device descriptor
 * @blockbits	of underlying block device
 */
struct efi_disk_obj {
	struct efi_object header;
	struct efi_block_io_protocol ops;
	struct efi_block_io_media media;
	struct efi_device_path *dp;
	struct cdev *cdev;
	struct efi_simple_file_system_protocol *volume;
	u8 blockbits;
};

#define to_efi_disk_obj(this) container_of(this, struct efi_disk_obj, ops)

/**
 * efi_disk_reset() - reset block device
 *
 * This function implements the Reset service of the EFI_BLOCK_IO_PROTOCOL.
 *
 * As barebox block devices do not have a reset function simply return
 * EFI_SUCCESS.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:			pointer to the BLOCK_IO_PROTOCOL
 * @extended_verification:	extended verification
 * Return:			status code
 */
static efi_status_t EFIAPI efi_disk_reset(struct efi_block_io_protocol *this,
					  bool extended_verification)
{
	EFI_ENTRY("%p, %x", this, extended_verification);
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI efi_disk_read(struct efi_block_io_protocol *this,
					  u32 media_id, u64 lba,
					  size_t buffer_size, void *buffer)
{
	struct efi_disk_obj *disk = to_efi_disk_obj(this);
	ssize_t ret;

	ret = cdev_read(disk->cdev, buffer, buffer_size, lba << disk->blockbits, 0);

	return ret < 0 ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}

static efi_status_t EFIAPI efi_disk_write(struct efi_block_io_protocol *this,
					   u32 media_id, u64 lba,
					   size_t buffer_size, void *buffer)
{
	struct efi_disk_obj *disk = to_efi_disk_obj(this);
	ssize_t ret;

	ret = cdev_write(disk->cdev, buffer, buffer_size, lba << disk->blockbits, 0);

	return ret ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}

static efi_status_t EFIAPI efi_disk_flush(struct efi_block_io_protocol *this)
{
	return cdev_flush(to_efi_disk_obj(this)->cdev) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}

static const struct efi_block_io_protocol block_io_disk_template = {
	.reset = efi_disk_reset,
	.read = efi_disk_read,
	.write = efi_disk_write,
	.flush = efi_disk_flush,
};

/**
 * efi_fs_from_path() - retrieve simple file system protocol
 *
 * Gets the simple file system protocol for a file device path.
 *
 * The full path provided is split into device part and into a file
 * part. The device part is used to find the handle on which the
 * simple file system protocol is installed.
 *
 * @full_path:	device path including device and file
 * Return:	simple file system protocol
 */
struct efi_simple_file_system_protocol *
efi_fs_from_path(struct efi_device_path *full_path)
{
	struct efi_object *efiobj;
	struct efi_handler *handler;
	struct efi_device_path *device_path;
	struct efi_device_path *file_path;
	efi_status_t ret;

	/* Split the path into a device part and a file part */
	ret = efi_dp_split_file_path(full_path, &device_path, &file_path);
	if (ret != EFI_SUCCESS)
		return NULL;
	efi_free_pool(file_path);

	/* Get the EFI object for the partition */
	efiobj = efi_dp_find_obj(device_path, NULL, NULL);
	efi_free_pool(device_path);
	if (!efiobj)
		return NULL;

	/* Find the simple file system protocol */
	ret = efi_search_protocol(efiobj, &efi_simple_file_system_protocol_guid,
				  &handler);
	if (ret != EFI_SUCCESS)
		return NULL;

	/* Return the simple file system protocol for the partition */
	return handler->protocol_interface;
}

static bool efi_fs_exists(struct cdev *cdev)
{
	struct driver *drv;

	bus_for_each_driver(&fs_bus, drv) {
		struct fs_driver *fdrv;
		enum filetype filetype;

		fdrv = drv_to_fs_driver(drv);

		if (cdev_detect_type(cdev, &filetype) == 0 &&
		    filetype == fdrv->type)
			return true;
	}

	return false;
}

/**
 * efi_disk_add_dev() - create a handle for a partition or disk
 *
 * @parent:		parent handle
 * @dp_parent:		parent device path
 * @cdev:		cdev of block device or partition
 * @disk:		pointer to receive the created handle
 * @removable:		whether part of a removable block device
 * Return:		disk object
 */
static efi_status_t efi_disk_add_cdev(efi_handle_t parent,
				struct efi_device_path *dp_parent,
				struct cdev *cdev,
				struct efi_disk_obj **disk,
				bool removable)
{
	struct efi_disk_obj *diskobj;
	struct efi_object *handle;
	const efi_guid_t *esp_guid = NULL;
	int score = 0;
	efi_status_t ret;

	diskobj = calloc(1, sizeof(*diskobj));
	if (!diskobj)
		return EFI_OUT_OF_RESOURCES;

	/* Hook up to the device list */
	efi_add_handle(&diskobj->header);

	/* Fill in object data */
	if (cdev->flags & DEVFS_PARTITION_FROM_TABLE) {
		struct efi_device_path *node;
		struct efi_handler *handler;
		void *protocol_interface;

		/* Parent must expose EFI_BLOCK_IO_PROTOCOL */
		ret = efi_search_protocol(parent, &efi_block_io_protocol_guid, &handler);
		if (ret != EFI_SUCCESS)
			goto error;

		/*
		 * Link the partition (child controller) to the block device
		 * (controller).
		 */
		ret = efi_protocol_open(handler, &protocol_interface, NULL,
					&diskobj->header,
					EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
		if (ret != EFI_SUCCESS)
			goto error;

		node = efi_dp_from_cdev(cdev, false);
		diskobj->dp = efi_dp_append_node(dp_parent, node);
		efi_free_pool(node);
		if (cdev->flags & DEVFS_PARTITION_BOOTABLE_ESP) {
			struct cdev *bootcdev;

			esp_guid = &efi_system_partition_guid;

			bootcdev = bootsource_of_cdev_find();

			/* Prefer eMMC over SD if neither is the boot medium */
			if (removable)
				score += 1;
			else
				score += 2;

			if (bootcdev && bootcdev == cdev)
				score += 4;
		}
	} else {
		diskobj->dp = efi_dp_from_cdev(cdev, true);
	}

	/*
	 * Install the device path and the block IO protocol.
	 *
	 * InstallMultipleProtocolInterfaces() checks if the device path is
	 * already installed on an other handle and returns EFI_ALREADY_STARTED
	 * in this case.
	 */
	handle = &diskobj->header;
	ret = efi_install_multiple_protocol_interfaces(&handle,
			&efi_device_path_protocol_guid, diskobj->dp,
			&efi_block_io_protocol_guid, &diskobj->ops,
			esp_guid, NULL, NULL);
	if (ret != EFI_SUCCESS)
		return ret;

	if ((cdev_is_partition(cdev)|| list_empty(&cdev->partitions)) && efi_fs_exists(cdev)) {
		diskobj->volume = efi_simple_file_system(cdev, diskobj->dp);
		ret = efi_add_protocol(&diskobj->header,
				       &efi_simple_file_system_protocol_guid,
				       diskobj->volume);
		if (ret != EFI_SUCCESS)
			return ret;
	}

	diskobj->media.removable_media = removable;
	diskobj->media.media_present = true;
	diskobj->media.read_only = cdev->flags & DEVFS_PARTITION_READONLY;
	diskobj->media.block_size = 512;
	diskobj->media.io_align = 512;
	diskobj->media.last_block = cdev->size / diskobj->media.block_size - 1;

	diskobj->ops = block_io_disk_template;
	diskobj->ops.media = &diskobj->media;

	diskobj->cdev = cdev;

	if (disk)
		*disk = diskobj;


	if (score && score > max_esp_score) {
		max_esp_score = score;
		chosen_esp_cdev = cdev;
	}

	return EFI_SUCCESS;
error:
	efi_delete_handle(&diskobj->header);
	return ret;
}

/**
 * efi_disk_create_partitions() - create handles and protocols for partitions
 *
 * Create handles and protocols for the partitions of a block device.
 *
 * @parent:		handle of the parent disk
 * @bdev:		block device
 * Return:		number of partitions created
 */
static int efi_disk_create_partitions(efi_handle_t parent, struct block_device *bdev)
{
	struct cdev *partcdev;
	int npartitions = 0;
	struct efi_device_path *dp = NULL;
	efi_status_t ret;
	struct efi_handler *handler;

	/* Get the device path of the parent */
	ret = efi_search_protocol(parent, &efi_device_path_protocol_guid, &handler);
	if (ret == EFI_SUCCESS)
		dp = handler->protocol_interface;

	/* Add devices for each partition */
	for_each_cdev_partition(partcdev, &bdev->cdev) {
		ret = efi_disk_add_cdev(parent, dp, partcdev, NULL, bdev->removable);
		if (ret != EFI_SUCCESS) {
			pr_err("Adding partition %s failed\n", partcdev->name);
			continue;
		}

		npartitions++;
	}

	return npartitions;
}

static void symlink_esp(void)
{
	const char *mntpath;

	if (!chosen_esp_cdev)
		return;

	mntpath = cdev_mount(chosen_esp_cdev);
	if (IS_ERR(mntpath)) {
		pr_warn("Failed to mount %s as /esp: %pe\n",
			chosen_esp_cdev->name, mntpath);
		return;
	}

	chosen_esp_dirfd = open(mntpath, O_PATH | O_DIRECTORY);
	if (chosen_esp_dirfd >= 0)
		symlink(mntpath, "/esp");

	pr_info("%s symlinked as /esp\n", mntpath);
}

/**
 * efi_disk_register() - register block devices
 *
 * This function is called in efi_init_obj_list().
 *
 * Return:	status code
 */
static efi_status_t efi_disk_register(void *data)
{
	struct block_device *bdev;
	struct efi_disk_obj *disk;
	unsigned ndisks = 0;
	efi_status_t ret;

	for_each_block_device(bdev) {
		/* Add block device for the full device */
		pr_debug("Scanning disk %s...\n", bdev->cdev.name);
		ret = efi_disk_add_cdev(NULL, NULL, &bdev->cdev, &disk, bdev->removable);
		if (ret) {
			pr_err("failure to add disk device %s, r = %lu\n",
				bdev->cdev.name, ret & ~EFI_ERROR_MASK);
			return ret;
		}

		disk->blockbits = bdev->blockbits;
		ndisks++;

		/* Partitions show up as block devices in EFI */
		ndisks += efi_disk_create_partitions(&disk->header, bdev);
	}

	pr_info("exporting %d disks\n", ndisks);

	symlink_esp();

	return EFI_SUCCESS;
}

static int efi_disk_init(void)
{
	efi_register_deferred_init(efi_disk_register, NULL);
	return 0;
}
coredevice_initcall(efi_disk_init);
