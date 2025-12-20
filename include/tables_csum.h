/* SPDX-License-Identifier: GPL-2.0+ */
/* SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e3db8d60becb9842eb382d78863dd6f3d3756009/include/tables_csum.h */
/* SPDX-FileCopyrightText: 2015, Bin Meng <bmeng.cn@gmail.com> */

#ifndef _TABLES_CSUM_H_
#define _TABLES_CSUM_H_

#include <linux/types.h>

/**
 * table_compute_checksum() - Compute a table checksum
 *
 * This computes an 8-bit checksum for the configuration table.
 * All bytes in the configuration table, including checksum itself and
 * reserved bytes must add up to zero.
 *
 * @v:		configuration table base address
 * @len:	configuration table size
 * @return:	the 8-bit checksum
 */
u8 table_compute_checksum(const void *v, const size_t len);

#endif
