/*
 * FPGA Framework
 *
 *  Copyright (C) 2013-2015 Altera Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _LINUX_FPGA_MGR_H
#define _LINUX_FPGA_MGR_H

#include <firmware.h>

struct fpga_manager;

/**
 * enum fpga_mgr_states - fpga framework states
 * @FPGA_MGR_STATE_UNKNOWN: can't determine state
 * @FPGA_MGR_STATE_POWER_OFF: FPGA power is off
 * @FPGA_MGR_STATE_POWER_UP: FPGA reports power is up
 * @FPGA_MGR_STATE_RESET: FPGA in reset state
 * @FPGA_MGR_STATE_FIRMWARE_REQ: firmware request in progress
 * @FPGA_MGR_STATE_FIRMWARE_REQ_ERR: firmware request failed
 * @FPGA_MGR_STATE_WRITE_INIT: preparing FPGA for programming
 * @FPGA_MGR_STATE_WRITE_INIT_ERR: Error during WRITE_INIT stage
 * @FPGA_MGR_STATE_WRITE: writing image to FPGA
 * @FPGA_MGR_STATE_WRITE_ERR: Error while writing FPGA
 * @FPGA_MGR_STATE_WRITE_COMPLETE: Doing post programming steps
 * @FPGA_MGR_STATE_WRITE_COMPLETE_ERR: Error during WRITE_COMPLETE
 * @FPGA_MGR_STATE_OPERATING: FPGA is programmed and operating
 */
enum fpga_mgr_states {
	/* default FPGA states */
	FPGA_MGR_STATE_UNKNOWN,
	FPGA_MGR_STATE_POWER_OFF,
	FPGA_MGR_STATE_POWER_UP,
	FPGA_MGR_STATE_RESET,

	/* getting an image for loading */
	FPGA_MGR_STATE_FIRMWARE_REQ,
	FPGA_MGR_STATE_FIRMWARE_REQ_ERR,

	/* write sequence: init, write, complete */
	FPGA_MGR_STATE_WRITE_INIT,
	FPGA_MGR_STATE_WRITE_INIT_ERR,
	FPGA_MGR_STATE_WRITE,
	FPGA_MGR_STATE_WRITE_ERR,
	FPGA_MGR_STATE_WRITE_COMPLETE,
	FPGA_MGR_STATE_WRITE_COMPLETE_ERR,

	/* fpga is programmed and operating */
	FPGA_MGR_STATE_OPERATING,
};

/*
 * FPGA Manager flags
 * FPGA_MGR_PARTIAL_RECONFIG: do partial reconfiguration if supported
 * FPGA_MGR_EXTERNAL_CONFIG: FPGA has been configured prior to Linux booting
 * FPGA_MGR_BITSTREAM_LSB_FIRST: SPI bitstream bit order is LSB first
 * FPGA_MGR_COMPRESSED_BITSTREAM: FPGA bitstream is compressed
 */
#define FPGA_MGR_PARTIAL_RECONFIG	BIT(0)
#define FPGA_MGR_EXTERNAL_CONFIG	BIT(1)
#define FPGA_MGR_ENCRYPTED_BITSTREAM	BIT(2)
#define FPGA_MGR_BITSTREAM_LSB_FIRST	BIT(3)
#define FPGA_MGR_COMPRESSED_BITSTREAM	BIT(4)

/**
 * struct fpga_image_info - information specific to a FPGA image
 * @flags: boolean flags as defined above
 * @enable_timeout_us: maximum time to enable traffic through bridge (uSec)
 * @disable_timeout_us: maximum time to disable traffic through bridge (uSec)
 * @config_complete_timeout_us: maximum time for FPGA to switch to operating
 *	   status in the write_complete op.
 */
struct fpga_image_info {
	u32 flags;
	u32 enable_timeout_us;
	u32 disable_timeout_us;
	u32 config_complete_timeout_us;
};

struct fpgamgr {
	struct firmware_handler fh;
	struct device_d dev;
	void *priv;
	void __iomem *regs;
	void __iomem *regs_data;
	int programmed;
};


#endif /*_LINUX_FPGA_MGR_H */
