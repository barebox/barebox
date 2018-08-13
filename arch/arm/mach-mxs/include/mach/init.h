/*
 * Freescale i.MX28 SPL functions
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef	__M28_INIT_H__
#define	__M28_INIT_H__

void mxs_early_delay(int delay);

/**
 * Power configuration of the system:
 * - POWER_USE_5V: use 5V input as power supply
 * - POWER_USE_BATTERY: use battery input when the system is supplied by a battery
 * - POWER_USE_BATTERY_INPUT: use battery input when the system is supplied by
 *   a DC source (instead of a real battery) on the battery input
 */
enum mxs_power_config {
	POWER_USE_5V			= 0b00000000,
	POWER_USE_BATTERY		= 0b00000001,
	POWER_USE_BATTERY_INPUT		= 0b00000010,
};
extern int power_config;
static inline enum mxs_power_config mxs_power_config_get_use(void) {
	return (power_config & 0b00000011);
}


void mx23_power_init(const int config);
void mx28_power_init(const int config);
void mxs_power_wait_pswitch(void);

extern const uint32_t mx28_dram_vals_default[190];
extern uint32_t mx23_dram_vals[];

#define PINCTRL_EMI_DS_CTRL_DDR_MODE_LPDDR1	(0b00 << 16)
#define PINCTRL_EMI_DS_CTRL_DDR_MODE_LVDDR2	(0b10 << 16)
#define PINCTRL_EMI_DS_CTRL_DDR_MODE_DDR2	(0b11 << 16)

void mx23_mem_init(void);
void mx28_mem_init(const int emi_ds_ctrl_ddr_mode,
		const uint32_t dram_vals[190]);
void mxs_mem_setup_cpu_and_hbus(void);
void mxs_mem_setup_vdda(void);
void mxs_mem_init_clock(const uint8_t clk_emi_div, const uint8_t clk_emi_frac);

void mxs_lradc_init(void);
void mxs_lradc_enable_batt_measurement(void);

#endif	/* __M28_INIT_H__ */
