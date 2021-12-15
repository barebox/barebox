// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2008, 2010-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP Semiconductor
 */

#include <common.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include "fsl_ddr.h"

struct dynamic_odt {
	unsigned int odt_rd_cfg;
	unsigned int odt_wr_cfg;
	unsigned int odt_rtt_norm;
	unsigned int odt_rtt_wr;
};

/* Quad rank is not verified yet due availability.
 * Replacing 20 OHM with 34 OHM since DDR4 doesn't have 20 OHM option
 */
static const struct dynamic_odt single_Q_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR4_RTT_34_OHM,	/* unverified */
		DDR4_RTT_120_OHM
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_120_OHM
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,	/* tied high */
		DDR4_RTT_OFF,
		DDR4_RTT_120_OHM
	}
};

static const struct dynamic_odt single_D_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt single_S_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
};

static const struct dynamic_odt dual_DD_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt dual_DS_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0}
};
static const struct dynamic_odt dual_SD_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR4_RTT_34_OHM,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt dual_SS_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR4_RTT_34_OHM,
		DDR4_RTT_120_OHM
	},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_D0_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_0D_ddr4[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR4_RTT_OFF,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt dual_S0_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}

};

static const struct dynamic_odt dual_0S_ddr4[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_40_OHM,
		DDR4_RTT_OFF
	},
	{0, 0, 0, 0}

};

static const struct dynamic_odt odt_unknown_ddr4[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR4_RTT_120_OHM,
		DDR4_RTT_OFF
	}
};

static const struct dynamic_odt single_Q_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR3_RTT_20_OHM,
		DDR3_RTT_120_OHM
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,	/* tied high */
		DDR3_RTT_OFF,
		DDR3_RTT_120_OHM
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS_AND_OTHER_DIMM,
		DDR3_RTT_20_OHM,
		DDR3_RTT_120_OHM
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,	/* tied high */
		DDR3_RTT_OFF,
		DDR3_RTT_120_OHM
	}
};

static const struct dynamic_odt single_D_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR3_RTT_OFF,
		DDR3_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt single_S_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
};

static const struct dynamic_odt dual_DD_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR3_RTT_30_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR3_RTT_30_OHM,
		DDR3_RTT_OFF
	}
};

static const struct dynamic_odt dual_DS_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR3_RTT_30_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_20_OHM,
		DDR3_RTT_120_OHM
	},
	{0, 0, 0, 0}
};
static const struct dynamic_odt dual_SD_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_20_OHM,
		DDR3_RTT_120_OHM
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR3_RTT_20_OHM,
		DDR3_RTT_OFF
	}
};

static const struct dynamic_odt dual_SS_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_30_OHM,
		DDR3_RTT_120_OHM
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_ALL,
		DDR3_RTT_30_OHM,
		DDR3_RTT_120_OHM
	},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_D0_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR3_RTT_OFF,
		DDR3_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_0D_ddr3[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_SAME_DIMM,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR3_RTT_OFF,
		DDR3_RTT_OFF
	}
};

static const struct dynamic_odt dual_S0_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}

};

static const struct dynamic_odt dual_0S_ddr3[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_40_OHM,
		DDR3_RTT_OFF
	},
	{0, 0, 0, 0}

};

static const struct dynamic_odt odt_unknown_ddr3[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR3_RTT_120_OHM,
		DDR3_RTT_OFF
	}
};

static const struct dynamic_odt single_Q_ddr12[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt single_D_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt single_S_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
};

static const struct dynamic_odt dual_DD_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	}
};

static const struct dynamic_odt dual_DS_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_SD_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	}
};

static const struct dynamic_odt dual_SS_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_OTHER_DIMM,
		FSL_DDR_ODT_OTHER_DIMM,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_D0_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static const struct dynamic_odt dual_0D_ddr12[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_ALL,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	}
};

static const struct dynamic_odt dual_S0_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}

};

static const struct dynamic_odt dual_0S_ddr12[4] = {
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR2_RTT_150_OHM,
		DDR2_RTT_OFF
	},
	{0, 0, 0, 0}

};

