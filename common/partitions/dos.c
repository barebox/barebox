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
#include <stdlib.h>
#include <asm/unaligned.h>
#include <linux/err.h>
#include <partitions.h>

struct disk_signature_priv {
	uint32_t signature;
	struct block_device *blk;
	struct param_d *param;
};

struct dos_partition_desc {
	struct partition_desc pd;
	uint32_t signature;
	struct disk_signature_priv disksig;
};

struct dos_partition {
	struct partition part;
	bool extended;
	bool logical;

	uint8_t boot_indicator;
	uint8_t chs_begin[3];
	uint8_t type;
	uint8_t chs_end[3];
};

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
	void *buf = xmalloc(SECTOR_SIZE);
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

static void dos_extended_partition(struct block_device *blk, struct dos_partition_desc *dpd,
		struct partition *partition, uint32_t signature)
{
	uint8_t *buf = xmalloc(SECTOR_SIZE);
	uint32_t ebr_sector = partition->first_sec;
	struct partition_entry *table = (struct partition_entry *)&buf[0x1be];
	unsigned partno = 4;
	struct dos_partition *dpart;
	struct partition *pentry;

	while (1) {
		int rc, i;

		dev_dbg(blk->dev, "expect EBR in sector 0x%x\n", ebr_sector);

		rc = block_read(blk, buf, ebr_sector, 1);
		if (rc != 0) {
			dev_err(blk->dev, "Cannot read EBR partition table\n");
			goto out;
		}

		/* sanity checks */
		if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa) {
			dev_err(blk->dev, "sector 0x%x doesn't contain an EBR signature\n", ebr_sector);
			goto out;
		}

		for (i = 0x1de; i < 0x1fe; ++i)
			if (buf[i]) {
				dev_err(blk->dev, "EBR's third or fourth partition non-empty\n");
				goto out;
			}
		/* /sanity checks */

		dpart = xzalloc(sizeof(*dpart));
		dpart->logical = true;
		dpart->boot_indicator = table[0].boot_indicator;
		memcpy(dpart->chs_begin, table[0].chs_begin, sizeof(table[0].chs_begin));
		dpart->type = table[0].type;
		memcpy(dpart->chs_end, table[0].chs_end, sizeof(table[0].chs_end));

		pentry = &dpart->part;

		/* the first entry defines the extended partition */
		pentry->first_sec = ebr_sector +
			get_unaligned_le32(&table[0].partition_start);
		pentry->size = get_unaligned_le32(&table[0].partition_size);
		pentry->dos_partition_type = table[0].type;
		pentry->num = partno;
		sprintf(pentry->partuuid, "%08x-%02u", signature, partno + 1);

		list_add_tail(&pentry->list, &dpd->pd.partitions);

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

static void extract_flags(const struct partition_entry *p,
			  struct partition *pentry)
{
	if (p->boot_indicator == 0x80)
		pentry->flags |= DEVFS_PARTITION_BOOTABLE_LEGACY;
	if (p->type == 0xef)
		pentry->flags |= DEVFS_PARTITION_BOOTABLE_ESP;
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
	struct dos_partition *dpart;
	struct partition *pentry;
	struct partition *extended_partition = NULL;
	uint8_t *buffer = buf;
	int i;
	struct disk_signature_priv *dsp;
	uint32_t signature = get_unaligned_le32(buf + 0x1b8);
	struct dos_partition_desc *dpd;

	sprintf(blk->cdev.diskuuid, "%08x", signature);

	blk->cdev.flags |= DEVFS_IS_MBR_PARTITIONED;

	table = (struct partition_entry *)&buffer[446];

	dpd = xzalloc(sizeof(*dpd));
	partition_desc_init(&dpd->pd, blk);

