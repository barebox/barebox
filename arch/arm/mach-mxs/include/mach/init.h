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
 * - POWER_ENABLE_4P2: power up the 4P2 regulator (implied for POWER_USE_5V)
 */
enum mxs_power_config {
	POWER_USE_5V			= 0b00000000,
	POWER_USE_BATTERY		= 0b00000001,
	POWER_USE_BATTERY_INPUT		= 0b00000010,
	POWER_ENABLE_4P2		= 0b00000100,
};
extern int power_config;
static inline enum mxs_power_config mxs_power_config_get_use(void) {
	return (power_config & 0b00000011);
}


struct mxs_power_ctrl {
	uint32_t target;	/*< target voltage */
	uint32_t brownout;	/*< brownout threshhold */
};
struct mxs_power_ctrls {
	struct mxs_power_ctrl * vdda;	/*< if non-null, set values for VDDA */
	struct mxs_power_ctrl * vddd;	/*< if non-null, set values for VDDD */
	struct mxs_power_ctrl * vddio;	/*< if non-null, set values for VDDIO */
	struct mxs_power_ctrl * vddmem;	/*< if non-null, set values for VDDMEM */
};

extern struct mxs_power_ctrl mxs_vddio_default;
extern struct mxs_power_ctrl mxs_vddd_default;
extern struct mxs_power_ctrl mxs_vdda_default;
extern struct mxs_power_ctrl mx23_vddmem_default;
extern struct mxs_power_ctrls mx23_power_default;
extern struct mxs_power_ctrls mx28_power_default;

void mx23_power_init(const int config, struct mxs_power_ctrls *ctrls);
void mx28_power_init(const int config, struct mxs_power_ctrls *ctrls);
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
