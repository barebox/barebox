/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef FSL_DDR_MEMCTL_H
#define FSL_DDR_MEMCTL_H

#include <ddr_spd.h>

/* Basic DDR Technology. */
#define SDRAM_TYPE_DDR1    2
#define SDRAM_TYPE_DDR2    3
#define SDRAM_TYPE_LPDDR1  6
#define SDRAM_TYPE_DDR3    7

#define DDR_BL4	4
#define DDR_BL8	8

#define DDR2_RTT_OFF		0
#define DDR2_RTT_75_OHM		1
#define DDR2_RTT_150_OHM	2
#define DDR2_RTT_50_OHM		3

#if defined(CONFIG_FSL_DDR2)
#define FSL_DDR_MIN_TCKE_PULSE_WIDTH_DDR	(3)
typedef struct ddr2_spd_eeprom_s generic_spd_eeprom_t;
#define FSL_SDRAM_TYPE	SDRAM_TYPE_DDR2
#endif

#define FSL_DDR_ODT_NEVER		0x0
#define FSL_DDR_ODT_CS			0x1
#define FSL_DDR_ODT_ALL_OTHER_CS	0x2
#define FSL_DDR_ODT_OTHER_DIMM		0x3
#define FSL_DDR_ODT_ALL			0x4
#define FSL_DDR_ODT_SAME_DIMM		0x5
#define FSL_DDR_ODT_CS_AND_OTHER_DIMM	0x6
#define FSL_DDR_ODT_OTHER_CS_ONSAMEDIMM	0x7

#define SDRAM_CS_CONFIG_EN		0x80000000

/* DDR_SDRAM_CFG - DDR SDRAM Control Configuration */
#define SDRAM_CFG_MEM_EN		0x80000000
#define SDRAM_CFG_SREN			0x40000000
#define SDRAM_CFG_ECC_EN		0x20000000
#define SDRAM_CFG_RD_EN			0x10000000
#define SDRAM_CFG_SDRAM_TYPE_DDR1	0x02000000
#define SDRAM_CFG_SDRAM_TYPE_DDR2	0x03000000
#define SDRAM_CFG_SDRAM_TYPE_MASK	0x07000000
#define SDRAM_CFG_SDRAM_TYPE_SHIFT	24
#define SDRAM_CFG_DYN_PWR		0x00200000
#define SDRAM_CFG_32_BE			0x00080000
#define SDRAM_CFG_16_BE			0x00100000
#define SDRAM_CFG_8_BE			0x00040000
#define SDRAM_CFG_NCAP			0x00020000
#define SDRAM_CFG_2T_EN			0x00008000
#define SDRAM_CFG_BI			0x00000001

#define SDRAM_CFG2_D_INIT		0x00000010
#define SDRAM_CFG2_ODT_CFG_MASK		0x00600000
#define SDRAM_CFG2_ODT_NEVER		0
#define SDRAM_CFG2_ODT_ONLY_WRITE	1
#define SDRAM_CFG2_ODT_ONLY_READ	2
#define SDRAM_CFG2_ODT_ALWAYS		3

#define MAX_CHIP_SELECTS_PER_CTRL	4
#define MAX_DIMM_SLOTS_PER_CTLR		4

/*
 * Memory controller characteristics and I2C parameters to
 * read the SPD data.
 */
struct ddr_board_info_s {
	uint32_t fsl_ddr_ver;
	void __iomem *ddr_base;
	uint32_t cs_per_ctrl;
	uint32_t dimm_slots_per_ctrl;
	uint32_t i2c_bus;
	uint32_t i2c_slave;
	uint32_t i2c_speed;
	void __iomem *i2c_base;
	uint8_t *spd_i2c_addr;
};

/*
 * Generalized parameters for memory controller configuration,
 * might be a little specific to the FSL memory controller
 */
struct memctl_options_s {
	struct ddr_board_info_s *board_info;
	uint32_t sdram_type;
	/*
	 * Memory organization parameters
	 *
	 * if DIMM is present in the system
	 * where DIMMs are with respect to chip select
	 * where chip selects are with respect to memory boundaries
	 */
	uint32_t registered_dimm_en;
	/* Options local to a Chip Select */
	struct cs_local_opts_s {
		uint32_t auto_precharge;
		uint32_t odt_rd_cfg;
		uint32_t odt_wr_cfg;
		uint32_t odt_rtt_norm;
		uint32_t odt_rtt_wr;
	} cs_local_opts[MAX_CHIP_SELECTS_PER_CTRL];
	/* DLL reset disable */
	uint32_t dll_rst_dis;
	/* Operational mode parameters */
	uint32_t ECC_mode;
	uint32_t ECC_init_using_memctl;
	uint32_t data_init;
	/* Use DQS? maybe only with DDR2? */
	uint32_t DQS_config;
	uint32_t self_refresh_in_sleep;
	uint32_t dynamic_power;
	uint32_t data_bus_width;
	uint32_t burst_length;
	/* Global Timing Parameters */
	uint32_t cas_latency_override;
	uint32_t cas_latency_override_value;
	uint32_t use_derated_caslat;
	uint32_t additive_latency_override;
	uint32_t additive_latency_override_value;
	uint32_t clk_adjust;
	uint32_t cpo_override;
	uint32_t write_data_delay;
	uint32_t half_strength_driver_enable;
	uint32_t twoT_en;
	uint32_t bstopre;
	uint32_t tCKE_clock_pulse_width_ps;
	uint32_t tFAW_window_four_activates_ps;
	/* Rtt impedance */
	uint32_t rtt_override;
	uint32_t rtt_override_value;
	/* Automatic self refresh */
	uint32_t auto_self_refresh_en;
	/* read-to-write turnaround */
	uint32_t trwt_override;
	uint32_t trwt;
	/* Powerdon timings. */
	uint32_t txp;
	uint32_t taxpd;
	uint32_t txard;
	/* Load mode cycle time */
	uint32_t tmrd;
};

extern phys_size_t fsl_ddr_sdram(void);
extern phys_size_t fixed_sdram(void);
#endif
