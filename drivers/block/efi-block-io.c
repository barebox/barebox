// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <string.h>
#include <command.h>
#include <errno.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <efi.h>
#include <block.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>
#include <bootsource.h>

#define EFI_BLOCK_IO_PROTOCOL_REVISION2 0x00020001
#define EFI_BLOCK_IO_PROTOCOL_REVISION3 ((2<<16) | (31))

struct efi_block_io_media{
	u32 media_id;
	bool removable_media;
	bool media_present;
	bool logical_partition;
	bool read_only;
	bool write_caching;
	u32 block_size;
	u32 io_align;
	sector_t last_block;
	u64 lowest_aligned_lba; /* added in Revision 2 */
	u32 logical_blocks_per_physical_block; /* added in Revision 2 */
	u32 optimal_transfer_length_granularity; /* added in Revision 3 */
};

struct efi_block_io_protocol {
	u64 revision;
	struct efi_block_io_media *media;
	efi_status_t(EFIAPI *reset)(struct efi_block_io_protocol *this,
			bool ExtendedVerification);
	efi_status_t(EFIAPI *read)(struct efi_block_io_protocol *this, u32 media_id,
			u64 lba, unsigned long buffer_size, void *buf);
	efi_status_t(EFIAPI *write)(struct efi_block_io_protocol *this, u32 media_id,
			u64 lba, unsigned long buffer_size, void *buf);
	efi_status_t(EFIAPI *flush)(struct efi_block_io_protocol *this);
};

struct efi_bio_priv {
	struct efi_block_io_protocol *protocol;
	struct device *dev;
	struct block_device blk;
	u32 media_id;
	void (*efi_info)(struct device *);
};

static int efi_bio_read(struct block_device *blk, void *buffer, sector_t block,
		blkcnt_t num_blocks)
{
	struct efi_bio_priv *priv = container_of(blk, struct efi_bio_priv, blk);
	efi_status_t efiret;

	efiret = priv->protocol->read(priv->protocol, priv->media_id,
			block, num_blocks * 512, buffer);

	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static int efi_bio_write(struct block_device *blk,
		const void *buffer, sector_t block, blkcnt_t num_blocks)
{
	struct efi_bio_priv *priv = container_of(blk, struct efi_bio_priv, blk);
	efi_status_t efiret;

	efiret = priv->protocol->write(priv->protocol, priv->media_id,
			block, num_blocks * 512, (void *)buffer);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static int efi_bio_flush(struct block_device *blk)
{
	struct efi_bio_priv *priv = container_of(blk, struct efi_bio_priv, blk);
	efi_status_t efiret;

	efiret = priv->protocol->flush(priv->protocol);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static struct block_device_ops efi_bio_ops = {
	.read = efi_bio_read,
	.write = efi_bio_write,
	.flush = efi_bio_flush,
};

static void efi_bio_print_info(struct device *dev)
{
	struct efi_bio_priv *priv = dev->priv;
	struct efi_block_io_media *media = priv->protocol->media;
	u64 revision = priv->protocol->revision;

	printf("Block I/O Media:\n");
	printf("  revision: 0x%016llx\n", revision);
	printf("  media_id: 0x%08x\n", media->media_id);
	printf("  removable_media: %d\n", media->removable_media);
	printf("  media_present: %d\n", media->media_present);
	printf("  logical_partition: %d\n", media->logical_partition);
	printf("  read_only: %d\n", media->read_only);
	printf("  write_caching: %d\n", media->write_caching);
	printf("  block_size: 0x%08x\n", media->block_size);
	printf("  io_align: 0x%08x\n", media->io_align);
	printf("  last_block: 0x%016llx\n", media->last_block);

	if (revision < EFI_BLOCK_IO_PROTOCOL_REVISION2)
		goto out;

	printf("  lowest_aligned_lba: 0x%08llx\n",
			media->lowest_aligned_lba);
	printf("  logical_blocks_per_physical_block: 0x%08x\n",
			media->logical_blocks_per_physical_block);

	if (revision < EFI_BLOCK_IO_PROTOCOL_REVISION3)
		goto out;

	printf("  optimal_transfer_length_granularity: 0x%08x\n",
			media->optimal_transfer_length_granularity);

out:
	if (priv->efi_info)
		priv->efi_info(dev);
}

static bool is_bio_usbdev(struct efi_device *efidev)
{
	return IS_ENABLED(CONFIG_EFI_BLK_SEPARATE_USBDISK) &&
		efi_device_has_guid(efidev, EFI_USB_IO_PROTOCOL_GUID);
}

static int efi_bio_probe(struct efi_device *efidev)
{
	int instance;
	struct efi_bio_priv *priv;
	struct efi_block_io_media *media;
	struct device *dev = &efidev->dev;

	priv = xzalloc(sizeof(*priv));

	BS->handle_protocol(efidev->handle, &efi_block_io_protocol_guid,
			(void **)&priv->protocol);
	if (!priv->protocol)
		return -ENODEV;

	dev->priv = priv;
	priv->efi_info = dev->info;
	dev->info = efi_bio_print_info;

	media = priv->protocol->media;
	if (__is_defined(DEBUG))
		efi_bio_print_info(dev);
	priv->dev = &efidev->dev;

	if (is_bio_usbdev(efidev)) {
		instance = cdev_find_free_index("usbdisk");
		priv->blk.cdev.name = xasprintf("usbdisk%d", instance);
	} else {
		instance = cdev_find_free_index("disk");
		priv->blk.cdev.name = xasprintf("disk%d", instance);
	}

	priv->blk.blockbits = ffs(media->block_size) - 1;
	priv->blk.num_blocks = media->last_block + 1;
	priv->blk.ops = &efi_bio_ops;
	priv->blk.dev = &efidev->dev;

	priv->media_id = media->media_id;

	if (efi_get_bootsource() == efidev)
		bootsource_set_raw_instance(instance);

	return blockdevice_register(&priv->blk);
}

static struct efi_driver efi_bio_driver = {
        .driver = {
		.name  = "efi-block-io",
	},
        .probe = efi_bio_probe,
	.guid = EFI_BLOCK_IO_PROTOCOL_GUID,
};
device_efi_driver(efi_bio_driver);
