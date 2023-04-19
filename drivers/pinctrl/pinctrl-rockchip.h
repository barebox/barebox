/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021 Rockchip Electronics Co. Ltd.
 *
 * Copyright (c) 2013 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * With some ideas taken from pinctrl-samsung:
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 * Copyright (c) 2012 Linaro Ltd
 *		https://www.linaro.org
 *
 * and pinctrl-at91:
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#ifndef _PINCTRL_ROCKCHIP_H
#define _PINCTRL_ROCKCHIP_H

enum rockchip_pinctrl_type {
	RK2928,
	RK3066B,
	RK3188,
	RK3568,
};

/**
 * struct rockchip_iomux
 * @type: iomux variant using IOMUX_* constants
 * @offset: if initialized to -1 it will be autocalculated, by specifying
 *	    an initial offset value the relevant source offset can be reset
 *	    to a new value for autocalculating the following iomux registers.
 */
struct rockchip_iomux {
	int type;
	int offset;
};

/*
 * enum type index corresponding to rockchip_perpin_drv_list arrays index.
 */
enum rockchip_pin_drv_type {
	DRV_TYPE_IO_DEFAULT = 0,
	DRV_TYPE_IO_1V8_OR_3V0,
	DRV_TYPE_IO_1V8_ONLY,
	DRV_TYPE_IO_1V8_3V0_AUTO,
	DRV_TYPE_IO_3V3_ONLY,
	DRV_TYPE_MAX
};

/*
 * enum type index corresponding to rockchip_pull_list arrays index.
 */
enum rockchip_pin_pull_type {
	PULL_TYPE_IO_DEFAULT = 0,
	PULL_TYPE_IO_1V8_ONLY,
	PULL_TYPE_MAX
};

/**
 * struct rockchip_drv
 * @drv_type: drive strength variant using rockchip_perpin_drv_type
 * @offset: if initialized to -1 it will be autocalculated, by specifying
 *	    an initial offset value the relevant source offset can be reset
 *	    to a new value for autocalculating the following drive strength
 *	    registers. if used chips own cal_drv func instead to calculate
 *	    registers offset, the variant could be ignored.
 */
struct rockchip_drv {
	enum rockchip_pin_drv_type	drv_type;
	int				offset;
};

enum rockchip_pin_bank_type {
	COMMON_BANK,
	RK3188_BANK0,
};

/**
 * struct rockchip_pin_bank
 * @reg_base: register base of the gpio bank
 * @clk: clock of the gpio bank
 * @pin_base: first pin number
 * @nr_pins: number of pins in this bank
 * @name: name of the bank
 * @bank_num: number of the bank, to account for holes
 * @iomux: array describing the 4 iomux sources of the bank
 * @drv: array describing the 4 drive strength sources of the bank
 * @valid: is all necessary information present
 * @of_node: dt node of this bank
 * @drvdata: common pinctrl basedata
 * @route_mask: bits describing the routing pins of per bank
 */
struct rockchip_pin_bank {
	void __iomem			*reg_base;
	struct clk			*clk;
	u32				pin_base;
	u8				nr_pins;
	char				*name;
	u8				bank_num;
	struct rockchip_iomux		iomux[4];
	struct rockchip_drv		drv[4];
	enum rockchip_pin_bank_type	bank_type;
	bool				valid;
	struct device_node		*of_node;
	struct rockchip_pinctrl		*drvdata;
	struct bgpio_chip		bgpio_chip;
	u32				route_mask;
};

enum rockchip_mux_route_location {
	ROCKCHIP_ROUTE_SAME = 0,
	ROCKCHIP_ROUTE_PMU,
	ROCKCHIP_ROUTE_GRF,
};

/**
 * struct rockchip_mux_recalced_data: represent a pin iomux data.
 * @bank_num: bank number.
 * @pin: index at register or used to calc index.
 * @func: the min pin.
 * @route_location: the mux route location (same, pmu, grf).
 * @route_offset: the max pin.
 * @route_val: the register offset.
 */
struct rockchip_mux_route_data {
	u8 bank_num;
	u8 pin;
	u8 func;
	enum rockchip_mux_route_location route_location;
	u32 route_offset;
	u32 route_val;
};

struct rockchip_pin_ctrl {
	struct rockchip_pin_bank	*pin_banks;
	u32				nr_banks;
	u32				nr_pins;
	char				*label;
	enum rockchip_pinctrl_type	type;
	int				grf_mux_offset;
	int				pmu_mux_offset;
	int				grf_drv_offset;
	int				pmu_drv_offset;
	struct rockchip_mux_route_data *iomux_routes;
	u32				niomux_routes;
	void	(*pull_calc_reg)(struct rockchip_pin_bank *bank, int pin_num,
				 void __iomem **reg, u8 *bit);
	void	(*drv_calc_reg)(struct rockchip_pin_bank *bank,
				    int pin_num, void __iomem **reg, u8 *bit);
};

struct rockchip_pinctrl {
	void __iomem			*reg_base;
	void __iomem			*reg_pmu;
	struct pinctrl_device		pctl_dev;
	struct rockchip_pin_ctrl	*ctrl;
};

#endif
