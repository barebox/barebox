/*
 * (C) Copyright 2005
 * 2N Telekomunikace, a.s. <www.2n.cz>
 * Ladislav Michl <michl@2n.cz>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <init.h>
#include <xfuncs.h>
#include <driver.h>
#include <malloc.h>
#include <ioctl.h>
#include <nand.h>
#include <errno.h>

static 	ssize_t mtd_read(struct cdev *cdev, void* buf, size_t count,
			  ulong offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen;
	int ret;

	debug("mtd_read: 0x%08lx 0x%08x\n", offset, count);

	ret = mtd->read(mtd, offset, count, &retlen, buf);

	if(ret) {
		printf("err %d\n", ret);
		return ret;
	}
	return retlen;
}

#define NOTALIGNED(x) (x & (mtd->writesize - 1)) != 0
#define MTDPGALG(x) ((x) & (mtd->writesize - 1))

#ifdef CONFIG_MTD_WRITE
static int all_ff(const void *buf, int len)
{
	int i;
	const uint8_t *p = buf;

	for (i = 0; i < len; i++)
		if (p[i] != 0xFF)
			return 0;
	return 1;
}

static ssize_t mtd_write(struct cdev* cdev, const void *buf, size_t _count,
			  ulong offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen, now;
	int ret = 0;
	void *wrbuf = NULL;
	size_t count = _count;

	if (NOTALIGNED(offset)) {
		printf("offset 0x%0lx not page aligned\n", offset);
		return -EINVAL;
	}

	dev_dbg(cdev->dev, "write: 0x%08lx 0x%08x\n", offset, count);
	while (count) {
		now = count > mtd->writesize ? mtd->writesize : count;

		if (NOTALIGNED(now)) {
			dev_dbg(cdev->dev, "not aligned: %d %ld\n",
				mtd->writesize,
				(offset % mtd->writesize));
			wrbuf = xmalloc(mtd->writesize);
			memset(wrbuf, 0xff, mtd->writesize);
			memcpy(wrbuf + (offset % mtd->writesize), buf, now);
			if (!all_ff(wrbuf, mtd->writesize))
				ret = mtd->write(mtd, MTDPGALG(offset),
						  mtd->writesize, &retlen,
						  wrbuf);
			free(wrbuf);
		} else {
			if (!all_ff(buf, mtd->writesize))
				ret = mtd->write(mtd, offset, now, &retlen,
						  buf);
			dev_dbg(cdev->dev,
				"offset: 0x%08lx now: 0x%08x retlen: 0x%08x\n",
				offset, now, retlen);
		}
		if (ret)
			goto out;

		offset += now;
		count -= now;
		buf += now;
	}

out:
	return ret ? ret : _count;
}
#endif

static int mtd_ioctl(struct cdev *cdev, int request, void *buf)
{
	struct mtd_info *mtd = cdev->priv;
	struct mtd_info_user *user = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		dev_dbg(cdev->dev, "MEMGETBADBLOCK: 0x%08lx\n", (off_t)buf);
		return mtd->block_isbad(mtd, (off_t)buf);
#ifdef CONFIG_MTD_WRITE
	case MEMSETBADBLOCK:
		dev_dbg(cdev->dev, "MEMSETBADBLOCK: 0x%08lx\n", (off_t)buf);
		return mtd->block_markbad(mtd, (off_t)buf);
#endif
	case MEMGETINFO:
		user->type	= mtd->type;
		user->flags	= mtd->flags;
		user->size	= mtd->size;
		user->erasesize	= mtd->erasesize;
		user->oobsize	= mtd->oobsize;
		user->mtd	= mtd;
		/* The below fields are obsolete */
		user->ecctype	= -1;
		user->eccsize	= 0;
		return 0;
	}

	return 0;
}

