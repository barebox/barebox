/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Xilinx Zynq MPSoC Firmware layer
 *
 * Copyright (c) 2018 Thomas Haemmerle <thomas.haemmerle@wolfvision.net>
 *
 * based on Linux xlnx-zynqmp
 *
 *  Michal Simek <michal.simek@xilinx.com>
 *  Davorin Mista <davorin.mista@aggios.com>
 *  Jolly Shah <jollys@xilinx.com>
 *  Rajan Vaja <rajanv@xilinx.com>
 */

#ifndef FIRMWARE_ZYNQMP_H_
#define FIRMWARE_ZYNQMP_H_

enum pm_ioctl_id {
	IOCTL_SET_PLL_FRAC_MODE = 8,
	IOCTL_GET_PLL_FRAC_MODE,
	IOCTL_SET_PLL_FRAC_DATA,
	IOCTL_GET_PLL_FRAC_DATA,
};

enum pm_query_id {
	PM_QID_INVALID,
	PM_QID_CLOCK_GET_NAME,
	PM_QID_CLOCK_GET_TOPOLOGY,
	PM_QID_CLOCK_GET_FIXEDFACTOR_PARAMS,
	PM_QID_CLOCK_GET_PARENTS,
	PM_QID_CLOCK_GET_ATTRIBUTES,
	PM_QID_CLOCK_GET_NUM_CLOCKS = 12,
};

/**
 * struct zynqmp_pm_query_data - PM query data
 * @qid:	query ID
 * @arg1:	Argument 1 of query data
 * @arg2:	Argument 2 of query data
 * @arg3:	Argument 3 of query data
 */
struct zynqmp_pm_query_data {
	u32 qid;
	u32 arg1;
	u32 arg2;
	u32 arg3;
};

struct zynqmp_eemi_ops {
	int (*get_api_version)(u32 *version);
	int (*query_data)(struct zynqmp_pm_query_data qdata, u32 *out);
	int (*clock_enable)(u32 clock_id);
	int (*clock_disable)(u32 clock_id);
	int (*clock_getstate)(u32 clock_id, u32 *state);
	int (*clock_setdivider)(u32 clock_id, u32 divider);
	int (*clock_getdivider)(u32 clock_id, u32 *divider);
	int (*clock_setrate)(u32 clock_id, u64 rate);
	int (*clock_getrate)(u32 clock_id, u64 *rate);
	int (*clock_setparent)(u32 clock_id, u32 parent_id);
	int (*clock_getparent)(u32 clock_id, u32 *parent_id);
	int (*ioctl)(u32 node_id, u32 ioctl_id, u32 arg1, u32 arg2, u32 *out);
};

const struct zynqmp_eemi_ops *zynqmp_pm_get_eemi_ops(void);

#endif /* FIRMWARE_ZYNQMP_H_ */
