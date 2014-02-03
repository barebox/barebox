/*
 * Copyright (C) 2005 Phytec Messtechnik GmbH
 * Juergen Kilb, H. Klaholz <armlinux@phytec.de>
 *
 * Copyright (C) 2006 Pengutronix
 * Sascha Hauer <s.hauer@pengutronix.de>
 * Robert Schwebel <r.schwebel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * phyCORE-PXA270 configuration settings
 * Set these to 0/1 to enable or disable the features.
 */

#define PHYCORE_PXA270_USE_K3FLASH	0

/* 260 MHz or 520 MHZ */
#define PHYCORE_PXA270_SPEED		520

/*********************************************************************
 * CONFIG PXA270 GPIO settings                                       *
 *********************************************************************/

/*
 * GPIO set "1"
 *
 *** REG GPSR0
 * GP15 == nCS1      is 1
 * GP20 == nSDCS2    is 1
 * GP21 == nSDCS3    is 1
 *** REG GPSR1
 * GP33 == nCS5      is 1
 *** REG GPSR2
 * GP78 == nCS2      is 1
 * GP80 == nCS4      is 1
 */
#define GPSR0_DFT		0x00308000
#define GPSR1_DFT		0x00000002
#define GPSR2_DFT		0x00014000

#define CONFIG_GPSR0_VAL	GPSR0_DFT
#define CONFIG_GPSR1_VAL	GPSR1_DFT
#define CONFIG_GPSR2_VAL	GPSR2_DFT
#define CONFIG_GPSR3_VAL	GPSR3_DFT

/*
 * set Direction "1" GPIO == output else input
 *
 ** REG GPDR0
 * GP03 == PWR_SDA   is output
 * GP04 == PWR_SCL   is output
 * GP15 == nCS1      is output
 * GP20 == nSDCS2    is output
 * GP21 == nSDCS3    is output
 ** REG GPDR1
 * GP33 == nCS5      is output
 ** REG GPDR2
 * GP78 == nCS2      is output
 * GP80 == nCS4      is output
 * GP90 == LED0      is output
 * GP91 == LED1      is output
 */

#define GPDR0_DFT		0x00308018
#define GPDR1_DFT		0x00000002
#define GPDR2_DFT		0x00014000

#define CONFIG_GPDR0_VAL	GPDR0_DFT
#define CONFIG_GPDR1_VAL	GPDR1_DFT
#define CONFIG_GPDR2_VAL	GPDR2_DFT

/*
 * set Alternate Funktions
 *
 ** REG GAFR0_L
 * GP15 == nCS1      is AF10
 ** REG GAFR0_U
 * GP18 == RDY       is AF01
 * GP20 == nSDCS2    is AF01
 * GP21 == nSDCS3    is AF01
 ** REG GAFR1_L
 * GP33 == nCS5      is AF10
 ** REG GAFR2_L
 * GP78 == nCS2      is AF10
 ** REG GAFR2_U
 * GP80 == nCS4      is AF10
 */

#define GAFR0_L_DFT		0x80000000
#define GAFR0_U_DFT		0x00000510
#define GAFR1_L_DFT		0x00000008
#define GAFR1_U_DFT		0x00000000
#define GAFR2_L_DFT		0x20000000
#define GAFR2_U_DFT		0x00000002

#define CONFIG_GAFR0_L_VAL	GAFR0_L_DFT
#define CONFIG_GAFR0_U_VAL	GAFR0_U_DFT
#define CONFIG_GAFR1_L_VAL	GAFR1_L_DFT
#define CONFIG_GAFR1_U_VAL	GAFR1_U_DFT
#define CONFIG_GAFR2_L_VAL	GAFR2_L_DFT
#define CONFIG_GAFR2_U_VAL	GAFR2_U_DFT


/*
 * Power Manager Sleep Status Register (PSSR)
 *
 * [6] = 0   OTG pad is not holding it's state
 * [5] = 1   Read Disable Hold: receivers of all gpio pins are disabled
 * [4] = 1   gpio pins are held in their sleep mode state
 * [3] = 0   The processor has not been placed in standby mode by
 *           configuring the PWRMODE register since STS was cleared
 *           by a reset or by software.
 * [2] = 1   nVDD_FAULT has been asserted and caused the processor to
 *           enter deep-sleep mode.
 * [1] = 1   nBATT_FAULT has been asserted and caused the processor to
 *           enter deep-sleep mode.
 * [0] = 1   The processor was placed in sleep mode by configuring the
 *           PWRMODE register.
 */

#define CONFIG_PSSR_VAL		0x37


/*********************************************************************
 * CONFIG PXA270 Chipselect settings                                 *
 *********************************************************************/