static const struct dynamic_odt odt_unknown_ddr12[4] = {
	{	/* cs0 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs1 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	},
	{	/* cs2 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_CS,
		DDR2_RTT_75_OHM,
		DDR2_RTT_OFF
	},
	{	/* cs3 */
		FSL_DDR_ODT_NEVER,
		FSL_DDR_ODT_NEVER,
		DDR2_RTT_OFF,
		DDR2_RTT_OFF
	}
};

/*
 * Automatically seleect bank interleaving mode based on DIMMs
 * in this order: cs0_cs1_cs2_cs3, cs0_cs1, null.
 * This function only deal with one or two slots per controller.
 */
static inline unsigned int auto_bank_intlv(struct fsl_ddr_controller *c,
					   struct dimm_params *pdimm)
{
	if (c->dimm_slots_per_ctrl == 1) {
		if (pdimm[0].n_ranks == 4)
			return FSL_DDR_CS0_CS1_CS2_CS3;
		else if (pdimm[0].n_ranks == 2)
			return FSL_DDR_CS0_CS1;
	}

	if (c->dimm_slots_per_ctrl == 2) {
		if (pdimm[0].n_ranks == 2) {
			if (pdimm[1].n_ranks == 2)
				return FSL_DDR_CS0_CS1_CS2_CS3;
			else
				return FSL_DDR_CS0_CS1;
		}
	}

	return 0;
}

