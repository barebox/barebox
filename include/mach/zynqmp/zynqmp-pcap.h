/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Xilinx ZynqMP PL loading layer
 */

#ifndef ZYNQMP_PCAP_H_
#define ZYNQMP_PCAP_H_

#include <xilinx-firmware.h>

#define ZYNQMP_PM_FEATURE_BYTE_ORDER_IRREL	BIT(0)
#define ZYNQMP_PM_FEATURE_SIZE_NOT_NEEDED	BIT(1)

#define ZYNQMP_PM_VERSION_1_0_FEATURES	0
#define ZYNQMP_PM_VERSION_1_1_FEATURES	(ZYNQMP_PM_FEATURE_BYTE_ORDER_IRREL | \
					 ZYNQMP_PM_FEATURE_SIZE_NOT_NEEDED)

#define ZYNQMP_BUS_WIDTH_AUTO_DETECT1_OFFSET	16
#define ZYNQMP_BUS_WIDTH_AUTO_DETECT2_OFFSET	17
#define ZYNQMP_SYNC_WORD_OFFSET			20
#define ZYNQMP_BIN_HEADER_LENGTH		21

#if defined(CONFIG_FIRMWARE_ZYNQMP_FPGA)
int zynqmp_init(struct fpgamgr *mgr, struct device *dev);
int zynqmp_programmed_get(struct fpgamgr *mgr);
int zynqmp_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags);
#else
static inline int zynqmp_init(struct fpgamgr *mgr, struct device *dev)
{
	return -ENOSYS;
}
static inline int zynqmp_programmed_get(struct fpgamgr *mgr)
{
	return -ENOSYS;
}
static inline int zynqmp_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags)
{
	return -ENOSYS;
}
#endif
#endif /* ZYNQMP_PCAP_H_ */
