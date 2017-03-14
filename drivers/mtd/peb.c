/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <malloc.h>
#include <stdlib.h>
#include <init.h>
#include <magicvar.h>
#include <globalvar.h>
#include <linux/mtd/mtd.h>
#include <mtd/mtd-peb.h>
#include <linux/err.h>

#define MTD_IO_RETRIES 3

static int __mtd_peb_chk_io;

static int mtd_peb_chk_io(void)
{
	if (!IS_ENABLED(CONFIG_MTD_PEB_DEBUG))
		return 0;

	if (!__mtd_peb_chk_io)
		return 0;

	return 1;
}

static u32 __mtd_peb_emulate_bitflip;

static int mtd_peb_emulate_bitflip(void)
{
	if (!IS_ENABLED(CONFIG_MTD_PEB_DEBUG))
		return 0;

	if (!__mtd_peb_emulate_bitflip)
		return 0;

	return !(random32() % __mtd_peb_emulate_bitflip);
}

static u32 __mtd_peb_emulate_write_failure;

static int mtd_peb_emulate_write_failure(void)
{
	if (!IS_ENABLED(CONFIG_MTD_PEB_DEBUG))
		return 0;

	if (!__mtd_peb_emulate_write_failure)
		return 0;

	return !(random32() % __mtd_peb_emulate_write_failure);
}

static u32 __mtd_peb_emulate_erase_failures;

static int mtd_peb_emulate_erase_failure(void)
{
	if (!IS_ENABLED(CONFIG_MTD_PEB_DEBUG))
		return 0;

	if (!__mtd_peb_emulate_erase_failures)
		return 0;

	return !(random32() % __mtd_peb_emulate_erase_failures);
}

#ifdef CONFIG_MTD_PEB_DEBUG
static int mtd_peb_debug_init(void)
{
	globalvar_add_simple_int("mtd_peb.mtd_peb_emulate_bitflip",
				 &__mtd_peb_emulate_bitflip, "%u");
	globalvar_add_simple_int("mtd_peb.mtd_peb_emulate_write_failure",
				 &__mtd_peb_emulate_write_failure, "%u");
	globalvar_add_simple_int("mtd_peb.mtd_peb_emulate_erase_failures",
				 &__mtd_peb_emulate_erase_failures, "%u");
	globalvar_add_simple_bool("mtd_peb.mtd_peb_chk_io",
				 &__mtd_peb_chk_io);
	return 0;
}
device_initcall(mtd_peb_debug_init);

BAREBOX_MAGICVAR_NAMED(global_mtd_peb_emulate_bitflip,
		       global.mtd_peb.emulate_bitflip,
		       "random bitflips, on average every #nth access returns -EUCLEAN");
BAREBOX_MAGICVAR_NAMED(global_mtd_peb_emulate_write_failure,
		       global.mtd_peb.emulate_write_failure,
		       "random write failures, on average every #nth access returns write failure");
BAREBOX_MAGICVAR_NAMED(global_mtd_peb_emulate_erase_failures,
		       global.mtd_peb.emulate_erase_failures,
		       "random erase failures, on average every #nth access returns erase failure");
BAREBOX_MAGICVAR_NAMED(global_mtd_peb_chk_io,
		       global.mtd_peb.chk_io,
		       "If true, written data will be verified");

#endif

static int mtd_peb_valid(struct mtd_info *mtd, int pnum)
{
	if (pnum < 0)
		return false;

	if ((uint64_t)pnum * mtd->erasesize >= mtd->size)
		return false;

	if (mtd->numeraseregions)
		return false;

	return true;
}

/**
 * mtd_num_pebs - return number of PEBs for this device
 * @mtd: mtd device
 *
 * This function returns the number of physical erase blocks this device
 * has.
 */
int mtd_num_pebs(struct mtd_info *mtd)
{
	return mtd_div_by_eb(mtd->size, mtd);
}

/**
 * mtd_peb_mark_bad - mark a physical eraseblock as bad
 * @mtd: mtd device
 * @pnum: The number of the block
 *
 * This function marks a physical eraseblock as bad.
 */
