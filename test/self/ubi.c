// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <bselftest.h>
#include <crc.h>
#include <driver.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/ubi.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <string.h>
#include <xfuncs.h>

/* Makefile pulls in drivers/mtd/ubi for the on-flash structures */
#include <ubi.h>

BSELFTEST_GLOBALS();

/*
 * A small-page NAND. UBI only looks for a fastmap on devices with more
 * than UBI_FM_MAX_START eraseblocks, so the fastmap tests use the large
 * one and the scanning tests the small one.
 */
#define PAGE_SIZE_TEST		512
#define PEB_SIZE		SZ_16K
#define PEB_COUNT_SCAN		32
#define PEB_COUNT_FASTMAP	72

/*
 * A static volume whose alignment leaves padding at the end of each LEB,
 * so a LEB holds less than leb_size bytes of its data.
 */
#define VOL_ID			0
#define VOL_ALIGNMENT		2048
#define VOL_LEBS		4

struct test_ubi {
	struct mtd_info mtd;
	u8 *img;
	u8 *pristine;
	size_t size;
	int peb_count;

	/* Geometry UBI chose when formatting */
	int vid_hdr_offset;
	int leb_start;
	int leb_size;
	int usable_leb_size;

	u8 *data;
	int data_len;
};

static int test_ubi_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
			     size_t *retlen, u_char *buf)
{
	struct test_ubi *tu = container_of(mtd, struct test_ubi, mtd);

	memcpy(buf, tu->img + from, len);
	*retlen = len;
	return 0;
}

static int test_ubi_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			      size_t *retlen, const u_char *buf)
{
	struct test_ubi *tu = container_of(mtd, struct test_ubi, mtd);

	memcpy(tu->img + to, buf, len);
	*retlen = len;
	return 0;
}

static int test_ubi_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct test_ubi *tu = container_of(mtd, struct test_ubi, mtd);

	memset(tu->img + instr->addr, 0xff, instr->len);
	return 0;
}

static int test_ubi_mtd_init(struct test_ubi *tu, int peb_count)
{
	struct mtd_info *mtd = &tu->mtd;
	int ret;

	memset(tu, 0, sizeof(*tu));

	tu->peb_count = peb_count;
	tu->size = peb_count * PEB_SIZE;
	tu->img = xmalloc(tu->size);
	tu->pristine = xmalloc(tu->size);
	memset(tu->img, 0xff, tu->size);

	mtd->name = "ubitest";
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->size = tu->size;
	mtd->erasesize = PEB_SIZE;
	mtd->writesize = PAGE_SIZE_TEST;
	mtd->writebufsize = PAGE_SIZE_TEST;
	mtd->_read = test_ubi_mtd_read;
	mtd->_write = test_ubi_mtd_write;
	mtd->_erase = test_ubi_mtd_erase;

	ret = add_mtd_device(mtd, "ubitest", DEVICE_ID_DYNAMIC);
	if (ret) {
		pr_err("Failed to add MTD device: %pe\n", ERR_PTR(ret));
		free(tu->img);
		free(tu->pristine);
	}

	return ret;
}

static void test_ubi_free(struct test_ubi *tu)
{
	del_mtd_device(&tu->mtd);
	free(tu->img);
	free(tu->pristine);
	free(tu->data);
}

/*
 * Format the device, create the static volume and fill it, all through
 * UBI itself. Detaching then writes the fastmap, if the device is large
 * enough to have one. The result is kept as the pristine image the tests
 * start from.
 */