unsigned int populate_memctl_options(struct fsl_ddr_controller *c)
{
	const struct common_timing_params *common_dimm = &c->common_timing_params;
	memctl_options_t *popts = &c->memctl_opts;
	struct dimm_params *pdimm = c->dimm_params;
	unsigned int i;
	const struct dynamic_odt *pdodt;
	const struct dynamic_odt *single_Q, *single_S, *single_D;
	const struct dynamic_odt *dual_DD, *dual_DS, *dual_0S;
	const struct dynamic_odt *dual_D0, *dual_SD, *dual_SS, *dual_S0, *dual_0D;

	if (is_ddr1(popts) || is_ddr2(popts)) {
		pdodt = odt_unknown_ddr12;
		single_Q = single_Q_ddr12;
		single_D = single_D_ddr12;
		single_S = single_S_ddr12;
		dual_DD = dual_DD_ddr12;
		dual_DS = dual_DS_ddr12;
		dual_0S = dual_0S_ddr12;
		dual_D0 = dual_D0_ddr12;
		dual_SD = dual_SD_ddr12;
		dual_SS = dual_SS_ddr12;
		dual_S0 = dual_S0_ddr12;
		dual_0D = dual_0D_ddr12;
	} else if (is_ddr3(popts)) {
		pdodt = odt_unknown_ddr3;
		single_Q = single_Q_ddr3;
		single_D = single_D_ddr3;
		single_S = single_S_ddr3;
		dual_DD = dual_DD_ddr3;
		dual_DS = dual_DS_ddr3;
		dual_0S = dual_0S_ddr3;
		dual_D0 = dual_D0_ddr3;
		dual_SD = dual_SD_ddr3;
		dual_SS = dual_SS_ddr3;
		dual_S0 = dual_S0_ddr3;
		dual_0D = dual_0D_ddr3;
	} else if (is_ddr4(popts)) {
		pdodt = odt_unknown_ddr4;
		single_Q = single_Q_ddr4;
		single_D = single_D_ddr4;
		single_S = single_S_ddr4;
		dual_DD = dual_DD_ddr4;
		dual_DS = dual_DS_ddr4;
		dual_0S = dual_0S_ddr4;
		dual_D0 = dual_D0_ddr4;
		dual_SD = dual_SD_ddr4;
		dual_SS = dual_SS_ddr4;
		dual_S0 = dual_S0_ddr4;
		dual_0D = dual_0D_ddr4;
	} else {
		return -EINVAL;
	}

	if (!is_ddr1(popts)) {
		/* Chip select options. */
		if (c->dimm_slots_per_ctrl == 1) {
			switch (pdimm[0].n_ranks) {
			case 1:
				pdodt = single_S;
				break;
			case 2:
				pdodt = single_D;
				break;
			case 4:
				pdodt = single_Q;
				break;
			}
		} else if (c->dimm_slots_per_ctrl == 2) {
			switch (pdimm[0].n_ranks) {
			case 4:
				pdodt = single_Q;
				if (pdimm[1].n_ranks)
					printf("Error: Quad- and Dual-rank DIMMs cannot be used together\n");
				break;
			case 2:
				switch (pdimm[1].n_ranks) {
				case 2:
					pdodt = dual_DD;
					break;
				case 1:
					pdodt = dual_DS;
					break;
				case 0:
					pdodt = dual_D0;
					break;
				}
				break;
			case 1:
				switch (pdimm[1].n_ranks) {
				case 2:
					pdodt = dual_SD;
					break;
				case 1:
					pdodt = dual_SS;
					break;
				case 0:
					pdodt = dual_S0;
					break;
				}
				break;
			case 0:
				switch (pdimm[1].n_ranks) {
				case 2:
					pdodt = dual_0D;
					break;
				case 1:
					pdodt = dual_0S;
					break;
				}
				break;
			}
		}
	}

	/* Pick chip-select local options. */
	for (i = 0; i < c->chip_selects_per_ctrl; i++) {
		if (is_ddr1(popts)) {
			popts->cs_local_opts[i].odt_rd_cfg = FSL_DDR_ODT_NEVER;
			popts->cs_local_opts[i].odt_wr_cfg = FSL_DDR_ODT_CS;
		} else {
			popts->cs_local_opts[i].odt_rd_cfg = pdodt[i].odt_rd_cfg;
			popts->cs_local_opts[i].odt_wr_cfg = pdodt[i].odt_wr_cfg;
			popts->cs_local_opts[i].odt_rtt_norm = pdodt[i].odt_rtt_norm;
			popts->cs_local_opts[i].odt_rtt_wr = pdodt[i].odt_rtt_wr;
		}
		popts->cs_local_opts[i].auto_precharge = 0;
	}

	/* Pick interleaving mode. */

	/*
	 * 0 = no interleaving
	 * 1 = interleaving between 2 controllers
	 */
	popts->memctl_interleaving = 0;

	/*
	 * 0 = cacheline
	 * 1 = page
	 * 2 = (logical) bank
	 * 3 = superbank (only if CS interleaving is enabled)
	 */
	popts->memctl_interleaving_mode = 0;

	/*
	 * 0: cacheline: bit 30 of the 36-bit physical addr selects the memctl
	 * 1: page:      bit to the left of the column bits selects the memctl
	 * 2: bank:      bit to the left of the bank bits selects the memctl
	 * 3: superbank: bit to the left of the chip select selects the memctl
	 *
	 * NOTE: ba_intlv (rank interleaving) is independent of memory
	 * controller interleaving; it is only within a memory controller.
	 * Must use superbank interleaving if rank interleaving is used and
	 * memory controller interleaving is enabled.
	 */

	/*
	 * 0 = no
	 * 0x40 = CS0,CS1
	 * 0x20 = CS2,CS3
	 * 0x60 = CS0,CS1 + CS2,CS3
	 * 0x04 = CS0,CS1,CS2,CS3
	 */
	popts->ba_intlv_ctl = 0;

	/* Memory Organization Parameters */
	popts->registered_dimm_en = common_dimm->all_dimms_registered;

	/*
	 * Choose DQS config
	 * 0 for DDR1
	 * 1 for DDR2
	 */
	if (is_ddr2(popts) || is_ddr3(popts))
		popts->dqs_config = 1;

	/* Choose self-refresh during sleep. */
	popts->self_refresh_in_sleep = 1;

	/* Choose dynamic power management mode. */
	popts->dynamic_power = 0;

	popts->x4_en = (pdimm[0].device_width == 4) ? 1 : 0;

	/* Choose ddr controller address mirror mode */
	if (is_ddr3_4(popts)) {
		if (pdimm[0].n_ranks != 0) {
			if (pdimm[0].primary_sdram_width == 64)
				popts->data_bus_width = 0;
			else if (pdimm[0].primary_sdram_width == 32)
				popts->data_bus_width = 1;
			else if (pdimm[0].primary_sdram_width == 16)
				popts->data_bus_width = 2;
			else {
				panic("Error: primary sdram width %u is invalid!\n",
					pdimm[0].primary_sdram_width);
			}
		}

		if ((popts->data_bus_width == 1) || (popts->data_bus_width == 2)) {
			/* 32-bit or 16-bit bus */
			popts->otf_burst_chop_en = 0;
			popts->burst_length = DDR_BL8;
		} else {
			popts->otf_burst_chop_en = 1;	/* on-the-fly burst chop */
			popts->burst_length = DDR_OTF;	/* on-the-fly BC4 and BL8 */
		}

		for (i = 0; i < c->dimm_slots_per_ctrl; i++) {
			if (pdimm[i].n_ranks) {
				popts->mirrored_dimm = pdimm[i].mirrored_dimm;
				break;
			}
		}
	} else {
		if (pdimm[0].n_ranks != 0) {
			if ((pdimm[0].data_width >= 64) && \
				(pdimm[0].data_width <= 72))
				popts->data_bus_width = 0;
			else if ((pdimm[0].data_width >= 32) && \
				(pdimm[0].data_width <= 40))
				popts->data_bus_width = 1;
			else {
				panic("Error: data width %u is invalid!\n",
					pdimm[0].data_width);
			}
		}

		popts->burst_length = DDR_BL4;	/* has to be 4 for DDR2 */
	}


	/* Global Timing Parameters. */
	debug("mclk_ps = %u ps\n", get_memory_clk_period_ps(c));

	/* Pick a caslat override. */
	popts->cas_latency_override = 0;
	popts->cas_latency_override_value = 3;
	if (popts->cas_latency_override) {
		debug("using caslat override value = %u\n",
		       popts->cas_latency_override_value);
	}

	/* Decide whether to use the computed derated latency */
	popts->use_derated_caslat = 0;

	/* Choose an additive latency. */
	popts->additive_latency_override = 0;
	popts->additive_latency_override_value = 3;
	if (popts->additive_latency_override) {
		debug("using additive latency override value = %u\n",
		       popts->additive_latency_override_value);
	}

	/*
	 * 2T_EN setting
	 *
	 * Factors to consider for 2T_EN:
	 *	- number of DIMMs installed
	 *	- number of components, number of active ranks
	 *	- how much time you want to spend playing around
	 */
	popts->twot_en = 0;
	popts->threet_en = 0;

	/* for RDIMM and DDR4 UDIMM/discrete memory, address parity enable */
	if (popts->registered_dimm_en)
		popts->ap_en = 1; /* 0 = disable,  1 = enable */
	else
		popts->ap_en = 0; /* disabled for DDR4 UDIMM/discrete default */

	/*
	 * BSTTOPRE precharge interval
	 *
	 * Set this to 0 for global auto precharge
	 * The value of 0x100 has been used for DDR1, DDR2, DDR3.
	 * It is not wrong. Any value should be OK. The performance depends on
	 * applications. There is no one good value for all. One way to set
	 * is to use 1/4 of refint value.
	 */
	popts->bstopre = picos_to_mclk(c, common_dimm->refresh_rate_ps)
			 >> 2;

	/*
	 * Window for four activates -- tFAW
	 *
	 * FIXME: UM: applies only to DDR2/DDR3 with eight logical banks only
	 * FIXME: varies depending upon number of column addresses or data
	 * FIXME: width, was considering looking at pdimm->primary_sdram_width
	 */
	if (is_ddr1(popts))
		popts->tfaw_window_four_activates_ps = mclk_to_picos(c, 1);
	else if (is_ddr2(popts))
		/*
		 * x4/x8;  some datasheets have 35000
		 * x16 wide columns only?  Use 50000?
		 */
		popts->tfaw_window_four_activates_ps = 37500;
	else
		popts->tfaw_window_four_activates_ps = pdimm[0].tfaw_ps;

	popts->zq_en = 0;
	popts->wrlvl_en = 0;

	if (is_ddr3_4(popts)) {
		/*
		 * due to ddr3 dimm is fly-by topology
		 * we suggest to enable write leveling to
		 * meet the tQDSS under different loading.
		 */
		popts->wrlvl_en = 1;
		popts->zq_en = 1;
		popts->wrlvl_override = 0;
	}

	if (pdimm[0].n_ranks == 4)
		popts->quad_rank_present = 1;

	popts->package_3ds = pdimm->package_3ds;

	if (!is_ddr4(popts)) {
		ulong ddr_freq = c->ddr_freq / 1000000;
		if (popts->registered_dimm_en) {
			popts->rcw_override = 1;
			popts->rcw_1 = 0x000a5a00;
			if (ddr_freq <= 800)
				popts->rcw_2 = 0x00000000;
			else if (ddr_freq <= 1066)
				popts->rcw_2 = 0x00100000;
			else if (ddr_freq <= 1333)
				popts->rcw_2 = 0x00200000;
			else
				popts->rcw_2 = 0x00300000;
		}
	}

	if (c->board_options)
		c->board_options(popts, pdimm, c);

	return 0;
}

