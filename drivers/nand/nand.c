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

static 	ssize_t nand_read(struct device_d *dev, void* buf, size_t count, ulong offset, ulong flags)
{
	struct nand_chip *nand = dev->priv;
	size_t retlen;
	int ret;

	printf("nand_read: 0x%08x 0x%08x\n", offset, count);

	ret = nand->read(nand, offset, count, &retlen, buf);

	if(ret)
		return ret;
	return retlen;
}

#define NOTALIGNED(x) (x & (nand->oobblock-1)) != 0

static ssize_t nand_write(struct device_d* dev, const void* buf, size_t _count, ulong offset, ulong flags)
{
	struct nand_chip *nand = dev->priv;
	size_t retlen, now;
	int ret;
	void *wrbuf = NULL;
	size_t count = _count;

	printf("write: 0x%08x 0x%08x\n", offset, count);

	while (count) {
		now = count > nand->oobblock ? nand->oobblock : count;
		
		if (NOTALIGNED(now) || NOTALIGNED(offset)) {
			printf("not aligned: %d %d\n", nand->oobblock, (offset % nand->oobblock));
			wrbuf = xmalloc(nand->oobblock);
			memset(wrbuf, 0xff, nand->oobblock);
			memcpy(wrbuf + (offset % nand->oobblock), buf, now);
			ret = nand->write_ecc(nand, offset & ~(nand->oobblock - 1), nand->oobblock, &retlen, wrbuf,
				NULL, &nand->oobinfo);
		} else {
			ret = nand->write_ecc(nand, offset, now, &retlen, buf,
				NULL, &nand->oobinfo);
			printf("huhu offset: 0x%08x now: 0x%08x retlen: 0x%08x\n", offset, now, retlen);
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

static int nand_ioctl(struct device_d *dev, int request, void *buf)
{
	struct nand_chip *nand = dev->priv;
	struct mtd_info_user *info = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		printf("MEMGETBADBLOCK: 0x%08x\n", (off_t)buf);
		return nand->block_isbad(nand, (off_t)buf);
	case MEMGETINFO:
		info->type	= nand->type;
		info->flags	= nand->flags;
		info->size	= nand->size;
		info->erasesize	= nand->erasesize;
		info->oobsize	= nand->oobsize;
		/* The below fields are obsolete */
		info->ecctype	= -1;
		info->eccsize	= 0;
		return 0;
	}

	return 0;
}

static ssize_t nand_erase(struct device_d *dev, size_t count, unsigned long offset)
{
	struct nand_chip *nand = dev->priv;
	struct erase_info erase;
	int ret;

	memset(&erase, 0, sizeof(erase));
	erase.nand = nand;
	erase.addr = offset;
	erase.len = nand->erasesize;

	while (count > 0) {
		debug("erase %d %d\n", erase.addr, erase.len);

		ret = nand->block_isbad(nand, erase.addr);
		if (ret > 0) {
			printf("Skipping bad block at 0x%08x\n", erase.addr);
		} else {
			ret = nand->erase(nand, &erase);
			if (ret)
				return ret;
		}

		erase.addr += nand->erasesize;
		count -= count > nand->erasesize ? nand->erasesize : count;
	}

	return 0;
}

static int nand_controller_probe(struct device_d *dev)
{
	struct nand_chip *nand;
	struct nand_platform_data *pdata = dev->platform_data;
	int ret;

	nand = xzalloc(sizeof(*nand));
	dev->priv = nand;

	nand->IO_ADDR_R = nand->IO_ADDR_W = (void *)dev->map_base;
	nand->hwcontrol = pdata->hwcontrol;
	nand->eccmode = pdata->eccmode;
	nand->dev_ready = pdata->dev_ready;
	nand->chip_delay = pdata->chip_delay;

	ret = nand_scan(nand, 1);
	if (ret)
		goto out;

	strcpy(nand->dev.name, "nand_device");
	get_free_deviceid(nand->dev.id, "nand");
	nand->dev.size = nand->chipsize;
	nand->dev.priv = nand;

	ret = register_device(&nand->dev);
	if (ret)
		goto out;

	return 0;

out:
	free(nand);
	return ret;
}

static struct driver_d nand_controller_driver = {
	.name   = "nand_controller",
	.probe  = nand_controller_probe,
};

static int nand_device_probe(struct device_d *dev)
{
	return 0;
}

static struct driver_d nand_device_driver = {
	.name   = "nand_device",
	.probe  = nand_device_probe,
	.read   = nand_read,
	.write  = nand_write,
	.ioctl  = nand_ioctl,
	.open   = dev_open_default,
	.close  = dev_close_default,
	.lseek  = dev_lseek_default,
	.erase  = nand_erase,
//	.info   = nand_info,
};

static int nand_init(void)
{
	register_driver(&nand_device_driver);
	register_driver(&nand_controller_driver);

	return 0;
}

device_initcall(nand_init);

