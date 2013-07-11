/*
 * Copyright 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * OF helpers for mtd.
 *
 * This file is released under the GPLv2
 */

#ifndef __LINUX_OF_MTD_H
#define __LINUX_OF_MTD_H

#include <of.h>

int of_get_nand_ecc_mode(struct device_node *np);
int of_get_nand_bus_width(struct device_node *np);
bool of_get_nand_on_flash_bbt(struct device_node *np);

#endif /* __LINUX_OF_MTD_H */