#ifdef CONFIG_MTD_WRITE
static ssize_t mtd_erase(struct cdev *cdev, size_t count, unsigned long offset)
{
	struct mtd_info *mtd = cdev->priv;
	struct erase_info erase;
	int ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.addr = offset;
	erase.len = mtd->erasesize;

	while (count > 0) {
		dev_dbg(cdev->dev, "erase %d %d\n", erase.addr, erase.len);

		ret = mtd->block_isbad(mtd, erase.addr);
		if (ret > 0) {
			printf("Skipping bad block at 0x%08x\n", erase.addr);
		} else {
			ret = mtd->erase(mtd, &erase);
			if (ret)
				return ret;
		}

		erase.addr += mtd->erasesize;
		count -= count > mtd->erasesize ? mtd->erasesize : count;
	}

	return 0;
}
#endif

static struct file_operations mtd_ops = {
	.read   = mtd_read,
#ifdef CONFIG_MTD_WRITE
	.write  = mtd_write,
	.erase  = mtd_erase,
#endif
	.ioctl  = mtd_ioctl,
	.lseek  = dev_lseek_default,
};

#ifdef CONFIG_NAND_OOB_DEVICE
static ssize_t mtd_read_oob(struct cdev *cdev, void *buf, size_t count,
			    ulong offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	struct mtd_oob_ops ops;
	int ret;

	if (count < mtd->oobsize)
		return -EINVAL;

	ops.mode = MTD_OOB_RAW;
	ops.ooboffs = 0;
	ops.ooblen = mtd->oobsize;
	ops.oobbuf = buf;
	ops.datbuf = NULL;
	ops.len = mtd->oobsize;

	offset /= mtd->oobsize;
	ret = mtd->read_oob(mtd, offset * mtd->writesize, &ops);
	if (ret)
		return ret;

	return mtd->oobsize;
}

static struct file_operations mtd_ops_oob = {
	.read   = mtd_read_oob,
	.ioctl  = mtd_ioctl,
	.lseek  = dev_lseek_default,
};

static int mtd_init_oob_cdev(struct mtd_info *mtd, char *devname)
{
	mtd->cdev_oob.ops = &mtd_ops_oob;
	mtd->cdev_oob.size = (mtd->size / mtd->writesize) * mtd->oobsize;
	mtd->cdev_oob.name = asprintf("%s_oob%d", devname, mtd->class_dev.id);
	mtd->cdev_oob.priv = mtd;
	mtd->cdev_oob.dev = &mtd->class_dev;
	devfs_create(&mtd->cdev_oob);

	return 0;
}

static void mtd_exit_oob_cdev(struct mtd_info *mtd)
{
	free(mtd->cdev_oob.name);
}
#else

static int mtd_init_oob_cdev(struct mtd_info *mtd, char *devname)
{
	return 0;
}

static void mtd_exit_oob_cdev(struct mtd_info *mtd)
{
	return;
}
#endif

int add_mtd_device(struct mtd_info *mtd, char *devname)
{
	char str[16];

	if (!devname)
		devname = "mtd";
	strcpy(mtd->class_dev.name, devname);
	mtd->class_dev.id = -1;
	register_device(&mtd->class_dev);

	mtd->cdev.ops = &mtd_ops;
	mtd->cdev.size = mtd->size;
	mtd->cdev.name = asprintf("%s%d", devname, mtd->class_dev.id);
	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->class_dev;
	mtd->cdev.mtd = mtd;

	sprintf(str, "%u", mtd->size);
	dev_add_param_fixed(&mtd->class_dev, "size", str);
	sprintf(str, "%u", mtd->erasesize);
	dev_add_param_fixed(&mtd->class_dev, "erasesize", str);
	sprintf(str, "%u", mtd->writesize);
	dev_add_param_fixed(&mtd->class_dev, "writesize", str);
	sprintf(str, "%u", mtd->oobsize);
	dev_add_param_fixed(&mtd->class_dev, "oobsize", str);

	devfs_create(&mtd->cdev);

	mtd_init_oob_cdev(mtd, devname);

	return 0;
}

int del_mtd_device (struct mtd_info *mtd)
{
	unregister_device(&mtd->class_dev);
	mtd_exit_oob_cdev(mtd);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
	return 0;
}

