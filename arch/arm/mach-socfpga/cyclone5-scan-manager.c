/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <mach/cyclone5-freeze-controller.h>
#include <mach/cyclone5-scan-manager.h>

/*
 * @fn scan_mgr_io_scan_chain_engine_is_idle
 *
 * @brief function to check IO scan chain engine status and wait if the
 *        engine is active. Poll the IO scan chain engine till maximum iteration
 *        reached.
 *
 * @param max_iter uint32_t [in] - maximum polling loop to revent infinite loop
 */
static int scan_mgr_io_scan_chain_engine_is_idle(uint32_t max_iter)
{
	uint32_t scanmgr_status;

	scanmgr_status = readl(SCANMGR_STAT_ADDRESS +
		CYCLONE5_SCANMGR_ADDRESS);

	/* Poll the engine until the scan engine is inactive */
	while (SCANMGR_STAT_ACTIVE_GET(scanmgr_status)
		|| (SCANMGR_STAT_WFIFOCNT_GET(scanmgr_status) > 0)) {

		max_iter--;

		if (max_iter > 0) {
			scanmgr_status = readl(
				CYCLONE5_SCANMGR_ADDRESS +
				SCANMGR_STAT_ADDRESS);
		} else {
			return 0;
		}
	}
	return 1;
}

/*
 * scan_mgr_io_scan_chain_prg
 * Program HPS IO Scan Chain
 */