/*
 * Memory settings
 *
 * This is the configuration for nCS1/0 -> PLD / flash
 * configuration for nCS1:
 * [31]    0    - Slower Device
 * [30:28] 001  - CS deselect to CS time: 1*(2*MemClk) = 20 ns
 * [27:24] 0010 - Address to data valid in bursts: (2+1)*MemClk = 30 ns
 * [23:20] 1011 - " for first access: (11+2)*MemClk = 130 ns
 * [19]    1    - 16 Bit bus width
 * [18:16] 011  - burst RAM or FLASH
 * configuration for nCS0 (J3 Flash):
 * [15]    0    - Slower Device
 * [14:12] 001  - CS deselect to CS time: 1*(2*MemClk) = 20 ns
 * [11:08] 0010 - Address to data valid in bursts: (2+1)*MemClk = 30 ns
 * [07:04] 1011 - " for first access: (11+2)*MemClk = 130 ns
 * [03]    0    - 32 Bit bus width
 * [02:00] 011  - burst RAM or FLASH
 */
#if PHYCORE_PXA270_USE_K3FLASH == 0
#define CONFIG_MSC0_VAL		0x128C1262
#else
/* configuration for nCS0 (K3 Flash):
 * [15]    0    - Slower Device
 * [14:12] 001  - CS deselect to CS time: 1*(2*MemClk) = 20 ns
 * [11:08] 0010 - Address to data valid in bursts: (2+1)*MemClk = 30 ns
 * [07:04] 1011 - " for first access: (11+2)*MemClk = 130 ns
 * [03]    0    - 32 Bit bus width
 * [02:00] 011  - burst RAM or FLASH
 */
#define CONFIG_MSC0_VAL		0x128C12B3
#endif

/*
 * This is the configuration for nCS3/2
 * configuration for nCS3: POWER
 *
 * [31]    0    - Slower Device
 * [30:28] 111  - RRR3: CS deselect to CS time: 7*(2*MemClk) = 140 ns
 * [27:24] 1111 - RDN3: Address to data valid in bursts: (15+1)*MemClk = 160 ns
 * [23:20] 1111 - RDF3: Address for first access: (23+1)*MemClk = 240 ns
 * [19]    0    - 32 Bit bus width
 * [18:16] 100  - variable latency I/O
 * configuration for nCS2: PLD
 * [15]    0    - Slower Device
 * [14:12] 111  - RRR2: CS deselect to CS time: 7*(2*MemClk) = 140 ns
 * [11:08] 1111 - RDN2: Address to data valid in bursts: (15+1)*MemClk = 160 ns
 * [07:04] 1111 - RDF2: Address for first access: (23+1)*MemClk = 240 ns
 * [03]    1    - 16 Bit bus width
 * [02:00] 100  - variable latency I/O
 */
#define CONFIG_MSC1_VAL		0x128c128c

/*
 * This is the configuration for nCS5/4
 *
 * configuration for nCS5: LAN Controller
 * [31]    0    - Slower Device
 * [30:28] 001  - RRR5: CS deselect to CS time: 1*(2*MemClk) = 20 ns
 * [27:24] 0010 - RDN5: Address to data valid in bursts: (2+1)*MemClk = 30 ns
 * [23:20] 0011 - RDF5: Address for first access: (3+1)*MemClk = 40 ns
 * [19]    0    - 32 Bit bus width
 * [18:16] 100  - variable latency I/O
 * configuration for nCS4: USB
 * [15]    0    - Slower Device
 * [14:12] 111  - RRR4: CS deselect to CS time: 7*(2*MemClk) = 140 ns
 * [11:08] 1111 - RDN4: Address to data valid in bursts: (15+1)*MemClk = 160 ns
 * [07:04] 1111 - RDF4: Address for first access: (23+1)*MemClk = 240 ns
 * [03]    1    - 16 Bit bus width
 * [02:00] 100  - variable latency I/O
 */
#define CONFIG_MSC2_VAL		0x1234128C

/*********************************************************************
 * CONFIG PXA270 SDRAM settings                                      *
 *********************************************************************/

#define CONFIG_DRAM_BASE	0xa0000000


