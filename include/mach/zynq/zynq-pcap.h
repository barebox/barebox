/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Xilinx Zynq Firmware layer
 */

#ifndef ZYNQ_PCAP_H_
#define ZYNQ_PCAP_H_

#include <xilinx-firmware.h>

#define ZYNQ_BUS_WIDTH_AUTO_DETECT1_OFFSET	8
#define ZYNQ_BUS_WIDTH_AUTO_DETECT2_OFFSET	9
#define ZYNQ_SYNC_WORD_OFFSET			12
#define ZYNQ_BIN_HEADER_LENGTH			13

#define ZYNQ_PM_FEATURE_SIZE_NOT_NEEDED		BIT(1)

#define CTRL_OFFSET				0x00
#define LOCK_OFFSET				0x04
#define INT_STS_OFFSET				0x0c
#define INT_MASK_OFFSET				0x10
#define STATUS_OFFSET				0x14
#define DMA_SRC_ADDR_OFFSET			0x18
#define DMA_DST_ADDR_OFFSET			0x1c
#define DMA_SRC_LEN_OFFSET			0x20
#define DMA_DEST_LEN_OFFSET			0x24
#define UNLOCK_OFFSET				0x34
#define MCTRL_OFFSET				0x80

#define CTRL_PCFG_PROG_B_MASK			BIT(30)
#define CTRL_PCAP_PR_MASK			BIT(27)
#define CTRL_PCAP_MODE_MASK			BIT(26)
#define CTRL_PCAP_RATE_EN_MASK			BIT(25)

#define STATUS_DMA_CMD_Q_F_MASK			BIT(31)
#define STATUS_PCFG_INIT_MASK			BIT(4)

#define INT_STS_D_P_DONE_MASK			BIT(12)
#define INT_STS_DONE_INT_MASK			BIT(2)
#define INT_STS_ERROR_FLAGS_MASK		0x00f4c860

#define MCTRL_INT_PCAP_LPBK_MASK		BIT(4)
#define DEVC_UNLOCK_CODE			0x757bdf0d

#if defined(CONFIG_FIRMWARE_ZYNQ7000_FPGA)
int zynq_init(struct fpgamgr *mgr, struct device *dev);
int zynq_programmed_get(struct fpgamgr *mgr);
int zynq_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags);
#else
static inline int zynq_init(struct fpgamgr *mgr, struct device *dev)
{
	return -ENOSYS;
}
static inline int zynq_programmed_get(struct fpgamgr *mgr)
{
	return -ENOSYS;
}
static inline int zynq_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags)
{
	return -ENOSYS;
}
#endif

#endif /* ZYNQ_PCAP_H_ */
