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
#include <nand.h>
#include <init.h>
#include <xfuncs.h>
#include <driver.h>

char *default_nand_name = "huhu";

static inline int board_nand_init(struct nand_chip *nand)
{
	return 0;
}

static void nand_init_chip(struct mtd_info *mtd, struct nand_chip *nand,
			   ulong base_addr)
{
	mtd->priv = nand;

	nand->IO_ADDR_R = nand->IO_ADDR_W = (void  __iomem *)base_addr;
	if (board_nand_init(nand) == 0) {
		if (nand_scan(mtd, 1) == 0) {
			if (!mtd->name)
				mtd->name = (char *)default_nand_name;
		} else
			mtd->name = NULL;
	} else {
		mtd->name = NULL;
		mtd->size = 0;
	}

}

struct nand_host {
	struct mtd_info info;
	struct nand_chip chip;
	struct device_d *dev;
};

static int nand_probe (struct device_d *dev)
{
	struct nand_host *host;

	host = xzalloc(sizeof(*host));

	nand_init_chip(&host->info, &host->chip, dev->map_base);

	return 0;
}

static struct driver_d nand_driver = {
	.name   = "nand_flash",
	.probe  = nand_probe,
//	.read   = nand_read,
//	.write  = nand_write,
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