int mtd_peb_mark_bad(struct mtd_info *mtd, int pnum)
{
	return mtd_block_markbad(mtd, (loff_t)pnum * mtd->erasesize);
}

/**
 * mtd_peb_is_bad - test if a physical eraseblock is bad
 * @mtd: mtd device
 * @pnum: The number of the block
 *
 * This function tests if a physical eraseblock is bad. Returns
 * 0 if it is good, 1 if it is bad or a negative error value if the
 * block is invalid
 */
int mtd_peb_is_bad(struct mtd_info *mtd, int pnum)
{
	if (!mtd_peb_valid(mtd, pnum))
		return -EINVAL;

	return mtd_block_isbad(mtd, (loff_t)pnum * mtd->erasesize);
}

/**
 * mtd_peb_read - read data from a physical eraseblock.
 * @mtd: mtd device
 * @buf: buffer where to store the read data
 * @pnum: physical eraseblock number to read from
 * @offset: offset within the physical eraseblock from where to read
 * @len: how many bytes to read
 *
 * This function reads data from offset @offset of physical eraseblock @pnum
 * and stores the read data in the @buf buffer. The following return codes are
 * possible:
 *
 * o %0 if all the requested data were successfully read;
 * o %-EUCLEAN if all the requested data were successfully read, but
 *   correctable bit-flips were detected; this is harmless but may indicate
 *   that this eraseblock may become bad soon (but do not have to);
 * o %-EBADMSG if the MTD subsystem reported about data integrity problems, for
 *   example it can be an ECC error in case of NAND; this most probably means
 *   that the data is corrupted;
 * o %-EIO if some I/O error occurred;
 * o other negative error codes in case of other errors.
 */
int mtd_peb_read(struct mtd_info *mtd, void *buf, int pnum, int offset,
		 int len)
{
	int err, retries = 0;
	size_t read;
	loff_t addr;

	if (!mtd_peb_valid(mtd, pnum))
		return -EINVAL;
	if (offset < 0 || offset + len > mtd->erasesize)
		return -EINVAL;
	if (len <= 0)
		return -EINVAL;
	if (mtd_peb_is_bad(mtd, pnum))
		return -EINVAL;

	/* Deliberately corrupt the buffer */
	*((uint8_t *)buf) ^= 0xFF;

	addr = (loff_t)pnum * mtd->erasesize + offset;
retry:
	err = mtd_read(mtd, addr, len, &read, buf);
	if (err) {
		const char *errstr = mtd_is_eccerr(err) ? " (ECC error)" : "";

		if (mtd_is_bitflip(err)) {
			/*
			 * -EUCLEAN is reported if there was a bit-flip which
			 * was corrected, so this is harmless.
			 *
			 * We do not report about it here unless debugging is
			 * enabled. A corresponding message will be printed
			 * later, when it is has been scrubbed.
			 */
			dev_dbg(&mtd->class_dev, "fixable bit-flip detected at PEB %d\n", pnum);
			if (len != read)
				return -EIO;
			return -EUCLEAN;
		}

		if (mtd_is_eccerr(err) && retries++ < MTD_IO_RETRIES)
			goto retry;

		dev_err(&mtd->class_dev, "error %d%s while reading %d bytes from PEB %d:%d\n",
			err, errstr, len, pnum, offset);
		return err;
	}

	if (len != read)
		return -EIO;

	if (mtd_peb_emulate_bitflip())
		return -EUCLEAN;

	return 0;
}

/**
 * mtd_peb_check_all_ff - check that a region of flash is empty.
 * @mtd: mtd device
 * @pnum: the physical eraseblock number to check
 * @offset: the starting offset within the physical eraseblock to check
 * @len: the length of the region to check
 *
 * This function returns zero if only 0xFF bytes are present at offset
 * @offset of the physical eraseblock @pnum, -EBADMSG if the buffer does
 * not contain all 0xFF or other negative error codes when other errors
 * occured
 */
