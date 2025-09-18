// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <block.h>
#include <bselftest.h>
#include <device-mapper.h>
#include <dirent.h>
#include <disks.h>
#include <driver.h>
#include <fcntl.h>
#include <fs.h>
#include <libfile.h>
#include <linux/sizes.h>
#include <ramdisk.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xfuncs.h>

BSELFTEST_GLOBALS();

struct rdctx {
	char mem[16][SECTOR_SIZE];
	struct ramdisk *rd;
	const char *name;
};

static struct rdctx rdctx[2];

static int rd_create(void)
{
	struct block_device *blk;
	struct rdctx *ctx;
	char base;
	int i, s;


	for (i = 0, ctx = rdctx; i < 2; i++, ctx++) {
		/* In case tests are run multiple times */
		memset(ctx->mem, '\0', sizeof(ctx->mem));

		/* Add an identifying mark ('a'-'p' and 'A'-'P') at
		 * the start and end of every sector in both disks, so
		 * that we have something to compare against when we
		 * read them back through the DM device.
		 */
		base = i ? 'A' : 'a';
		for (s = 0; s < 16; s++) {
			ctx->mem[s][0] = base + s;
			ctx->mem[s][SECTOR_SIZE - 1] = base + s;
		}

		ctx->rd = ramdisk_init(SECTOR_SIZE);
		if (!ctx->rd) {
			failed_tests++;
			pr_err("Could not create ramdisk\n");
			return 1;
		}

		ramdisk_setup_rw(ctx->rd, ctx->mem, sizeof(ctx->mem));
		blk = ramdisk_get_block_device(ctx->rd);
		ctx->name = cdev_name(&blk->cdev);
	}

	return 0;
}

static void rd_destroy(void)
{
	ramdisk_free(rdctx[0].rd);
	ramdisk_free(rdctx[1].rd);
}

static void verify_read(const char *pattern, const char *buf)
{
	off_t first, last;
	int s, len;

	for (s = 0, len = strlen(pattern); s < len; s++) {
		first = s << SECTOR_SHIFT;
		last = first + SECTOR_SIZE - 1;

		if (buf[first] != pattern[s]) {
			failed_tests++;
			pr_err("Expected '%c' at beginning of sector %d, read '%c'\n",
			       pattern[s], s, buf[first]);
			return;
		}

		if (buf[last] != pattern[s]) {
			failed_tests++;
			pr_err("Expected '%c' at end of sector %d, read '%c'\n",
			       pattern[s], s, buf[last]);
			return;
		}
	}
}

static void test_dm_linear(void)
{
	static const char pattern[] = "DEFaghijklmnopNOP";
	const size_t dmsize = (sizeof(pattern) - 1) * SECTOR_SIZE;
	struct dm_device *dm;
	struct cdev *cdev;
	char *buf, *table;

	total_tests++;

	if (!IS_ENABLED(CONFIG_DM_BLK_LINEAR)) {
		pr_info("skipping dm-linear test: disabled in config\n");
		skipped_tests++;
		return;
	}

	if (rd_create())
		return;

	table = xasprintf(" 0  3 linear /dev/%s  3\n" /* "DEF" */
			  " 3  1 linear /dev/%s  0\n" /* "a"   */
			  " 4 10 linear /dev/%s  6\n" /* "ghijklmnop" */
			  "14  3 linear /dev/%s 13\n" /* "NOP" */,
			  rdctx[1].name,
			  rdctx[0].name,
			  rdctx[0].name,
			  rdctx[1].name);

	dm = dm_create("dmtest", table);
	free(table);

	if (IS_ERR_OR_NULL(dm)) {
		failed_tests++;
		pr_err("Could not create dm device\n");
		goto out_destroy;
	}

	cdev = cdev_by_name("dmtest");
	if (!cdev) {
		failed_tests++;
		pr_err("Could not find dm device\n");
		goto out_destroy;
	}

	buf = xmalloc(dmsize);

	if (cdev_read(cdev, buf, dmsize, 0, 0) < dmsize) {
		failed_tests++;
		pr_err("Could not read dm device\n");
		goto out_free_buf;
	}

	verify_read(pattern, buf);

out_free_buf:
	free(buf);
out_destroy:
	dm_destroy(dm);
	rd_destroy();
}
bselftest(core, test_dm_linear);