	for (i = 0; i < 4; i++) {
		uint64_t first_sec = get_unaligned_le32(&table[i].partition_start);

		if (first_sec == 0) {
			dev_dbg(blk->dev, "Skipping empty partition %d\n", i);
			continue;
		}

		dpart = xzalloc(sizeof(*dpart));
		dpart->boot_indicator = table[i].boot_indicator;
		memcpy(dpart->chs_begin, table[i].chs_begin, sizeof(table[i].chs_begin));
		dpart->type = table[i].type;
		memcpy(dpart->chs_end, table[i].chs_end, sizeof(table[i].chs_end));

		pentry = &dpart->part;

		pentry->first_sec = first_sec;
		pentry->size = get_unaligned_le32(&table[i].partition_size);
		pentry->dos_partition_type = table[i].type;
		extract_flags(&table[i], pentry);
		pentry->num = i;

		sprintf(pentry->partuuid, "%08x-%02d", signature, i + 1);
		dpd->signature = signature;

		if (is_extended_partition(pentry)) {
			dpart->extended = true;
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

		list_add_tail(&pentry->list, &dpd->pd.partitions);
	}

	if (extended_partition)
		dos_extended_partition(blk, dpd, extended_partition, signature);

	dsp = &dpd->disksig;
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
	dsp->param = dev_add_param_uint32(blk->dev, "nt_signature",
			dos_set_disk_signature, dos_get_disk_signature,
			&dsp->signature, "%08x", dsp);

	return &dpd->pd;
}

static void dos_partition_free(struct partition_desc *pd)
{
	struct dos_partition_desc *dpd
		= container_of(pd, struct dos_partition_desc, pd);
	struct partition *part, *tmp;

	list_for_each_entry_safe(part, tmp, &pd->partitions, list) {
		struct dos_partition *dpart = container_of(part, struct dos_partition, part);

		free(dpart);
	}

	dev_remove_param(dpd->disksig.param);

	free(pd);
}

static __maybe_unused struct partition_desc *dos_partition_create_table(struct block_device *blk)
{
	struct dos_partition_desc *dpd = xzalloc(512);

	partition_desc_init(&dpd->pd, blk);

	dpd->signature = random32();

	return &dpd->pd;
}

static int fs_type_to_type(const char *fstype)
{
	unsigned long type;
	int ret;

	if (!strcmp(fstype, "ext2"))
		return 0x83;
	if (!strcmp(fstype, "ext3"))
		return 0x83;
	if (!strcmp(fstype, "ext4"))
		return 0x83;
	if (!strcmp(fstype, "fat16"))
		return 0xe;
	if (!strcmp(fstype, "fat32"))
		return 0xc;

	ret = kstrtoul(fstype, 16, &type);
	if (ret)
		return ret;

	if (type > 0xff)
		return -EINVAL;

	return type;
}

static __maybe_unused int dos_partition_mkpart(struct partition_desc *pd,
					       const char *name, const char *fs_type,
					       uint64_t start_lba, uint64_t end_lba)
{
	struct dos_partition *dpart;
	struct partition *part, *part_extended = NULL;
	int npart = 0, npart_logical = 0;
	uint64_t size;
	int mask_free = 0xf;
	int type = fs_type_to_type(fs_type);

	if (type < 0) {
		pr_err("invalid fs_type \"%s\"\n", fs_type);
		return -EINVAL;
	}

	if (start_lba < 1) {
		pr_err("invalid start LBA, minimum is 1\n");
		return -EINVAL;
	}

	list_for_each_entry(part, &pd->partitions, list) {
		dpart = container_of(part, struct dos_partition, part);

		mask_free &= ~(1 << npart);

		if (dpart->extended)
			part_extended = part;
		if (dpart->logical)
			npart_logical++;
		else
			npart++;
	}

	if (!strcmp(name, "extended")) {
		if (part_extended) {
			pr_err("extended partition already exists\n");
			return -ENOSPC;
		}
		goto create;
	} else if (!strcmp(name, "primary")) {
		if (npart == 4) {
			pr_err("Can't create any more partitions\n");
			return -ENOSPC;
		}

		goto create;
	} else if (!strcmp(name, "logical")) {
		if (!part_extended) {
			pr_err("No extended partition\n");
			return -EINVAL;
		}

		if (npart_logical >= 4) {
			pr_err("Can't create any more partitions\n");
			return -ENOSPC;
		}

		pr_err("Can't create logical partitions yet\n");
		return -EINVAL;
	} else {
		pr_err("Invalid name \"%s\"\n", name);
		return -EINVAL;
	}

	return 0;

create:
	dpart = xzalloc(sizeof(*dpart));
	part = &dpart->part;

	if (start_lba > UINT_MAX) {
		free(dpart);
		return -ENOSPC;
	}
	size = end_lba - start_lba + 1;

	if (size > UINT_MAX) {
		free(dpart);
		return -ENOSPC;
	}

	dpart->type = fs_type_to_type(fs_type);

	part->first_sec = start_lba;
	part->size = size;
	part->num = ffs(mask_free);

	list_add_tail(&part->list, &pd->partitions);

	return 0;
}

static __maybe_unused int dos_partition_rmpart(struct partition_desc *pd, struct partition *part)
{
	struct dos_partition *dpart = container_of(part, struct dos_partition, part);

	list_del(&part->list);
	free(dpart);

	return 0;
}

static __maybe_unused int dos_partition_write(struct partition_desc *pd)
{
	struct dos_partition_desc *dpd = container_of(pd, struct dos_partition_desc, pd);
	struct dos_partition *dpart;
	struct partition *part;
	void *buf;
	struct partition_entry *table, *entry;
	int ret;

	list_for_each_entry(part, &pd->partitions, list) {
		dpart = container_of(part, struct dos_partition, part);
		if (dpart->logical) {
			pr_err("Cannot write tables with logical partitions yet\n");
			return -EINVAL;
		}
	}

	buf = read_mbr(pd->blk);
	if (!buf)
		return -EIO;

	put_unaligned_le32(dpd->signature, buf + 0x1b8);

	table = buf + 0x1be;
	memset(table, 0x0, sizeof(*table) * 4);

	*(u8 *)(buf + 0x1fe) = 0x55;
	*(u8 *)(buf + 0x1ff) = 0xaa;

	list_for_each_entry(part, &pd->partitions, list) {
		dpart = container_of(part, struct dos_partition, part);

		entry = &table[part->num - 1];

		entry->boot_indicator = dpart->boot_indicator;
		memcpy(entry->chs_begin, dpart->chs_begin, sizeof(entry->chs_begin));
		entry->type = dpart->type;
		memcpy(entry->chs_end, dpart->chs_end, sizeof(entry->chs_end));
		put_unaligned_le32(part->first_sec, &entry->partition_start);
		put_unaligned_le32(part->size, &entry->partition_size);
	}

	ret = block_write(pd->blk, buf, 0, 1);

	free(buf);

	return ret;
}

static struct partition_parser dos = {
	.parse = dos_partition,
	.partition_free = dos_partition_free,
#ifdef CONFIG_PARTITION_MANIPULATION
	.create = dos_partition_create_table,
	.mkpart = dos_partition_mkpart,
	.rmpart = dos_partition_rmpart,
	.write = dos_partition_write,
#endif
	.type = filetype_mbr,
	.name = "msdos",
};

static int dos_partition_init(void)
{
	return partition_parser_register(&dos);
}
postconsole_initcall(dos_partition_init);
