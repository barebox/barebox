/************************************************************
 * EFI GUID Partition Table handling
 *
 * http://www.uefi.org/specs/
 * http://www.intel.com/technology/efi/
 *
 * efi.[ch] by Matt Domsch <Matt_Domsch@dell.com>
 *   Copyright 2000,2001,2002,2004 Dell Inc.
 *
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <disks.h>
#include <init.h>
#include <asm/unaligned.h>
#include <crc.h>
#include <linux/ctype.h>

#include <efi/partition.h>
#include <partitions.h>

struct efi_partition_desc {
	struct partition_desc pd;
	gpt_header *gpt;
	gpt_entry *ptes;
};

struct efi_partition {
	struct partition part;
	gpt_entry *pte;
};

static const int force_gpt = IS_ENABLED(CONFIG_PARTITION_DISK_EFI_GPT_NO_FORCE);

/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 *
 * This function uses the little endian Ethernet polynomial
 * but seeds the function with ~0, and xor's with ~0 at the end.
 * Note, the EFI Specification, v1.02, has a reference to
 * Dr. Dobbs Journal, May 1994 (actually it's in May 1992).
 */
static inline u32
efi_crc32(const void *buf, unsigned long len)
{
	return crc32(0, buf, len);
}

/**
 * last_lba(): return number of last logical block of device
 * @bdev: block device
 *
 * Description: Returns last LBA value on success, 0 on error.
 * This is stored (by sd and ide-geometry) in
 *  the part[0] entry for this disk, and is the number of
 *  physical sectors available on the disk.
 */