static int test_ubi_build(struct test_ubi *tu, int peb_count)
{
	struct ubi_mkvol_req req = {
		.vol_id = VOL_ID,
		.alignment = VOL_ALIGNMENT,
		.vol_type = UBI_STATIC_VOLUME,
		.name = "test",
	};
	struct ubi_device *ubi;
	struct ubi_volume *vol;
	int ubi_num, ret, i;

	ret = test_ubi_mtd_init(tu, peb_count);
	if (ret)
		return ret;

	ubi_num = ubi_attach_mtd_dev(&tu->mtd, UBI_DEV_NUM_AUTO, 0, 0);
	if (ubi_num < 0) {
		pr_err("Failed to format: %pe\n", ERR_PTR(ubi_num));
		ret = ubi_num;
		goto err_free;
	}

	ubi = ubi_get_device(ubi_num);
	tu->vid_hdr_offset = ubi->vid_hdr_offset;
	tu->leb_start = ubi->leb_start;
	tu->leb_size = ubi->leb_size;
	ubi_put_device(ubi);

	req.name_len = strlen(req.name);
	req.bytes = (s64)VOL_LEBS *
		    (tu->leb_size - tu->leb_size % VOL_ALIGNMENT);
	ret = ubi_api_create_volume(ubi_num, &req);
	if (ret) {
		pr_err("Failed to create volume: %pe\n", ERR_PTR(ret));
		goto err_detach;
	}

	ubi = ubi_get_device(ubi_num);
	vol = ubi->volumes[VOL_ID];
	tu->usable_leb_size = vol->usable_leb_size;

	/* Two full LEBs and a short last one */
	tu->data_len = 2 * tu->usable_leb_size + 100;
	tu->data = xmalloc(tu->data_len);
	for (i = 0; i < tu->data_len; i++)
		tu->data[i] = i * 7 + i / 251;

	ret = ubi_start_update(ubi, vol, tu->data_len);
	if (!ret)
		ret = ubi_more_update_data(ubi, vol, tu->data, tu->data_len);
	if (!ret)
		ret = ubi_finish_update(ubi, vol);
	ubi_put_device(ubi);

	if (ret) {
		pr_err("Failed to write volume: %pe\n", ERR_PTR(ret));
		goto err_detach;
	}

	ret = ubi_detach(ubi_num);
	if (ret) {
		pr_err("Failed to detach: %pe\n", ERR_PTR(ret));
		goto err_free;
	}

	memcpy(tu->pristine, tu->img, tu->size);
	return 0;

err_detach:
	ubi_detach(ubi_num);
err_free:
	test_ubi_free(tu);
	return ret;
}

static void test_ubi_restore(struct test_ubi *tu)
{
	memcpy(tu->img, tu->pristine, tu->size);
}

static struct ubi_vid_hdr *test_ubi_vid_hdr(struct test_ubi *tu, int pnum)
{
	return (void *)tu->img + pnum * PEB_SIZE + tu->vid_hdr_offset;
}

static void *test_ubi_leb_data(struct test_ubi *tu, int pnum)
{
	return tu->img + pnum * PEB_SIZE + tu->leb_start;
}

/* Find the newest PEB holding LEB @lnum of volume @vol_id */
static int test_ubi_find_peb(struct test_ubi *tu, int vol_id, int lnum)
{
	unsigned long long sqnum = 0;
	int pnum, found = -1;

	for (pnum = 0; pnum < tu->peb_count; pnum++) {
		struct ubi_vid_hdr *vidh = test_ubi_vid_hdr(tu, pnum);

		if (be32_to_cpu(vidh->magic) != UBI_VID_HDR_MAGIC ||
		    be32_to_cpu(vidh->vol_id) != vol_id ||
		    be32_to_cpu(vidh->lnum) != lnum)
			continue;

		if (found < 0 || be64_to_cpu(vidh->sqnum) > sqnum) {
			found = pnum;
			sqnum = be64_to_cpu(vidh->sqnum);
		}
	}

	return found;
}

/*
 * Attach the device and check the volume holds what was written. Returns
 * the UBI device number or a negative error code if attaching failed.
 */
static int test_ubi_attach(struct test_ubi *tu, bool *used_fastmap)
{
	struct ubi_volume_desc *desc;
	struct ubi_device *ubi;
	char *buf;
	int ubi_num, lnum, pos, len;

	*used_fastmap = false;

	ubi_num = ubi_attach_mtd_dev(&tu->mtd, UBI_DEV_NUM_AUTO, 0, 0);
	if (ubi_num < 0)
		return ubi_num;

	ubi = ubi_get_device(ubi_num);
	*used_fastmap = ubi->fm;
	ubi_put_device(ubi);

	desc = ubi_open_volume(ubi_num, VOL_ID, UBI_READONLY);
	if (!assert_cond(!IS_ERR(desc)))
		return ubi_num;

	buf = xmalloc(tu->usable_leb_size);

	for (lnum = 0, pos = 0; pos < tu->data_len; lnum++, pos += len) {
		len = min(tu->data_len - pos, tu->usable_leb_size);

		if (!assert_cond(!ubi_leb_read(desc, lnum, buf, 0, len, 1)))
			break;
		if (!assert_cond(!memcmp(buf, tu->data + pos, len)))
			break;
	}

	free(buf);
	ubi_close_volume(desc);

	return ubi_num;
}

static void test_ubi_detach(int ubi_num)
{
	if (ubi_num >= 0)
		ubi_detach(ubi_num);
}

/* The newest fastmap, after checking it fits into a single LEB */
static struct ubi_fm_sb *test_ubi_fastmap(struct test_ubi *tu)
{
	struct ubi_fm_sb *fmsb;
	int pnum;

