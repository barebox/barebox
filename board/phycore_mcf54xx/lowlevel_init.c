/*
 * (C) 2007 konzeptpark, Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  @brief This file contains ...
 *
 */
#include <common.h>
#include <config.h>
#include <mach/mcf54xx-regs.h>

/** Initialize board specific very early inits
 *
 * @note This code is not allowed to call other code - just init
 * your Chipselects and SDRAM stuff here!
 */
void board_init_lowlevel(void)
{
	/*
	 * The phyCORE-MCF548x has a 32MB or 64MB boot flash.
	 * The is a CF Card and ControlRegs on CS1 and CS2
	 */

	/* Setup SysGlue Chip-Select for user IOs */
	MCF_FBCS_CSAR2 = MCF_FBCS_CSAR_BA(CFG_XPLD_ADDRESS);

	MCF_FBCS_CSCR2 = (MCF_FBCS_CSCR_PS_16
	                  | MCF_FBCS_CSCR_AA
	                  | MCF_FBCS_CSCR_ASET(1)
	                  | MCF_FBCS_CSCR_WS(CFG_XPLD_WAIT_STATES));

	MCF_FBCS_CSMR2 = (MCF_FBCS_CSMR_BAM_16M
	                  | MCF_FBCS_CSMR_V);

	/* Setup SysGlue Chip-Select for CFCARD */
	MCF_FBCS_CSAR1 = MCF_FBCS_CSAR_BA(CFG_CFCARD_ADDRESS);

	MCF_FBCS_CSCR1 = (MCF_FBCS_CSCR_PS_16
	                  | MCF_FBCS_CSCR_AA
	                  | MCF_FBCS_CSCR_ASET(1)
	                  | MCF_FBCS_CSCR_WS(CFG_CFCARD_WAIT_STATES));

	MCF_FBCS_CSMR1 = (MCF_FBCS_CSMR_BAM_16M
	                  | MCF_FBCS_CSMR_V);

	/* Setup boot flash chip-select */
	MCF_FBCS_CSAR0 = MCF_FBCS_CSAR_BA(CFG_FLASH_ADDRESS);

	MCF_FBCS_CSCR0 = (MCF_FBCS_CSCR_PS_32
	                  | MCF_FBCS_CSCR_AA
	                  | MCF_FBCS_CSCR_ASET(1)
	                  | MCF_FBCS_CSCR_WS(CFG_FLASH_WAIT_STATES));

	MCF_FBCS_CSMR0 = (MCF_FBCS_CSMR_BAM_32M
	                  | MCF_FBCS_CSMR_V);

	/*
	* Check to see if the SDRAM has already been initialized
	* by a run control tool
	*/
	if (!(MCF_SDRAMC_SDCR & MCF_SDRAMC_SDCR_REF))
	{
		/*
		* Basic configuration and initialization
		*/
		// 0x000002AA
		MCF_SDRAMC_SDRAMDS = (0
		                      | MCF_SDRAMC_SDRAMDS_SB_E(MCF_SDRAMC_SDRAMDS_DRIVE_8MA)
		                      | MCF_SDRAMC_SDRAMDS_SB_C(MCF_SDRAMC_SDRAMDS_DRIVE_8MA)
		                      | MCF_SDRAMC_SDRAMDS_SB_A(MCF_SDRAMC_SDRAMDS_DRIVE_8MA)
		                      | MCF_SDRAMC_SDRAMDS_SB_S(MCF_SDRAMC_SDRAMDS_DRIVE_8MA)
		                      | MCF_SDRAMC_SDRAMDS_SB_D(MCF_SDRAMC_SDRAMDS_DRIVE_8MA)
		                     );

		// 0x0000001A
		MCF_SDRAMC_CS0CFG = (0
		                     | MCF_SDRAMC_CSnCFG_CSBA(CFG_SDRAM_ADDRESS)
		                     | MCF_SDRAMC_CSnCFG_CSSZ(MCF_SDRAMC_CSnCFG_CSSZ_128MBYTE)
		                    );

		MCF_SDRAMC_CS1CFG = 0;
		MCF_SDRAMC_CS2CFG = 0;
		MCF_SDRAMC_CS3CFG = 0;

		// 0x73611730
		MCF_SDRAMC_SDCFG1 = (0
		                     | MCF_SDRAMC_SDCFG1_SRD2RW((unsigned int)((CFG_SDRAM_CASL + CFG_SDRAM_BL / 2 + 1) + 0.5))
		                     | MCF_SDRAMC_SDCFG1_SWT2RD((unsigned int) (CFG_SDRAM_TWR + 1))
		                     | MCF_SDRAMC_SDCFG1_RDLAT((unsigned int)((CFG_SDRAM_CASL * 2) + 2))
		                     | MCF_SDRAMC_SDCFG1_ACT2RW((unsigned int)(((CFG_SDRAM_TRCD / CFG_SYSTEM_CORE_PERIOD) - 1) + 0.5))
		                     | MCF_SDRAMC_SDCFG1_PRE2ACT((unsigned int)(((CFG_SDRAM_TRP / CFG_SYSTEM_CORE_PERIOD) - 1) + 0.5))
		                     | MCF_SDRAMC_SDCFG1_REF2ACT((unsigned int)(((CFG_SDRAM_TRFC / CFG_SYSTEM_CORE_PERIOD) - 1) + 0.5))
		                     | MCF_SDRAMC_SDCFG1_WTLAT(3)
		                    );

		// 0x46770000
		MCF_SDRAMC_SDCFG2 = (0
		                     | MCF_SDRAMC_SDCFG2_BRD2PRE(CFG_SDRAM_BL / 2)
		                     | MCF_SDRAMC_SDCFG2_BWT2RW(CFG_SDRAM_BL / 2 + CFG_SDRAM_TWR)
		                     | MCF_SDRAMC_SDCFG2_BRD2WT(7)
		                     | MCF_SDRAMC_SDCFG2_BL(CFG_SDRAM_BL - 1)
		                    );

		/*
		* Precharge and enable write to SDMR
		*/
		// 0xE10B0002
		MCF_SDRAMC_SDCR = (0
		                   | MCF_SDRAMC_SDCR_MODE_EN
		                   | MCF_SDRAMC_SDCR_CKE
		                   | MCF_SDRAMC_SDCR_DDR
		                   | MCF_SDRAMC_SDCR_MUX(1)    // 13 x 10 x 2 ==> MUX=1
		                   | MCF_SDRAMC_SDCR_RCNT((int)(((CFG_SDRAM_TREFI / (CFG_SYSTEM_CORE_PERIOD * 64)) - 1) + 0.5))
		                   | MCF_SDRAMC_SDCR_IPALL
		                  );

		/*
		* Write extended mode register
		*/
		// 0x40010000
		MCF_SDRAMC_SDMR = (0
		                   | MCF_SDRAMC_SDMR_BNKAD_LEMR
		                   | MCF_SDRAMC_SDMR_AD(0x0)
		                   | MCF_SDRAMC_SDMR_CMD
		                  );

		/*
		* Write mode register and reset DLL
		*/
		// 0x048d0000
		MCF_SDRAMC_SDMR = (0
		                   | MCF_SDRAMC_SDMR_BNKAD_LMR
		                   | MCF_SDRAMC_SDMR_AD(CFG_SDRAM_RESET_DLL | CFG_SDRAM_MOD)
		                   | MCF_SDRAMC_SDMR_CMD
		                  );

		/*
		* Execute a PALL command
		*/
		// 0xE10B0002
		MCF_SDRAMC_SDCR |= MCF_SDRAMC_SDCR_IPALL;

		/*
		* Perform two REF cycles
		*/
		// 0xE10B0004
		MCF_SDRAMC_SDCR |= MCF_SDRAMC_SDCR_IREF;
		MCF_SDRAMC_SDCR |= MCF_SDRAMC_SDCR_IREF;

		/*
		* Write mode register and clear reset DLL
		*/
		// 0x008D0000
		MCF_SDRAMC_SDMR = (0
		                   | MCF_SDRAMC_SDMR_BNKAD_LMR
		                   | MCF_SDRAMC_SDMR_AD(CFG_SDRAM_MOD)
		                   | MCF_SDRAMC_SDMR_CMD
		                  );

		/*
		* Enable auto refresh and lock SDMR
		*/
		// 0x610B0000
		MCF_SDRAMC_SDCR &= ~MCF_SDRAMC_SDCR_MODE_EN;

		// 0x710B0F00
		MCF_SDRAMC_SDCR |= (0
		                    | MCF_SDRAMC_SDCR_REF
		                    | MCF_SDRAMC_SDCR_DQS_OE(0xF)
		                   );
	}
}

/** @file
 *
 * Target specific early chipselect and SDRAM init.
 */