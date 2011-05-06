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
#include <errno.h>

static 	ssize_t nand_read(struct cdev *cdev, void* buf, size_t count, ulong offset, ulong flags)
{
	struct mtd_info *info = cdev->priv;
	size_t retlen;
	int ret;

	debug("nand_read: 0x%08lx 0x%08x\n", offset, count);

	ret = info->read(info, offset, count, &retlen, buf);

	if(ret) {
		printf("err %d\n", ret);
		return ret;
	}
	return retlen;
}

#define NOTALIGNED(x) (x & (info->writesize - 1)) != 0

#ifdef CONFIG_NAND_WRITE
static int all_ff(const void *buf, int len)
{
	int i;
	const uint8_t *p = buf;

	for (i = 0; i < len; i++)
		if (p[i] != 0xFF)
			return 0;
	return 1;
}

static ssize_t nand_write(struct cdev* cdev, const void *buf, size_t _count, ulong offset, ulong flags)
{
	struct mtd_info *info = cdev->priv;
	size_t retlen, now;
	int ret = 0;
	void *wrbuf = NULL;
	size_t count = _count;

	if (NOTALIGNED(offset)) {
		printf("offset 0x%0lx not page aligned\n", offset);
		return -EINVAL;
	}

	debug("write: 0x%08lx 0x%08x\n", offset, count);

	while (count) {
		now = count > info->writesize ? info->writesize : count;

		if (NOTALIGNED(now)) {
			debug("not aligned: %d %ld\n", info->writesize, (offset % info->writesize));
			wrbuf = xmalloc(info->writesize);
			memset(wrbuf, 0xff, info->writesize);
			memcpy(wrbuf + (offset % info->writesize), buf, now);
			if (!all_ff(wrbuf, info->writesize))
				ret = info->write(info, offset & ~(info->writesize - 1),
						info->writesize, &retlen, wrbuf);
			free(wrbuf);
		} else {
			if (!all_ff(buf, info->writesize))
				ret = info->write(info, offset, now, &retlen, buf);
			debug("offset: 0x%08lx now: 0x%08x retlen: 0x%08x\n", offset, now, retlen);
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

static int nand_ioctl(struct cdev *cdev, int request, void *buf)
{
	struct mtd_info *info = cdev->priv;
	struct mtd_info_user *user = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		debug("MEMGETBADBLOCK: 0x%08lx\n", (off_t)buf);
		return info->block_isbad(info, (off_t)buf);
#ifdef CONFIG_NAND_WRITE
	case MEMSETBADBLOCK:
		debug("MEMSETBADBLOCK: 0x%08lx\n", (off_t)buf);
		return info->block_markbad(info, (off_t)buf);
#endif
	case MEMGETINFO:
		user->type	= info->type;
		user->flags	= info->flags;
		user->size	= info->size;
		user->erasesize	= info->erasesize;
		user->oobsize	= info->oobsize;
		user->mtd	= info;
		/* The below fields are obsolete */
		user->ecctype	= -1;
		user->eccsize	= 0;
		return 0;
	}

	return 0;
}

#ifdef CONFIG_NAND_WRITE
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
#endif

#if 0
static char* mtd_get_size(struct device_d *, struct param_d *param)
{
	static char 
}
#endif

static struct file_operations nand_ops = {
	.read   = nand_read,
#ifdef CONFIG_NAND_WRITE
	.write  = nand_write,
	.erase  = nand_erase,
#endif
	.ioctl  = nand_ioctl,
	.lseek  = dev_lseek_default,
};

#ifdef CONFIG_NAND_READ_OOB
static ssize_t nand_read_oob(struct cdev *cdev, void *buf, size_t count, ulong offset, ulong flags)
{
	struct mtd_info *info = cdev->priv;
	struct nand_chip *chip = info->priv;
	struct mtd_oob_ops ops;
	int ret;

	if (count < info->oobsize)
		return -EINVAL;

	ops.mode = MTD_OOB_RAW;
	ops.ooboffs = 0;
	ops.ooblen = info->oobsize;
	ops.oobbuf = buf;
	ops.datbuf = NULL;
	ops.len = info->oobsize;

	offset /= info->oobsize;
	ret = info->read_oob(info, offset << chip->page_shift, &ops);
	if (ret)
		return ret;

	return info->oobsize;
}

static struct file_operations nand_ops_oob = {
	.read   = nand_read_oob,
	.ioctl  = nand_ioctl,
	.lseek  = dev_lseek_default,
};

static int nand_init_oob_cdev(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	mtd->cdev_oob.ops = &nand_ops_oob;
	mtd->cdev_oob.size = (mtd->size >> chip->page_shift) * mtd->oobsize;
	mtd->cdev_oob.name = asprintf("nand_oob%d", mtd->class_dev.id);
	mtd->cdev_oob.priv = mtd;
	mtd->cdev_oob.dev = &mtd->class_dev;
	devfs_create(&mtd->cdev_oob);

	return 0;
}

static void nand_exit_oob_cdev(struct mtd_info *mtd)
{
	free(mtd->cdev_oob.name);
}
#else

static int nand_init_oob_cdev(struct mtd_info *mtd)
{
	return 0;
}

static void nand_exit_oob_cdev(struct mtd_info *mtd)
{
	return;
}
#endif

int add_mtd_device(struct mtd_info *mtd)
{
	char str[16];

	strcpy(mtd->class_dev.name, "nand");
	register_device(&mtd->class_dev);

	mtd->cdev.ops = &nand_ops;
	mtd->cdev.size = mtd->size;
	mtd->cdev.name = asprintf("nand%d", mtd->class_dev.id);
	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->class_dev;
	mtd->cdev.mtd = mtd;

	sprintf(str, "%u", mtd->size);
	dev_add_param_fixed(&mtd->class_dev, "size", str);

	devfs_create(&mtd->cdev);

	nand_init_oob_cdev(mtd);

	return 0;
}

int del_mtd_device (struct mtd_info *mtd)
{
	unregister_device(&mtd->class_dev);
	nand_exit_oob_cdev(mtd);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
	return 0;
}

