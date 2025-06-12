/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef XILINX_FIRMWARE_H
#define XILINX_FIRMWARE_H

#include <firmware.h>

#define DUMMY_WORD			0xFFFFFFFF
#define BUS_WIDTH_AUTO_DETECT1		0x000000BB
#define BUS_WIDTH_AUTO_DETECT2		0x11220044
#define SYNC_WORD			0xAA995566

enum xilinx_byte_order {
	XILINX_BYTE_ORDER_BIT,
	XILINX_BYTE_ORDER_BIN,
};

struct fpgamgr;

struct xilinx_fpga_devdata {
	u8 bus_width_auto_detect1_offset;
	u8 bus_width_auto_detect2_offset;
	u8 sync_word_offset;
	u8 bin_header_length;
	int (*dev_init)(struct fpgamgr *, struct device *);
	int (*dev_programmed_get)(struct fpgamgr *);
	int (*dev_fpga_load)(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags);
};

struct fpgamgr {
	struct firmware_handler fh;
	struct device dev;
	void *private;
	int programmed;
	char *buf;
	size_t size;
	u32 features;
	const struct xilinx_fpga_devdata *devdata;
};

struct bs_header {
	__be16 length;
	u8 padding[9];
	__be16 size;
	char entries[0];
} __attribute__ ((packed));

struct bs_header_entry {
	char type;
	__be16 length;
	char data[0];
} __attribute__ ((packed));

#endif /* XILINX_FIRMWARE_H */
