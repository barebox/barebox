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

#define PAYLOAD_ARG_CNT			4

#define ZYNQMP_PM_VERSION(MAJOR, MINOR)	((MAJOR << 16) | MINOR)

#define ZYNQMP_FPGA_BIT_AUTH_DDR	BIT(1)
#define ZYNQMP_FPGA_BIT_AUTH_OCM	BIT(2)
#define ZYNQMP_FPGA_BIT_ENC_USR_KEY	BIT(3)
#define ZYNQMP_FPGA_BIT_ENC_DEV_KEY	BIT(4)
#define ZYNQMP_FPGA_BIT_ONLY_BIN	BIT(5)

#define ZYNQMP_PCAP_STATUS_FPGA_DONE	BIT(3)

enum pm_ioctl_id {
	IOCTL_GET_RPU_OPER_MODE = 0,
	IOCTL_SET_RPU_OPER_MODE = 1,
	IOCTL_RPU_BOOT_ADDR_CONFIG = 2,
	IOCTL_TCM_COMB_CONFIG = 3,
	IOCTL_SET_TAPDELAY_BYPASS = 4,
	IOCTL_SD_DLL_RESET = 6,
	IOCTL_SET_SD_TAPDELAY = 7,
	IOCTL_SET_PLL_FRAC_MODE = 8,
	IOCTL_GET_PLL_FRAC_MODE = 9,
	IOCTL_SET_PLL_FRAC_DATA = 10,
	IOCTL_GET_PLL_FRAC_DATA = 11,
	IOCTL_WRITE_GGS = 12,
	IOCTL_READ_GGS = 13,
	IOCTL_WRITE_PGGS = 14,
	IOCTL_READ_PGGS = 15,
	/* Set healthy bit value */
	IOCTL_SET_BOOT_HEALTH_STATUS = 17,
	IOCTL_OSPI_MUX_SELECT = 21,
	/* Register SGI to ATF */
	IOCTL_REGISTER_SGI = 25,
	/* Runtime feature configuration */
	IOCTL_SET_FEATURE_CONFIG = 26,
	IOCTL_GET_FEATURE_CONFIG = 27,
	/* Dynamic SD/GEM configuration */
	IOCTL_SET_SD_CONFIG = 30,
	IOCTL_SET_GEM_CONFIG = 31,
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
	int (*fpga_getstatus)(u32 *status);
	int (*fpga_load)(u64 address, u32 size, u32 flags);
};

const struct zynqmp_eemi_ops *zynqmp_pm_get_eemi_ops(void);

int zynqmp_pm_write_ggs(u32 index, u32 value);
int zynqmp_pm_read_ggs(u32 index, u32 *value);
int zynqmp_pm_write_pggs(u32 index, u32 value);
int zynqmp_pm_read_pggs(u32 index, u32 *value);

#endif /* FIRMWARE_ZYNQMP_H_ */