int scan_mgr_io_scan_chain_prg(enum io_scan_chain io_scan_chain_id,
		uint32_t io_scan_chain_len_in_bits,
		const unsigned long *iocsr_scan_chain)
{
	uint16_t tdi_tdo_header;
	uint32_t io_program_iter;
	uint32_t io_scan_chain_data_residual;
	uint32_t residual;
	uint32_t i;
	uint32_t index = 0;
	uint32_t val;
	int ret;
	void __iomem *sysmgr = (void *)CYCLONE5_SYSMGR_ADDRESS;
	void __iomem *scanmgr = (void *)CYCLONE5_SCANMGR_ADDRESS;

	/* De-assert reinit if the IO scan chain is intended for HIO */
	if (io_scan_chain_id == IO_SCAN_CHAIN_3) {
		val = readl(sysmgr + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
		val &= ~SYSMGR_FRZCTRL_HIOCTRL_DLLRST_MASK;
		writel(val, sysmgr + SYSMGR_FRZCTRL_HIOCTRL_ADDRESS);
	} /* if (HIO) */

	/*
	 * Check if the scan chain engine is inactive and the
	 * WFIFO is empty before enabling the IO scan chain
	 */
	if (!scan_mgr_io_scan_chain_engine_is_idle(MAX_WAITING_DELAY_IO_SCAN_ENGINE))
		return -EBUSY;

	/*
	 * Enable IO Scan chain based on scan chain id
	 * Note: only one chain can be enabled at a time
	 */
	val = readl(scanmgr + SCANMGR_EN_ADDRESS);
	val |= 1 << io_scan_chain_id;
	writel(val, scanmgr + SCANMGR_EN_ADDRESS);

	/*
	 * Calculate number of iteration needed for
	 * full 128-bit (4 x32-bits) bits shifting.
	 * Each TDI_TDO packet can shift in maximum 128-bits
	 */
	io_program_iter = io_scan_chain_len_in_bits >> IO_SCAN_CHAIN_128BIT_SHIFT;
	io_scan_chain_data_residual = io_scan_chain_len_in_bits & IO_SCAN_CHAIN_128BIT_MASK;

	/*
	 * Construct TDI_TDO packet for
	 * 128-bit IO scan chain (2 bytes)
	 */
	tdi_tdo_header = TDI_TDO_HEADER_FIRST_BYTE |
		(TDI_TDO_MAX_PAYLOAD << TDI_TDO_HEADER_SECOND_BYTE_SHIFT);

	/* Program IO scan chain in 128-bit iteration */
	for (i = 0; i < io_program_iter; i++) {

		/* write TDI_TDO packet header to scan manager */
		writel(tdi_tdo_header, (scanmgr + SCANMGR_FIFODOUBLEBYTE_ADDRESS));

		/* calculate array index */
		index = i * 4;

		/*
		 * write 4 successive 32-bit IO scan
		 * chain data into WFIFO
		 */
		writel(iocsr_scan_chain[index], (scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));
		writel(iocsr_scan_chain[index + 1], (scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));
		writel(iocsr_scan_chain[index + 2], (scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));
		writel(iocsr_scan_chain[index + 3], (scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));

		/*
		 * Check if the scan chain engine has completed the
		 * IO scan chain data shifting
		 */
		if (!scan_mgr_io_scan_chain_engine_is_idle(MAX_WAITING_DELAY_IO_SCAN_ENGINE)) {
			ret = -EBUSY;
			goto out_disable;
		}
	}

	/* Calculate array index for final TDI_TDO packet */
	index = io_program_iter * 4;

	/* Final TDI_TDO packet if any */
	if (0 != io_scan_chain_data_residual) {
		/*
		 * Calculate number of quad bytes FIFO write
		 * needed for the final TDI_TDO packet
		 */
		io_program_iter = io_scan_chain_data_residual >> IO_SCAN_CHAIN_32BIT_SHIFT;

		/*
		 * Construct TDI_TDO packet for remaining IO
		 * scan chain (2 bytes)
		 */
		tdi_tdo_header = TDI_TDO_HEADER_FIRST_BYTE |
			((io_scan_chain_data_residual - 1) << TDI_TDO_HEADER_SECOND_BYTE_SHIFT);

		/*
		 * Program the last part of IO scan chain
		 * write TDI_TDO packet header (2 bytes) to
		 * scan manager
		 */
		writel(tdi_tdo_header, (scanmgr + SCANMGR_FIFODOUBLEBYTE_ADDRESS));

		for (i = 0; i < io_program_iter; i++) {

			/*
			 * write remaining scan chain data into scan
			 * manager WFIFO with 4 bytes write
			*/
			writel(iocsr_scan_chain[index + i],
					(scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));
		}

		index += io_program_iter;
		residual = io_scan_chain_data_residual & IO_SCAN_CHAIN_32BIT_MASK;

		if (IO_SCAN_CHAIN_PAYLOAD_24BIT < residual) {
			/*
			 * write the last 4B scan chain data
			 * into scan manager WFIFO
			 */
			writel(iocsr_scan_chain[index],
					(scanmgr + SCANMGR_FIFOQUADBYTE_ADDRESS));
		} else {
			/*
			 * write the remaining 1 - 3 bytes scan chain
			 * data into scan manager WFIFO byte by byte
			 * to prevent JTAG engine shifting unused data
			 * from the FIFO and mistaken the data as a
			 * valid command (even though unused bits are
			 * set to 0, but just to prevent hardware
			 * glitch)
			 */
			for (i = 0; i < residual; i += 8) {
				writel(((iocsr_scan_chain[index] >> i) & IO_SCAN_CHAIN_BYTE_MASK),
						(scanmgr + SCANMGR_FIFOSINGLEBYTE_ADDRESS));
			}
		}

		/*
		 * Check if the scan chain engine has completed the
		 * IO scan chain data shifting
		 */
		if (!scan_mgr_io_scan_chain_engine_is_idle(MAX_WAITING_DELAY_IO_SCAN_ENGINE)) {
			ret = -EBUSY;
			goto out_disable;
		}
	} /* if (io_scan_chain_data_residual) */

	ret = 0;

out_disable:
	/* Disable IO Scan chain when configuration done*/
	val = readl(scanmgr + SCANMGR_EN_ADDRESS);
	val &= ~(1 << io_scan_chain_id);
	writel(val, scanmgr + SCANMGR_EN_ADDRESS);

	return ret;
}
