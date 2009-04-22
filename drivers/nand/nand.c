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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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

static 	ssize_t nand_read(struct cdev *cdev, void* buf, size_t count, ulong offset, ulong flags)
{
	struct mtd_info *info = cdev->priv;
	size_t retlen;
	int ret;

	debug("nand_read: 0x%08x 0x%08x\n", offset, count);

	ret = info->read(info, offset, count, &retlen, buf);

	if(ret) {
		printf("err %d\n", ret);
		return ret;
	}
	return retlen;
}

#define NOTALIGNED(x) (x & (info->writesize - 1)) != 0

static ssize_t nand_write(struct cdev* cdev, const void *buf, size_t _count, ulong offset, ulong flags)
{
	struct mtd_info *info = cdev->priv;
	size_t retlen, now;
	int ret;
	void *wrbuf = NULL;
	size_t count = _count;

	debug("write: 0x%08x 0x%08x\n", offset, count);

	while (count) {
		now = count > info->writesize ? info->writesize : count;

		if (NOTALIGNED(now) || NOTALIGNED(offset)) {
			debug("not aligned: %d %d\n", info->writesize, (offset % info->writesize));
			wrbuf = xmalloc(info->writesize);
			memset(wrbuf, 0xff, info->writesize);
			memcpy(wrbuf + (offset % info->writesize), buf, now);
			ret = info->write(info, offset & ~(info->writesize - 1), info->writesize, &retlen, wrbuf);
		} else {
			ret = info->write(info, offset, now, &retlen, buf);
			debug("offset: 0x%08x now: 0x%08x retlen: 0x%08x\n", offset, now, retlen);
		}
		if (ret)
			goto out;

		offset += now;
		count -= now;
		buf += now;
	}

out:
	if (wrbuf)
		free(wrbuf);

	return ret ? ret : _count;
}

static int nand_ioctl(struct cdev *cdev, int request, void *buf)
{
	struct mtd_info *info = cdev->priv;
	struct mtd_info_user *user = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		debug("MEMGETBADBLOCK: 0x%08x\n", (off_t)buf);
		return info->block_isbad(info, (off_t)buf);
	case MEMSETBADBLOCK:
		debug("MEMSETBADBLOCK: 0x%08x\n", (off_t)buf);
		return info->block_markbad(info, (off_t)buf);
	case MEMGETINFO:
		user->type	= info->type;
		user->flags	= info->flags;
		user->size	= info->size;
		user->erasesize	= info->erasesize;
		user->oobsize	= info->oobsize;
		/* The below fields are obsolete */
		user->ecctype	= -1;
		user->eccsize	= 0;
		return 0;
	}

	return 0;
}

static ssize_t nand_erase(struct cdev *cdev, size_t count, unsigned long offset)
{
	struct mtd_info *info = cdev->priv;
	struct erase_info erase;
	int ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = info;
	erase.addr = offset;
	erase.len = info->erasesize;

	while (count > 0) {
		debug("erase %d %d\n", erase.addr, erase.len);

		ret = info->block_isbad(info, erase.addr);
		if (ret > 0) {
			printf("Skipping bad block at 0x%08x\n", erase.addr);
		} else {
			ret = info->erase(info, &erase);
			if (ret)
				return ret;
		}

		erase.addr += info->erasesize;
		count -= count > info->erasesize ? info->erasesize : count;
	}

	return 0;
}

static struct file_operations nand_ops = {
	.read   = nand_read,
	.write  = nand_write,
	.ioctl  = nand_ioctl,
	.lseek  = dev_lseek_default,
	.erase  = nand_erase,
};

static int nand_device_probe(struct device_d *dev)
{
	return 0;
}

static struct driver_d nand_device_driver = {
	.name   = "nand_device",
	.probe  = nand_device_probe,
	.type	= DEVICE_TYPE_NAND,
};

static int nand_init(void)
{
	register_driver(&nand_device_driver);

	return 0;
}

device_initcall(nand_init);

int add_mtd_device(struct mtd_info *mtd)
{
	struct device_d *dev = &mtd->class_dev;
	char name[MAX_DRIVER_NAME];

	get_free_deviceid(name, "nand");

	mtd->cdev.ops = &nand_ops;
	mtd->cdev.size = mtd->size;
	mtd->cdev.name = strdup(name);
	mtd->cdev.dev = dev;
	mtd->cdev.priv = mtd;

	devfs_create(&mtd->cdev);

	return 0;
}

int del_mtd_device (struct mtd_info *mtd)
{
	unregister_device(&mtd->class_dev);
	return 0;
}

