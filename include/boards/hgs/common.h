/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2025 Pengutronix */
/* SPDX-FileCopyrightText: Leica Geosystems AG */

#ifndef HGS_COMMON_H
#define HGS_COMMON_H

#include <linux/types.h>

enum hgs_hw {
	HGS_HW_GS05,
	NUM_HGS_HW
};

enum hgs_built_type {
	HGS_DEV_BUILD,
	HGS_REL_BUILD
};

struct hgs_part_trace_code {
	/* Art. Number (e.g. 0983441 – if 6 digit add 0 before) */
	u8 art_number[7];
	/* Material Revision (_A,_B,...,_Z,AA,AB,...,ZZ) */
	u8 material_revision[2];
	/* Vendor id of component manufacturer */
	u8 vendor_id[7];
	/* Production lot date (YYYYMMDD) */
	u8 production_date[8];
	/* Consecutive number (restart 00001 for each production date) */
	u8 consecutive_number[5];
} __packed;

enum hgs_board_rev {
	HGS_BOARD_REV_A,
	HGS_BOARD_REV_B,
	HGS_BOARD_REV_C,
	HGS_BOARD_REV_D,
	HGS_BOARD_REV_E,
};

struct hgs_board_revision {
	enum hgs_board_rev id;
	const char *str;
};

#define HGS_MACHINE(_revid, _comp, _model) {	\
	.revision	= _revid,		\
	.dts_compatible	= _comp,		\
	.model		= _model		\
}

struct hgs_machine {
	enum hgs_hw type;
	struct device *dev;
	enum hgs_board_rev revision;
	const char *dts_compatible;
	const char *console_alias;
	const char *model;
	const char *mmc_alias;
	struct cdev *mmc_cdev;
	char *hostname;

	bool skip_firstboot_setup;
};

static inline enum hgs_built_type hgs_get_built_type(void)
{
	return CONFIG_HABV4_SRK_INDEX == 0 ? HGS_DEV_BUILD : HGS_REL_BUILD;
}

enum hgs_tag;

void hgs_early_hw_init(enum hgs_hw hw);
int hgs_common_boot(struct hgs_machine *machine);

/* Helper functions */
const struct hgs_board_revision *
hgs_get_rev_from_part_trace(const struct hgs_part_trace_code *code);
const u32
hgs_get_artno_from_part_trace(const struct hgs_part_trace_code *code);

#endif /* HGS_COMMON_H */
