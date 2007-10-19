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
#include <init.h>
#include <xfuncs.h>
#include <driver.h>

static 	ssize_t nand_read(struct device_d *dev, void* buf, size_t count, ulong offset, ulong flags)
{
	struct nand_chip *nand = dev->priv;
	size_t retlen;
	int ret;
	char oobuf[NAND_MAX_OOBSIZE];

	printf("%s\n", __FUNCTION__);

	ret = nand->read_ecc(nand, offset, count, &retlen, buf, oobuf, &nand->oobinfo);

	if(ret)
		return ret;
	return retlen;
}

static ssize_t nand_write(struct device_d* dev, const void* buf, size_t count, ulong offset, ulong flags)
{
	struct nand_chip *nand = dev->priv;
	size_t retlen;
	int ret;

	ret = nand->write_ecc(nand, offset, count, &retlen, buf, NULL, &nand->oobinfo);
	if (ret)
		return ret;
	return retlen;
}

static int nand_probe (struct device_d *dev)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static struct driver_d nand_driver = {
	.name   = "nand_flash",
	.probe  = nand_probe,
	.read   = nand_read,
	.write  = nand_write,
//	.erase  = nand_erase,
//	.protect= nand_protect,
//	.memmap = generic_memmap_ro,
//	.info   = nand_info,
};

static int nand_init(void)
{
	return register_driver(&nand_driver);
}

device_initcall(nand_init);

