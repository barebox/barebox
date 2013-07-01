/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef COMMON_TIMING_PARAMS_H
#define COMMON_TIMING_PARAMS_H

struct common_timing_params_s {
	uint32_t tCKmin_X_ps;
	uint32_t tCKmax_ps;
	uint32_t tCKmax_max_ps;
	uint32_t tRCD_ps;
	uint32_t tRP_ps;
	uint32_t tRAS_ps;
	uint32_t tWR_ps;	/* maximum = 63750 ps */
	uint32_t tWTR_ps;	/* maximum = 63750 ps */
	uint32_t tRFC_ps;	/* maximum = 255 ns + 256 ns + .75 ns
					   = 511750 ps */
	uint32_t tRRD_ps;	/* maximum = 63750 ps */
	uint32_t tRC_ps;	/* maximum = 254 ns + .75 ns = 254750 ps */
	uint32_t refresh_rate_ps;
	uint32_t tIS_ps;	/* byte 32, spd->ca_setup */
	uint32_t tIH_ps;	/* byte 33, spd->ca_hold */
	uint32_t tDS_ps;	/* byte 34, spd->data_setup */
	uint32_t tDH_ps;	/* byte 35, spd->data_hold */
	uint32_t tRTP_ps;	/* byte 38, spd->trtp */
	uint32_t tDQSQ_max_ps;	/* byte 44, spd->tdqsq */
	uint32_t tQHS_ps;	/* byte 45, spd->tqhs */
	uint32_t ndimms_present;
	uint32_t lowest_common_SPD_caslat;
	uint32_t highest_common_derated_caslat;
	uint32_t additive_latency;
	uint32_t all_DIMMs_burst_lengths_bitmask;
	uint32_t all_DIMMs_registered;
	uint32_t all_DIMMs_ECC_capable;
	uint64_t total_mem;
	uint64_t base_address;
};

#endif