static u64 last_lba(struct block_device *bdev)
{
	if (!bdev)
		return 0;
	return bdev->num_blocks - 1;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @dev_desc
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
static gpt_entry *alloc_read_gpt_entries(struct block_device *blk,
					 gpt_header * pgpt_head)
{
	size_t count = 0;
	gpt_entry *pte = NULL;
	unsigned long from, size;
	int ret;

	count = le32_to_cpu(pgpt_head->num_partition_entries) *
		le32_to_cpu(pgpt_head->sizeof_partition_entry);

	if (!count)
		return NULL;

	pte = calloc(count, 1);
	if (!pte)
		return NULL;

	from = le64_to_cpu(pgpt_head->partition_entry_lba);
	size = count / GPT_BLOCK_SIZE;
	ret = block_read(blk, pte, from, size);
	if (ret) {
		free(pte);
		pte=NULL;
		return NULL;
	}
	return pte;
}

static inline unsigned short bdev_logical_block_size(struct block_device
*bdev)
{
	return SECTOR_SIZE;
}

/**
 * alloc_read_gpt_header(): Allocates GPT header, reads into it from disk
 * @state
 * @lba is the Logical Block Address of the partition table
 *
 * Description: returns GPT header on success, NULL on error.   Allocates
 * and fills a GPT header starting at @ from @state->bdev.
 * Note: remember to free gpt when finished with it.
 */
static gpt_header *alloc_read_gpt_header(struct block_device *blk,
					 u64 lba)
{
	gpt_header *gpt;
	unsigned ssz = bdev_logical_block_size(blk);
	int ret;

	gpt = calloc(ssz, 1);
	if (!gpt)
		return NULL;

	ret = block_read(blk, gpt, lba, 1);
	if (ret) {
		free(gpt);
		gpt=NULL;
		return NULL;
	}

	return gpt;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 *
 * lba is the logical block address of the GPT header to test
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error.
 * If valid, returns pointers to PTEs.
 */
static int is_gpt_valid(struct block_device *blk, u64 lba,
			gpt_header **gpt, gpt_entry **ptes)
{
	u32 crc, origcrc;
	u64 lastlba;

	if (!ptes)
		return 0;
	if (!(*gpt = alloc_read_gpt_header(blk, lba)))
		return 0;

	/* Check the GPT header signature */
	if (le64_to_cpu((*gpt)->signature) != GPT_HEADER_SIGNATURE) {
		dev_dbg(blk->dev, "GUID Partition Table Header signature at LBA"
			"%llu is wrong: 0x%llX != 0x%llX\n", lba,
			(unsigned long long)le64_to_cpu((*gpt)->signature),
			(unsigned long long)GPT_HEADER_SIGNATURE);
		goto fail;
	}

	/* Check the GUID Partition Table CRC */
	origcrc = le32_to_cpu((*gpt)->header_crc32);
	(*gpt)->header_crc32 = 0;
	crc = efi_crc32((const unsigned char *) (*gpt), le32_to_cpu((*gpt)->header_size));

	if (crc != origcrc) {
		dev_dbg(blk->dev, "GUID Partition Table Header CRC is wrong: %x != %x\n",
			 crc, origcrc);
		goto fail;
	}
	(*gpt)->header_crc32 = cpu_to_le32(origcrc);

	/* Check that the my_lba entry points to the LBA that contains
	 * the GUID Partition Table */
	if (le64_to_cpu((*gpt)->my_lba) != lba) {
		dev_dbg(blk->dev, "GPT: my_lba incorrect: %llX != %llX\n",
			(unsigned long long)le64_to_cpu((*gpt)->my_lba),
			(unsigned long long)lba);
		goto fail;
	}

	/* Check the first_usable_lba and last_usable_lba are within the disk. */
	lastlba = last_lba(blk);
	if (le64_to_cpu((*gpt)->first_usable_lba) > lastlba) {
		dev_dbg(blk->dev, "GPT: first_usable_lba incorrect: %lld > %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->first_usable_lba),
			 (unsigned long long)lastlba);
		goto fail;
	}
	if (le64_to_cpu((*gpt)->last_usable_lba) > lastlba) {
		dev_dbg(blk->dev, "GPT: last_usable_lba incorrect: %lld > %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->last_usable_lba),
			 (unsigned long long)lastlba);
		goto fail;
	}

	if (!(*ptes = alloc_read_gpt_entries(blk, *gpt)))
		goto fail;

	/* Check the GUID Partition Table Entry Array CRC */
	crc = efi_crc32((const unsigned char *)*ptes,
		le32_to_cpu((*gpt)->num_partition_entries) *
		le32_to_cpu((*gpt)->sizeof_partition_entry));

	if (crc != le32_to_cpu((*gpt)->partition_entry_array_crc32)) {
		dev_dbg(blk->dev, "GUID Partitition Entry Array CRC check failed: 0x%08x 0x%08x\n",
			crc, le32_to_cpu((*gpt)->partition_entry_array_crc32));
		goto fail_ptes;
	}

	/* We're done, all's well */
	return 1;

 fail_ptes:
	free(*ptes);
	*ptes = NULL;
 fail:
	free(*gpt);
	*gpt = NULL;
	return 0;
}

/**
 * is_pte_valid() - tests one PTE for validity
 * @pte is the pte to check
 * @lastlba is last lba of the disk
 *
 * Description: returns 1 if valid,  0 on error.
 */
static inline int
is_pte_valid(const gpt_entry *pte, const u64 lastlba)
{
	if (guid_is_null(&pte->partition_type_guid) ||
	    le64_to_cpu(pte->starting_lba) > lastlba	 ||
	    le64_to_cpu(pte->ending_lba)   > lastlba)
		return 0;
	return 1;
}

/**
 * compare_gpts() - Search disk for valid GPT headers and PTEs
 * @pgpt is the primary GPT header
 * @agpt is the alternate GPT header
 * @lastlba is the last LBA number
 * Description: Returns nothing.  Sanity checks pgpt and agpt fields
 * and prints warnings on discrepancies.
 *
 */
static void
compare_gpts(struct device *dev, gpt_header *pgpt, gpt_header *agpt,
	     u64 lastlba)
{
	int error_found = 0;
	if (!pgpt || !agpt)
		return;
	if (le64_to_cpu(pgpt->my_lba) != le64_to_cpu(agpt->alternate_lba)) {
		dev_warn(dev,
		       "GPT:Primary header LBA != Alt. header alternate_lba\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->my_lba),
		       (unsigned long long)le64_to_cpu(agpt->alternate_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != le64_to_cpu(agpt->my_lba)) {
		dev_warn(dev,
		       "GPT:Primary header alternate_lba != Alt. header my_lba\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->alternate_lba),
		       (unsigned long long)le64_to_cpu(agpt->my_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->first_usable_lba) !=
	    le64_to_cpu(agpt->first_usable_lba)) {
		dev_warn(dev, "GPT:first_usable_lbas don't match.\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->first_usable_lba),
		       (unsigned long long)le64_to_cpu(agpt->first_usable_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->last_usable_lba) !=
	    le64_to_cpu(agpt->last_usable_lba)) {
		dev_warn(dev, "GPT:last_usable_lbas don't match.\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->last_usable_lba),
		       (unsigned long long)le64_to_cpu(agpt->last_usable_lba));
		error_found++;
	}
	if (!guid_equal(&pgpt->disk_guid, &agpt->disk_guid)) {
		dev_warn(dev, "GPT:disk_guids don't match.\n");
		error_found++;
	}
	if (le32_to_cpu(pgpt->num_partition_entries) !=
	    le32_to_cpu(agpt->num_partition_entries)) {
		dev_warn(dev, "GPT:num_partition_entries don't match: "
		       "0x%x != 0x%x\n",
		       le32_to_cpu(pgpt->num_partition_entries),
		       le32_to_cpu(agpt->num_partition_entries));
		error_found++;
	}
	if (le32_to_cpu(pgpt->sizeof_partition_entry) !=
	    le32_to_cpu(agpt->sizeof_partition_entry)) {
		dev_warn(dev,
		       "GPT:sizeof_partition_entry values don't match: "
		       "0x%x != 0x%x\n",
		       le32_to_cpu(pgpt->sizeof_partition_entry),
		       le32_to_cpu(agpt->sizeof_partition_entry));
		error_found++;
	}
	if (le32_to_cpu(pgpt->partition_entry_array_crc32) !=
	    le32_to_cpu(agpt->partition_entry_array_crc32)) {
		dev_warn(dev,
		       "GPT:partition_entry_array_crc32 values don't match: "
		       "0x%x != 0x%x\n",
		       le32_to_cpu(pgpt->partition_entry_array_crc32),
		       le32_to_cpu(agpt->partition_entry_array_crc32));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != lastlba) {
		dev_warn(dev,
		       "GPT:Primary header thinks Alt. header is not at the end of the disk.\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(pgpt->alternate_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (le64_to_cpu(agpt->my_lba) != lastlba) {
		dev_warn(dev,
		       "GPT:Alternate GPT header not at the end of the disk.\n");
		dev_warn(dev, "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(agpt->my_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (error_found)
		dev_warn(dev, "GPT: Use GNU Parted to correct GPT errors.\n");
	return;
}

/**
 * find_valid_gpt() - Search disk for valid GPT headers and PTEs
 * @state
 * @gpt is a GPT header ptr, filled on return.
 * @ptes is a PTEs ptr, filled on return.
 * Description: Returns 1 if valid, 0 on error.
 * If valid, returns pointers to newly allocated GPT header and PTEs.
 * Validity depends on PMBR being valid (or being overridden by the
 * 'gpt' kernel command line option) and finding either the Primary
 * GPT header and PTEs valid, or the Alternate GPT header and PTEs
 * valid.  If the Primary GPT header is not valid, the Alternate GPT header
 * is not checked unless the 'gpt' kernel command line option is passed.
 * This protects against devices which misreport their size, and forces
 * the user to decide to use the Alternate GPT.
 */
static int find_valid_gpt(void *buf, struct block_device *blk, gpt_header **gpt,
			  gpt_entry **ptes)
{
	int good_pgpt = 0, good_agpt = 0;
	gpt_header *pgpt = NULL, *agpt = NULL;
	gpt_entry *pptes = NULL, *aptes = NULL;
	u64 lastlba;

	if (!ptes)
		return 0;

	lastlba = last_lba(blk);
	if (force_gpt) {
		/* This will be added to the EFI Spec. per Intel after v1.02. */
		if (file_detect_partition_table(buf, SECTOR_SIZE * 2) != filetype_gpt)
			goto fail;
	}

	good_pgpt = is_gpt_valid(blk, GPT_PRIMARY_PARTITION_TABLE_LBA,
				 &pgpt, &pptes);
	if (good_pgpt)
		good_agpt = is_gpt_valid(blk,
					 le64_to_cpu(pgpt->alternate_lba),
					 &agpt, &aptes);
	if (!good_agpt && force_gpt)
		good_agpt = is_gpt_valid(blk, lastlba, &agpt, &aptes);

	/* The obviously unsuccessful case */
	if (!good_pgpt && !good_agpt)
		goto fail;

	if (IS_ENABLED(CONFIG_PARTITION_DISK_EFI_GPT_COMPARE))
		compare_gpts(blk->dev, pgpt, agpt, lastlba);

	/* The good cases */
	if (good_pgpt) {
		*gpt  = pgpt;
		*ptes = pptes;
		free(agpt);
		free(aptes);
		if (!good_agpt)
			dev_warn(blk->dev, "Alternate GPT is invalid, using primary GPT.\n");
		return 1;
	}
	else if (good_agpt) {
		*gpt  = agpt;
		*ptes = aptes;
		free(pgpt);
		free(pptes);
		dev_warn(blk->dev, "Primary GPT is invalid, using alternate GPT.\n");
		return 1;
	}

 fail:
	free(pgpt);
	free(agpt);
	free(pptes);
	free(aptes);
	*gpt = NULL;
	*ptes = NULL;
	return 0;
}

static void part_set_efi_name(gpt_entry *pte, char *dest)
{
	int i;

	for (i = 0; i < GPT_PARTNAME_MAX_SIZE ; i++) {
		u8 c;
		c = pte->partition_name[i] & 0xff;
		c = (c && !isprint(c)) ? '.' : c;
		dest[i] = c;
	}
	dest[i] = 0;
}

static void extract_flags(const gpt_entry *p, struct partition *pentry)
{
	static const guid_t system_guid = PARTITION_SYSTEM_GUID;

	if (p->attributes.required_to_function)
		pentry->flags |= DEVFS_PARTITION_REQUIRED;
	if (p->attributes.no_block_io_protocol)
		pentry->flags |= DEVFS_PARTITION_NO_EXPORT;
	if (p->attributes.legacy_bios_bootable)
		pentry->flags |= DEVFS_PARTITION_BOOTABLE_LEGACY;

	if (guid_equal(&p->partition_type_guid, &system_guid))
		pentry->flags |=  DEVFS_PARTITION_BOOTABLE_ESP;

	pentry->typeflags = p->attributes.type_guid_specific;
}

static void part_get_efi_name(gpt_entry *pte, const char *src)
{
	int i;
	int len = strlen(src);

	for (i = 0; i < GPT_PARTNAME_MAX_SIZE ; i++) {
		if (i <= len)
			pte->partition_name[i] = src[i];
		else
			pte->partition_name[i] = 0;
	}
}

static struct partition_desc *efi_partition(void *buf, struct block_device *blk)
{
	gpt_header *gpt = NULL;
	gpt_entry *ptes = NULL;
	int i = 0;
	int nb_part;
	struct efi_partition *epart;
	struct partition *pentry;
	struct efi_partition_desc *epd;

	if (!find_valid_gpt(buf, blk, &gpt, &ptes) || !gpt || !ptes)
		return NULL;

	snprintf(blk->cdev.diskuuid, sizeof(blk->cdev.diskuuid), "%pUl", &gpt->disk_guid);
	dev_add_param_string_fixed(blk->dev, "guid", blk->cdev.diskuuid);

	blk->cdev.flags |= DEVFS_IS_GPT_PARTITIONED;

	nb_part = le32_to_cpu(gpt->num_partition_entries);

	if (nb_part > MAX_PARTITION) {
		dev_warn(blk->dev, "GPT has more partitions than we support (%d) > max partition number (%d)\n",
			 nb_part, MAX_PARTITION);
		nb_part = MAX_PARTITION;
	}

	epd = xzalloc(sizeof(*epd));
	partition_desc_init(&epd->pd, blk);

	epd->gpt = gpt;
	epd->ptes = ptes;

	for (i = 0; i < nb_part; i++) {
		if (!is_pte_valid(&ptes[i], last_lba(blk))) {
			dev_dbg(blk->dev, "Invalid pte %d\n", i);
			continue;
		}

		epart = xzalloc(sizeof(*epart));
		epart->pte = &ptes[i];
		pentry = &epart->part;
		extract_flags(&ptes[i], pentry);
		pentry->first_sec = le64_to_cpu(ptes[i].starting_lba);
		pentry->size = le64_to_cpu(ptes[i].ending_lba) - pentry->first_sec;
		pentry->size++;
		part_set_efi_name(&ptes[i], pentry->name);
		snprintf(pentry->partuuid, sizeof(pentry->partuuid), "%pUl", &ptes[i].unique_partition_guid);
		pentry->typeuuid = ptes[i].partition_type_guid;
		pentry->num = i;
		list_add_tail(&pentry->list, &epd->pd.partitions);
	}

	return &epd->pd;
}

static void efi_partition_free(struct partition_desc *pd)
{
	struct efi_partition_desc *epd = container_of(pd, struct efi_partition_desc, pd);
	struct partition *part, *tmp;

	list_for_each_entry_safe(part, tmp, &pd->partitions, list) {
		struct efi_partition *epart = container_of(part, struct efi_partition, part);

		free(epart);
	}

	free(epd->ptes);
	free(epd->gpt);
	free(epd);
}

static __maybe_unused struct partition_desc *efi_partition_create_table(struct block_device *blk)
{
	struct efi_partition_desc *epd = xzalloc(sizeof(*epd));
	gpt_header *gpt;

	partition_desc_init(&epd->pd, blk);

	epd->gpt = xzalloc(512);
	gpt = epd->gpt;

	gpt->signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	gpt->revision = cpu_to_le32(0x100);
	gpt->header_size = cpu_to_le32(sizeof(*gpt));
	gpt->my_lba = cpu_to_le64(1);
	gpt->alternate_lba = cpu_to_le64(last_lba(blk));
	gpt->first_usable_lba = cpu_to_le64(34);
	gpt->last_usable_lba = cpu_to_le64(last_lba(blk) - 34);;
	generate_random_guid((unsigned char *)&gpt->disk_guid);
	gpt->partition_entry_lba = cpu_to_le64(2);
	gpt->num_partition_entries = cpu_to_le32(128);
	gpt->sizeof_partition_entry = cpu_to_le32(sizeof(gpt_entry));

	pr_info("Created new disk label with GUID %pU\n", &gpt->disk_guid);

	epd->ptes = xzalloc(128 * sizeof(gpt_entry));

	return &epd->pd;
}

static guid_t partition_linux_data_guid = PARTITION_LINUX_DATA_GUID;
static guid_t partition_basic_data_guid = PARTITION_BASIC_DATA_GUID;
static guid_t partition_barebox_env_guid = PARTITION_BAREBOX_ENVIRONMENT_GUID;

static const guid_t *fs_type_to_guid(const char *fstype)
{
	if (!strcmp(fstype, "ext2"))
		return &partition_linux_data_guid;
	if (!strcmp(fstype, "ext3"))
		return &partition_linux_data_guid;
	if (!strcmp(fstype, "ext4"))
		return &partition_linux_data_guid;
	if (!strcmp(fstype, "fat16"))
		return &partition_basic_data_guid;
	if (!strcmp(fstype, "fat32"))
		return &partition_basic_data_guid;
	if (!strcmp(fstype, "bbenv"))
		return &partition_barebox_env_guid;

	return NULL;
}

static __maybe_unused int efi_partition_mkpart(struct partition_desc *pd,
					       const char *name, const char *fs_type,
					       uint64_t start_lba, uint64_t end_lba)
{
	struct efi_partition_desc *epd = container_of(pd, struct efi_partition_desc, pd);
	struct efi_partition *epart;
	struct partition *part;
	gpt_header *gpt = epd->gpt;
	int num_parts = le32_to_cpu(gpt->num_partition_entries);
	gpt_entry *pte;
	int i;
	const guid_t *guid;

	if (start_lba < 34) {
		pr_err("invalid start LBA %lld, minimum is 34\n", start_lba);
		return -EINVAL;
	}

	if (end_lba >= last_lba(pd->blk) - 33) {
		pr_err("invalid end LBA %lld, maximum is %lld\n", start_lba,
		       last_lba(pd->blk) - 33);
		return -EINVAL;
	}

	for (i = 0; i < num_parts; i++) {
		if (!is_pte_valid(&epd->ptes[i], last_lba(pd->blk)))
			break;
	}

	if (i == num_parts) {
		pr_err("partition table is full\n");
		return -ENOSPC;
	}

	guid = fs_type_to_guid(fs_type);
	if (!guid) {
		pr_err("Unknown fs type %s\n", fs_type);
		return -EINVAL;
	}

	pte = &epd->ptes[i];
	epart = xzalloc(sizeof(*epart));
	part = &epart->part;

	part->first_sec = start_lba;
	part->size = end_lba - start_lba + 1;
	part->typeuuid = *guid;

	pte->partition_type_guid = *guid;
	generate_random_guid((unsigned char *)&pte->unique_partition_guid);
	pte->starting_lba = cpu_to_le64(start_lba);
	pte->ending_lba = cpu_to_le64(end_lba);
	part_get_efi_name(pte, name);
	part_set_efi_name(pte, part->name);
	part->num = i;

	list_add_tail(&part->list, &pd->partitions);

	return 0;
}

static __maybe_unused int efi_partition_rmpart(struct partition_desc *pd, struct partition *part)
{
	struct efi_partition *epart = container_of(part, struct efi_partition, part);

	memset(epart->pte, 0, sizeof(*epart->pte));

	list_del(&part->list);
	free(epart);

	return 0;
}

static int efi_protective_mbr(struct block_device *blk)
{
	struct partition_desc *pdesc;
	int ret;

	pdesc = partition_table_new(blk, "msdos");
	if (IS_ERR(pdesc)) {
		printf("Error: Cannot create partition table: %pe\n", pdesc);
		return PTR_ERR(pdesc);
	}

	ret = partition_create(pdesc, "primary", "0xee", 1, last_lba(blk));
	if (ret) {
		pr_err("Cannot create partition: %pe\n", ERR_PTR(ret));
		goto out;
	}

	ret = partition_table_write(pdesc);
	if (ret) {
		pr_err("Cannot write partition: %pe\n", ERR_PTR(ret));
		goto out;
	}
out:
	partition_table_free(pdesc);

	return ret;
}

static __maybe_unused int efi_partition_write(struct partition_desc *pd)
{
	struct block_device *blk = pd->blk;
	struct efi_partition_desc *epd = container_of(pd, struct efi_partition_desc, pd);
	gpt_header *gpt = epd->gpt, *altgpt;
	int ret;
	uint32_t count;
	uint64_t from, size;

	if (le32_to_cpu(gpt->num_partition_entries) != 128) {
		/*
		 * This is not yet properly implemented. At least writing of the
		 * alternative GPT is not correctly implemented for this case as
		 * we can't assume that the partition entries are written at
		 * last_lba() - 32, we would have to calculate that from the number
		 * of partition entries.
		 */
		pr_err("num_partition_entries is != 128. This is not yet supported for writing\n");
		return -EINVAL;
	}

	count = le32_to_cpu(gpt->num_partition_entries) *
		le32_to_cpu(gpt->sizeof_partition_entry);

	gpt->my_lba = cpu_to_le64(1);
	gpt->partition_entry_array_crc32 = cpu_to_le32(efi_crc32(
			(const unsigned char *)epd->ptes, count));
	gpt->header_crc32 = 0;
	gpt->header_crc32 = cpu_to_le32(efi_crc32((const unsigned char *)gpt,
						  le32_to_cpu(gpt->header_size)));

	ret = efi_protective_mbr(blk);
	if (ret)
		return ret;

	ret = block_write(blk, gpt, 1, 1);
	if (ret)
		goto err_block_write;

	from = le64_to_cpu(gpt->partition_entry_lba);
	size = count / GPT_BLOCK_SIZE;

	ret = block_write(blk, epd->ptes, from, size);
	if (ret)
		goto err_block_write;

	altgpt = xmemdup(gpt, SECTOR_SIZE);

	altgpt->alternate_lba = cpu_to_le64(1);
	altgpt->my_lba = cpu_to_le64(last_lba(blk));
	altgpt->partition_entry_lba = cpu_to_le64(last_lba(blk) - 32);
	altgpt->header_crc32 = 0;
	altgpt->header_crc32 = cpu_to_le32(efi_crc32((const unsigned char *)altgpt,
						  le32_to_cpu(altgpt->header_size)));
	ret = block_write(blk, altgpt, last_lba(blk), 1);

	free(altgpt);

	if (ret)
		goto err_block_write;
	ret = block_write(blk, epd->ptes, last_lba(blk) - 32, 32);
	if (ret)
		goto err_block_write;

	return 0;

err_block_write:
	pr_err("Cannot write to block device: %pe\n", ERR_PTR(ret));

	return ret;
}

static struct partition_parser efi_partition_parser = {
	.parse = efi_partition,
	.partition_free = efi_partition_free,
#ifdef CONFIG_PARTITION_MANIPULATION
	.create = efi_partition_create_table,
	.mkpart = efi_partition_mkpart,
	.rmpart = efi_partition_rmpart,
	.write = efi_partition_write,
#endif
	.type = filetype_gpt,
	.name = "gpt",
};

static int efi_partition_init(void)
{
	return partition_parser_register(&efi_partition_parser);
}
postconsole_initcall(efi_partition_init);
