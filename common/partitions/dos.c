/*
 * Copyright (C) 2009...2011 Juergen Beisert, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <disks.h>
#include <init.h>
#include <asm/unaligned.h>
#include <linux/err.h>

#include "parser.h"


enum {
/* These three have identical behaviour; use the second one if DOS FDISK gets
   confused about extended/logical partitions starting past cylinder 1023. */
	DOS_EXTENDED_PARTITION = 5,
	LINUX_EXTENDED_PARTITION = 0x85,
	WIN98_EXTENDED_PARTITION = 0x0f,
};

static inline int is_extended_partition(struct partition *p)
{
	return (p->dos_partition_type == DOS_EXTENDED_PARTITION ||
	        p->dos_partition_type == WIN98_EXTENDED_PARTITION ||
		p->dos_partition_type == LINUX_EXTENDED_PARTITION);
}

static void *read_mbr(struct block_device *blk)
{
	void *buf = malloc(SECTOR_SIZE);
	int ret;

	ret = block_read(blk, buf, 0, 1);
	if (ret) {
		free(buf);
		return NULL;
	}

	return buf;
}

static int write_mbr(struct block_device *blk, void *buf)
{
	int ret;

	ret = block_write(blk, buf, 0, 1);
	if (ret)
		return ret;

	return block_flush(blk);
}

struct disk_signature_priv {
	uint32_t signature;
	struct block_device *blk;
};

static int dos_set_disk_signature(struct param_d *p, void *_priv)
{
	struct disk_signature_priv *priv = _priv;
	struct block_device *blk = priv->blk;
	void *buf;
	__le32 *disksigp;
	int ret;

	buf = read_mbr(blk);
	if (!buf)
		return -EIO;

	disksigp = buf + 0x1b8;

	*disksigp = cpu_to_le32(priv->signature);

	ret = write_mbr(blk, buf);

	free(buf);

	return ret;
}

static int dos_get_disk_signature(struct param_d *p, void *_priv)
{
	struct disk_signature_priv *priv = _priv;
	struct block_device *blk = priv->blk;
	void *buf;

	buf = read_mbr(blk);
	if (!buf)
		return -EIO;

	priv->signature = get_unaligned_le32(buf + 0x1b8);

	free(buf);

	return 0;
}

static void dos_extended_partition(struct block_device *blk, struct partition_desc *pd,
		struct partition *partition, uint32_t signature)
{
	uint8_t *buf = malloc(SECTOR_SIZE);
	uint32_t ebr_sector = partition->first_sec;
	struct partition_entry *table = (struct partition_entry *)&buf[0x1be];
	unsigned partno = 5;
	struct partition *pentry;

	while (1) {
		int rc, i;

		dev_dbg(blk->dev, "expect EBR in sector %x\n", ebr_sector);

		rc = block_read(blk, buf, ebr_sector, 1);
		if (rc != 0) {
			dev_err(blk->dev, "Cannot read EBR partition table\n");
			goto out;
		}

		/* sanity checks */
		if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa) {
			dev_err(blk->dev, "sector %x doesn't contain an EBR signature\n", ebr_sector);
			goto out;
		}

		for (i = 0x1de; i < 0x1fe; ++i)
			if (buf[i]) {
				dev_err(blk->dev, "EBR's third or fourth partition non-empty\n");
				goto out;
			}
		/* /sanity checks */

		pentry = xzalloc(sizeof(*pentry));

		/* the first entry defines the extended partition */
		pentry->first_sec = ebr_sector +
			get_unaligned_le32(&table[0].partition_start);
		pentry->size = get_unaligned_le32(&table[0].partition_size);
		pentry->dos_partition_type = table[0].type;
		pentry->num = partno;
		if (signature)
			sprintf(pentry->partuuid, "%08x-%02u",
				signature, partno);

		list_add_tail(&pentry->list, &pd->partitions);
		partno++;

		/* the second entry defines the start of the next ebr if != 0 */
		if (get_unaligned_le32(&table[1].partition_start))
			ebr_sector = partition->first_sec +
				get_unaligned_le32(&table[1].partition_start);
		else
			break;
	}

out:
	free(buf);
	return;
}

/**
 * Check if a DOS like partition describes this block device
 * @param blk Block device to register to
 * @param pd Where to store the partition information
 *
 * It seems at least on ARM this routine cannot use temp. stack space for the
 * sector. So, keep the malloc/free.
 */
static struct partition_desc *dos_partition(void *buf, struct block_device *blk)
{
	struct partition_entry *table;
	struct partition *pentry;
	struct partition *extended_partition = NULL;
	uint8_t *buffer = buf;
	int i;
	struct disk_signature_priv *dsp;
	uint32_t signature = get_unaligned_le32(buf + 0x1b8);
	struct partition_desc *pd;

	if (signature)
		sprintf(blk->cdev.diskuuid, "%08x", signature);

	blk->cdev.flags |= DEVFS_IS_MBR_PARTITIONED;

	table = (struct partition_entry *)&buffer[446];

	pd = xzalloc(sizeof(*pd));
	INIT_LIST_HEAD(&pd->partitions);

	for (i = 0; i < 4; i++) {
		uint64_t first_sec = get_unaligned_le32(&table[i].partition_start);

		if (first_sec == 0) {
			dev_dbg(blk->dev, "Skipping empty partition %d\n", i);
			continue;
		}

		pentry = xzalloc(sizeof(*pentry));

		pentry->first_sec = first_sec;
		pentry->size = get_unaligned_le32(&table[i].partition_size);
		pentry->dos_partition_type = table[i].type;

		if (signature)
			sprintf(pentry->partuuid, "%08x-%02d", signature, i + 1);

		if (is_extended_partition(pentry)) {
			pentry->size = 2;

			if (!extended_partition)
				extended_partition = pentry;
			else
				/*
				 * An DOS MBR must only contain a single
				 * extended partition. Just ignore all
				 * but the first.
				 */
				dev_warn(blk->dev, "Skipping additional extended partition\n");
		}

		list_add_tail(&pentry->list, &pd->partitions);
	}

	if (extended_partition)
		dos_extended_partition(blk, pd, extended_partition, signature);

	dsp = xzalloc(sizeof(*dsp));
	dsp->blk = blk;

	/*
	 * This parameter contains the NT disk signature. This allows to
	 * to specify the Linux rootfs using the following syntax:
	 *
	 *   root=PARTUUID=ssssssss-pp
	 *
	 * where ssssssss is a zero-filled hex representation of the 32-bit
	 * signature and pp is a zero-filled hex representation of the 1-based
	 * partition number.
	 */
	dev_add_param_uint32(blk->dev, "nt_signature",
			dos_set_disk_signature, dos_get_disk_signature,
			&dsp->signature, "%08x", dsp);

	return pd;
}

static void dos_partition_free(struct partition_desc *pd)
{
	struct partition *part, *tmp;

	list_for_each_entry_safe(part, tmp, &pd->partitions, list)
		free(part);

	free(pd);
}

static struct partition_parser dos = {
	.parse = dos_partition,
	.partition_free = dos_partition_free,
	.type = filetype_mbr,
};

static int dos_partition_init(void)
{
	return partition_parser_register(&dos);
}
postconsole_initcall(dos_partition_init);