/* MDCNFG: SDRAM Configuration Register
 *
 * [31]      0	 - Stack1
 * [30]      0   - dcacx2
 * [20]      0   - reserved
 * [31:29]   000 - reserved
 * [28]      1	 - SA1111 compatiblity mode
 * [27]      1   - latch return data with return clock
 * [26]      0   - alternate addressing for pair 2/3
 * [25:24]   10  - timings
 * [23]      1   - internal banks in lower partition 2/3 (not used)
 * [22:21]   10  - row address bits for partition 2/3 (not used)
 * [20:19]   01  - column address bits for partition 2/3 (not used)
 * [18]      0   - SDRAM partition 2/3 width is 32 bit
 * [17]      0   - SDRAM partition 3 disabled
 * [16]      0   - SDRAM partition 2 disabled
 * [15]      0	 - Stack1
 * [14]      0   - dcacx0
 * [13]      0   - Stack0
 * [12]      0	 - SA1110 compatiblity mode
 * [11]      1   - always 1
 * [10]      0   - no alternate addressing for pair 0/1
 * [09:08]   10  - tRP=2*MemClk CL=2 tRCD=2*MemClk tRAS=5*MemClk tRC=8*MemClk
 * [7]       1   - 4 internal banks in lower partition pair
 * [06:05]   10  - 13 row address bits for partition 0/1
 * [04:03]   01  - 9 column address bits for partition 0/1
 * [02]      0   - SDRAM partition 0/1 width is 32 bit
 * [01]      0   - disable SDRAM partition 1
 * [00]      1   - enable  SDRAM partition 0
 */

/* K4S561633*/
#define CONFIG_MDCNFG_VAL	0x0AC90AC9

/* MDREFR: SDRAM Refresh Control Register
 *
 * [31]    0     - ALTREFA
 * [30]    0     - ALTREFB
 * [29]    1     - K0DB4
 * [28]    0     - reserved
 * [27]    0     - reserved
 * [26]    0     - reserved
 * [25]    1     - K2FREE: not free running
 * [24]    0     - K1FREE: not free running
 * [23]    1     - K0FREE: not free running
 * [22]    0     - SLFRSH: self refresh disabled
 * [21]    0     - reserved
 * [20]    0     - APD: no auto power down
 * [19]    0     - K2DB2: SDCLK2 is MemClk
 * [18]    0     - K2RUN: disable SDCLK2
 * [17]    0     - K1DB2: SDCLK1 is MemClk
 * [16]    1     - K1RUN: enable SDCLK1
 * [15]    1     - E1PIN: SDRAM clock enable
 * [14]    1     - K0DB2: SDCLK0 is MemClk
 * [13]    0     - K0RUN: disable SDCLK0
 * [12]    0     - RESERVED
 * [11:00] 000000011000 - (64ms/8192)*MemClkFreq/32 = 24
 */
#define CONFIG_MDREFR_VAL	0x2281C018

/* MDMRS: Mode Register Set Configuration Register
 *
 * [31]      0       - reserved
 * [30:23]   00000000- MDMRS2: SDRAM2/3 MRS Value. (not used)
 * [22:20]   000     - MDCL2:  SDRAM2/3 Cas Latency.  (not used)
 * [19]      0       - MDADD2: SDRAM2/3 burst Type. Fixed to sequential.  (not used)
 * [18:16]   010     - MDBL2:  SDRAM2/3 burst Length. Fixed to 4.  (not used)
 * [15]      0       - reserved
 * [14:07]   00000000- MDMRS0: SDRAM0/1 MRS Value.
 * [06:04]   010     - MDCL0:  SDRAM0/1 Cas Latency.
 * [03]      0       - MDADD0: SDRAM0/1 burst Type. Fixed to sequential.
 * [02:00]   010     - MDBL0:  SDRAM0/1 burst Length. Fixed to 4.
 */
#define CONFIG_MDMRS_VAL	0x00020022

/*********************************************************************
 * CONFIG PXA270 Clock generation                                    *
 *********************************************************************/
#define CONFIG_FLYCNFG_VAL	0x00010001
#define CONFIG_SXCNFG_VAL	0x40044004
#define CONFIG_CKEN		(CKEN_MEMC |  CKEN_OSTIMER)

#if PHYCORE_PXA270_SPEED == 520
#define CONFIG_CCCR		0x00000290	/* Memory Clock is f. Table;         N=2.5, L=16 => 16x13=208, 208x2,5=520 MHz */
#elif PHYCORE_PXA270_SPEED == 260
#define CONFIG_CCCR		0x02000288	/* Memory Clock is System-Bus Freq., N=2.5, L=8  =>  8x13=104, 104x2,5=260 MHz */
#else
#error You have specified an illegal speed.
#endif

/*********************************************************************
 * CONFIG PXA270 CF interface                                        *
 *********************************************************************/
#define CONFIG_MECR_VAL		0x00000003
#define CONFIG_MCMEM0_VAL	0x00010504
#define CONFIG_MCMEM1_VAL	0x00010504
#define CONFIG_MCATT0_VAL	0x00010504
#define CONFIG_MCATT1_VAL	0x00010504
#define CONFIG_MCIO0_VAL	0x00004715
#define CONFIG_MCIO1_VAL	0x00004715

#endif  /* __CONFIG_H */