int mtd_peb_check_all_ff(struct mtd_info *mtd, int pnum, int offset, int len,
			 int warn)
{
	int err;
	void *buf;

	buf = malloc(len);
	if (!buf)
		return -ENOMEM;

	err = mtd_peb_read(mtd, buf, pnum, offset, len);
	if (err && !mtd_is_bitflip(err)) {
		dev_err(&mtd->class_dev,
			"error %d while reading %d bytes from PEB %d:%d\n",
			err, len, pnum, offset);
		goto out;
	}

	err = mtd_buf_all_ff(buf, len);
	if (err == 0) {
		if (warn)
			dev_err(&mtd->class_dev, "all-ff check failed for PEB %d\n",
				pnum);
		err = -EBADMSG;
		goto out;
	}

	err = 0;

out:
	free(buf);

	return err;
}

/**
 * mtd_peb_verify - make sure write succeeded.
 * @mtd: mtd device
 * @buf: buffer with data which were written
 * @pnum: physical eraseblock number the data were written to
 * @offset: offset within the physical eraseblock the data were written to
 * @len: how many bytes were written
 *
 * This functions reads data which were recently written and compares it with
 * the original data buffer - the data have to match. Returns zero if the data
 * match and a negative error code if not or in case of failure.
 */
int mtd_peb_verify(struct mtd_info *mtd, const void *buf, int pnum,
				int offset, int len)
{
	int err, i;
	size_t read;
	void *buf1;
	loff_t addr = (loff_t)pnum * mtd->erasesize + offset;

	buf1 = malloc(len);
	if (!buf1)
		return 0;

	err = mtd_read(mtd, addr, len, &read, buf1);
	if (err && !mtd_is_bitflip(err))
		goto out_free;

	for (i = 0; i < len; i++) {
		uint8_t c = ((uint8_t *)buf)[i];
		uint8_t c1 = ((uint8_t *)buf1)[i];
		int dump_len;

		if (c == c1)
			continue;

		dev_err(&mtd->class_dev, "self-check failed for PEB %d:%d, len %d\n",
			pnum, offset, len);
		dev_info(&mtd->class_dev, "data differs at position %d\n", i);
		dump_len = max_t(int, 128, len - i);
#ifdef DEBUG
		dev_info(&mtd->class_dev, "hex dump of the original buffer from %d to %d\n",
			i, i + dump_len);
		memory_display(buf + i, i, dump_len, 4, 0);
		dev_info(&mtd->class_dev, "hex dump of the read buffer from %d to %d\n",
			i, i + dump_len);
		memory_display(buf1 + i, i, dump_len, 4, 0);
		dump_stack();
#endif
		err = -EBADMSG;
		goto out_free;
	}

	err = 0;

out_free:
	free(buf1);
	return err;
}

/**
 * mtd_peb_write - write data to a physical eraseblock.
 * @mtd: mtd device
 * @buf: buffer with the data to write
 * @pnum: physical eraseblock number to write to
 * @offset: offset within the physical eraseblock where to write
 * @len: how many bytes to write
 *
 * This function writes @len bytes of data from buffer @buf to offset @offset
 * of physical eraseblock @pnum. If all the data was successfully written,
 * zero is returned. If an error occurred, this function returns a negative
 * error code. If %-EBADMSG is returned, the physical eraseblock most probably
 * went bad.
 *
 * Note, in case of an error, it is possible that something was still written
 * to the flash media, but may be some garbage.
 */
int mtd_peb_write(struct mtd_info *mtd, const void *buf, int pnum, int offset,
		  int len)
{
	int err;
	size_t written;
	loff_t addr;

	dev_dbg(&mtd->class_dev, "write %d bytes to PEB %d:%d\n", len, pnum, offset);

	if (!mtd_peb_valid(mtd, pnum))
		return -EINVAL;
	if (offset < 0 || offset + len > mtd->erasesize)
		return -EINVAL;
	if (len <= 0)
		return -EINVAL;
	if (len % (mtd->writesize >> mtd->subpage_sft))
		return -EINVAL;
	if (mtd_peb_is_bad(mtd, pnum))
		return -EINVAL;

	if (mtd_peb_emulate_write_failure()) {
		dev_err(&mtd->class_dev, "Cannot write %d bytes to PEB %d:%d (emulated)\n",
			len, pnum, offset);
		return -EIO;
	}

	if (mtd_peb_chk_io()) {
		/* The area we are writing to has to contain all 0xFF bytes */
		err = mtd_peb_check_all_ff(mtd, pnum, offset, len, 1);
		if (err)
			return err;
	}

	addr = (loff_t)pnum * mtd->erasesize + offset;
	err = mtd_write(mtd, addr, len, &written, buf);
	if (err) {
		dev_err(&mtd->class_dev, "error %d while writing %d bytes to PEB %d:%d, written %zu bytes\n",
			err, len, pnum, offset, written);
	} else {
		if (written != len)
			err = -EIO;
	}

	if (!err && mtd_peb_chk_io())
		err = mtd_peb_verify(mtd, buf, pnum, offset, len);

	return err;
}

