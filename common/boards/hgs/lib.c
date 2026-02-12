// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix
// SPDX-FileCopyrightText: Leica Geosystems AG

#define pr_fmt(fmt) "hgs-lib: " fmt

#include <common.h>
#include <linux/stddef.h>
#include <linux/hex.h>

#include <boards/hgs/common.h>

#define HGS_BOARD_REV(_id, _str) {	\
	.id = _id,			\
	.str = _str,			\
}

#define HGS_BOARD_REV_SIMPLE(_id)	\
	HGS_BOARD_REV(HGS_BOARD_REV##_id, #_id)

static const struct hgs_board_revision hgs_revision_table[] = {
	HGS_BOARD_REV_SIMPLE(_A),
	HGS_BOARD_REV_SIMPLE(_B),
	HGS_BOARD_REV_SIMPLE(_C),
	HGS_BOARD_REV_SIMPLE(_D),
	HGS_BOARD_REV_SIMPLE(_E),
};

const struct hgs_board_revision *
hgs_get_rev_from_part_trace(const struct hgs_part_trace_code *code)
{
	const struct hgs_board_revision *rev;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(hgs_revision_table); i++) {
		rev = &hgs_revision_table[i];
		if (!memcmp(code->material_revision, rev->str,
		    sizeof(code->material_revision)))
			return rev;
	}

	return NULL;
}

const u32
hgs_get_artno_from_part_trace(const struct hgs_part_trace_code *code)
{
	__be32 res = 0;
	u8 tmp[8];
	int err;

	/*
	 * Art-no. has 7-digits, add a leading '0' to align it to 8 and make
	 * use of hex2bin
	 */
	tmp[0] = '0';
	memcpy(tmp + 1, code->art_number, sizeof(code->art_number));
	err = hex2bin((u8 *)&res, tmp, sizeof(res));

	return err ? 0 : be32_to_cpu(res);
}
