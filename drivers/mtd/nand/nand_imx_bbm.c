/*
 * imx_nand_bbm.c - create a flash bad block table for i.MX NAND
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/err.h>

/*
 * The i.MX NAND controller has the problem that it handles the
 * data in chunks of 512 bytes. It doesn't treat 2k NAND chips as
 * 2048 byte data + 64 OOB, but instead:
 *
 * 512b data + 16b OOB +
 * 512b data + 16b OOB +
 * 512b data + 16b OOB +
 * 512b data + 16b OOB
 *
 * This means that the factory provided bad block marker ends up
 * in the page data at offset 2000 instead of in the OOB data.
 *
 * To preserve the factory bad block information we take the following
 * strategy:
 *
 * - If the NAND driver detects that no flasg BBT is present on 2k NAND
 *   chips it will not create one because it would do so based on the wrong
 *   BBM position
 * - This command is used to create a flash BBT then.
 *
 * From this point on we can forget about the BBMs and rely completely
 * on the flash BBT.
 *
 */
static int checkbad(struct mtd_info *mtd, loff_t ofs, void *__buf)
{
	int ret, retlen;
	uint8_t *buf = __buf;

	ret = mtd->read(mtd, ofs, mtd->writesize, &retlen, buf);
	if (ret < 0)
		return ret;

	if (buf[2000] != 0xff)
		return 1;

	return 0;
}

static void *create_bbt(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	int len, i, numblocks, ret;
	loff_t from = 0;
	void *buf;
	uint8_t *bbt;

	if ((chip->bbt_td && chip->bbt_td->pages[0] != -1) ||
			(chip->bbt_md && chip->bbt_md->pages[0] != -1)) {
		printf("Flash bbt already present\n");
		return ERR_PTR(-EEXIST);
	}

	len = mtd->size >> (chip->bbt_erase_shift + 2);

	/* Allocate memory (2bit per block) and clear the memory bad block table */
	bbt = kzalloc(len, GFP_KERNEL);
	if (!bbt)
		return ERR_PTR(-ENOMEM);

	buf = malloc(mtd->writesize);
	if (!buf) {
		ret = -ENOMEM;
		goto out2;
	}

	numblocks = mtd->size >> (chip->bbt_erase_shift - 1);

	for (i = 0; i < numblocks;) {
		ret = checkbad(mtd, from, buf);
		if (ret < 0)
			goto out1;

		if (ret) {
			bbt[i >> 3] |= 0x03 << (i & 0x6);
			printf("Bad eraseblock %d at 0x%08x\n", i >> 1,
					(unsigned int)from);
		}

		i += 2;
		from += (1 << chip->bbt_erase_shift);
	}

	return bbt;

out1:
	free(buf);
out2:
	free(bbt);

	return ERR_PTR(ret);
}

static int attach_bbt(struct mtd_info *mtd, void *bbt)
{
	struct nand_chip *chip = mtd->priv;

	chip->bbt_td->options |= NAND_BBT_WRITE | NAND_BBT_CREATE;
	chip->bbt_md->options |= NAND_BBT_WRITE | NAND_BBT_CREATE;
	free(chip->bbt);
	chip->bbt = bbt;

	return nand_update_bbt(mtd, 0);
}

static int do_imx_nand_bbm(int argc, char *argv[])
{
	int opt, ret;
	struct cdev *cdev;
	struct mtd_info *mtd;
	int yes = 0;
	void *bbt;

	while ((opt = getopt(argc, argv, "y")) > 0) {
		switch (opt) {
		case 'y':
			yes = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	cdev = cdev_open("nand0", O_RDWR);
	if (!cdev)
		return -ENOENT;

	mtd = cdev->mtd;
	if (!mtd)
		return -EINVAL;

	if (strcmp(mtd->name, "imx_nand")) {
		printf("This is not an i.MX nand but a %s\n", mtd->name);
		ret = -EINVAL;
		goto out;
	}

	switch (mtd->writesize) {
	case 512:
		printf("writesize is 512. This command is not needed\n");
		ret = 1;
		goto out;
	case 2048:
		break;
	default:
		printf("not implemented for writesize %d\n", mtd->writesize);
		ret = 1;
		goto out;
	}

	bbt = create_bbt(mtd);
	if (IS_ERR(bbt)) {
		ret = 1;
		goto out;
	}

	if (!yes) {
		int c;

		printf("create flash bbt (y/n)?");
		c = getc();
		if (c == 'y')
			yes = 1;
		printf("\n");
	}

	if (!yes) {
		free(bbt);
		ret = 1;

		goto out;
	}

	ret = attach_bbt(mtd, bbt);
	if (!ret)
		printf("bbt successfully added\n");
	else
		free(bbt);

out:
	cdev_close(cdev);

	return ret;
}

static const __maybe_unused char cmd_imx_nand_bbm_help[] =
"Usage: imx_nand_bbm\n";

BAREBOX_CMD_START(imx_nand_bbm)
	.cmd		= do_imx_nand_bbm,
	.usage		= "create bbt for i.MX NAND",
	BAREBOX_CMD_HELP(cmd_imx_nand_bbm_help)
BAREBOX_CMD_END
