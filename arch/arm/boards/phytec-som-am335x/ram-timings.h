/*
 * Copyright (C) 2015 Wadim Egorov, PHYTEC Messtechnik GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RAM_TIMINGS_H
#define __RAM_TIMINGS_H

struct am335x_sdram_timings {
	struct am33xx_emif_regs regs;
	struct am33xx_ddr_data data;
};

enum {
	PHYFLEX_MT41K128M16JT_256MB,
	PHYFLEX_MT41K256M16HA_512MB,

	PHYCORE_MT41J128M16125IT_256MB,
	PHYCORE_MT41J64M1615IT_128MB,
	PHYCORE_MT41J256M16HA15EIT_512MB,
	PHYCORE_MT41J512M8125IT_2x512MB,
	PHYCORE_R2_MT41K256M16TW107IT_512MB,
	PHYCORE_R2_MT41K128M16JT_256MB,
	PHYCORE_R2_MT41K512M16HA125IT_1024MB,

	PHYCARD_NT5CB128M16BP_256MB,
};

struct am335x_sdram_timings physom_timings[] = {
	/* 256 MB */
	[PHYFLEX_MT41K128M16JT_256MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x26437FDA,
			.emif_tim3		= 0x501F83FF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C052B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x34,
			.wr_dqs_slave_ratio0	= 0x47,
			.fifo_we_slave_ratio0	= 0x9a,
			.wr_slave_ratio0	= 0x7e,
			.use_rank0_delay	= 0x0,
			.dll_lock_diff0		= 0x0,
		},
	},

	/* 512 MB */
	[PHYFLEX_MT41K256M16HA_512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C05332,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x36,
			.wr_dqs_slave_ratio0	= 0x47,
			.fifo_we_slave_ratio0	= 0x95,
			.wr_slave_ratio0	= 0x7f,
			.use_rank0_delay	= 0x0,
			.dll_lock_diff0		= 0x0,
		},
	},

	/* 256MB */
	[PHYCORE_MT41J128M16125IT_256MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x26437FDA,
			.emif_tim3		= 0x501F83FF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C052B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x3B,
			.wr_dqs_slave_ratio0	= 0x33,
			.fifo_we_slave_ratio0	= 0x9c,
			.wr_slave_ratio0	= 0x6f,
		},
	},

	/* 128MB */
	[PHYCORE_MT41J64M1615IT_128MB] = {
		.regs =  {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x262F7FDA,
			.emif_tim3		= 0x501F82BF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C05232,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x38,
			.wr_dqs_slave_ratio0	= 0x34,
			.fifo_we_slave_ratio0	= 0xA2,
			.wr_slave_ratio0	= 0x72,
		},
	},

	/* 512MB */
	[PHYCORE_MT41J256M16HA15EIT_512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C05332,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30
		},
		.data = {
			.rd_slave_ratio0	= 0x35,
			.wr_dqs_slave_ratio0	= 0x43,
			.fifo_we_slave_ratio0	= 0x97,
			.wr_slave_ratio0	= 0x7b,
		},
	},

	/* 2x512MB */
	[PHYCORE_MT41J512M8125IT_2x512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C053B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30
		},
		.data = {
			.rd_slave_ratio0	= 0x32,
			.wr_dqs_slave_ratio0	= 0x48,
			.fifo_we_slave_ratio0	= 0x99,
			.wr_slave_ratio0	= 0x80,
		},
	},

	/* 256MB */
	[PHYCARD_NT5CB128M16BP_256MB] = {
		.regs = {
			.emif_read_latency      = 0x7,
			.emif_tim1              = 0x0AAAD4DB,
			.emif_tim2              = 0x26437FDA,
			.emif_tim3              = 0x501F83FF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config           = 0x61C052B2,
			.zq_config              = 0x50074BE4,
			.sdram_ref_ctrl         = 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0        = 0x35,
			.wr_dqs_slave_ratio0    = 0x3A,
			.fifo_we_slave_ratio0   = 0x9b,
			.wr_slave_ratio0        = 0x73,
			.use_rank0_delay        = 0x01,
			.dll_lock_diff0         = 0x0,
		},
	},

	/* 512MB R2 */
	[PHYCORE_R2_MT41K256M16TW107IT_512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C05332,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x37,
			.wr_dqs_slave_ratio0	= 0x38,
			.fifo_we_slave_ratio0	= 0x92,
			.wr_slave_ratio0	= 0x72,
		},
	},

	/* 256MB R2 */
	[PHYCORE_R2_MT41K128M16JT_256MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x26437FDA,
			.emif_tim3		= 0x501F83FF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C052B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x36,
			.wr_dqs_slave_ratio0	= 0x38,
			.fifo_we_slave_ratio0	= 0x99,
			.wr_slave_ratio0	= 0x73,
		},
	},

	/* 1024MB R2 */
	[PHYCORE_R2_MT41K512M16HA125IT_1024MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x268F7FDA,
			.emif_tim3		= 0x501F88BF,
			.ocp_config		= 0x003d3d3d,
			.sdram_config		= 0x61C053B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x38,
			.wr_dqs_slave_ratio0	= 0x4d,
			.fifo_we_slave_ratio0	= 0x9d,
			.wr_slave_ratio0	= 0x82,
		},
	},
};

#endif
