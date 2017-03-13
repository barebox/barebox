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
#include <disks.h>
#include <efi/efi.h>
#include <efi/efi-device.h>

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
	u64 last_block;
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
	struct device_d *dev;
	struct block_device blk;
	u32 media_id;
};

static int efi_bio_read(struct block_device *blk, void *buffer, int block,
		int num_blocks)
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
		const void *buffer, int block, int num_blocks)
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

static void efi_bio_print_info(struct efi_bio_priv *priv)
{
	struct efi_block_io_media *media = priv->protocol->media;
	u64 revision = priv->protocol->revision;

	dev_dbg(priv->dev, "revision: 0x%016llx\n", revision);
	dev_dbg(priv->dev, "media_id: 0x%08x\n", media->media_id);
	dev_dbg(priv->dev, "removable_media: %d\n", media->removable_media);
	dev_dbg(priv->dev, "media_present: %d\n", media->media_present);
	dev_dbg(priv->dev, "logical_partition: %d\n", media->logical_partition);
	dev_dbg(priv->dev, "read_only: %d\n", media->read_only);
	dev_dbg(priv->dev, "write_caching: %d\n", media->write_caching);
	dev_dbg(priv->dev, "block_size: 0x%08x\n", media->block_size);
	dev_dbg(priv->dev, "io_align: 0x%08x\n", media->io_align);
	dev_dbg(priv->dev, "last_block: 0x%016llx\n", media->last_block);

	if (revision < EFI_BLOCK_IO_PROTOCOL_REVISION2)
		return;

	dev_dbg(priv->dev, "u64 lowest_aligned_lba: 0x%08llx\n",
			media->lowest_aligned_lba);
	dev_dbg(priv->dev, "logical_blocks_per_physical_block: 0x%08x\n",
			media->logical_blocks_per_physical_block);

	if (revision < EFI_BLOCK_IO_PROTOCOL_REVISION3)
		return;

	dev_dbg(priv->dev, "optimal_transfer_length_granularity: 0x%08x\n",
			media->optimal_transfer_length_granularity);
}

int efi_bio_probe(struct efi_device *efidev)
{
	int ret;
	struct efi_bio_priv *priv;
	struct efi_block_io_media *media;

	priv = xzalloc(sizeof(*priv));

	BS->handle_protocol(efidev->handle, &efi_block_io_protocol_guid,
			(void **)&priv->protocol);
	if (!priv->protocol)
		return -ENODEV;

	media = priv->protocol->media;
	efi_bio_print_info(priv);
	priv->dev = &efidev->dev;

	priv->blk.cdev.name = xasprintf("disk%d", cdev_find_free_index("disk"));
	priv->blk.blockbits = ffs(media->block_size) - 1;
	priv->blk.num_blocks = media->last_block + 1;
	priv->blk.ops = &efi_bio_ops;
	priv->blk.dev = &efidev->dev;

	priv->media_id = media->media_id;

	ret = blockdevice_register(&priv->blk);
	if (ret)
		return ret;

	parse_partition_table(&priv->blk);

	return 0;
}

static struct efi_driver efi_fs_driver = {
        .driver = {
		.name  = "efi-block-io",
	},
        .probe = efi_bio_probe,
	.guid = EFI_BLOCK_IO_PROTOCOL_GUID,
};
device_efi_driver(efi_fs_driver);