/**
 * mtd_peb_erase - erase a physical eraseblock.
 * @mtd: mtd device
 * @pnum: physical eraseblock number to erase
 *
 * This function erases physical eraseblock @pnum.
 *
 * This function returns 0 in case of success, %-EIO if the erasure failed,
 * and other negative error codes in case of other errors. Note, %-EIO means
 * that the physical eraseblock is bad.
 */
int mtd_peb_erase(struct mtd_info *mtd, int pnum)
{
	int ret;
	struct erase_info ei = {};

	dev_dbg(&mtd->class_dev, "erase PEB %d\n", pnum);

	if (!mtd_peb_valid(mtd, pnum))
		return -EINVAL;

	ei.mtd = mtd;
	ei.addr = (loff_t)pnum * mtd->erasesize;
	ei.len = mtd->erasesize;

	ret = mtd_erase(mtd, &ei);
	if (ret)
		return ret;

	if (mtd_peb_chk_io()) {
		ret = mtd_peb_check_all_ff(mtd, pnum, 0, mtd->erasesize, 1);
		if (ret == -EBADMSG)
			ret = -EIO;
	}

	if (mtd_peb_emulate_erase_failure()) {
		dev_err(&mtd->class_dev, "cannot erase PEB %d (emulated)", pnum);
		return -EIO;
	}

	return 0;
}

/* Patterns to write to a physical eraseblock when torturing it */
static uint8_t patterns[] = {0xa5, 0x5a, 0x0};

/**
 * mtd_peb_torture - test a supposedly bad physical eraseblock.
 * @mtd: mtd device
 * @pnum: the physical eraseblock number to test
 *
 * This function is a last resort when an eraseblock is assumed to be
 * bad. It will write and check some patterns to the block. If the test
 * is passed then this function will with the block freshly erased and
 * the positive number returned indicaties how often the block has been
 * erased during this test.
 * If the block does not pass the test the block is marked as bad and
 * -EIO is returned.
 *  Other negative errors are returned in case of other errors.
 */
int mtd_peb_torture(struct mtd_info *mtd, int pnum)
{
	int err, i, patt_count;
	void *peb_buf;

	if (!mtd_peb_valid(mtd, pnum))
		return -EINVAL;

	peb_buf = malloc(mtd->erasesize);
	if (!peb_buf)
		return -ENOMEM;

	dev_dbg(&mtd->class_dev, "run torture test for PEB %d\n", pnum);

	patt_count = ARRAY_SIZE(patterns);

	for (i = 0; i < patt_count; i++) {
		err = mtd_peb_erase(mtd, pnum);
		if (err)
			goto out;

		/* Make sure the PEB contains only 0xFF bytes after erasing */
		err = mtd_peb_check_all_ff(mtd, pnum, 0, mtd->writesize, 0);
		if (err)
			goto out;

		/* Write a pattern and check it */
		memset(peb_buf, patterns[i], mtd->erasesize);
		err = mtd_peb_write(mtd, peb_buf, pnum, 0, mtd->erasesize);
		if (err)
			goto out;

		err = mtd_peb_verify(mtd, peb_buf, pnum, 0, mtd->erasesize);
		if (err)
			goto out;
	}

	err = mtd_peb_erase(mtd, pnum);
	if (err)
		goto out;

	err = patt_count + 1;
	dev_dbg(&mtd->class_dev, "PEB %d passed torture test, do not mark it as bad\n",
		pnum);

out:
	if (err == -EUCLEAN || mtd_is_eccerr(err)) {
		/*
		 * If a bit-flip or data integrity error was detected, the test
		 * has not passed because it happened on a freshly erased
		 * physical eraseblock which means something is wrong with it.
		 */
		dev_err(&mtd->class_dev, "read problems on freshly erased PEB %d, marking it bad\n",
			pnum);

		mtd_peb_mark_bad(mtd, pnum);

		err = -EIO;
	}

	free(peb_buf);

	return err;
}