void check_interleaving_options(struct fsl_ddr_info *pinfo)
{
	int i, j, k, check_n_ranks, intlv_invalid = 0;
	unsigned int check_intlv, check_n_row_addr, check_n_col_addr;
	unsigned long long check_rank_density;
	struct dimm_params *dimm;

	/*
	 * Check if all controllers are configured for memory
	 * controller interleaving. Identical dimms are recommended. At least
	 * the size, row and col address should be checked.
	 */
	j = 0;
	check_n_ranks = pinfo->c[0].dimm_params[0].n_ranks;
	check_rank_density = pinfo->c[0].dimm_params[0].rank_density;
	check_n_row_addr =  pinfo->c[0].dimm_params[0].n_row_addr;
	check_n_col_addr = pinfo->c[0].dimm_params[0].n_col_addr;
	check_intlv = pinfo->c[0].memctl_opts.memctl_interleaving_mode;
	for (i = 0; i < pinfo->num_ctrls; i++) {
		dimm = &pinfo->c[i].dimm_params[0];
		if (!pinfo->c[i].memctl_opts.memctl_interleaving) {
			continue;
		} else if (((check_rank_density != dimm->rank_density) ||
		     (check_n_ranks != dimm->n_ranks) ||
		     (check_n_row_addr != dimm->n_row_addr) ||
		     (check_n_col_addr != dimm->n_col_addr) ||
		     (check_intlv !=
			pinfo->c[i].memctl_opts.memctl_interleaving_mode))){
			intlv_invalid = 1;
			break;
		} else {
			j++;
		}

	}
	if (intlv_invalid) {
		for (i = 0; i < pinfo->num_ctrls; i++)
			pinfo->c[i].memctl_opts.memctl_interleaving = 0;
		printf("Not all DIMMs are identical. "
			"Memory controller interleaving disabled.\n");
	} else {
		switch (check_intlv) {
		case FSL_DDR_256B_INTERLEAVING:
		case FSL_DDR_CACHE_LINE_INTERLEAVING:
		case FSL_DDR_PAGE_INTERLEAVING:
		case FSL_DDR_BANK_INTERLEAVING:
		case FSL_DDR_SUPERBANK_INTERLEAVING:
			if (pinfo->num_ctrls == 3)
				k = 2;
			else
				k = pinfo->num_ctrls;
			break;
		case FSL_DDR_3WAY_1KB_INTERLEAVING:
		case FSL_DDR_3WAY_4KB_INTERLEAVING:
		case FSL_DDR_3WAY_8KB_INTERLEAVING:
		case FSL_DDR_4WAY_1KB_INTERLEAVING:
		case FSL_DDR_4WAY_4KB_INTERLEAVING:
		case FSL_DDR_4WAY_8KB_INTERLEAVING:
		default:
			k = pinfo->num_ctrls;
			break;
		}
		debug("%d of %d controllers are interleaving.\n", j, k);
		if (j && (j != k)) {
			for (i = 0; i < pinfo->num_ctrls; i++)
				pinfo->c[i].memctl_opts.memctl_interleaving = 0;
			if (pinfo->num_ctrls > 1)
				printf("Not all controllers have compatible interleaving mode. All disabled.\n");
		}
	}
	debug("Checking interleaving options completed\n");
}