	pnum = test_ubi_find_peb(tu, UBI_FM_SB_VOLUME_ID, 0);
	if (!assert_cond(pnum >= 0))
		return NULL;

	fmsb = test_ubi_leb_data(tu, pnum);
	if (!assert_cond(be32_to_cpu(fmsb->used_blocks) == 1))
		return NULL;

	return fmsb;
}

static void test_ubi_fastmap_update_crc(struct test_ubi *tu,
					struct ubi_fm_sb *fmsb)
{
	fmsb->data_crc = 0;
	fmsb->data_crc = cpu_to_be32(crc32(UBI_CRC32_INIT, fmsb,
					   tu->leb_size));
}

static struct ubi_fm_hdr *test_ubi_fm_hdr(struct ubi_fm_sb *fmsb)
{
	return (void *)(fmsb + 1);
}

/* The free list follows the header and both pools */
static struct ubi_fm_ec *test_ubi_fm_free(struct ubi_fm_sb *fmsb)
{
	return (void *)(test_ubi_fm_hdr(fmsb) + 1) +
		2 * sizeof(struct ubi_fm_scan_pool);
}

/* The volume header of @vol_id, which follows the four PEB lists */
static struct ubi_fm_volhdr *test_ubi_fm_volhdr(struct ubi_fm_sb *fmsb,
						int vol_id)
{
	struct ubi_fm_hdr *fmhdr = test_ubi_fm_hdr(fmsb);
	void *pos = test_ubi_fm_free(fmsb);
	int i;

	pos += sizeof(struct ubi_fm_ec) *
		(be32_to_cpu(fmhdr->free_peb_count) +
		 be32_to_cpu(fmhdr->used_peb_count) +
		 be32_to_cpu(fmhdr->scrub_peb_count) +
		 be32_to_cpu(fmhdr->erase_peb_count));

	for (i = 0; i < be32_to_cpu(fmhdr->vol_count); i++) {
		struct ubi_fm_volhdr *fmvhdr = pos;
		struct ubi_fm_eba *fm_eba = (void *)(fmvhdr + 1);

		if (be32_to_cpu(fmvhdr->vol_id) == vol_id)
			return fmvhdr;

		pos = &fm_eba->pnum[be32_to_cpu(fm_eba->reserved_pebs)];
	}

	return NULL;
}

/*
 * The VID header of a static volume's last LEB says how much data it
 * holds. Up to the usable LEB size is fine, more must be rejected even
 * though the padding would leave room for it on flash.
 */
static void test_ubi_static_last_data_size(void)
{
	struct ubi_vid_hdr *vidh;
	struct test_ubi tu;
	int data_sizes[2];
	int ubi_num, pnum, lnum, data_size, i;
	bool valid;

	if (!assert_cond(!test_ubi_build(&tu, PEB_COUNT_SCAN)))
		return;

	/* The rejected case needs room between the two sizes */
	if (!assert_cond(tu.usable_leb_size < tu.leb_size))
		goto out;

	lnum = tu.data_len / tu.usable_leb_size;
	data_sizes[0] = tu.usable_leb_size;
	data_sizes[1] = tu.leb_size;

	for (i = 0; i < ARRAY_SIZE(data_sizes); i++) {
		data_size = data_sizes[i];
		test_ubi_restore(&tu);

		pnum = test_ubi_find_peb(&tu, VOL_ID, lnum);
		if (!assert_cond(pnum >= 0))
			break;

		vidh = test_ubi_vid_hdr(&tu, pnum);
		if (!assert_cond(be32_to_cpu(vidh->used_ebs) == lnum + 1))
			break;

		vidh->data_size = cpu_to_be32(data_size);
		vidh->data_crc = cpu_to_be32(crc32(UBI_CRC32_INIT,
						   test_ubi_leb_data(&tu, pnum),
						   data_size));
		vidh->hdr_crc = cpu_to_be32(crc32(UBI_CRC32_INIT, vidh,
						  UBI_VID_HDR_SIZE_CRC));

		/* The data written ends early, so only check the attach */
		ubi_num = ubi_attach_mtd_dev(&tu.mtd, UBI_DEV_NUM_AUTO, 0, 0);
		valid = data_size <= tu.usable_leb_size;

		if (!assert_cond(valid == (ubi_num >= 0)))
			pr_err("last LEB with %d of %d bytes %s\n", data_size,
			       tu.usable_leb_size,
			       valid ? "rejected" : "accepted");

		test_ubi_detach(ubi_num);
	}
out:
	test_ubi_free(&tu);
}
bselftest(core, test_ubi_static_last_data_size);

