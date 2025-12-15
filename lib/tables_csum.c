// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e3db8d60becb9842eb382d78863dd6f3d3756009/lib/tables_csum.c
/* SPDX-FileCopyrightText: 2015, Bin Meng <bmeng.cn@gmail.com> */

#include <linux/types.h>
#include <tables_csum.h>

u8 table_compute_checksum(const void *v, const size_t len)
{
	const u8 *bytes = v;
	u8 checksum = 0;
	int i;

	for (i = 0; i < len; i++)
		checksum -= bytes[i];

	return checksum;
}