/**
 * mtd_peb_create_bitflips - create bitflips on Nand pages
 * @mtd: mtd device
 * @pnum: Physical erase block number
 * @offset: offset within erase block
 * @len: The length of the area to create bitflips in
 * @num_bitflips: The number of bitflips to create
 * @random: If true, create bitflips at random offsets
 * @info: If true, print information where bitflips are created
 *
 * This uses the mtd raw ops to create bitflips on a Nand page for
 * testing purposes. If %random is false then the positions to flip are
 * reproducible (thus, a second call with the same arguments reverts the
 * bitflips).
 *
 * Return: 0 for success, otherwise a negative error code is returned
 */
int mtd_peb_create_bitflips(struct mtd_info *mtd, int pnum, int offset,
				   int len, int num_bitflips, int random,
				   int info)
{
	struct mtd_oob_ops ops;
	int pages_per_block = mtd->erasesize / mtd->writesize;
	int i;
	int ret;
	void *buf = NULL, *oobbuf = NULL;
	int step;

	if (offset < 0 || offset + len > mtd->erasesize)
		return -EINVAL;
	if (offset % mtd->writesize)
		return -EINVAL;
	if (len <= 0)
		return -EINVAL;
	if (num_bitflips <= 0)
		return -EINVAL;
	if (mtd_peb_is_bad(mtd, pnum))
		return -EINVAL;

	buf = malloc(mtd->writesize * pages_per_block);
	if (!buf) {
		ret = -ENOMEM;
		goto err;
	}

	oobbuf = malloc(mtd->oobsize * pages_per_block);
	if (!oobbuf) {
		ret = -ENOMEM;
		goto err;
	}

	ops.mode = MTD_OPS_RAW;
	ops.ooboffs = 0;
	ops.len = mtd->writesize;
	ops.ooblen = mtd->oobsize;

	for (i = 0; i < pages_per_block; i++) {
		loff_t offs = (loff_t)pnum * mtd->erasesize + i * mtd->writesize;

		ops.datbuf = buf + i * mtd->writesize;
		ops.oobbuf = oobbuf + i * mtd->oobsize;

		ret = mtd_read_oob(mtd, offs, &ops);
		if (ret) {
			dev_err(&mtd->class_dev, "Cannot read raw data at 0x%08llx\n", offs);
			goto err;
		}
	}

	if (random)
		step = random32() % num_bitflips;
	else
		step = len / num_bitflips;

	for (i = 0; i < num_bitflips; i++) {
		int offs;
		int bit;
		u8 *pos = buf + offset;

		if (random) {
			offs = random32() % len;
			bit = random32() % 8;
		} else {
			offs = i * len / num_bitflips;
			bit = i % 8;
		}

		pos[offs] ^= 1 << bit;

		if (info)
			dev_info(&mtd->class_dev, "Flipping bit %d @ %d\n", bit, offs);
	}

	ret = mtd_peb_erase(mtd, pnum);
	if (ret < 0) {
		dev_err(&mtd->class_dev, "Cannot erase PEB %d\n", pnum);
		goto err;
	}

	for (i = 0; i < pages_per_block; i++) {
		loff_t offs = (loff_t)pnum * mtd->erasesize + i * mtd->writesize;

		ops.datbuf = buf + i * mtd->writesize;
		ops.oobbuf = oobbuf + i * mtd->oobsize;

		ret = mtd_write_oob(mtd, offs, &ops);
		if (ret) {
			dev_err(&mtd->class_dev, "Cannot write page at 0x%08llx\n", offs);
			goto err;
		}
	}

	ret = 0;
err:
	if (ret)
		dev_err(&mtd->class_dev, "Failed to create bitflips: %s\n", strerror(-ret));

	free(buf);
	free(oobbuf);

	return ret;
}