/* The same bound for the size of the last LEB a fastmap records */
static void test_ubi_fastmap_last_eb_bytes(void)
{
	struct ubi_fm_volhdr *fmvhdr;
	struct ubi_fm_sb *fmsb;
	struct test_ubi tu;
	int last_eb_bytes[2];
	int ubi_num, i;

	if (!IS_ENABLED(CONFIG_MTD_UBI_FASTMAP)) {
		skipped_tests++;
		return;
	}

	if (!assert_cond(!test_ubi_build(&tu, PEB_COUNT_FASTMAP)))
		return;

	last_eb_bytes[0] = tu.usable_leb_size + 1;
	last_eb_bytes[1] = -1;

	for (i = 0; i < ARRAY_SIZE(last_eb_bytes); i++) {
		test_ubi_restore(&tu);

		fmsb = test_ubi_fastmap(&tu);
		if (!fmsb)
			break;

		fmvhdr = test_ubi_fm_volhdr(fmsb, VOL_ID);
		if (!assert_cond(fmvhdr))
			break;

		fmvhdr->last_eb_bytes = cpu_to_be32(last_eb_bytes[i]);
		test_ubi_fastmap_update_crc(&tu, fmsb);

		ubi_num = ubi_attach_mtd_dev(&tu.mtd, UBI_DEV_NUM_AUTO, 0, 0);
		if (!assert_cond(ubi_num < 0))
			pr_err("fastmap with last LEB of %d bytes accepted\n",
			       last_eb_bytes[i]);

		test_ubi_detach(ubi_num);
	}

	test_ubi_free(&tu);
}
bselftest(core, test_ubi_fastmap_last_eb_bytes);

/* The first PEB of the first pool */
static __be32 *test_ubi_fm_pool_peb(struct ubi_fm_sb *fmsb)
{
	struct ubi_fm_scan_pool *fmpl = (void *)(test_ubi_fm_hdr(fmsb) + 1);

	if (!assert_cond(be16_to_cpu(fmpl->size) > 0))
		return NULL;

	return &fmpl->pebs[0];
}

/* The first PEB of the free list */
static __be32 *test_ubi_fm_free_peb(struct ubi_fm_sb *fmsb)
{
	struct ubi_fm_hdr *fmhdr = test_ubi_fm_hdr(fmsb);

	if (!assert_cond(be32_to_cpu(fmhdr->free_peb_count) > 0))
		return NULL;

	return &test_ubi_fm_free(fmsb)->pnum;
}

/*
 * A fastmap's PEB numbers are used as indices into UBI's tables. One
 * outside the device must make UBI reject the fastmap and fall back to
 * scanning, which still finds the volume intact.
 */
static void test_ubi_fastmap_pnum(void)
{
	static const struct {
		const char *name;
		__be32 *(*peb)(struct ubi_fm_sb *fmsb);
	} lists[] = {
		{ "pool", test_ubi_fm_pool_peb },
		{ "free", test_ubi_fm_free_peb },
	};
	static const int pnums[] = { PEB_COUNT_FASTMAP, -1 };
	struct ubi_fm_sb *fmsb;
	struct test_ubi tu;
	bool used_fastmap;
	__be32 *peb;
	int ubi_num, i, j;

	if (!IS_ENABLED(CONFIG_MTD_UBI_FASTMAP)) {
		skipped_tests++;
		return;
	}

	if (!assert_cond(!test_ubi_build(&tu, PEB_COUNT_FASTMAP)))
		return;

	/* The unmodified fastmap is used */
	ubi_num = test_ubi_attach(&tu, &used_fastmap);
	assert_cond(ubi_num >= 0);
	assert_cond(used_fastmap);
	test_ubi_detach(ubi_num);

	for (i = 0; i < ARRAY_SIZE(lists); i++) {
		for (j = 0; j < ARRAY_SIZE(pnums); j++) {
			test_ubi_restore(&tu);

			fmsb = test_ubi_fastmap(&tu);
			if (!fmsb)
				goto out;

			peb = lists[i].peb(fmsb);
			if (!peb)
				goto out;

			*peb = cpu_to_be32(pnums[j]);
			test_ubi_fastmap_update_crc(&tu, fmsb);

			ubi_num = test_ubi_attach(&tu, &used_fastmap);
			if (!assert_cond(ubi_num >= 0) ||
			    !assert_cond(!used_fastmap))
				pr_err("fastmap with %s PEB %d: %s\n",
				       lists[i].name, pnums[j],
				       ubi_num < 0 ? "attach failed" : "used");

			test_ubi_detach(ubi_num);
		}
	}
out:
	test_ubi_free(&tu);
}
bselftest(core, test_ubi_fastmap_pnum);
