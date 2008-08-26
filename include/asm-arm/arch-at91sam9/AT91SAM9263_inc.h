/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// File Name           : AT91SAM9262.h
// Object              : AT91SAM9262 definitions
// Generated           : AT91 SW Application Group  01/16/2006 (17:06:54)
//
// CVS Reference       : /AT91SAM9262.pl/1.41/Fri Oct 14 12:51:20 2005//
// CVS Reference       : /SYS_SAM9262.pl/1.4/Tue Jan 25 09:21:40 2005//
// CVS Reference       : /HMATRIX1_SAM9262.pl/1.10/Fri Oct 14 12:51:25 2005//
// CVS Reference       : /CCR_SAM9262.pl/1.4/Fri Jan  6 08:52:19 2006//
// CVS Reference       : /PMC_SAM9262.pl/1.4/Tue Mar  8 10:14:53 2005//
// CVS Reference       : /HSDRAMC1_6100A.pl/1.2/Mon Aug  9 10:31:42 2004//
// CVS Reference       : /HSMC3_6105A.pl/1.4/Tue Nov 16 09:00:34 2004//
// CVS Reference       : /AIC_6075A.pl/1.1/Tue May 10 12:08:46 2005//
// CVS Reference       : /PDC_6074C.pl/1.2/Thu Feb  3 08:48:54 2005//
// CVS Reference       : /DBGU_6059D.pl/1.1/Mon Jan 31 13:15:32 2005//
// CVS Reference       : /PIO_6057A.pl/1.2/Thu Feb  3 10:18:28 2005//
// CVS Reference       : /RSTC_6098A.pl/1.3/Tue Feb  1 16:38:53 2005//
// CVS Reference       : /SHDWC_6122A.pl/1.3/Tue Mar  8 14:44:58 2005//
// CVS Reference       : /RTTC_6081A.pl/1.2/Tue Nov  9 14:43:58 2004//
// CVS Reference       : /PITC_6079A.pl/1.2/Tue Nov  9 14:43:56 2004//
// CVS Reference       : /WDTC_6080A.pl/1.3/Tue Nov  9 14:44:00 2004//
// CVS Reference       : /TC_6082A.pl/1.7/Fri Mar 11 12:52:17 2005//
// CVS Reference       : /MCI_6101E.pl/1.1/Fri Jun  3 13:15:33 2005//
// CVS Reference       : /TWI_6061A.pl/1.1/Tue Jul 13 07:38:06 2004//
// CVS Reference       : /US_6089C.pl/1.1/Mon Jul 12 18:23:26 2004//
// CVS Reference       : /SSC_6078A.pl/1.1/Tue Jul 13 07:45:40 2004//
// CVS Reference       : /SPI_6088D.pl/1.3/Fri May 20 14:08:59 2005//
// CVS Reference       : /AC97C_XXXX.pl/1.3/Wed Feb 23 15:51:28 2005//
// CVS Reference       : /CAN_6019B.pl/1.1/Tue Mar  8 12:42:22 2005//
// CVS Reference       : /PWM_6044D.pl/1.2/Tue May 10 11:52:07 2005//
// CVS Reference       : /LCDC_6063A.pl/1.3/Thu Dec 15 09:44:02 2005//
// CVS Reference       : /EMACB_6119A.pl/1.6/Wed Jul 13 15:05:35 2005//
// CVS Reference       : /GPS_XXXX.pl/1.8/Tue Apr 12 11:44:03 2005//
// CVS Reference       : /DMA_XXXX.pl/1.6/Mon Jan 17 16:06:45 2005//
// CVS Reference       : /OTG_XXXX.pl/1.10/Mon Jan 17 16:06:51 2005//
// CVS Reference       : /UDP_6083C.pl/1.2/Tue May 10 09:39:04 2005//
// CVS Reference       : /UHP_6127A.pl/1.1/Mon Jul 22 13:21:58 2002//
// CVS Reference       : /TBOX_XXXX.pl/1.15/Mon Jun 27 15:02:36 2005//
// CVS Reference       : /EBI_nadia2.pl/1.1/Wed Dec 29 11:28:03 2004//
// CVS Reference       : /AES_6149A.pl/1.12/Wed Nov  2 14:15:23 2005//
// CVS Reference       : /HECC_6143A.pl/1.1/Wed Feb  9 17:16:57 2005//
// CVS Reference       : /ISI_xxxxx.pl/1.3/Tue Mar  8 10:14:51 2005//
//  ----------------------------------------------------------------------------

#ifndef AT91SAM9263_INC_H
#define AT91SAM9263_INC_H
// Hardware register definition

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR System Peripherals
// *****************************************************************************
// -------- GPBR : (SYS Offset: 0x1d60) GPBR General Purpose Register --------
#define AT91C_GPBR_GPRV           (0x0 <<  0) // (SYS) General Purpose Register Value

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR External Bus Interface 0
// *****************************************************************************
// *** Register offset in AT91S_EBI0 structure ***
#define EBI0_DUMMY      ( 0) // Dummy register - Do not use

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR SDRAM Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_SDRAMC structure ***
#define SDRAMC_MR       ( 0) // SDRAM Controller Mode Register
#define SDRAMC_TR       ( 4) // SDRAM Controller Refresh Timer Register
#define SDRAMC_CR       ( 8) // SDRAM Controller Configuration Register
#define SDRAMC_HSR      (12) // SDRAM Controller High Speed Register
#define SDRAMC_LPR      (16) // SDRAM Controller Low Power Register
#define SDRAMC_IER      (20) // SDRAM Controller Interrupt Enable Register
#define SDRAMC_IDR      (24) // SDRAM Controller Interrupt Disable Register
#define SDRAMC_IMR      (28) // SDRAM Controller Interrupt Mask Register
#define SDRAMC_ISR      (32) // SDRAM Controller Interrupt Mask Register
#define SDRAMC_MDR      (36) // SDRAM Memory Device Register
// -------- SDRAMC_MR : (SDRAMC Offset: 0x0) SDRAM Controller Mode Register --------
#define AT91C_SDRAMC_MODE         (0xF <<  0) // (SDRAMC) Mode
#define 	AT91C_SDRAMC_MODE_NORMAL_CMD           (0x0) // (SDRAMC) Normal Mode
#define 	AT91C_SDRAMC_MODE_NOP_CMD              (0x1) // (SDRAMC) Issue a NOP Command at every access
#define 	AT91C_SDRAMC_MODE_PRCGALL_CMD          (0x2) // (SDRAMC) Issue a All Banks Precharge Command at every access
#define 	AT91C_SDRAMC_MODE_LMR_CMD              (0x3) // (SDRAMC) Issue a Load Mode Register at every access
#define 	AT91C_SDRAMC_MODE_RFSH_CMD             (0x4) // (SDRAMC) Issue a Refresh
#define 	AT91C_SDRAMC_MODE_EXT_LMR_CMD          (0x5) // (SDRAMC) Issue an Extended Load Mode Register
#define 	AT91C_SDRAMC_MODE_DEEP_CMD             (0x6) // (SDRAMC) Enter Deep Power Mode
// -------- SDRAMC_TR : (SDRAMC Offset: 0x4) SDRAMC Refresh Timer Register --------
#define AT91C_SDRAMC_COUNT        (0xFFF <<  0) // (SDRAMC) Refresh Counter
// -------- SDRAMC_CR : (SDRAMC Offset: 0x8) SDRAM Configuration Register --------
#define AT91C_SDRAMC_NC           (0x3 <<  0) // (SDRAMC) Number of Column Bits
#define 	AT91C_SDRAMC_NC_8                    (0x0) // (SDRAMC) 8 Bits
#define 	AT91C_SDRAMC_NC_9                    (0x1) // (SDRAMC) 9 Bits
#define 	AT91C_SDRAMC_NC_10                   (0x2) // (SDRAMC) 10 Bits
#define 	AT91C_SDRAMC_NC_11                   (0x3) // (SDRAMC) 11 Bits
#define AT91C_SDRAMC_NR           (0x3 <<  2) // (SDRAMC) Number of Row Bits
#define 	AT91C_SDRAMC_NR_11                   (0x0 <<  2) // (SDRAMC) 11 Bits
#define 	AT91C_SDRAMC_NR_12                   (0x1 <<  2) // (SDRAMC) 12 Bits
#define 	AT91C_SDRAMC_NR_13                   (0x2 <<  2) // (SDRAMC) 13 Bits
#define AT91C_SDRAMC_NB           (0x1 <<  4) // (SDRAMC) Number of Banks
#define 	AT91C_SDRAMC_NB_2_BANKS              (0x0 <<  4) // (SDRAMC) 2 banks
#define 	AT91C_SDRAMC_NB_4_BANKS              (0x1 <<  4) // (SDRAMC) 4 banks
#define AT91C_SDRAMC_CAS          (0x3 <<  5) // (SDRAMC) CAS Latency
#define 	AT91C_SDRAMC_CAS_2                    (0x2 <<  5) // (SDRAMC) 2 cycles
#define 	AT91C_SDRAMC_CAS_3                    (0x3 <<  5) // (SDRAMC) 3 cycles
#define AT91C_SDRAMC_DBW          (0x1 <<  7) // (SDRAMC) Data Bus Width
#define 	AT91C_SDRAMC_DBW_32_BITS              (0x0 <<  7) // (SDRAMC) 32 Bits datas bus
#define 	AT91C_SDRAMC_DBW_16_BITS              (0x1 <<  7) // (SDRAMC) 16 Bits datas bus
#define AT91C_SDRAMC_TWR          (0xF <<  8) // (SDRAMC) Number of Write Recovery Time Cycles
#define 	AT91C_SDRAMC_TWR_0                    (0x0 <<  8) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TWR_1                    (0x1 <<  8) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TWR_2                    (0x2 <<  8) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TWR_3                    (0x3 <<  8) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TWR_4                    (0x4 <<  8) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TWR_5                    (0x5 <<  8) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TWR_6                    (0x6 <<  8) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TWR_7                    (0x7 <<  8) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TWR_8                    (0x8 <<  8) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TWR_9                    (0x9 <<  8) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TWR_10                   (0xA <<  8) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TWR_11                   (0xB <<  8) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TWR_12                   (0xC <<  8) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TWR_13                   (0xD <<  8) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TWR_14                   (0xE <<  8) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TWR_15                   (0xF <<  8) // (SDRAMC) Value : 15
#define AT91C_SDRAMC_TRC          (0xF << 12) // (SDRAMC) Number of RAS Cycle Time Cycles
#define 	AT91C_SDRAMC_TRC_0                    (0x0 << 12) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TRC_1                    (0x1 << 12) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TRC_2                    (0x2 << 12) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TRC_3                    (0x3 << 12) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TRC_4                    (0x4 << 12) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TRC_5                    (0x5 << 12) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TRC_6                    (0x6 << 12) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TRC_7                    (0x7 << 12) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TRC_8                    (0x8 << 12) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TRC_9                    (0x9 << 12) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TRC_10                   (0xA << 12) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TRC_11                   (0xB << 12) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TRC_12                   (0xC << 12) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TRC_13                   (0xD << 12) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TRC_14                   (0xE << 12) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TRC_15                   (0xF << 12) // (SDRAMC) Value : 15
#define AT91C_SDRAMC_TRP          (0xF << 16) // (SDRAMC) Number of RAS Precharge Time Cycles
#define 	AT91C_SDRAMC_TRP_0                    (0x0 << 16) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TRP_1                    (0x1 << 16) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TRP_2                    (0x2 << 16) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TRP_3                    (0x3 << 16) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TRP_4                    (0x4 << 16) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TRP_5                    (0x5 << 16) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TRP_6                    (0x6 << 16) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TRP_7                    (0x7 << 16) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TRP_8                    (0x8 << 16) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TRP_9                    (0x9 << 16) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TRP_10                   (0xA << 16) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TRP_11                   (0xB << 16) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TRP_12                   (0xC << 16) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TRP_13                   (0xD << 16) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TRP_14                   (0xE << 16) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TRP_15                   (0xF << 16) // (SDRAMC) Value : 15
#define AT91C_SDRAMC_TRCD         (0xF << 20) // (SDRAMC) Number of RAS to CAS Delay Cycles
#define 	AT91C_SDRAMC_TRCD_0                    (0x0 << 20) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TRCD_1                    (0x1 << 20) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TRCD_2                    (0x2 << 20) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TRCD_3                    (0x3 << 20) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TRCD_4                    (0x4 << 20) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TRCD_5                    (0x5 << 20) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TRCD_6                    (0x6 << 20) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TRCD_7                    (0x7 << 20) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TRCD_8                    (0x8 << 20) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TRCD_9                    (0x9 << 20) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TRCD_10                   (0xA << 20) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TRCD_11                   (0xB << 20) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TRCD_12                   (0xC << 20) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TRCD_13                   (0xD << 20) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TRCD_14                   (0xE << 20) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TRCD_15                   (0xF << 20) // (SDRAMC) Value : 15
#define AT91C_SDRAMC_TRAS         (0xF << 24) // (SDRAMC) Number of RAS Active Time Cycles
#define 	AT91C_SDRAMC_TRAS_0                    (0x0 << 24) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TRAS_1                    (0x1 << 24) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TRAS_2                    (0x2 << 24) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TRAS_3                    (0x3 << 24) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TRAS_4                    (0x4 << 24) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TRAS_5                    (0x5 << 24) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TRAS_6                    (0x6 << 24) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TRAS_7                    (0x7 << 24) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TRAS_8                    (0x8 << 24) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TRAS_9                    (0x9 << 24) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TRAS_10                   (0xA << 24) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TRAS_11                   (0xB << 24) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TRAS_12                   (0xC << 24) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TRAS_13                   (0xD << 24) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TRAS_14                   (0xE << 24) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TRAS_15                   (0xF << 24) // (SDRAMC) Value : 15
#define AT91C_SDRAMC_TXSR         (0xF << 28) // (SDRAMC) Number of Command Recovery Time Cycles
#define 	AT91C_SDRAMC_TXSR_0                    (0x0 << 28) // (SDRAMC) Value :  0
#define 	AT91C_SDRAMC_TXSR_1                    (0x1 << 28) // (SDRAMC) Value :  1
#define 	AT91C_SDRAMC_TXSR_2                    (0x2 << 28) // (SDRAMC) Value :  2
#define 	AT91C_SDRAMC_TXSR_3                    (0x3 << 28) // (SDRAMC) Value :  3
#define 	AT91C_SDRAMC_TXSR_4                    (0x4 << 28) // (SDRAMC) Value :  4
#define 	AT91C_SDRAMC_TXSR_5                    (0x5 << 28) // (SDRAMC) Value :  5
#define 	AT91C_SDRAMC_TXSR_6                    (0x6 << 28) // (SDRAMC) Value :  6
#define 	AT91C_SDRAMC_TXSR_7                    (0x7 << 28) // (SDRAMC) Value :  7
#define 	AT91C_SDRAMC_TXSR_8                    (0x8 << 28) // (SDRAMC) Value :  8
#define 	AT91C_SDRAMC_TXSR_9                    (0x9 << 28) // (SDRAMC) Value :  9
#define 	AT91C_SDRAMC_TXSR_10                   (0xA << 28) // (SDRAMC) Value : 10
#define 	AT91C_SDRAMC_TXSR_11                   (0xB << 28) // (SDRAMC) Value : 11
#define 	AT91C_SDRAMC_TXSR_12                   (0xC << 28) // (SDRAMC) Value : 12
#define 	AT91C_SDRAMC_TXSR_13                   (0xD << 28) // (SDRAMC) Value : 13
#define 	AT91C_SDRAMC_TXSR_14                   (0xE << 28) // (SDRAMC) Value : 14
#define 	AT91C_SDRAMC_TXSR_15                   (0xF << 28) // (SDRAMC) Value : 15
// -------- SDRAMC_HSR : (SDRAMC Offset: 0xc) SDRAM Controller High Speed Register --------
#define AT91C_SDRAMC_DA           (0x1 <<  0) // (SDRAMC) Decode Cycle Enable Bit
#define 	AT91C_SDRAMC_DA_DISABLE              (0x0) // (SDRAMC) Disable Decode Cycle
#define 	AT91C_SDRAMC_DA_ENABLE               (0x1) // (SDRAMC) Enable Decode Cycle
// -------- SDRAMC_LPR : (SDRAMC Offset: 0x10) SDRAM Controller Low-power Register --------
#define AT91C_SDRAMC_LPCB         (0x3 <<  0) // (SDRAMC) Low-power Configurations
#define 	AT91C_SDRAMC_LPCB_DISABLE              (0x0) // (SDRAMC) Disable Low Power Features
#define 	AT91C_SDRAMC_LPCB_SELF_REFRESH         (0x1) // (SDRAMC) Enable SELF_REFRESH
#define 	AT91C_SDRAMC_LPCB_POWER_DOWN           (0x2) // (SDRAMC) Enable POWER_DOWN
#define 	AT91C_SDRAMC_LPCB_DEEP_POWER_DOWN      (0x3) // (SDRAMC) Enable DEEP_POWER_DOWN
#define AT91C_SDRAMC_PASR         (0x7 <<  4) // (SDRAMC) Partial Array Self Refresh (only for Low Power SDRAM)
#define AT91C_SDRAMC_TCSR         (0x3 <<  8) // (SDRAMC) Temperature Compensated Self Refresh (only for Low Power SDRAM)
#define AT91C_SDRAMC_DS           (0x3 << 10) // (SDRAMC) Drive Strenght (only for Low Power SDRAM)
#define AT91C_SDRAMC_TIMEOUT      (0x3 << 12) // (SDRAMC) Time to define when Low Power Mode is enabled
#define 	AT91C_SDRAMC_TIMEOUT_0_CLK_CYCLES         (0x0 << 12) // (SDRAMC) Activate SDRAM Low Power Mode Immediately
#define 	AT91C_SDRAMC_TIMEOUT_64_CLK_CYCLES        (0x1 << 12) // (SDRAMC) Activate SDRAM Low Power Mode after 64 clock cycles after the end of the last transfer
#define 	AT91C_SDRAMC_TIMEOUT_128_CLK_CYCLES       (0x2 << 12) // (SDRAMC) Activate SDRAM Low Power Mode after 64 clock cycles after the end of the last transfer
// -------- SDRAMC_IER : (SDRAMC Offset: 0x14) SDRAM Controller Interrupt Enable Register --------
#define AT91C_SDRAMC_RES          (0x1 <<  0) // (SDRAMC) Refresh Error Status
// -------- SDRAMC_IDR : (SDRAMC Offset: 0x18) SDRAM Controller Interrupt Disable Register --------
// -------- SDRAMC_IMR : (SDRAMC Offset: 0x1c) SDRAM Controller Interrupt Mask Register --------
// -------- SDRAMC_ISR : (SDRAMC Offset: 0x20) SDRAM Controller Interrupt Status Register --------
// -------- SDRAMC_MDR : (SDRAMC Offset: 0x24) SDRAM Controller Memory Device Register --------
#define AT91C_SDRAMC_MD           (0x3 <<  0) // (SDRAMC) Memory Device Type
#define 	AT91C_SDRAMC_MD_SDRAM                (0x0) // (SDRAMC) SDRAM Mode
#define 	AT91C_SDRAMC_MD_LOW_POWER_SDRAM      (0x1) // (SDRAMC) SDRAM Low Power Mode

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Static Memory Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_SMC structure ***
#define SMC_SETUP0      ( 0) //  Setup Register for CS 0
#define SMC_PULSE0      ( 4) //  Pulse Register for CS 0
#define SMC_CYCLE0      ( 8) //  Cycle Register for CS 0
#define SMC_CTRL0       (12) //  Control Register for CS 0
#define SMC_SETUP1      (16) //  Setup Register for CS 1
#define SMC_PULSE1      (20) //  Pulse Register for CS 1
#define SMC_CYCLE1      (24) //  Cycle Register for CS 1
#define SMC_CTRL1       (28) //  Control Register for CS 1
#define SMC_SETUP2      (32) //  Setup Register for CS 2
#define SMC_PULSE2      (36) //  Pulse Register for CS 2
#define SMC_CYCLE2      (40) //  Cycle Register for CS 2
#define SMC_CTRL2       (44) //  Control Register for CS 2
#define SMC_SETUP3      (48) //  Setup Register for CS 3
#define SMC_PULSE3      (52) //  Pulse Register for CS 3
#define SMC_CYCLE3      (56) //  Cycle Register for CS 3
#define SMC_CTRL3       (60) //  Control Register for CS 3
#define SMC_SETUP4      (64) //  Setup Register for CS 4
#define SMC_PULSE4      (68) //  Pulse Register for CS 4
#define SMC_CYCLE4      (72) //  Cycle Register for CS 4
#define SMC_CTRL4       (76) //  Control Register for CS 4
#define SMC_SETUP5      (80) //  Setup Register for CS 5
#define SMC_PULSE5      (84) //  Pulse Register for CS 5
#define SMC_CYCLE5      (88) //  Cycle Register for CS 5
#define SMC_CTRL5       (92) //  Control Register for CS 5
#define SMC_SETUP6      (96) //  Setup Register for CS 6
#define SMC_PULSE6      (100) //  Pulse Register for CS 6
#define SMC_CYCLE6      (104) //  Cycle Register for CS 6
#define SMC_CTRL6       (108) //  Control Register for CS 6
#define SMC_SETUP7      (112) //  Setup Register for CS 7
#define SMC_PULSE7      (116) //  Pulse Register for CS 7
#define SMC_CYCLE7      (120) //  Cycle Register for CS 7
#define SMC_CTRL7       (124) //  Control Register for CS 7
// -------- SMC_SETUP : (SMC Offset: 0x0) Setup Register for CS x --------
#define AT91C_SMC_NWESETUP        (0x3F <<  0) // (SMC) NWE Setup Length
#define AT91C_SMC_NCSSETUPWR      (0x3F <<  8) // (SMC) NCS Setup Length in WRite Access
#define AT91C_SMC_NRDSETUP        (0x3F << 16) // (SMC) NRD Setup Length
#define AT91C_SMC_NCSSETUPRD      (0x3F << 24) // (SMC) NCS Setup Length in ReaD Access
// -------- SMC_PULSE : (SMC Offset: 0x4) Pulse Register for CS x --------
#define AT91C_SMC_NWEPULSE        (0x7F <<  0) // (SMC) NWE Pulse Length
#define AT91C_SMC_NCSPULSEWR      (0x7F <<  8) // (SMC) NCS Pulse Length in WRite Access
#define AT91C_SMC_NRDPULSE        (0x7F << 16) // (SMC) NRD Pulse Length
#define AT91C_SMC_NCSPULSERD      (0x7F << 24) // (SMC) NCS Pulse Length in ReaD Access
// -------- SMC_CYC : (SMC Offset: 0x8) Cycle Register for CS x --------
#define AT91C_SMC_NWECYCLE        (0x1FF <<  0) // (SMC) Total Write Cycle Length
#define AT91C_SMC_NRDCYCLE        (0x1FF << 16) // (SMC) Total Read Cycle Length
// -------- SMC_CTRL : (SMC Offset: 0xc) Control Register for CS x --------
#define AT91C_SMC_READMODE        (0x1 <<  0) // (SMC) Read Mode
#define AT91C_SMC_WRITEMODE       (0x1 <<  1) // (SMC) Write Mode
#define AT91C_SMC_NWAITM          (0x3 <<  5) // (SMC) NWAIT Mode
#define 	AT91C_SMC_NWAITM_NWAIT_DISABLE        (0x0 <<  5) // (SMC) External NWAIT disabled.
#define 	AT91C_SMC_NWAITM_NWAIT_ENABLE_FROZEN  (0x2 <<  5) // (SMC) External NWAIT enabled in frozen mode.
#define 	AT91C_SMC_NWAITM_NWAIT_ENABLE_READY   (0x3 <<  5) // (SMC) External NWAIT enabled in ready mode.
#define AT91C_SMC_BAT             (0x1 <<  8) // (SMC) Byte Access Type
#define 	AT91C_SMC_BAT_BYTE_SELECT          (0x0 <<  8) // (SMC) Write controled by ncs, nbs0, nbs1, nbs2, nbs3. Read controled by ncs, nrd, nbs0, nbs1, nbs2, nbs3.
#define 	AT91C_SMC_BAT_BYTE_WRITE           (0x1 <<  8) // (SMC) Write controled by ncs, nwe0, nwe1, nwe2, nwe3. Read controled by ncs and nrd.
#define AT91C_SMC_DBW             (0x3 << 12) // (SMC) Data Bus Width
#define 	AT91C_SMC_DBW_WIDTH_EIGTH_BITS     (0x0 << 12) // (SMC) 8 bits.
#define 	AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS   (0x1 << 12) // (SMC) 16 bits.
#define 	AT91C_SMC_DBW_WIDTH_THIRTY_TWO_BITS (0x2 << 12) // (SMC) 32 bits.
#define AT91C_SMC_TDF             (0xF << 16) // (SMC) Data Float Time.
#define AT91C_SMC_TDFEN           (0x1 << 20) // (SMC) TDF Enabled.
#define AT91C_SMC_PMEN            (0x1 << 24) // (SMC) Page Mode Enabled.
#define AT91C_SMC_PS              (0x3 << 28) // (SMC) Page Size
#define 	AT91C_SMC_PS_SIZE_FOUR_BYTES      (0x0 << 28) // (SMC) 4 bytes.
#define 	AT91C_SMC_PS_SIZE_EIGHT_BYTES     (0x1 << 28) // (SMC) 8 bytes.
#define 	AT91C_SMC_PS_SIZE_SIXTEEN_BYTES   (0x2 << 28) // (SMC) 16 bytes.
#define 	AT91C_SMC_PS_SIZE_THIRTY_TWO_BYTES (0x3 << 28) // (SMC) 32 bytes.
// -------- SMC_SETUP : (SMC Offset: 0x10) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x14) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x18) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x1c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x20) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x24) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x28) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x2c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x30) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x34) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x38) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x3c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x40) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x44) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x48) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x4c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x50) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x54) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x58) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x5c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x60) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x64) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x68) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x6c) Control Register for CS x --------
// -------- SMC_SETUP : (SMC Offset: 0x70) Setup Register for CS x --------
// -------- SMC_PULSE : (SMC Offset: 0x74) Pulse Register for CS x --------
// -------- SMC_CYC : (SMC Offset: 0x78) Cycle Register for CS x --------
// -------- SMC_CTRL : (SMC Offset: 0x7c) Control Register for CS x --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR External Bus Interface 1
// *****************************************************************************
// *** Register offset in AT91S_EBI1 structure ***
#define EBI1_DUMMY      ( 0) // Dummy register - Do not use

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR AHB Matrix Interface
// *****************************************************************************
// *** Register offset in AT91S_MATRIX structure ***
#define MATRIX_MCFG0    ( 0) //  Master Configuration Register 0
#define MATRIX_MCFG1    ( 4) //  Master Configuration Register 1
#define MATRIX_MCFG2    ( 8) //  Master Configuration Register 2
#define MATRIX_MCFG3    (12) //  Master Configuration Register 3
#define MATRIX_MCFG4    (16) //  Master Configuration Register 4
#define MATRIX_MCFG5    (20) //  Master Configuration Register 5
#define MATRIX_MCFG6    (24) //  Master Configuration Register 6
#define MATRIX_MCFG7    (28) //  Master Configuration Register 7
#define MATRIX_MCFG8    (32) //  Master Configuration Register 8
#define MATRIX_SCFG0    (64) //  Slave Configuration Register 0
#define MATRIX_SCFG1    (68) //  Slave Configuration Register 1
#define MATRIX_SCFG2    (72) //  Slave Configuration Register 2
#define MATRIX_SCFG3    (76) //  Slave Configuration Register 3
#define MATRIX_SCFG4    (80) //  Slave Configuration Register 4
#define MATRIX_SCFG5    (84) //  Slave Configuration Register 5
#define MATRIX_SCFG6    (88) //  Slave Configuration Register 6
#define MATRIX_SCFG7    (92) //  Slave Configuration Register 7
#define MATRIX_PRAS0    (128) //  PRAS0
#define MATRIX_PRBS0    (132) //  PRBS0
#define MATRIX_PRAS1    (136) //  PRAS1
#define MATRIX_PRBS1    (140) //  PRBS1
#define MATRIX_PRAS2    (144) //  PRAS2
#define MATRIX_PRBS2    (148) //  PRBS2
#define MATRIX_PRAS3    (152) //  PRAS3
#define MATRIX_PRBS3    (156) //  PRBS3
#define MATRIX_PRAS4    (160) //  PRAS4
#define MATRIX_PRBS4    (164) //  PRBS4
#define MATRIX_PRAS5    (168) //  PRAS5
#define MATRIX_PRBS5    (172) //  PRBS5
#define MATRIX_PRAS6    (176) //  PRAS6
#define MATRIX_PRBS6    (180) //  PRBS6
#define MATRIX_PRAS7    (184) //  PRAS7
#define MATRIX_PRBS7    (188) //  PRBS7
#define MATRIX_MRCR     (256) //  Master Remp Control Register
// -------- MATRIX_MCFG0 : (MATRIX Offset: 0x0) Master Configuration Register rom --------
#define AT91C_MATRIX_ULBT         (0x7 <<  0) // (MATRIX) Undefined Length Burst Type
// -------- MATRIX_MCFG1 : (MATRIX Offset: 0x4) Master Configuration Register htcm --------
// -------- MATRIX_MCFG2 : (MATRIX Offset: 0x8) Master Configuration Register gps_tcm --------
// -------- MATRIX_MCFG3 : (MATRIX Offset: 0xc) Master Configuration Register hperiphs --------
// -------- MATRIX_MCFG4 : (MATRIX Offset: 0x10) Master Configuration Register ebi0 --------
// -------- MATRIX_MCFG5 : (MATRIX Offset: 0x14) Master Configuration Register ebi1 --------
// -------- MATRIX_MCFG6 : (MATRIX Offset: 0x18) Master Configuration Register bridge --------
// -------- MATRIX_MCFG7 : (MATRIX Offset: 0x1c) Master Configuration Register gps --------
// -------- MATRIX_MCFG8 : (MATRIX Offset: 0x20) Master Configuration Register gps --------
// -------- MATRIX_SCFG0 : (MATRIX Offset: 0x40) Slave Configuration Register 0 --------
#define AT91C_MATRIX_SLOT_CYCLE   (0xFF <<  0) // (MATRIX) Maximum Number of Allowed Cycles for a Burst
#define AT91C_MATRIX_DEFMSTR_TYPE (0x3 << 16) // (MATRIX) Default Master Type
#define 	AT91C_MATRIX_DEFMSTR_TYPE_NO_DEFMSTR           (0x0 << 16) // (MATRIX) No Default Master. At the end of current slave access, if no other master request is pending, the slave is deconnected from all masters. This results in having a one cycle latency for the first transfer of a burst.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_LAST_DEFMSTR         (0x1 << 16) // (MATRIX) Last Default Master. At the end of current slave access, if no other master request is pending, the slave stay connected with the last master having accessed it. This results in not having the one cycle latency when the last master re-trying access on the slave.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR        (0x2 << 16) // (MATRIX) Fixed Default Master. At the end of current slave access, if no other master request is pending, the slave connects with fixed which number is in FIXED_DEFMSTR field. This results in not having the one cycle latency when the fixed master re-trying access on the slave.
#define AT91C_MATRIX_FIXED_DEFMSTR0 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
#define AT91C_MATRIX_ARBT         (0x3 << 24) // (MATRIX) Arbitration Type
// -------- MATRIX_SCFG1 : (MATRIX Offset: 0x44) Slave Configuration Register 1 --------
#define AT91C_MATRIX_FIXED_DEFMSTR1 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
// -------- MATRIX_SCFG2 : (MATRIX Offset: 0x48) Slave Configuration Register 2 --------
#define AT91C_MATRIX_FIXED_DEFMSTR2 (0x1 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR2_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR2_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR2_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
// -------- MATRIX_SCFG3 : (MATRIX Offset: 0x4c) Slave Configuration Register 3 --------
#define AT91C_MATRIX_FIXED_DEFMSTR3 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
// -------- MATRIX_SCFG4 : (MATRIX Offset: 0x50) Slave Configuration Register 4 --------
#define AT91C_MATRIX_FIXED_DEFMSTR4 (0x3 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
// -------- MATRIX_SCFG5 : (MATRIX Offset: 0x54) Slave Configuration Register 5 --------
#define AT91C_MATRIX_FIXED_DEFMSTR5 (0x3 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR5_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
// -------- MATRIX_SCFG6 : (MATRIX Offset: 0x58) Slave Configuration Register 6 --------
#define AT91C_MATRIX_FIXED_DEFMSTR6 (0x3 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR6_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
// -------- MATRIX_SCFG7 : (MATRIX Offset: 0x5c) Slave Configuration Register 7 --------
#define AT91C_MATRIX_FIXED_DEFMSTR7 (0x3 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR7_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR7_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR7_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR7_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
// -------- MATRIX_PRAS0 : (MATRIX Offset: 0x80) PRAS0 Register --------
#define AT91C_MATRIX_M0PR         (0x3 <<  0) // (MATRIX) ARM926EJ-S Instruction priority
#define AT91C_MATRIX_M1PR         (0x3 <<  4) // (MATRIX) ARM926EJ-S Data priority
#define AT91C_MATRIX_M2PR         (0x3 <<  8) // (MATRIX) PDC priority
#define AT91C_MATRIX_M3PR         (0x3 << 12) // (MATRIX) LCDC priority
#define AT91C_MATRIX_M4PR         (0x3 << 16) // (MATRIX) 2DGC priority
#define AT91C_MATRIX_M5PR         (0x3 << 20) // (MATRIX) ISI priority
#define AT91C_MATRIX_M6PR         (0x3 << 24) // (MATRIX) DMA priority
#define AT91C_MATRIX_M7PR         (0x3 << 28) // (MATRIX) EMAC priority
// -------- MATRIX_PRBS0 : (MATRIX Offset: 0x84) PRBS0 Register --------
#define AT91C_MATRIX_M8PR         (0x3 <<  0) // (MATRIX) USB priority
// -------- MATRIX_PRAS1 : (MATRIX Offset: 0x88) PRAS1 Register --------
// -------- MATRIX_PRBS1 : (MATRIX Offset: 0x8c) PRBS1 Register --------
// -------- MATRIX_PRAS2 : (MATRIX Offset: 0x90) PRAS2 Register --------
// -------- MATRIX_PRBS2 : (MATRIX Offset: 0x94) PRBS2 Register --------
// -------- MATRIX_PRAS3 : (MATRIX Offset: 0x98) PRAS3 Register --------
// -------- MATRIX_PRBS3 : (MATRIX Offset: 0x9c) PRBS3 Register --------
// -------- MATRIX_PRAS4 : (MATRIX Offset: 0xa0) PRAS4 Register --------
// -------- MATRIX_PRBS4 : (MATRIX Offset: 0xa4) PRBS4 Register --------
// -------- MATRIX_PRAS5 : (MATRIX Offset: 0xa8) PRAS5 Register --------
// -------- MATRIX_PRBS5 : (MATRIX Offset: 0xac) PRBS5 Register --------
// -------- MATRIX_PRAS6 : (MATRIX Offset: 0xb0) PRAS6 Register --------
// -------- MATRIX_PRBS6 : (MATRIX Offset: 0xb4) PRBS6 Register --------
// -------- MATRIX_PRAS7 : (MATRIX Offset: 0xb8) PRAS7 Register --------
// -------- MATRIX_PRBS7 : (MATRIX Offset: 0xbc) PRBS7 Register --------
// -------- MATRIX_MRCR : (MATRIX Offset: 0x100) MRCR Register --------
#define AT91C_MATRIX_RCA926I      (0x1 <<  0) // (MATRIX) Remap Command Bit for ARM926EJ-S Instruction
#define AT91C_MATRIX_RCA926D      (0x1 <<  1) // (MATRIX) Remap Command Bit for ARM926EJ-S Data
#define AT91C_MATRIX_RCB2         (0x1 <<  2) // (MATRIX) Remap Command Bit for PDC
#define AT91C_MATRIX_RCB3         (0x1 <<  3) // (MATRIX) Remap Command Bit for LCD
#define AT91C_MATRIX_RCB4         (0x1 <<  4) // (MATRIX) Remap Command Bit for 2DGC
#define AT91C_MATRIX_RCB5         (0x1 <<  5) // (MATRIX) Remap Command Bit for ISI
#define AT91C_MATRIX_RCB6         (0x1 <<  6) // (MATRIX) Remap Command Bit for DMA
#define AT91C_MATRIX_RCB7         (0x1 <<  7) // (MATRIX) Remap Command Bit for EMAC
#define AT91C_MATRIX_RCB8         (0x1 <<  8) // (MATRIX) Remap Command Bit for USB

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR AHB CCFG Interface
// *****************************************************************************
// *** Register offset in AT91S_CCFG structure ***
#define CCFG_TCMR       ( 4) //  TCM configuration
#define CCFG_EBI0CSA    (16) //  EBI0 Chip Select Assignement Register
#define CCFG_EBI1CSA    (20) //  EBI1 Chip Select Assignement Register
#define CCFG_MATRIXVERSION (236) //  Version Register
// -------- CCFG_TCMR : (CCFG Offset: 0x4) TCM Configuration --------
#define AT91C_CCFG_ITCM_SIZE      (0xF <<  0) // (CCFG) Size of ITCM enabled memory block
#define 	AT91C_CCFG_ITCM_SIZE_0KB                  (0x0) // (CCFG) 0 KB (No ITCM Memory)
#define 	AT91C_CCFG_ITCM_SIZE_16KB                 (0x5) // (CCFG) 16 KB
#define 	AT91C_CCFG_ITCM_SIZE_32KB                 (0x6) // (CCFG) 32 KB
#define AT91C_CCFG_DTCM_SIZE      (0xF <<  4) // (CCFG) Size of DTCM enabled memory block
#define 	AT91C_CCFG_DTCM_SIZE_0KB                  (0x0 <<  4) // (CCFG) 0 KB (No DTCM Memory)
#define 	AT91C_CCFG_DTCM_SIZE_16KB                 (0x5 <<  4) // (CCFG) 16 KB
#define 	AT91C_CCFG_DTCM_SIZE_32KB                 (0x6 <<  4) // (CCFG) 32 KB
#define AT91C_CCFG_RM             (0xF <<  8) // (CCFG) Read Margin registers
// -------- CCFG_EBI0CSA : (CCFG Offset: 0x10) EBI0 Chip Select Assignement Register --------
#define AT91C_EBI_CS1A            (0x1 <<  1) // (CCFG) Chip Select 1 Assignment
#define 	AT91C_EBI_CS1A_SMC                  (0x0 <<  1) // (CCFG) Chip Select 1 is assigned to the Static Memory Controller.
#define 	AT91C_EBI_CS1A_SDRAMC               (0x1 <<  1) // (CCFG) Chip Select 1 is assigned to the SDRAM Controller.
#define AT91C_EBI_CS3A            (0x1 <<  3) // (CCFG) Chip Select 3 Assignment
#define 	AT91C_EBI_CS3A_SMC                  (0x0 <<  3) // (CCFG) Chip Select 3 is only assigned to the Static Memory Controller and NCS3 behaves as defined by the SMC.
#define 	AT91C_EBI_CS3A_SM                   (0x1 <<  3) // (CCFG) Chip Select 3 is assigned to the Static Memory Controller and the SmartMedia Logic is activated.
#define AT91C_EBI_CS4A            (0x1 <<  4) // (CCFG) Chip Select 4 Assignment
#define 	AT91C_EBI_CS4A_SMC                  (0x0 <<  4) // (CCFG) Chip Select 4 is only assigned to the Static Memory Controller and NCS4 behaves as defined by the SMC.
#define 	AT91C_EBI_CS4A_CF                   (0x1 <<  4) // (CCFG) Chip Select 4 is assigned to the Static Memory Controller and the CompactFlash Logic (first slot) is activated.
#define AT91C_EBI_CS5A            (0x1 <<  5) // (CCFG) Chip Select 5 Assignment
#define 	AT91C_EBI_CS5A_SMC                  (0x0 <<  5) // (CCFG) Chip Select 5 is only assigned to the Static Memory Controller and NCS5 behaves as defined by the SMC
#define 	AT91C_EBI_CS5A_CF                   (0x1 <<  5) // (CCFG) Chip Select 5 is assigned to the Static Memory Controller and the CompactFlash Logic (second slot) is activated.
#define AT91C_EBI_DBPUC           (0x1 <<  8) // (CCFG) Data Bus Pull-up Configuration
// -------- CCFG_EBI1CSA : (CCFG Offset: 0x14) EBI1 Chip Select Assignement Register --------
#define AT91C_EBI_CS2A            (0x1 <<  3) // (CCFG) EBI1 Chip Select 2 Assignment
#define 	AT91C_EBI_CS2A_SMC                  (0x0 <<  3) // (CCFG) Chip Select 2 is assigned to the Static Memory Controller.
#define 	AT91C_EBI_CS2A_SM                   (0x1 <<  3) // (CCFG) Chip Select 2 is assigned to the Static Memory Controller and the SmartMedia Logic is activated.

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Peripheral DMA Controller
// *****************************************************************************
// *** Register offset in AT91S_PDC structure ***
#define PDC_RPR         ( 0) // Receive Pointer Register
#define PDC_RCR         ( 4) // Receive Counter Register
#define PDC_TPR         ( 8) // Transmit Pointer Register
#define PDC_TCR         (12) // Transmit Counter Register
#define PDC_RNPR        (16) // Receive Next Pointer Register
#define PDC_RNCR        (20) // Receive Next Counter Register
#define PDC_TNPR        (24) // Transmit Next Pointer Register
#define PDC_TNCR        (28) // Transmit Next Counter Register
#define PDC_PTCR        (32) // PDC Transfer Control Register
#define PDC_PTSR        (36) // PDC Transfer Status Register
// -------- PDC_PTCR : (PDC Offset: 0x20) PDC Transfer Control Register --------
#define AT91C_PDC_RXTEN           (0x1 <<  0) // (PDC) Receiver Transfer Enable
#define AT91C_PDC_RXTDIS          (0x1 <<  1) // (PDC) Receiver Transfer Disable
#define AT91C_PDC_TXTEN           (0x1 <<  8) // (PDC) Transmitter Transfer Enable
#define AT91C_PDC_TXTDIS          (0x1 <<  9) // (PDC) Transmitter Transfer Disable
// -------- PDC_PTSR : (PDC Offset: 0x24) PDC Transfer Status Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Debug Unit
// *****************************************************************************
// *** Register offset in AT91S_DBGU structure ***
#define DBGU_CR         ( 0) // Control Register
#define DBGU_MR         ( 4) // Mode Register
#define DBGU_IER        ( 8) // Interrupt Enable Register
#define DBGU_IDR        (12) // Interrupt Disable Register
#define DBGU_IMR        (16) // Interrupt Mask Register
#define DBGU_CSR        (20) // Channel Status Register
#define DBGU_RHR        (24) // Receiver Holding Register
#define DBGU_THR        (28) // Transmitter Holding Register
#define DBGU_BRGR       (32) // Baud Rate Generator Register
#define DBGU_CIDR       (64) // Chip ID Register
#define DBGU_EXID       (68) // Chip ID Extension Register
#define DBGU_FNTR       (72) // Force NTRST Register
#define DBGU_RPR        (256) // Receive Pointer Register
#define DBGU_RCR        (260) // Receive Counter Register
#define DBGU_TPR        (264) // Transmit Pointer Register
#define DBGU_TCR        (268) // Transmit Counter Register
#define DBGU_RNPR       (272) // Receive Next Pointer Register
#define DBGU_RNCR       (276) // Receive Next Counter Register
#define DBGU_TNPR       (280) // Transmit Next Pointer Register
#define DBGU_TNCR       (284) // Transmit Next Counter Register
#define DBGU_PTCR       (288) // PDC Transfer Control Register
#define DBGU_PTSR       (292) // PDC Transfer Status Register
// -------- DBGU_CR : (DBGU Offset: 0x0) Debug Unit Control Register --------
#define AT91C_US_RSTRX            (0x1 <<  2) // (DBGU) Reset Receiver
#define AT91C_US_RSTTX            (0x1 <<  3) // (DBGU) Reset Transmitter
#define AT91C_US_RXEN             (0x1 <<  4) // (DBGU) Receiver Enable
#define AT91C_US_RXDIS            (0x1 <<  5) // (DBGU) Receiver Disable
#define AT91C_US_TXEN             (0x1 <<  6) // (DBGU) Transmitter Enable
#define AT91C_US_TXDIS            (0x1 <<  7) // (DBGU) Transmitter Disable
#define AT91C_US_RSTSTA           (0x1 <<  8) // (DBGU) Reset Status Bits
// -------- DBGU_MR : (DBGU Offset: 0x4) Debug Unit Mode Register --------
#define AT91C_US_PAR              (0x7 <<  9) // (DBGU) Parity type
#define 	AT91C_US_PAR_EVEN                 (0x0 <<  9) // (DBGU) Even Parity
#define 	AT91C_US_PAR_ODD                  (0x1 <<  9) // (DBGU) Odd Parity
#define 	AT91C_US_PAR_SPACE                (0x2 <<  9) // (DBGU) Parity forced to 0 (Space)
#define 	AT91C_US_PAR_MARK                 (0x3 <<  9) // (DBGU) Parity forced to 1 (Mark)
#define 	AT91C_US_PAR_NONE                 (0x4 <<  9) // (DBGU) No Parity
#define 	AT91C_US_PAR_MULTI_DROP           (0x6 <<  9) // (DBGU) Multi-drop mode
#define AT91C_US_CHMODE           (0x3 << 14) // (DBGU) Channel Mode
#define 	AT91C_US_CHMODE_NORMAL               (0x0 << 14) // (DBGU) Normal Mode: The USART channel operates as an RX/TX USART.
#define 	AT91C_US_CHMODE_AUTO                 (0x1 << 14) // (DBGU) Automatic Echo: Receiver Data Input is connected to the TXD pin.
#define 	AT91C_US_CHMODE_LOCAL                (0x2 << 14) // (DBGU) Local Loopback: Transmitter Output Signal is connected to Receiver Input Signal.
#define 	AT91C_US_CHMODE_REMOTE               (0x3 << 14) // (DBGU) Remote Loopback: RXD pin is internally connected to TXD pin.
// -------- DBGU_IER : (DBGU Offset: 0x8) Debug Unit Interrupt Enable Register --------
#define AT91C_US_RXRDY            (0x1 <<  0) // (DBGU) RXRDY Interrupt
#define AT91C_US_TXRDY            (0x1 <<  1) // (DBGU) TXRDY Interrupt
#define AT91C_US_ENDRX            (0x1 <<  3) // (DBGU) End of Receive Transfer Interrupt
#define AT91C_US_ENDTX            (0x1 <<  4) // (DBGU) End of Transmit Interrupt
#define AT91C_US_OVRE             (0x1 <<  5) // (DBGU) Overrun Interrupt
#define AT91C_US_FRAME            (0x1 <<  6) // (DBGU) Framing Error Interrupt
#define AT91C_US_PARE             (0x1 <<  7) // (DBGU) Parity Error Interrupt
#define AT91C_US_TXEMPTY          (0x1 <<  9) // (DBGU) TXEMPTY Interrupt
#define AT91C_US_TXBUFE           (0x1 << 11) // (DBGU) TXBUFE Interrupt
#define AT91C_US_RXBUFF           (0x1 << 12) // (DBGU) RXBUFF Interrupt
#define AT91C_US_COMM_TX          (0x1 << 30) // (DBGU) COMM_TX Interrupt
#define AT91C_US_COMM_RX          (0x1 << 31) // (DBGU) COMM_RX Interrupt
// -------- DBGU_IDR : (DBGU Offset: 0xc) Debug Unit Interrupt Disable Register --------
// -------- DBGU_IMR : (DBGU Offset: 0x10) Debug Unit Interrupt Mask Register --------
// -------- DBGU_CSR : (DBGU Offset: 0x14) Debug Unit Channel Status Register --------
// -------- DBGU_FNTR : (DBGU Offset: 0x48) Debug Unit FORCE_NTRST Register --------
#define AT91C_US_FORCE_NTRST      (0x1 <<  0) // (DBGU) Force NTRST in JTAG

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Advanced Interrupt Controller
// *****************************************************************************
// *** Register offset in AT91S_AIC structure ***
#define AIC_SMR         ( 0) // Source Mode Register
#define AIC_SVR         (128) // Source Vector Register
#define AIC_IVR         (256) // IRQ Vector Register
#define AIC_FVR         (260) // FIQ Vector Register
#define AIC_ISR         (264) // Interrupt Status Register
#define AIC_IPR         (268) // Interrupt Pending Register
#define AIC_IMR         (272) // Interrupt Mask Register
#define AIC_CISR        (276) // Core Interrupt Status Register
#define AIC_IECR        (288) // Interrupt Enable Command Register
#define AIC_IDCR        (292) // Interrupt Disable Command Register
#define AIC_ICCR        (296) // Interrupt Clear Command Register
#define AIC_ISCR        (300) // Interrupt Set Command Register
#define AIC_EOICR       (304) // End of Interrupt Command Register
#define AIC_SPU         (308) // Spurious Vector Register
#define AIC_DCR         (312) // Debug Control Register (Protect)
#define AIC_FFER        (320) // Fast Forcing Enable Register
#define AIC_FFDR        (324) // Fast Forcing Disable Register
#define AIC_FFSR        (328) // Fast Forcing Status Register
// -------- AIC_SMR : (AIC Offset: 0x0) Control Register --------
#define AT91C_AIC_PRIOR           (0x7 <<  0) // (AIC) Priority Level
#define 	AT91C_AIC_PRIOR_LOWEST               (0x0) // (AIC) Lowest priority level
#define 	AT91C_AIC_PRIOR_HIGHEST              (0x7) // (AIC) Highest priority level
#define AT91C_AIC_SRCTYPE         (0x3 <<  5) // (AIC) Interrupt Source Type
#define 	AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL        (0x0 <<  5) // (AIC) External Sources Code Label Low-level Sensitive
#define 	AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL       (0x0 <<  5) // (AIC) Internal Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE    (0x1 <<  5) // (AIC) Internal Sources Code Label Positive Edge triggered
#define 	AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE    (0x1 <<  5) // (AIC) External Sources Code Label Negative Edge triggered
#define 	AT91C_AIC_SRCTYPE_HIGH_LEVEL           (0x2 <<  5) // (AIC) Internal Or External Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_POSITIVE_EDGE        (0x3 <<  5) // (AIC) Internal Or External Sources Code Label Positive Edge triggered
// -------- AIC_CISR : (AIC Offset: 0x114) AIC Core Interrupt Status Register --------
#define AT91C_AIC_NFIQ            (0x1 <<  0) // (AIC) NFIQ Status
#define AT91C_AIC_NIRQ            (0x1 <<  1) // (AIC) NIRQ Status
// -------- AIC_DCR : (AIC Offset: 0x138) AIC Debug Control Register (Protect) --------
#define AT91C_AIC_DCR_PROT        (0x1 <<  0) // (AIC) Protection Mode
#define AT91C_AIC_DCR_GMSK        (0x1 <<  1) // (AIC) General Mask

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Parallel Input Output Controler
// *****************************************************************************
// *** Register offset in AT91S_PIO structure ***
#define PIO_PER(p)         ( 0 + (p) * 0x200) // PIO Enable Register
#define PIO_PDR(p)         ( 4 + (p) * 0x200) // PIO Disable Register
#define PIO_PSR(p)         ( 8 + (p) * 0x200) // PIO Status Register
#define PIO_OER(p)         (16 + (p) * 0x200) // Output Enable Register
#define PIO_ODR(p)         (20 + (p) * 0x200) // Output Disable Registerr
#define PIO_OSR(p)         (24 + (p) * 0x200) // Output Status Register
#define PIO_IFER(p)        (32 + (p) * 0x200) // Input Filter Enable Register
#define PIO_IFDR(p)        (36 + (p) * 0x200) // Input Filter Disable Register
#define PIO_IFSR(p)        (40 + (p) * 0x200) // Input Filter Status Register
#define PIO_SODR(p)        (48 + (p) * 0x200) // Set Output Data Register
#define PIO_CODR(p)        (52 + (p) * 0x200) // Clear Output Data Register
#define PIO_ODSR(p)        (56 + (p) * 0x200) // Output Data Status Register
#define PIO_PDSR(p)        (60 + (p) * 0x200) // Pin Data Status Register
#define PIO_IER(p)         (64 + (p) * 0x200) // Interrupt Enable Register
#define PIO_IDR(p)         (68 + (p) * 0x200) // Interrupt Disable Register
#define PIO_IMR(p)         (72 + (p) * 0x200) // Interrupt Mask Register
#define PIO_ISR(p)         (76 + (p) * 0x200) // Interrupt Status Register
#define PIO_MDER(p)        (80 + (p) * 0x200) // Multi-driver Enable Register
#define PIO_MDDR(p)        (84 + (p) * 0x200) // Multi-driver Disable Register
#define PIO_MDSR(p)        (88 + (p) * 0x200) // Multi-driver Status Register
#define PIO_PPUDR(p)       (96 + (p) * 0x200) // Pull-up Disable Register
#define PIO_PPUER(p)       (100 + (p) * 0x200) // Pull-up Enable Register
#define PIO_PPUSR(p)       (104 + (p) * 0x200) // Pull-up Status Register
#define PIO_ASR(p)         (112 + (p) * 0x200) // Select A Register
#define PIO_BSR(p)         (116 + (p) * 0x200) // Select B Register
#define PIO_ABSR(p)        (120 + (p) * 0x200) // AB Select Status Register
#define PIO_OWER(p)        (160 + (p) * 0x200) // Output Write Enable Register
#define PIO_OWDR(p)        (164 + (p) * 0x200) // Output Write Disable Register
#define PIO_OWSR(p)        (168 + (p) * 0x200) // Output Write Status Register

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Clock Generator Controler
// *****************************************************************************
// *** Register offset in AT91S_CKGR structure ***
#define CKGR_MOR        ( 0) // Main Oscillator Register
#define CKGR_MCFR       ( 4) // Main Clock  Frequency Register
#define CKGR_PLLAR      ( 8) // PLL A Register
#define CKGR_PLLBR      (12) // PLL B Register
// -------- CKGR_MOR : (CKGR Offset: 0x0) Main Oscillator Register --------
#define AT91C_CKGR_MOSCEN         (0x1 <<  0) // (CKGR) Main Oscillator Enable
#define AT91C_CKGR_OSCBYPASS      (0x1 <<  1) // (CKGR) Main Oscillator Bypass
#define AT91C_CKGR_OSCOUNT        (0xFF <<  8) // (CKGR) Main Oscillator Start-up Time
// -------- CKGR_MCFR : (CKGR Offset: 0x4) Main Clock Frequency Register --------
#define AT91C_CKGR_MAINF          (0xFFFF <<  0) // (CKGR) Main Clock Frequency
#define AT91C_CKGR_MAINRDY        (0x1 << 16) // (CKGR) Main Clock Ready
// -------- CKGR_PLLAR : (CKGR Offset: 0x8) PLL A Register --------
#define AT91C_CKGR_DIVA           (0xFF <<  0) // (CKGR) Divider A Selected
#define 	AT91C_CKGR_DIVA_0                    (0x0) // (CKGR) Divider A output is 0
#define 	AT91C_CKGR_DIVA_BYPASS               (0x1) // (CKGR) Divider A is bypassed
#define AT91C_CKGR_PLLACOUNT      (0x3F <<  8) // (CKGR) PLL A Counter
#define AT91C_CKGR_OUTA           (0x3 << 14) // (CKGR) PLL A Output Frequency Range
#define 	AT91C_CKGR_OUTA_0                    (0x0 << 14) // (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_1                    (0x1 << 14) // (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_2                    (0x2 << 14) // (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_3                    (0x3 << 14) // (CKGR) Please refer to the PLLA datasheet
#define AT91C_CKGR_MULA           (0x7FF << 16) // (CKGR) PLL A Multiplier
#define AT91C_CKGR_SRCA           (0x1 << 29) // (CKGR)
// -------- CKGR_PLLBR : (CKGR Offset: 0xc) PLL B Register --------
#define AT91C_CKGR_DIVB           (0xFF <<  0) // (CKGR) Divider B Selected
#define 	AT91C_CKGR_DIVB_0                    (0x0) // (CKGR) Divider B output is 0
#define 	AT91C_CKGR_DIVB_BYPASS               (0x1) // (CKGR) Divider B is bypassed
#define AT91C_CKGR_PLLBCOUNT      (0x3F <<  8) // (CKGR) PLL B Counter
#define AT91C_CKGR_OUTB           (0x3 << 14) // (CKGR) PLL B Output Frequency Range
#define 	AT91C_CKGR_OUTB_0                    (0x0 << 14) // (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_1                    (0x1 << 14) // (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_2                    (0x2 << 14) // (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_3                    (0x3 << 14) // (CKGR) Please refer to the PLLB datasheet
#define AT91C_CKGR_MULB           (0x7FF << 16) // (CKGR) PLL B Multiplier
#define AT91C_CKGR_USBDIV         (0x3 << 28) // (CKGR) Divider for USB Clocks
#define 	AT91C_CKGR_USBDIV_0                    (0x0 << 28) // (CKGR) Divider output is PLL clock output
#define 	AT91C_CKGR_USBDIV_1                    (0x1 << 28) // (CKGR) Divider output is PLL clock output divided by 2
#define 	AT91C_CKGR_USBDIV_2                    (0x2 << 28) // (CKGR) Divider output is PLL clock output divided by 4

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Power Management Controler
// *****************************************************************************
// *** Register offset in AT91S_PMC structure ***
#define PMC_SCER        ( 0) // System Clock Enable Register
#define PMC_SCDR        ( 4) // System Clock Disable Register
#define PMC_SCSR        ( 8) // System Clock Status Register
#define PMC_PCER        (16) // Peripheral Clock Enable Register
#define PMC_PCDR        (20) // Peripheral Clock Disable Register
#define PMC_PCSR        (24) // Peripheral Clock Status Register
#define PMC_MOR         (32) // Main Oscillator Register
#define PMC_MCFR        (36) // Main Clock  Frequency Register
#define PMC_PLLAR       (40) // PLL A Register
#define PMC_PLLBR       (44) // PLL B Register
#define PMC_MCKR        (48) // Master Clock Register
#define PMC_PCKR        (64) // Programmable Clock Register
#define PMC_IER         (96) // Interrupt Enable Register
#define PMC_IDR         (100) // Interrupt Disable Register
#define PMC_SR          (104) // Status Register
#define PMC_IMR         (108) // Interrupt Mask Register
// -------- PMC_SCER : (PMC Offset: 0x0) System Clock Enable Register --------
#define AT91C_PMC_PCK             (0x1 <<  0) // (PMC) Processor Clock
#define AT91C_PMC_OTG             (0x1 <<  5) // (PMC) USB OTG Clock
#define AT91C_PMC_UHP             (0x1 <<  6) // (PMC) USB Host Port Clock
#define AT91C_PMC_UDP             (0x1 <<  7) // (PMC) USB Device Port Clock
#define AT91C_PMC_PCK0            (0x1 <<  8) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK1            (0x1 <<  9) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK2            (0x1 << 10) // (PMC) Programmable Clock Output
#define AT91C_PMC_PCK3            (0x1 << 11) // (PMC) Programmable Clock Output
// -------- PMC_SCDR : (PMC Offset: 0x4) System Clock Disable Register --------
// -------- PMC_SCSR : (PMC Offset: 0x8) System Clock Status Register --------
// -------- CKGR_MOR : (PMC Offset: 0x20) Main Oscillator Register --------
// -------- CKGR_MCFR : (PMC Offset: 0x24) Main Clock Frequency Register --------
// -------- CKGR_PLLAR : (PMC Offset: 0x28) PLL A Register --------
// -------- CKGR_PLLBR : (PMC Offset: 0x2c) PLL B Register --------
// -------- PMC_MCKR : (PMC Offset: 0x30) Master Clock Register --------
#define AT91C_PMC_CSS             (0x3 <<  0) // (PMC) Programmable Clock Selection
#define 	AT91C_PMC_CSS_SLOW_CLK             (0x0) // (PMC) Slow Clock is selected
#define 	AT91C_PMC_CSS_MAIN_CLK             (0x1) // (PMC) Main Clock is selected
#define 	AT91C_PMC_CSS_PLLA_CLK             (0x2) // (PMC) Clock from PLL A is selected
#define 	AT91C_PMC_CSS_PLLB_CLK             (0x3) // (PMC) Clock from PLL B is selected
#define AT91C_PMC_PRES            (0x7 <<  2) // (PMC) Programmable Clock Prescaler
#define 	AT91C_PMC_PRES_CLK                  (0x0 <<  2) // (PMC) Selected clock
#define 	AT91C_PMC_PRES_CLK_2                (0x1 <<  2) // (PMC) Selected clock divided by 2
#define 	AT91C_PMC_PRES_CLK_4                (0x2 <<  2) // (PMC) Selected clock divided by 4
#define 	AT91C_PMC_PRES_CLK_8                (0x3 <<  2) // (PMC) Selected clock divided by 8
#define 	AT91C_PMC_PRES_CLK_16               (0x4 <<  2) // (PMC) Selected clock divided by 16
#define 	AT91C_PMC_PRES_CLK_32               (0x5 <<  2) // (PMC) Selected clock divided by 32
#define 	AT91C_PMC_PRES_CLK_64               (0x6 <<  2) // (PMC) Selected clock divided by 64
#define AT91C_PMC_MDIV            (0x3 <<  8) // (PMC) Master Clock Division
#define 	AT91C_PMC_MDIV_1                    (0x0 <<  8) // (PMC) The master clock and the processor clock are the same
#define 	AT91C_PMC_MDIV_2                    (0x1 <<  8) // (PMC) The processor clock is twice as fast as the master clock
#define 	AT91C_PMC_MDIV_3                    (0x2 <<  8) // (PMC) The processor clock is four times faster than the master clock
// -------- PMC_PCKR : (PMC Offset: 0x40) Programmable Clock Register --------
// -------- PMC_IER : (PMC Offset: 0x60) PMC Interrupt Enable Register --------
#define AT91C_PMC_MOSCS           (0x1 <<  0) // (PMC) MOSC Status/Enable/Disable/Mask
#define AT91C_PMC_LOCKA           (0x1 <<  1) // (PMC) PLL A Status/Enable/Disable/Mask
#define AT91C_PMC_LOCKB           (0x1 <<  2) // (PMC) PLL B Status/Enable/Disable/Mask
#define AT91C_PMC_MCKRDY          (0x1 <<  3) // (PMC) Master Clock Status/Enable/Disable/Mask
#define AT91C_PMC_PCK0RDY         (0x1 <<  8) // (PMC) PCK0_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK1RDY         (0x1 <<  9) // (PMC) PCK1_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK2RDY         (0x1 << 10) // (PMC) PCK2_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK3RDY         (0x1 << 11) // (PMC) PCK3_RDY Status/Enable/Disable/Mask
// -------- PMC_IDR : (PMC Offset: 0x64) PMC Interrupt Disable Register --------
// -------- PMC_SR : (PMC Offset: 0x68) PMC Status Register --------
// -------- PMC_IMR : (PMC Offset: 0x6c) PMC Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Reset Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_RSTC structure ***
#define RSTC_RCR        ( 0) // Reset Control Register
#define RSTC_RSR        ( 4) // Reset Status Register
#define RSTC_RMR        ( 8) // Reset Mode Register
// -------- RSTC_RCR : (RSTC Offset: 0x0) Reset Control Register --------
#define AT91C_RSTC_PROCRST        (0x1 <<  0) // (RSTC) Processor Reset
#define AT91C_RSTC_ICERST         (0x1 <<  1) // (RSTC) ICE Interface Reset
#define AT91C_RSTC_PERRST         (0x1 <<  2) // (RSTC) Peripheral Reset
#define AT91C_RSTC_EXTRST         (0x1 <<  3) // (RSTC) External Reset
#define AT91C_RSTC_KEY            (0xFF << 24) // (RSTC) Password
// -------- RSTC_RSR : (RSTC Offset: 0x4) Reset Status Register --------
#define AT91C_RSTC_URSTS          (0x1 <<  0) // (RSTC) User Reset Status
#define AT91C_RSTC_RSTTYP         (0x7 <<  8) // (RSTC) Reset Type
#define 	AT91C_RSTC_RSTTYP_GENERAL              (0x0 <<  8) // (RSTC) General reset. Both VDDCORE and VDDBU rising.
#define 	AT91C_RSTC_RSTTYP_WAKEUP               (0x1 <<  8) // (RSTC) WakeUp Reset. VDDCORE rising.
#define 	AT91C_RSTC_RSTTYP_WATCHDOG             (0x2 <<  8) // (RSTC) Watchdog Reset. Watchdog overflow occured.
#define 	AT91C_RSTC_RSTTYP_SOFTWARE             (0x3 <<  8) // (RSTC) Software Reset. Processor reset required by the software.
#define 	AT91C_RSTC_RSTTYP_USER                 (0x4 <<  8) // (RSTC) User Reset. NRST pin detected low.
#define AT91C_RSTC_NRSTL          (0x1 << 16) // (RSTC) NRST pin level
#define AT91C_RSTC_SRCMP          (0x1 << 17) // (RSTC) Software Reset Command in Progress.
// -------- RSTC_RMR : (RSTC Offset: 0x8) Reset Mode Register --------
#define AT91C_RSTC_URSTEN         (0x1 <<  0) // (RSTC) User Reset Enable
#define AT91C_RSTC_URSTIEN        (0x1 <<  4) // (RSTC) User Reset Interrupt Enable
#define AT91C_RSTC_ERSTL          (0xF <<  8) // (RSTC) User Reset Enable

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Shut Down Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_SHDWC structure ***
#define SHDWC_SHCR      ( 0) // Shut Down Control Register
#define SHDWC_SHMR      ( 4) // Shut Down Mode Register
#define SHDWC_SHSR      ( 8) // Shut Down Status Register
// -------- SHDWC_SHCR : (SHDWC Offset: 0x0) Shut Down Control Register --------
#define AT91C_SHDWC_SHDW          (0x1 <<  0) // (SHDWC) Processor Reset
#define AT91C_SHDWC_KEY           (0xFF << 24) // (SHDWC) Shut down KEY Password
// -------- SHDWC_SHMR : (SHDWC Offset: 0x4) Shut Down Mode Register --------
#define AT91C_SHDWC_WKMODE0       (0x3 <<  0) // (SHDWC) Wake Up 0 Mode Selection
#define 	AT91C_SHDWC_WKMODE0_NONE                 (0x0) // (SHDWC) None. No detection is performed on the wake up input.
#define 	AT91C_SHDWC_WKMODE0_HIGH                 (0x1) // (SHDWC) High Level.
#define 	AT91C_SHDWC_WKMODE0_LOW                  (0x2) // (SHDWC) Low Level.
#define 	AT91C_SHDWC_WKMODE0_ANYLEVEL             (0x3) // (SHDWC) Any level change.
#define AT91C_SHDWC_CPTWK0        (0xF <<  4) // (SHDWC) Counter On Wake Up 0
#define AT91C_SHDWC_RTTWKEN       (0x1 << 16) // (SHDWC) Real Time Timer Wake Up Enable
// -------- SHDWC_SHSR : (SHDWC Offset: 0x8) Shut Down Status Register --------
#define AT91C_SHDWC_WAKEUP0       (0x1 <<  0) // (SHDWC) Wake Up 0 Status
#define AT91C_SHDWC_RTTWK         (0x1 << 16) // (SHDWC) Real Time Timer wake Up

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Real Time Timer Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_RTTC structure ***
#define RTTC_RTMR       ( 0) // Real-time Mode Register
#define RTTC_RTAR       ( 4) // Real-time Alarm Register
#define RTTC_RTVR       ( 8) // Real-time Value Register
#define RTTC_RTSR       (12) // Real-time Status Register
// -------- RTTC_RTMR : (RTTC Offset: 0x0) Real-time Mode Register --------
#define AT91C_RTTC_RTPRES         (0xFFFF <<  0) // (RTTC) Real-time Timer Prescaler Value
#define AT91C_RTTC_ALMIEN         (0x1 << 16) // (RTTC) Alarm Interrupt Enable
#define AT91C_RTTC_RTTINCIEN      (0x1 << 17) // (RTTC) Real Time Timer Increment Interrupt Enable
#define AT91C_RTTC_RTTRST         (0x1 << 18) // (RTTC) Real Time Timer Restart
// -------- RTTC_RTAR : (RTTC Offset: 0x4) Real-time Alarm Register --------
#define AT91C_RTTC_ALMV           (0x0 <<  0) // (RTTC) Alarm Value
// -------- RTTC_RTVR : (RTTC Offset: 0x8) Current Real-time Value Register --------
#define AT91C_RTTC_CRTV           (0x0 <<  0) // (RTTC) Current Real-time Value
// -------- RTTC_RTSR : (RTTC Offset: 0xc) Real-time Status Register --------
#define AT91C_RTTC_ALMS           (0x1 <<  0) // (RTTC) Real-time Alarm Status
#define AT91C_RTTC_RTTINC         (0x1 <<  1) // (RTTC) Real-time Timer Increment

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Periodic Interval Timer Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_PITC structure ***
#define PITC_PIMR       ( 0) // Period Interval Mode Register
#define PITC_PISR       ( 4) // Period Interval Status Register
#define PITC_PIVR       ( 8) // Period Interval Value Register
#define PITC_PIIR       (12) // Period Interval Image Register
// -------- PITC_PIMR : (PITC Offset: 0x0) Periodic Interval Mode Register --------
#define AT91C_PITC_PIV            (0xFFFFF <<  0) // (PITC) Periodic Interval Value
#define AT91C_PITC_PITEN          (0x1 << 24) // (PITC) Periodic Interval Timer Enabled
#define AT91C_PITC_PITIEN         (0x1 << 25) // (PITC) Periodic Interval Timer Interrupt Enable
// -------- PITC_PISR : (PITC Offset: 0x4) Periodic Interval Status Register --------
#define AT91C_PITC_PITS           (0x1 <<  0) // (PITC) Periodic Interval Timer Status
// -------- PITC_PIVR : (PITC Offset: 0x8) Periodic Interval Value Register --------
#define AT91C_PITC_CPIV           (0xFFFFF <<  0) // (PITC) Current Periodic Interval Value
#define AT91C_PITC_PICNT          (0xFFF << 20) // (PITC) Periodic Interval Counter
// -------- PITC_PIIR : (PITC Offset: 0xc) Periodic Interval Image Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Watchdog Timer Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_WDTC structure ***
#define WDTC_WDCR       ( 0) // Watchdog Control Register
#define WDTC_WDMR       ( 4) // Watchdog Mode Register
#define WDTC_WDSR       ( 8) // Watchdog Status Register
// -------- WDTC_WDCR : (WDTC Offset: 0x0) Periodic Interval Image Register --------
#define AT91C_WDTC_WDRSTT         (0x1 <<  0) // (WDTC) Watchdog Restart
#define AT91C_WDTC_KEY            (0xFF << 24) // (WDTC) Watchdog KEY Password
// -------- WDTC_WDMR : (WDTC Offset: 0x4) Watchdog Mode Register --------
#define AT91C_WDTC_WDV            (0xFFF <<  0) // (WDTC) Watchdog Timer Restart
#define AT91C_WDTC_WDFIEN         (0x1 << 12) // (WDTC) Watchdog Fault Interrupt Enable
#define AT91C_WDTC_WDRSTEN        (0x1 << 13) // (WDTC) Watchdog Reset Enable
#define AT91C_WDTC_WDRPROC        (0x1 << 14) // (WDTC) Watchdog Timer Restart
#define AT91C_WDTC_WDDIS          (0x1 << 15) // (WDTC) Watchdog Disable
#define AT91C_WDTC_WDD            (0xFFF << 16) // (WDTC) Watchdog Delta Value
#define AT91C_WDTC_WDDBGHLT       (0x1 << 28) // (WDTC) Watchdog Debug Halt
#define AT91C_WDTC_WDIDLEHLT      (0x1 << 29) // (WDTC) Watchdog Idle Halt
// -------- WDTC_WDSR : (WDTC Offset: 0x8) Watchdog Status Register --------
#define AT91C_WDTC_WDUNF          (0x1 <<  0) // (WDTC) Watchdog Underflow
#define AT91C_WDTC_WDERR          (0x1 <<  1) // (WDTC) Watchdog Error

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Timer Counter Channel Interface
// *****************************************************************************
// *** Register offset in AT91S_TC structure ***
#define TC_CCR          ( 0) // Channel Control Register
#define TC_CMR          ( 4) // Channel Mode Register (Capture Mode / Waveform Mode)
#define TC_CV           (16) // Counter Value
#define TC_RA           (20) // Register A
#define TC_RB           (24) // Register B
#define TC_RC           (28) // Register C
#define TC_SR           (32) // Status Register
#define TC_IER          (36) // Interrupt Enable Register
#define TC_IDR          (40) // Interrupt Disable Register
#define TC_IMR          (44) // Interrupt Mask Register
// -------- TC_CCR : (TC Offset: 0x0) TC Channel Control Register --------
#define AT91C_TC_CLKEN            (0x1 <<  0) // (TC) Counter Clock Enable Command
#define AT91C_TC_CLKDIS           (0x1 <<  1) // (TC) Counter Clock Disable Command
#define AT91C_TC_SWTRG            (0x1 <<  2) // (TC) Software Trigger Command
// -------- TC_CMR : (TC Offset: 0x4) TC Channel Mode Register: Capture Mode / Waveform Mode --------
#define AT91C_TC_CLKS             (0x7 <<  0) // (TC) Clock Selection
#define 	AT91C_TC_CLKS_TIMER_DIV1_CLOCK     (0x0) // (TC) Clock selected: TIMER_DIV1_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV2_CLOCK     (0x1) // (TC) Clock selected: TIMER_DIV2_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV3_CLOCK     (0x2) // (TC) Clock selected: TIMER_DIV3_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV4_CLOCK     (0x3) // (TC) Clock selected: TIMER_DIV4_CLOCK
#define 	AT91C_TC_CLKS_TIMER_DIV5_CLOCK     (0x4) // (TC) Clock selected: TIMER_DIV5_CLOCK
#define 	AT91C_TC_CLKS_XC0                  (0x5) // (TC) Clock selected: XC0
#define 	AT91C_TC_CLKS_XC1                  (0x6) // (TC) Clock selected: XC1
#define 	AT91C_TC_CLKS_XC2                  (0x7) // (TC) Clock selected: XC2
#define AT91C_TC_CLKI             (0x1 <<  3) // (TC) Clock Invert
#define AT91C_TC_BURST            (0x3 <<  4) // (TC) Burst Signal Selection
#define 	AT91C_TC_BURST_NONE                 (0x0 <<  4) // (TC) The clock is not gated by an external signal
#define 	AT91C_TC_BURST_XC0                  (0x1 <<  4) // (TC) XC0 is ANDed with the selected clock
#define 	AT91C_TC_BURST_XC1                  (0x2 <<  4) // (TC) XC1 is ANDed with the selected clock
#define 	AT91C_TC_BURST_XC2                  (0x3 <<  4) // (TC) XC2 is ANDed with the selected clock
#define AT91C_TC_CPCSTOP          (0x1 <<  6) // (TC) Counter Clock Stopped with RC Compare
#define AT91C_TC_LDBSTOP          (0x1 <<  6) // (TC) Counter Clock Stopped with RB Loading
#define AT91C_TC_LDBDIS           (0x1 <<  7) // (TC) Counter Clock Disabled with RB Loading
#define AT91C_TC_CPCDIS           (0x1 <<  7) // (TC) Counter Clock Disable with RC Compare
#define AT91C_TC_ETRGEDG          (0x3 <<  8) // (TC) External Trigger Edge Selection
#define 	AT91C_TC_ETRGEDG_NONE                 (0x0 <<  8) // (TC) Edge: None
#define 	AT91C_TC_ETRGEDG_RISING               (0x1 <<  8) // (TC) Edge: rising edge
#define 	AT91C_TC_ETRGEDG_FALLING              (0x2 <<  8) // (TC) Edge: falling edge
#define 	AT91C_TC_ETRGEDG_BOTH                 (0x3 <<  8) // (TC) Edge: each edge
#define AT91C_TC_EEVTEDG          (0x3 <<  8) // (TC) External Event Edge Selection
#define 	AT91C_TC_EEVTEDG_NONE                 (0x0 <<  8) // (TC) Edge: None
#define 	AT91C_TC_EEVTEDG_RISING               (0x1 <<  8) // (TC) Edge: rising edge
#define 	AT91C_TC_EEVTEDG_FALLING              (0x2 <<  8) // (TC) Edge: falling edge
#define 	AT91C_TC_EEVTEDG_BOTH                 (0x3 <<  8) // (TC) Edge: each edge
#define AT91C_TC_ABETRG           (0x1 << 10) // (TC) TIOA or TIOB External Trigger Selection
#define AT91C_TC_EEVT             (0x3 << 10) // (TC) External Event  Selection
#define 	AT91C_TC_EEVT_TIOB                 (0x0 << 10) // (TC) Signal selected as external event: TIOB TIOB direction: input
#define 	AT91C_TC_EEVT_XC0                  (0x1 << 10) // (TC) Signal selected as external event: XC0 TIOB direction: output
#define 	AT91C_TC_EEVT_XC1                  (0x2 << 10) // (TC) Signal selected as external event: XC1 TIOB direction: output
#define 	AT91C_TC_EEVT_XC2                  (0x3 << 10) // (TC) Signal selected as external event: XC2 TIOB direction: output
#define AT91C_TC_ENETRG           (0x1 << 12) // (TC) External Event Trigger enable
#define AT91C_TC_WAVESEL          (0x3 << 13) // (TC) Waveform  Selection
#define 	AT91C_TC_WAVESEL_UP                   (0x0 << 13) // (TC) UP mode without atomatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN               (0x1 << 13) // (TC) UPDOWN mode without automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UP_AUTO              (0x2 << 13) // (TC) UP mode with automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN_AUTO          (0x3 << 13) // (TC) UPDOWN mode with automatic trigger on RC Compare
#define AT91C_TC_CPCTRG           (0x1 << 14) // (TC) RC Compare Trigger Enable
#define AT91C_TC_WAVE             (0x1 << 15) // (TC)
#define AT91C_TC_LDRA             (0x3 << 16) // (TC) RA Loading Selection
#define 	AT91C_TC_LDRA_NONE                 (0x0 << 16) // (TC) Edge: None
#define 	AT91C_TC_LDRA_RISING               (0x1 << 16) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRA_FALLING              (0x2 << 16) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRA_BOTH                 (0x3 << 16) // (TC) Edge: each edge of TIOA
#define AT91C_TC_ACPA             (0x3 << 16) // (TC) RA Compare Effect on TIOA
#define 	AT91C_TC_ACPA_NONE                 (0x0 << 16) // (TC) Effect: none
#define 	AT91C_TC_ACPA_SET                  (0x1 << 16) // (TC) Effect: set
#define 	AT91C_TC_ACPA_CLEAR                (0x2 << 16) // (TC) Effect: clear
#define 	AT91C_TC_ACPA_TOGGLE               (0x3 << 16) // (TC) Effect: toggle
#define AT91C_TC_LDRB             (0x3 << 18) // (TC) RB Loading Selection
#define 	AT91C_TC_LDRB_NONE                 (0x0 << 18) // (TC) Edge: None
#define 	AT91C_TC_LDRB_RISING               (0x1 << 18) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRB_FALLING              (0x2 << 18) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRB_BOTH                 (0x3 << 18) // (TC) Edge: each edge of TIOA
#define AT91C_TC_ACPC             (0x3 << 18) // (TC) RC Compare Effect on TIOA
#define 	AT91C_TC_ACPC_NONE                 (0x0 << 18) // (TC) Effect: none
#define 	AT91C_TC_ACPC_SET                  (0x1 << 18) // (TC) Effect: set
#define 	AT91C_TC_ACPC_CLEAR                (0x2 << 18) // (TC) Effect: clear
#define 	AT91C_TC_ACPC_TOGGLE               (0x3 << 18) // (TC) Effect: toggle
#define AT91C_TC_AEEVT            (0x3 << 20) // (TC) External Event Effect on TIOA
#define 	AT91C_TC_AEEVT_NONE                 (0x0 << 20) // (TC) Effect: none
#define 	AT91C_TC_AEEVT_SET                  (0x1 << 20) // (TC) Effect: set
#define 	AT91C_TC_AEEVT_CLEAR                (0x2 << 20) // (TC) Effect: clear
#define 	AT91C_TC_AEEVT_TOGGLE               (0x3 << 20) // (TC) Effect: toggle
#define AT91C_TC_ASWTRG           (0x3 << 22) // (TC) Software Trigger Effect on TIOA
#define 	AT91C_TC_ASWTRG_NONE                 (0x0 << 22) // (TC) Effect: none
#define 	AT91C_TC_ASWTRG_SET                  (0x1 << 22) // (TC) Effect: set
#define 	AT91C_TC_ASWTRG_CLEAR                (0x2 << 22) // (TC) Effect: clear
#define 	AT91C_TC_ASWTRG_TOGGLE               (0x3 << 22) // (TC) Effect: toggle
#define AT91C_TC_BCPB             (0x3 << 24) // (TC) RB Compare Effect on TIOB
#define 	AT91C_TC_BCPB_NONE                 (0x0 << 24) // (TC) Effect: none
#define 	AT91C_TC_BCPB_SET                  (0x1 << 24) // (TC) Effect: set
#define 	AT91C_TC_BCPB_CLEAR                (0x2 << 24) // (TC) Effect: clear
#define 	AT91C_TC_BCPB_TOGGLE               (0x3 << 24) // (TC) Effect: toggle
#define AT91C_TC_BCPC             (0x3 << 26) // (TC) RC Compare Effect on TIOB
#define 	AT91C_TC_BCPC_NONE                 (0x0 << 26) // (TC) Effect: none
#define 	AT91C_TC_BCPC_SET                  (0x1 << 26) // (TC) Effect: set
#define 	AT91C_TC_BCPC_CLEAR                (0x2 << 26) // (TC) Effect: clear
#define 	AT91C_TC_BCPC_TOGGLE               (0x3 << 26) // (TC) Effect: toggle
#define AT91C_TC_BEEVT            (0x3 << 28) // (TC) External Event Effect on TIOB
#define 	AT91C_TC_BEEVT_NONE                 (0x0 << 28) // (TC) Effect: none
#define 	AT91C_TC_BEEVT_SET                  (0x1 << 28) // (TC) Effect: set
#define 	AT91C_TC_BEEVT_CLEAR                (0x2 << 28) // (TC) Effect: clear
#define 	AT91C_TC_BEEVT_TOGGLE               (0x3 << 28) // (TC) Effect: toggle
#define AT91C_TC_BSWTRG           (0x3 << 30) // (TC) Software Trigger Effect on TIOB
#define 	AT91C_TC_BSWTRG_NONE                 (0x0 << 30) // (TC) Effect: none
#define 	AT91C_TC_BSWTRG_SET                  (0x1 << 30) // (TC) Effect: set
#define 	AT91C_TC_BSWTRG_CLEAR                (0x2 << 30) // (TC) Effect: clear
#define 	AT91C_TC_BSWTRG_TOGGLE               (0x3 << 30) // (TC) Effect: toggle
// -------- TC_SR : (TC Offset: 0x20) TC Channel Status Register --------
#define AT91C_TC_COVFS            (0x1 <<  0) // (TC) Counter Overflow
#define AT91C_TC_LOVRS            (0x1 <<  1) // (TC) Load Overrun
#define AT91C_TC_CPAS             (0x1 <<  2) // (TC) RA Compare
#define AT91C_TC_CPBS             (0x1 <<  3) // (TC) RB Compare
#define AT91C_TC_CPCS             (0x1 <<  4) // (TC) RC Compare
#define AT91C_TC_LDRAS            (0x1 <<  5) // (TC) RA Loading
#define AT91C_TC_LDRBS            (0x1 <<  6) // (TC) RB Loading
#define AT91C_TC_ETRGS            (0x1 <<  7) // (TC) External Trigger
#define AT91C_TC_CLKSTA           (0x1 << 16) // (TC) Clock Enabling
#define AT91C_TC_MTIOA            (0x1 << 17) // (TC) TIOA Mirror
#define AT91C_TC_MTIOB            (0x1 << 18) // (TC) TIOA Mirror
// -------- TC_IER : (TC Offset: 0x24) TC Channel Interrupt Enable Register --------
// -------- TC_IDR : (TC Offset: 0x28) TC Channel Interrupt Disable Register --------
// -------- TC_IMR : (TC Offset: 0x2c) TC Channel Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Timer Counter Interface
// *****************************************************************************
// *** Register offset in AT91S_TCB structure ***
#define TCB_TC0         ( 0) // TC Channel 0
#define TCB_TC1         (64) // TC Channel 1
#define TCB_TC2         (128) // TC Channel 2
#define TCB_BCR         (192) // TC Block Control Register
#define TCB_BMR         (196) // TC Block Mode Register
// -------- TCB_BCR : (TCB Offset: 0xc0) TC Block Control Register --------
#define AT91C_TCB_SYNC            (0x1 <<  0) // (TCB) Synchro Command
// -------- TCB_BMR : (TCB Offset: 0xc4) TC Block Mode Register --------
#define AT91C_TCB_TC0XC0S         (0x3 <<  0) // (TCB) External Clock Signal 0 Selection
#define 	AT91C_TCB_TC0XC0S_TCLK0                (0x0) // (TCB) TCLK0 connected to XC0
#define 	AT91C_TCB_TC0XC0S_NONE                 (0x1) // (TCB) None signal connected to XC0
#define 	AT91C_TCB_TC0XC0S_TIOA1                (0x2) // (TCB) TIOA1 connected to XC0
#define 	AT91C_TCB_TC0XC0S_TIOA2                (0x3) // (TCB) TIOA2 connected to XC0
#define AT91C_TCB_TC1XC1S         (0x3 <<  2) // (TCB) External Clock Signal 1 Selection
#define 	AT91C_TCB_TC1XC1S_TCLK1                (0x0 <<  2) // (TCB) TCLK1 connected to XC1
#define 	AT91C_TCB_TC1XC1S_NONE                 (0x1 <<  2) // (TCB) None signal connected to XC1
#define 	AT91C_TCB_TC1XC1S_TIOA0                (0x2 <<  2) // (TCB) TIOA0 connected to XC1
#define 	AT91C_TCB_TC1XC1S_TIOA2                (0x3 <<  2) // (TCB) TIOA2 connected to XC1
#define AT91C_TCB_TC2XC2S         (0x3 <<  4) // (TCB) External Clock Signal 2 Selection
#define 	AT91C_TCB_TC2XC2S_TCLK2                (0x0 <<  4) // (TCB) TCLK2 connected to XC2
#define 	AT91C_TCB_TC2XC2S_NONE                 (0x1 <<  4) // (TCB) None signal connected to XC2
#define 	AT91C_TCB_TC2XC2S_TIOA0                (0x2 <<  4) // (TCB) TIOA0 connected to XC2
#define 	AT91C_TCB_TC2XC2S_TIOA1                (0x3 <<  4) // (TCB) TIOA2 connected to XC2

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Multimedia Card Interface
// *****************************************************************************
// *** Register offset in AT91S_MCI structure ***
#define MCI_CR          ( 0) // MCI Control Register
#define MCI_MR          ( 4) // MCI Mode Register
#define MCI_DTOR        ( 8) // MCI Data Timeout Register
#define MCI_SDCR        (12) // MCI SD Card Register
#define MCI_ARGR        (16) // MCI Argument Register
#define MCI_CMDR        (20) // MCI Command Register
#define MCI_RSPR        (32) // MCI Response Register
#define MCI_RDR         (48) // MCI Receive Data Register
#define MCI_TDR         (52) // MCI Transmit Data Register
#define MCI_SR          (64) // MCI Status Register
#define MCI_IER         (68) // MCI Interrupt Enable Register
#define MCI_IDR         (72) // MCI Interrupt Disable Register
#define MCI_IMR         (76) // MCI Interrupt Mask Register
#define MCI_RPR         (256) // Receive Pointer Register
#define MCI_RCR         (260) // Receive Counter Register
#define MCI_TPR         (264) // Transmit Pointer Register
#define MCI_TCR         (268) // Transmit Counter Register
#define MCI_RNPR        (272) // Receive Next Pointer Register
#define MCI_RNCR        (276) // Receive Next Counter Register
#define MCI_TNPR        (280) // Transmit Next Pointer Register
#define MCI_TNCR        (284) // Transmit Next Counter Register
#define MCI_PTCR        (288) // PDC Transfer Control Register
#define MCI_PTSR        (292) // PDC Transfer Status Register
// -------- MCI_CR : (MCI Offset: 0x0) MCI Control Register --------
#define AT91C_MCI_MCIEN           (0x1 <<  0) // (MCI) Multimedia Interface Enable
#define AT91C_MCI_MCIDIS          (0x1 <<  1) // (MCI) Multimedia Interface Disable
#define AT91C_MCI_PWSEN           (0x1 <<  2) // (MCI) Power Save Mode Enable
#define AT91C_MCI_PWSDIS          (0x1 <<  3) // (MCI) Power Save Mode Disable
#define AT91C_MCI_SWRST           (0x1 <<  7) // (MCI) MCI Software reset
// -------- MCI_MR : (MCI Offset: 0x4) MCI Mode Register --------
#define AT91C_MCI_CLKDIV          (0xFF <<  0) // (MCI) Clock Divider
#define AT91C_MCI_PWSDIV          (0x7 <<  8) // (MCI) Power Saving Divider
#define AT91C_MCI_PDCPADV         (0x1 << 14) // (MCI) PDC Padding Value
#define AT91C_MCI_PDCMODE         (0x1 << 15) // (MCI) PDC Oriented Mode
#define AT91C_MCI_BLKLEN          (0xFFF << 18) // (MCI) Data Block Length
// -------- MCI_DTOR : (MCI Offset: 0x8) MCI Data Timeout Register --------
#define AT91C_MCI_DTOCYC          (0xF <<  0) // (MCI) Data Timeout Cycle Number
#define AT91C_MCI_DTOMUL          (0x7 <<  4) // (MCI) Data Timeout Multiplier
#define 	AT91C_MCI_DTOMUL_1                    (0x0 <<  4) // (MCI) DTOCYC x 1
#define 	AT91C_MCI_DTOMUL_16                   (0x1 <<  4) // (MCI) DTOCYC x 16
#define 	AT91C_MCI_DTOMUL_128                  (0x2 <<  4) // (MCI) DTOCYC x 128
#define 	AT91C_MCI_DTOMUL_256                  (0x3 <<  4) // (MCI) DTOCYC x 256
#define 	AT91C_MCI_DTOMUL_1024                 (0x4 <<  4) // (MCI) DTOCYC x 1024
#define 	AT91C_MCI_DTOMUL_4096                 (0x5 <<  4) // (MCI) DTOCYC x 4096
#define 	AT91C_MCI_DTOMUL_65536                (0x6 <<  4) // (MCI) DTOCYC x 65536
#define 	AT91C_MCI_DTOMUL_1048576              (0x7 <<  4) // (MCI) DTOCYC x 1048576
// -------- MCI_SDCR : (MCI Offset: 0xc) MCI SD Card Register --------
#define AT91C_MCI_SCDSEL          (0xF <<  0) // (MCI) SD Card Selector
#define AT91C_MCI_SCDBUS          (0x1 <<  7) // (MCI) SD Card Bus Width
// -------- MCI_CMDR : (MCI Offset: 0x14) MCI Command Register --------
#define AT91C_MCI_CMDNB           (0x3F <<  0) // (MCI) Command Number
#define AT91C_MCI_RSPTYP          (0x3 <<  6) // (MCI) Response Type
#define 	AT91C_MCI_RSPTYP_NO                   (0x0 <<  6) // (MCI) No response
#define 	AT91C_MCI_RSPTYP_48                   (0x1 <<  6) // (MCI) 48-bit response
#define 	AT91C_MCI_RSPTYP_136                  (0x2 <<  6) // (MCI) 136-bit response
#define AT91C_MCI_SPCMD           (0x7 <<  8) // (MCI) Special CMD
#define 	AT91C_MCI_SPCMD_NONE                 (0x0 <<  8) // (MCI) Not a special CMD
#define 	AT91C_MCI_SPCMD_INIT                 (0x1 <<  8) // (MCI) Initialization CMD
#define 	AT91C_MCI_SPCMD_SYNC                 (0x2 <<  8) // (MCI) Synchronized CMD
#define 	AT91C_MCI_SPCMD_IT_CMD               (0x4 <<  8) // (MCI) Interrupt command
#define 	AT91C_MCI_SPCMD_IT_REP               (0x5 <<  8) // (MCI) Interrupt response
#define AT91C_MCI_OPDCMD          (0x1 << 11) // (MCI) Open Drain Command
#define AT91C_MCI_MAXLAT          (0x1 << 12) // (MCI) Maximum Latency for Command to respond
#define AT91C_MCI_TRCMD           (0x3 << 16) // (MCI) Transfer CMD
#define 	AT91C_MCI_TRCMD_NO                   (0x0 << 16) // (MCI) No transfer
#define 	AT91C_MCI_TRCMD_START                (0x1 << 16) // (MCI) Start transfer
#define 	AT91C_MCI_TRCMD_STOP                 (0x2 << 16) // (MCI) Stop transfer
#define AT91C_MCI_TRDIR           (0x1 << 18) // (MCI) Transfer Direction
#define AT91C_MCI_TRTYP           (0x3 << 19) // (MCI) Transfer Type
#define 	AT91C_MCI_TRTYP_BLOCK                (0x0 << 19) // (MCI) Block Transfer type
#define 	AT91C_MCI_TRTYP_MULTIPLE             (0x1 << 19) // (MCI) Multiple Block transfer type
#define 	AT91C_MCI_TRTYP_STREAM               (0x2 << 19) // (MCI) Stream transfer type
// -------- MCI_SR : (MCI Offset: 0x40) MCI Status Register --------
#define AT91C_MCI_CMDRDY          (0x1 <<  0) // (MCI) Command Ready flag
#define AT91C_MCI_RXRDY           (0x1 <<  1) // (MCI) RX Ready flag
#define AT91C_MCI_TXRDY           (0x1 <<  2) // (MCI) TX Ready flag
#define AT91C_MCI_BLKE            (0x1 <<  3) // (MCI) Data Block Transfer Ended flag
#define AT91C_MCI_DTIP            (0x1 <<  4) // (MCI) Data Transfer in Progress flag
#define AT91C_MCI_NOTBUSY         (0x1 <<  5) // (MCI) Data Line Not Busy flag
#define AT91C_MCI_ENDRX           (0x1 <<  6) // (MCI) End of RX Buffer flag
#define AT91C_MCI_ENDTX           (0x1 <<  7) // (MCI) End of TX Buffer flag
#define AT91C_MCI_RXBUFF          (0x1 << 14) // (MCI) RX Buffer Full flag
#define AT91C_MCI_TXBUFE          (0x1 << 15) // (MCI) TX Buffer Empty flag
#define AT91C_MCI_RINDE           (0x1 << 16) // (MCI) Response Index Error flag
#define AT91C_MCI_RDIRE           (0x1 << 17) // (MCI) Response Direction Error flag
#define AT91C_MCI_RCRCE           (0x1 << 18) // (MCI) Response CRC Error flag
#define AT91C_MCI_RENDE           (0x1 << 19) // (MCI) Response End Bit Error flag
#define AT91C_MCI_RTOE            (0x1 << 20) // (MCI) Response Time-out Error flag
#define AT91C_MCI_DCRCE           (0x1 << 21) // (MCI) data CRC Error flag
#define AT91C_MCI_DTOE            (0x1 << 22) // (MCI) Data timeout Error flag
#define AT91C_MCI_OVRE            (0x1 << 30) // (MCI) Overrun flag
#define AT91C_MCI_UNRE            (0x1 << 31) // (MCI) Underrun flag
// -------- MCI_IER : (MCI Offset: 0x44) MCI Interrupt Enable Register --------
// -------- MCI_IDR : (MCI Offset: 0x48) MCI Interrupt Disable Register --------
// -------- MCI_IMR : (MCI Offset: 0x4c) MCI Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Two-wire Interface
// *****************************************************************************
// *** Register offset in AT91S_TWI structure ***
#define TWI_CR          ( 0) // Control Register
#define TWI_MMR         ( 4) // Master Mode Register
#define TWI_IADR        (12) // Internal Address Register
#define TWI_CWGR        (16) // Clock Waveform Generator Register
#define TWI_SR          (32) // Status Register
#define TWI_IER         (36) // Interrupt Enable Register
#define TWI_IDR         (40) // Interrupt Disable Register
#define TWI_IMR         (44) // Interrupt Mask Register
#define TWI_RHR         (48) // Receive Holding Register
#define TWI_THR         (52) // Transmit Holding Register
// -------- TWI_CR : (TWI Offset: 0x0) TWI Control Register --------
#define AT91C_TWI_START           (0x1 <<  0) // (TWI) Send a START Condition
#define AT91C_TWI_STOP            (0x1 <<  1) // (TWI) Send a STOP Condition
#define AT91C_TWI_MSEN            (0x1 <<  2) // (TWI) TWI Master Transfer Enabled
#define AT91C_TWI_MSDIS           (0x1 <<  3) // (TWI) TWI Master Transfer Disabled
#define AT91C_TWI_SWRST           (0x1 <<  7) // (TWI) Software Reset
// -------- TWI_MMR : (TWI Offset: 0x4) TWI Master Mode Register --------
#define AT91C_TWI_IADRSZ          (0x3 <<  8) // (TWI) Internal Device Address Size
#define 	AT91C_TWI_IADRSZ_NO                   (0x0 <<  8) // (TWI) No internal device address
#define 	AT91C_TWI_IADRSZ_1_BYTE               (0x1 <<  8) // (TWI) One-byte internal device address
#define 	AT91C_TWI_IADRSZ_2_BYTE               (0x2 <<  8) // (TWI) Two-byte internal device address
#define 	AT91C_TWI_IADRSZ_3_BYTE               (0x3 <<  8) // (TWI) Three-byte internal device address
#define AT91C_TWI_MREAD           (0x1 << 12) // (TWI) Master Read Direction
#define AT91C_TWI_DADR            (0x7F << 16) // (TWI) Device Address
// -------- TWI_CWGR : (TWI Offset: 0x10) TWI Clock Waveform Generator Register --------
#define AT91C_TWI_CLDIV           (0xFF <<  0) // (TWI) Clock Low Divider
#define AT91C_TWI_CHDIV           (0xFF <<  8) // (TWI) Clock High Divider
#define AT91C_TWI_CKDIV           (0x7 << 16) // (TWI) Clock Divider
// -------- TWI_SR : (TWI Offset: 0x20) TWI Status Register --------
#define AT91C_TWI_TXCOMP          (0x1 <<  0) // (TWI) Transmission Completed
#define AT91C_TWI_RXRDY           (0x1 <<  1) // (TWI) Receive holding register ReaDY
#define AT91C_TWI_TXRDY           (0x1 <<  2) // (TWI) Transmit holding register ReaDY
#define AT91C_TWI_OVRE            (0x1 <<  6) // (TWI) Overrun Error
#define AT91C_TWI_UNRE            (0x1 <<  7) // (TWI) Underrun Error
#define AT91C_TWI_NACK            (0x1 <<  8) // (TWI) Not Acknowledged
// -------- TWI_IER : (TWI Offset: 0x24) TWI Interrupt Enable Register --------
// -------- TWI_IDR : (TWI Offset: 0x28) TWI Interrupt Disable Register --------
// -------- TWI_IMR : (TWI Offset: 0x2c) TWI Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Usart
// *****************************************************************************
// *** Register offset in AT91S_USART structure ***
#define US_CR           ( 0) // Control Register
#define US_MR           ( 4) // Mode Register
#define US_IER          ( 8) // Interrupt Enable Register
#define US_IDR          (12) // Interrupt Disable Register
#define US_IMR          (16) // Interrupt Mask Register
#define US_CSR          (20) // Channel Status Register
#define US_RHR          (24) // Receiver Holding Register
#define US_THR          (28) // Transmitter Holding Register
#define US_BRGR         (32) // Baud Rate Generator Register
#define US_RTOR         (36) // Receiver Time-out Register
#define US_TTGR         (40) // Transmitter Time-guard Register
#define US_FIDI         (64) // FI_DI_Ratio Register
#define US_NER          (68) // Nb Errors Register
#define US_IF           (76) // IRDA_FILTER Register
#define US_RPR          (256) // Receive Pointer Register
#define US_RCR          (260) // Receive Counter Register
#define US_TPR          (264) // Transmit Pointer Register
#define US_TCR          (268) // Transmit Counter Register
#define US_RNPR         (272) // Receive Next Pointer Register
#define US_RNCR         (276) // Receive Next Counter Register
#define US_TNPR         (280) // Transmit Next Pointer Register
#define US_TNCR         (284) // Transmit Next Counter Register
#define US_PTCR         (288) // PDC Transfer Control Register
#define US_PTSR         (292) // PDC Transfer Status Register
// -------- US_CR : (USART Offset: 0x0) Debug Unit Control Register --------
#define AT91C_US_STTBRK           (0x1 <<  9) // (USART) Start Break
#define AT91C_US_STPBRK           (0x1 << 10) // (USART) Stop Break
#define AT91C_US_STTTO            (0x1 << 11) // (USART) Start Time-out
#define AT91C_US_SENDA            (0x1 << 12) // (USART) Send Address
#define AT91C_US_RSTIT            (0x1 << 13) // (USART) Reset Iterations
#define AT91C_US_RSTNACK          (0x1 << 14) // (USART) Reset Non Acknowledge
#define AT91C_US_RETTO            (0x1 << 15) // (USART) Rearm Time-out
#define AT91C_US_DTREN            (0x1 << 16) // (USART) Data Terminal ready Enable
#define AT91C_US_DTRDIS           (0x1 << 17) // (USART) Data Terminal ready Disable
#define AT91C_US_RTSEN            (0x1 << 18) // (USART) Request to Send enable
#define AT91C_US_RTSDIS           (0x1 << 19) // (USART) Request to Send Disable
// -------- US_MR : (USART Offset: 0x4) Debug Unit Mode Register --------
#define AT91C_US_USMODE           (0xF <<  0) // (USART) Usart mode
#define 	AT91C_US_USMODE_NORMAL               (0x0) // (USART) Normal
#define 	AT91C_US_USMODE_RS485                (0x1) // (USART) RS485
#define 	AT91C_US_USMODE_HWHSH                (0x2) // (USART) Hardware Handshaking
#define 	AT91C_US_USMODE_MODEM                (0x3) // (USART) Modem
#define 	AT91C_US_USMODE_ISO7816_0            (0x4) // (USART) ISO7816 protocol: T = 0
#define 	AT91C_US_USMODE_ISO7816_1            (0x6) // (USART) ISO7816 protocol: T = 1
#define 	AT91C_US_USMODE_IRDA                 (0x8) // (USART) IrDA
#define 	AT91C_US_USMODE_SWHSH                (0xC) // (USART) Software Handshaking
#define AT91C_US_CLKS             (0x3 <<  4) // (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CLKS_CLOCK                (0x0 <<  4) // (USART) Clock
#define 	AT91C_US_CLKS_FDIV1                (0x1 <<  4) // (USART) fdiv1
#define 	AT91C_US_CLKS_SLOW                 (0x2 <<  4) // (USART) slow_clock (ARM)
#define 	AT91C_US_CLKS_EXT                  (0x3 <<  4) // (USART) External (SCK)
#define AT91C_US_CHRL             (0x3 <<  6) // (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CHRL_5_BITS               (0x0 <<  6) // (USART) Character Length: 5 bits
#define 	AT91C_US_CHRL_6_BITS               (0x1 <<  6) // (USART) Character Length: 6 bits
#define 	AT91C_US_CHRL_7_BITS               (0x2 <<  6) // (USART) Character Length: 7 bits
#define 	AT91C_US_CHRL_8_BITS               (0x3 <<  6) // (USART) Character Length: 8 bits
#define AT91C_US_SYNC             (0x1 <<  8) // (USART) Synchronous Mode Select
#define AT91C_US_NBSTOP           (0x3 << 12) // (USART) Number of Stop bits
#define 	AT91C_US_NBSTOP_1_BIT                (0x0 << 12) // (USART) 1 stop bit
#define 	AT91C_US_NBSTOP_15_BIT               (0x1 << 12) // (USART) Asynchronous (SYNC=0) 2 stop bits Synchronous (SYNC=1) 2 stop bits
#define 	AT91C_US_NBSTOP_2_BIT                (0x2 << 12) // (USART) 2 stop bits
#define AT91C_US_MSBF             (0x1 << 16) // (USART) Bit Order
#define AT91C_US_MODE9            (0x1 << 17) // (USART) 9-bit Character length
#define AT91C_US_CKLO             (0x1 << 18) // (USART) Clock Output Select
#define AT91C_US_OVER             (0x1 << 19) // (USART) Over Sampling Mode
#define AT91C_US_INACK            (0x1 << 20) // (USART) Inhibit Non Acknowledge
#define AT91C_US_DSNACK           (0x1 << 21) // (USART) Disable Successive NACK
#define AT91C_US_MAX_ITER         (0x1 << 24) // (USART) Number of Repetitions
#define AT91C_US_FILTER           (0x1 << 28) // (USART) Receive Line Filter
// -------- US_IER : (USART Offset: 0x8) Debug Unit Interrupt Enable Register --------
#define AT91C_US_RXBRK            (0x1 <<  2) // (USART) Break Received/End of Break
#define AT91C_US_TIMEOUT          (0x1 <<  8) // (USART) Receiver Time-out
#define AT91C_US_ITERATION        (0x1 << 10) // (USART) Max number of Repetitions Reached
#define AT91C_US_NACK             (0x1 << 13) // (USART) Non Acknowledge
#define AT91C_US_RIIC             (0x1 << 16) // (USART) Ring INdicator Input Change Flag
#define AT91C_US_DSRIC            (0x1 << 17) // (USART) Data Set Ready Input Change Flag
#define AT91C_US_DCDIC            (0x1 << 18) // (USART) Data Carrier Flag
#define AT91C_US_CTSIC            (0x1 << 19) // (USART) Clear To Send Input Change Flag
// -------- US_IDR : (USART Offset: 0xc) Debug Unit Interrupt Disable Register --------
// -------- US_IMR : (USART Offset: 0x10) Debug Unit Interrupt Mask Register --------
// -------- US_CSR : (USART Offset: 0x14) Debug Unit Channel Status Register --------
#define AT91C_US_RI               (0x1 << 20) // (USART) Image of RI Input
#define AT91C_US_DSR              (0x1 << 21) // (USART) Image of DSR Input
#define AT91C_US_DCD              (0x1 << 22) // (USART) Image of DCD Input
#define AT91C_US_CTS              (0x1 << 23) // (USART) Image of CTS Input

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Synchronous Serial Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_SSC structure ***
#define SSC_CR          ( 0) // Control Register
#define SSC_CMR         ( 4) // Clock Mode Register
#define SSC_RCMR        (16) // Receive Clock ModeRegister
#define SSC_RFMR        (20) // Receive Frame Mode Register
#define SSC_TCMR        (24) // Transmit Clock Mode Register
#define SSC_TFMR        (28) // Transmit Frame Mode Register
#define SSC_RHR         (32) // Receive Holding Register
#define SSC_THR         (36) // Transmit Holding Register
#define SSC_RSHR        (48) // Receive Sync Holding Register
#define SSC_TSHR        (52) // Transmit Sync Holding Register
#define SSC_SR          (64) // Status Register
#define SSC_IER         (68) // Interrupt Enable Register
#define SSC_IDR         (72) // Interrupt Disable Register
#define SSC_IMR         (76) // Interrupt Mask Register
#define SSC_RPR         (256) // Receive Pointer Register
#define SSC_RCR         (260) // Receive Counter Register
#define SSC_TPR         (264) // Transmit Pointer Register
#define SSC_TCR         (268) // Transmit Counter Register
#define SSC_RNPR        (272) // Receive Next Pointer Register
#define SSC_RNCR        (276) // Receive Next Counter Register
#define SSC_TNPR        (280) // Transmit Next Pointer Register
#define SSC_TNCR        (284) // Transmit Next Counter Register
#define SSC_PTCR        (288) // PDC Transfer Control Register
#define SSC_PTSR        (292) // PDC Transfer Status Register
// -------- SSC_CR : (SSC Offset: 0x0) SSC Control Register --------
#define AT91C_SSC_RXEN            (0x1 <<  0) // (SSC) Receive Enable
#define AT91C_SSC_RXDIS           (0x1 <<  1) // (SSC) Receive Disable
#define AT91C_SSC_TXEN            (0x1 <<  8) // (SSC) Transmit Enable
#define AT91C_SSC_TXDIS           (0x1 <<  9) // (SSC) Transmit Disable
#define AT91C_SSC_SWRST           (0x1 << 15) // (SSC) Software Reset
// -------- SSC_RCMR : (SSC Offset: 0x10) SSC Receive Clock Mode Register --------
#define AT91C_SSC_CKS             (0x3 <<  0) // (SSC) Receive/Transmit Clock Selection
#define 	AT91C_SSC_CKS_DIV                  (0x0) // (SSC) Divided Clock
#define 	AT91C_SSC_CKS_TK                   (0x1) // (SSC) TK Clock signal
#define 	AT91C_SSC_CKS_RK                   (0x2) // (SSC) RK pin
#define AT91C_SSC_CKO             (0x7 <<  2) // (SSC) Receive/Transmit Clock Output Mode Selection
#define 	AT91C_SSC_CKO_NONE                 (0x0 <<  2) // (SSC) Receive/Transmit Clock Output Mode: None RK pin: Input-only
#define 	AT91C_SSC_CKO_CONTINOUS            (0x1 <<  2) // (SSC) Continuous Receive/Transmit Clock RK pin: Output
#define 	AT91C_SSC_CKO_DATA_TX              (0x2 <<  2) // (SSC) Receive/Transmit Clock only during data transfers RK pin: Output
#define AT91C_SSC_CKI             (0x1 <<  5) // (SSC) Receive/Transmit Clock Inversion
#define AT91C_SSC_CKG             (0x3 <<  6) // (SSC) Receive/Transmit Clock Gating Selection
#define 	AT91C_SSC_CKG_NONE                 (0x0 <<  6) // (SSC) Receive/Transmit Clock Gating: None, continuous clock
#define 	AT91C_SSC_CKG_LOW                  (0x1 <<  6) // (SSC) Receive/Transmit Clock enabled only if RF Low
#define 	AT91C_SSC_CKG_HIGH                 (0x2 <<  6) // (SSC) Receive/Transmit Clock enabled only if RF High
#define AT91C_SSC_START           (0xF <<  8) // (SSC) Receive/Transmit Start Selection
#define 	AT91C_SSC_START_CONTINOUS            (0x0 <<  8) // (SSC) Continuous, as soon as the receiver is enabled, and immediately after the end of transfer of the previous data.
#define 	AT91C_SSC_START_TX                   (0x1 <<  8) // (SSC) Transmit/Receive start
#define 	AT91C_SSC_START_LOW_RF               (0x2 <<  8) // (SSC) Detection of a low level on RF input
#define 	AT91C_SSC_START_HIGH_RF              (0x3 <<  8) // (SSC) Detection of a high level on RF input
#define 	AT91C_SSC_START_FALL_RF              (0x4 <<  8) // (SSC) Detection of a falling edge on RF input
#define 	AT91C_SSC_START_RISE_RF              (0x5 <<  8) // (SSC) Detection of a rising edge on RF input
#define 	AT91C_SSC_START_LEVEL_RF             (0x6 <<  8) // (SSC) Detection of any level change on RF input
#define 	AT91C_SSC_START_EDGE_RF              (0x7 <<  8) // (SSC) Detection of any edge on RF input
#define 	AT91C_SSC_START_0                    (0x8 <<  8) // (SSC) Compare 0
#define AT91C_SSC_STOP            (0x1 << 12) // (SSC) Receive Stop Selection
#define AT91C_SSC_STTDLY          (0xFF << 16) // (SSC) Receive/Transmit Start Delay
#define AT91C_SSC_PERIOD          (0xFF << 24) // (SSC) Receive/Transmit Period Divider Selection
// -------- SSC_RFMR : (SSC Offset: 0x14) SSC Receive Frame Mode Register --------
#define AT91C_SSC_DATLEN          (0x1F <<  0) // (SSC) Data Length
#define AT91C_SSC_LOOP            (0x1 <<  5) // (SSC) Loop Mode
#define AT91C_SSC_MSBF            (0x1 <<  7) // (SSC) Most Significant Bit First
#define AT91C_SSC_DATNB           (0xF <<  8) // (SSC) Data Number per Frame
#define AT91C_SSC_FSLEN           (0xF << 16) // (SSC) Receive/Transmit Frame Sync length
#define AT91C_SSC_FSOS            (0x7 << 20) // (SSC) Receive/Transmit Frame Sync Output Selection
#define 	AT91C_SSC_FSOS_NONE                 (0x0 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: None RK pin Input-only
#define 	AT91C_SSC_FSOS_NEGATIVE             (0x1 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Negative Pulse
#define 	AT91C_SSC_FSOS_POSITIVE             (0x2 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Positive Pulse
#define 	AT91C_SSC_FSOS_LOW                  (0x3 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Driver Low during data transfer
#define 	AT91C_SSC_FSOS_HIGH                 (0x4 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Driver High during data transfer
#define 	AT91C_SSC_FSOS_TOGGLE               (0x5 << 20) // (SSC) Selected Receive/Transmit Frame Sync Signal: Toggling at each start of data transfer
#define AT91C_SSC_FSEDGE          (0x1 << 24) // (SSC) Frame Sync Edge Detection
// -------- SSC_TCMR : (SSC Offset: 0x18) SSC Transmit Clock Mode Register --------
// -------- SSC_TFMR : (SSC Offset: 0x1c) SSC Transmit Frame Mode Register --------
#define AT91C_SSC_DATDEF          (0x1 <<  5) // (SSC) Data Default Value
#define AT91C_SSC_FSDEN           (0x1 << 23) // (SSC) Frame Sync Data Enable
// -------- SSC_SR : (SSC Offset: 0x40) SSC Status Register --------
#define AT91C_SSC_TXRDY           (0x1 <<  0) // (SSC) Transmit Ready
#define AT91C_SSC_TXEMPTY         (0x1 <<  1) // (SSC) Transmit Empty
#define AT91C_SSC_ENDTX           (0x1 <<  2) // (SSC) End Of Transmission
#define AT91C_SSC_TXBUFE          (0x1 <<  3) // (SSC) Transmit Buffer Empty
#define AT91C_SSC_RXRDY           (0x1 <<  4) // (SSC) Receive Ready
#define AT91C_SSC_OVRUN           (0x1 <<  5) // (SSC) Receive Overrun
#define AT91C_SSC_ENDRX           (0x1 <<  6) // (SSC) End of Reception
#define AT91C_SSC_RXBUFF          (0x1 <<  7) // (SSC) Receive Buffer Full
#define AT91C_SSC_CP0             (0x1 <<  8) // (SSC) Compare 0
#define AT91C_SSC_CP1             (0x1 <<  9) // (SSC) Compare 1
#define AT91C_SSC_TXSYN           (0x1 << 10) // (SSC) Transmit Sync
#define AT91C_SSC_RXSYN           (0x1 << 11) // (SSC) Receive Sync
#define AT91C_SSC_TXENA           (0x1 << 16) // (SSC) Transmit Enable
#define AT91C_SSC_RXENA           (0x1 << 17) // (SSC) Receive Enable
// -------- SSC_IER : (SSC Offset: 0x44) SSC Interrupt Enable Register --------
// -------- SSC_IDR : (SSC Offset: 0x48) SSC Interrupt Disable Register --------
// -------- SSC_IMR : (SSC Offset: 0x4c) SSC Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR AC97 Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_AC97C structure ***
#define AC97C_MR        ( 8) // Mode Register
#define AC97C_ICA       (16) // Input Channel AssignementRegister
#define AC97C_OCA       (20) // Output Channel Assignement Register
#define AC97C_CARHR     (32) // Channel A Receive Holding Register
#define AC97C_CATHR     (36) // Channel A Transmit Holding Register
#define AC97C_CASR      (40) // Channel A Status Register
#define AC97C_CAMR      (44) // Channel A Mode Register
#define AC97C_CBRHR     (48) // Channel B Receive Holding Register (optional)
#define AC97C_CBTHR     (52) // Channel B Transmit Holding Register (optional)
#define AC97C_CBSR      (56) // Channel B Status Register
#define AC97C_CBMR      (60) // Channel B Mode Register
#define AC97C_CORHR     (64) // COdec Transmit Holding Register
#define AC97C_COTHR     (68) // COdec Transmit Holding Register
#define AC97C_COSR      (72) // CODEC Status Register
#define AC97C_COMR      (76) // CODEC Mask Status Register
#define AC97C_SR        (80) // Status Register
#define AC97C_IER       (84) // Interrupt Enable Register
#define AC97C_IDR       (88) // Interrupt Disable Register
#define AC97C_IMR       (92) // Interrupt Mask Register
#define AC97C_VERSION   (252) // Version Register
#define AC97C_RPR       (256) // Receive Pointer Register
#define AC97C_RCR       (260) // Receive Counter Register
#define AC97C_TPR       (264) // Transmit Pointer Register
#define AC97C_TCR       (268) // Transmit Counter Register
#define AC97C_RNPR      (272) // Receive Next Pointer Register
#define AC97C_RNCR      (276) // Receive Next Counter Register
#define AC97C_TNPR      (280) // Transmit Next Pointer Register
#define AC97C_TNCR      (284) // Transmit Next Counter Register
#define AC97C_PTCR      (288) // PDC Transfer Control Register
#define AC97C_PTSR      (292) // PDC Transfer Status Register
// -------- AC97C_MR : (AC97C Offset: 0x8) AC97C Mode Register --------
#define AT91C_AC97C_ENA           (0x1 <<  0) // (AC97C) AC97 Controller Global Enable
#define AT91C_AC97C_WRST          (0x1 <<  1) // (AC97C) Warm Reset
#define AT91C_AC97C_VRA           (0x1 <<  2) // (AC97C) Variable RAte (for Data Slots)
// -------- AC97C_ICA : (AC97C Offset: 0x10) AC97C Input Channel Assignement Register --------
#define AT91C_AC97C_CHID3         (0x7 <<  0) // (AC97C) Channel Id for the input slot 3
#define 	AT91C_AC97C_CHID3_NONE                 (0x0) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID3_CA                   (0x1) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID3_CB                   (0x2) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID3_CC                   (0x3) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID4         (0x7 <<  3) // (AC97C) Channel Id for the input slot 4
#define 	AT91C_AC97C_CHID4_NONE                 (0x0 <<  3) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID4_CA                   (0x1 <<  3) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID4_CB                   (0x2 <<  3) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID4_CC                   (0x3 <<  3) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID5         (0x7 <<  6) // (AC97C) Channel Id for the input slot 5
#define 	AT91C_AC97C_CHID5_NONE                 (0x0 <<  6) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID5_CA                   (0x1 <<  6) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID5_CB                   (0x2 <<  6) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID5_CC                   (0x3 <<  6) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID6         (0x7 <<  9) // (AC97C) Channel Id for the input slot 6
#define 	AT91C_AC97C_CHID6_NONE                 (0x0 <<  9) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID6_CA                   (0x1 <<  9) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID6_CB                   (0x2 <<  9) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID6_CC                   (0x3 <<  9) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID7         (0x7 << 12) // (AC97C) Channel Id for the input slot 7
#define 	AT91C_AC97C_CHID7_NONE                 (0x0 << 12) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID7_CA                   (0x1 << 12) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID7_CB                   (0x2 << 12) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID7_CC                   (0x3 << 12) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID8         (0x7 << 15) // (AC97C) Channel Id for the input slot 8
#define 	AT91C_AC97C_CHID8_NONE                 (0x0 << 15) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID8_CA                   (0x1 << 15) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID8_CB                   (0x2 << 15) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID8_CC                   (0x3 << 15) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID9         (0x7 << 18) // (AC97C) Channel Id for the input slot 9
#define 	AT91C_AC97C_CHID9_NONE                 (0x0 << 18) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID9_CA                   (0x1 << 18) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID9_CB                   (0x2 << 18) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID9_CC                   (0x3 << 18) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID10        (0x7 << 21) // (AC97C) Channel Id for the input slot 10
#define 	AT91C_AC97C_CHID10_NONE                 (0x0 << 21) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID10_CA                   (0x1 << 21) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID10_CB                   (0x2 << 21) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID10_CC                   (0x3 << 21) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID11        (0x7 << 24) // (AC97C) Channel Id for the input slot 11
#define 	AT91C_AC97C_CHID11_NONE                 (0x0 << 24) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID11_CA                   (0x1 << 24) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID11_CB                   (0x2 << 24) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID11_CC                   (0x3 << 24) // (AC97C) Channel C data will be transmitted during this slot
#define AT91C_AC97C_CHID12        (0x7 << 27) // (AC97C) Channel Id for the input slot 12
#define 	AT91C_AC97C_CHID12_NONE                 (0x0 << 27) // (AC97C) No data will be transmitted during this slot
#define 	AT91C_AC97C_CHID12_CA                   (0x1 << 27) // (AC97C) Channel A data will be transmitted during this slot
#define 	AT91C_AC97C_CHID12_CB                   (0x2 << 27) // (AC97C) Channel B data will be transmitted during this slot
#define 	AT91C_AC97C_CHID12_CC                   (0x3 << 27) // (AC97C) Channel C data will be transmitted during this slot
// -------- AC97C_OCA : (AC97C Offset: 0x14) AC97C Output Channel Assignement Register --------
// -------- AC97C_CARHR : (AC97C Offset: 0x20) AC97C Channel A Receive Holding Register --------
#define AT91C_AC97C_RDATA         (0xFFFFF <<  0) // (AC97C) Receive data
// -------- AC97C_CATHR : (AC97C Offset: 0x24) AC97C Channel A Transmit Holding Register --------
#define AT91C_AC97C_TDATA         (0xFFFFF <<  0) // (AC97C) Transmit data
// -------- AC97C_CASR : (AC97C Offset: 0x28) AC97C Channel A Status Register --------
#define AT91C_AC97C_TXRDY         (0x1 <<  0) // (AC97C)
#define AT91C_AC97C_TXEMPTY       (0x1 <<  1) // (AC97C)
#define AT91C_AC97C_UNRUN         (0x1 <<  2) // (AC97C)
#define AT91C_AC97C_RXRDY         (0x1 <<  4) // (AC97C)
#define AT91C_AC97C_OVRUN         (0x1 <<  5) // (AC97C)
#define AT91C_AC97C_ENDTX         (0x1 << 10) // (AC97C)
#define AT91C_AC97C_TXBUFE        (0x1 << 11) // (AC97C)
#define AT91C_AC97C_ENDRX         (0x1 << 14) // (AC97C)
#define AT91C_AC97C_RXBUFF        (0x1 << 15) // (AC97C)
// -------- AC97C_CAMR : (AC97C Offset: 0x2c) AC97C Channel A Mode Register --------
#define AT91C_AC97C_SIZE          (0x3 << 16) // (AC97C)
#define 	AT91C_AC97C_SIZE_20_BITS              (0x0 << 16) // (AC97C) Data size is 20 bits
#define 	AT91C_AC97C_SIZE_18_BITS              (0x1 << 16) // (AC97C) Data size is 18 bits
#define 	AT91C_AC97C_SIZE_16_BITS              (0x2 << 16) // (AC97C) Data size is 16 bits
#define 	AT91C_AC97C_SIZE_10_BITS              (0x3 << 16) // (AC97C) Data size is 10 bits
#define AT91C_AC97C_CEM           (0x1 << 18) // (AC97C)
#define AT91C_AC97C_CEN           (0x1 << 21) // (AC97C)
#define AT91C_AC97C_PDCEN         (0x1 << 22) // (AC97C)
// -------- AC97C_CBRHR : (AC97C Offset: 0x30) AC97C Channel B Receive Holding Register --------
// -------- AC97C_CBTHR : (AC97C Offset: 0x34) AC97C Channel B Transmit Holding Register --------
// -------- AC97C_CBSR : (AC97C Offset: 0x38) AC97C Channel B Status Register --------
// -------- AC97C_CBMR : (AC97C Offset: 0x3c) AC97C Channel B Mode Register --------
// -------- AC97C_CORHR : (AC97C Offset: 0x40) AC97C Codec Channel Receive Holding Register --------
#define AT91C_AC97C_SDATA         (0xFFFF <<  0) // (AC97C) Status Data
// -------- AC97C_COTHR : (AC97C Offset: 0x44) AC97C Codec Channel Transmit Holding Register --------
#define AT91C_AC97C_CDATA         (0xFFFF <<  0) // (AC97C) Command Data
#define AT91C_AC97C_CADDR         (0x7F << 16) // (AC97C) COdec control register index
#define AT91C_AC97C_READ          (0x1 << 23) // (AC97C) Read/Write command
// -------- AC97C_COSR : (AC97C Offset: 0x48) AC97C CODEC Status Register --------
// -------- AC97C_COMR : (AC97C Offset: 0x4c) AC97C CODEC Mode Register --------
// -------- AC97C_SR : (AC97C Offset: 0x50) AC97C Status Register --------
#define AT91C_AC97C_SOF           (0x1 <<  0) // (AC97C)
#define AT91C_AC97C_WKUP          (0x1 <<  1) // (AC97C)
#define AT91C_AC97C_COEVT         (0x1 <<  2) // (AC97C)
#define AT91C_AC97C_CAEVT         (0x1 <<  3) // (AC97C)
#define AT91C_AC97C_CBEVT         (0x1 <<  4) // (AC97C)
// -------- AC97C_IER : (AC97C Offset: 0x54) AC97C Interrupt Enable Register --------
// -------- AC97C_IDR : (AC97C Offset: 0x58) AC97C Interrupt Disable Register --------
// -------- AC97C_IMR : (AC97C Offset: 0x5c) AC97C Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Serial Parallel Interface
// *****************************************************************************
// *** Register offset in AT91S_SPI structure ***
#define SPI_CR          ( 0) // Control Register
#define SPI_MR          ( 4) // Mode Register
#define SPI_RDR         ( 8) // Receive Data Register
#define SPI_TDR         (12) // Transmit Data Register
#define SPI_SR          (16) // Status Register
#define SPI_IER         (20) // Interrupt Enable Register
#define SPI_IDR         (24) // Interrupt Disable Register
#define SPI_IMR         (28) // Interrupt Mask Register
#define SPI_CSR         (48) // Chip Select Register
#define SPI_RPR         (256) // Receive Pointer Register
#define SPI_RCR         (260) // Receive Counter Register
#define SPI_TPR         (264) // Transmit Pointer Register
#define SPI_TCR         (268) // Transmit Counter Register
#define SPI_RNPR        (272) // Receive Next Pointer Register
#define SPI_RNCR        (276) // Receive Next Counter Register
#define SPI_TNPR        (280) // Transmit Next Pointer Register
#define SPI_TNCR        (284) // Transmit Next Counter Register
#define SPI_PTCR        (288) // PDC Transfer Control Register
#define SPI_PTSR        (292) // PDC Transfer Status Register
// -------- SPI_CR : (SPI Offset: 0x0) SPI Control Register --------
#define AT91C_SPI_SPIEN           (0x1 <<  0) // (SPI) SPI Enable
#define AT91C_SPI_SPIDIS          (0x1 <<  1) // (SPI) SPI Disable
#define AT91C_SPI_SWRST           (0x1 <<  7) // (SPI) SPI Software reset
#define AT91C_SPI_LASTXFER        (0x1 << 24) // (SPI) SPI Last Transfer
// -------- SPI_MR : (SPI Offset: 0x4) SPI Mode Register --------
#define AT91C_SPI_MSTR            (0x1 <<  0) // (SPI) Master/Slave Mode
#define AT91C_SPI_PS              (0x1 <<  1) // (SPI) Peripheral Select
#define 	AT91C_SPI_PS_FIXED                (0x0 <<  1) // (SPI) Fixed Peripheral Select
#define 	AT91C_SPI_PS_VARIABLE             (0x1 <<  1) // (SPI) Variable Peripheral Select
#define AT91C_SPI_PCSDEC          (0x1 <<  2) // (SPI) Chip Select Decode
#define AT91C_SPI_FDIV            (0x1 <<  3) // (SPI) Clock Selection
#define AT91C_SPI_MODFDIS         (0x1 <<  4) // (SPI) Mode Fault Detection
#define AT91C_SPI_LLB             (0x1 <<  7) // (SPI) Clock Selection
#define AT91C_SPI_PCS             (0xF << 16) // (SPI) Peripheral Chip Select
#define AT91C_SPI_DLYBCS          (0xFF << 24) // (SPI) Delay Between Chip Selects
// -------- SPI_RDR : (SPI Offset: 0x8) Receive Data Register --------
#define AT91C_SPI_RD              (0xFFFF <<  0) // (SPI) Receive Data
#define AT91C_SPI_RPCS            (0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_TDR : (SPI Offset: 0xc) Transmit Data Register --------
#define AT91C_SPI_TD              (0xFFFF <<  0) // (SPI) Transmit Data
#define AT91C_SPI_TPCS            (0xF << 16) // (SPI) Peripheral Chip Select Status
// -------- SPI_SR : (SPI Offset: 0x10) Status Register --------
#define AT91C_SPI_RDRF            (0x1 <<  0) // (SPI) Receive Data Register Full
#define AT91C_SPI_TDRE            (0x1 <<  1) // (SPI) Transmit Data Register Empty
#define AT91C_SPI_MODF            (0x1 <<  2) // (SPI) Mode Fault Error
#define AT91C_SPI_OVRES           (0x1 <<  3) // (SPI) Overrun Error Status
#define AT91C_SPI_ENDRX           (0x1 <<  4) // (SPI) End of Receiver Transfer
#define AT91C_SPI_ENDTX           (0x1 <<  5) // (SPI) End of Receiver Transfer
#define AT91C_SPI_RXBUFF          (0x1 <<  6) // (SPI) RXBUFF Interrupt
#define AT91C_SPI_TXBUFE          (0x1 <<  7) // (SPI) TXBUFE Interrupt
#define AT91C_SPI_NSSR            (0x1 <<  8) // (SPI) NSSR Interrupt
#define AT91C_SPI_TXEMPTY         (0x1 <<  9) // (SPI) TXEMPTY Interrupt
#define AT91C_SPI_SPIENS          (0x1 << 16) // (SPI) Enable Status
// -------- SPI_IER : (SPI Offset: 0x14) Interrupt Enable Register --------
// -------- SPI_IDR : (SPI Offset: 0x18) Interrupt Disable Register --------
// -------- SPI_IMR : (SPI Offset: 0x1c) Interrupt Mask Register --------
// -------- SPI_CSR : (SPI Offset: 0x30) Chip Select Register --------
#define AT91C_SPI_CPOL            (0x1 <<  0) // (SPI) Clock Polarity
#define AT91C_SPI_NCPHA           (0x1 <<  1) // (SPI) Clock Phase
#define AT91C_SPI_CSAAT           (0x1 <<  3) // (SPI) Chip Select Active After Transfer
#define AT91C_SPI_BITS            (0xF <<  4) // (SPI) Bits Per Transfer
#define 	AT91C_SPI_BITS_8                    (0x0 <<  4) // (SPI) 8 Bits Per transfer
#define 	AT91C_SPI_BITS_9                    (0x1 <<  4) // (SPI) 9 Bits Per transfer
#define 	AT91C_SPI_BITS_10                   (0x2 <<  4) // (SPI) 10 Bits Per transfer
#define 	AT91C_SPI_BITS_11                   (0x3 <<  4) // (SPI) 11 Bits Per transfer
#define 	AT91C_SPI_BITS_12                   (0x4 <<  4) // (SPI) 12 Bits Per transfer
#define 	AT91C_SPI_BITS_13                   (0x5 <<  4) // (SPI) 13 Bits Per transfer
#define 	AT91C_SPI_BITS_14                   (0x6 <<  4) // (SPI) 14 Bits Per transfer
#define 	AT91C_SPI_BITS_15                   (0x7 <<  4) // (SPI) 15 Bits Per transfer
#define 	AT91C_SPI_BITS_16                   (0x8 <<  4) // (SPI) 16 Bits Per transfer
#define AT91C_SPI_SCBR            (0xFF <<  8) // (SPI) Serial Clock Baud Rate
#define AT91C_SPI_DLYBS           (0xFF << 16) // (SPI) Delay Before SPCK
#define AT91C_SPI_DLYBCT          (0xFF << 24) // (SPI) Delay Between Consecutive Transfers

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Control Area Network MailBox Interface
// *****************************************************************************
// *** Register offset in AT91S_CAN_MB structure ***
#define CAN_MB_MMR      ( 0) // MailBox Mode Register
#define CAN_MB_MAM      ( 4) // MailBox Acceptance Mask Register
#define CAN_MB_MID      ( 8) // MailBox ID Register
#define CAN_MB_MFID     (12) // MailBox Family ID Register
#define CAN_MB_MSR      (16) // MailBox Status Register
#define CAN_MB_MDL      (20) // MailBox Data Low Register
#define CAN_MB_MDH      (24) // MailBox Data High Register
#define CAN_MB_MCR      (28) // MailBox Control Register
// -------- CAN_MMR : (CAN_MB Offset: 0x0) CAN Message Mode Register --------
#define AT91C_CAN_MTIMEMARK       (0xFFFF <<  0) // (CAN_MB) Mailbox Timemark
#define AT91C_CAN_PRIOR           (0xF << 16) // (CAN_MB) Mailbox Priority
#define AT91C_CAN_MOT             (0x7 << 24) // (CAN_MB) Mailbox Object Type
#define 	AT91C_CAN_MOT_DIS                  (0x0 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_RX                   (0x1 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_RXOVERWRITE          (0x2 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_TX                   (0x3 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_CONSUMER             (0x4 << 24) // (CAN_MB)
#define 	AT91C_CAN_MOT_PRODUCER             (0x5 << 24) // (CAN_MB)
// -------- CAN_MAM : (CAN_MB Offset: 0x4) CAN Message Acceptance Mask Register --------
#define AT91C_CAN_MIDvB           (0x3FFFF <<  0) // (CAN_MB) Complementary bits for identifier in extended mode
#define AT91C_CAN_MIDvA           (0x7FF << 18) // (CAN_MB) Identifier for standard frame mode
#define AT91C_CAN_MIDE            (0x1 << 29) // (CAN_MB) Identifier Version
// -------- CAN_MID : (CAN_MB Offset: 0x8) CAN Message ID Register --------
// -------- CAN_MFID : (CAN_MB Offset: 0xc) CAN Message Family ID Register --------
// -------- CAN_MSR : (CAN_MB Offset: 0x10) CAN Message Status Register --------
#define AT91C_CAN_MTIMESTAMP      (0xFFFF <<  0) // (CAN_MB) Timer Value
#define AT91C_CAN_MDLC            (0xF << 16) // (CAN_MB) Mailbox Data Length Code
#define AT91C_CAN_MRTR            (0x1 << 20) // (CAN_MB) Mailbox Remote Transmission Request
#define AT91C_CAN_MABT            (0x1 << 22) // (CAN_MB) Mailbox Message Abort
#define AT91C_CAN_MRDY            (0x1 << 23) // (CAN_MB) Mailbox Ready
#define AT91C_CAN_MMI             (0x1 << 24) // (CAN_MB) Mailbox Message Ignored
// -------- CAN_MDL : (CAN_MB Offset: 0x14) CAN Message Data Low Register --------
// -------- CAN_MDH : (CAN_MB Offset: 0x18) CAN Message Data High Register --------
// -------- CAN_MCR : (CAN_MB Offset: 0x1c) CAN Message Control Register --------
#define AT91C_CAN_MACR            (0x1 << 22) // (CAN_MB) Abort Request for Mailbox
#define AT91C_CAN_MTCR            (0x1 << 23) // (CAN_MB) Mailbox Transfer Command

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Control Area Network Interface
// *****************************************************************************
// *** Register offset in AT91S_CAN structure ***
#define CAN_MR          ( 0) // Mode Register
#define CAN_IER         ( 4) // Interrupt Enable Register
#define CAN_IDR         ( 8) // Interrupt Disable Register
#define CAN_IMR         (12) // Interrupt Mask Register
#define CAN_SR          (16) // Status Register
#define CAN_BR          (20) // Baudrate Register
#define CAN_TIM         (24) // Timer Register
#define CAN_TIMESTP     (28) // Time Stamp Register
#define CAN_ECR         (32) // Error Counter Register
#define CAN_TCR         (36) // Transfer Command Register
#define CAN_ACR         (40) // Abort Command Register
#define CAN_VR          (252) // Version Register
#define CAN_MB0         (512) // CAN Mailbox 0
#define CAN_MB1         (544) // CAN Mailbox 1
#define CAN_MB2         (576) // CAN Mailbox 2
#define CAN_MB3         (608) // CAN Mailbox 3
#define CAN_MB4         (640) // CAN Mailbox 4
#define CAN_MB5         (672) // CAN Mailbox 5
#define CAN_MB6         (704) // CAN Mailbox 6
#define CAN_MB7         (736) // CAN Mailbox 7
#define CAN_MB8         (768) // CAN Mailbox 8
#define CAN_MB9         (800) // CAN Mailbox 9
#define CAN_MB10        (832) // CAN Mailbox 10
#define CAN_MB11        (864) // CAN Mailbox 11
#define CAN_MB12        (896) // CAN Mailbox 12
#define CAN_MB13        (928) // CAN Mailbox 13
#define CAN_MB14        (960) // CAN Mailbox 14
#define CAN_MB15        (992) // CAN Mailbox 15
// -------- CAN_MR : (CAN Offset: 0x0) CAN Mode Register --------
#define AT91C_CAN_CANEN           (0x1 <<  0) // (CAN) CAN Controller Enable
#define AT91C_CAN_LPM             (0x1 <<  1) // (CAN) Disable/Enable Low Power Mode
#define AT91C_CAN_ABM             (0x1 <<  2) // (CAN) Disable/Enable Autobaud/Listen Mode
#define AT91C_CAN_OVL             (0x1 <<  3) // (CAN) Disable/Enable Overload Frame
#define AT91C_CAN_TEOF            (0x1 <<  4) // (CAN) Time Stamp messages at each end of Frame
#define AT91C_CAN_TTM             (0x1 <<  5) // (CAN) Disable/Enable Time Trigger Mode
#define AT91C_CAN_TIMFRZ          (0x1 <<  6) // (CAN) Enable Timer Freeze
#define AT91C_CAN_DRPT            (0x1 <<  7) // (CAN) Disable Repeat
// -------- CAN_IER : (CAN Offset: 0x4) CAN Interrupt Enable Register --------
#define AT91C_CAN_MB0             (0x1 <<  0) // (CAN) Mailbox 0 Flag
#define AT91C_CAN_MB1             (0x1 <<  1) // (CAN) Mailbox 1 Flag
#define AT91C_CAN_MB2             (0x1 <<  2) // (CAN) Mailbox 2 Flag
#define AT91C_CAN_MB3             (0x1 <<  3) // (CAN) Mailbox 3 Flag
#define AT91C_CAN_MB4             (0x1 <<  4) // (CAN) Mailbox 4 Flag
#define AT91C_CAN_MB5             (0x1 <<  5) // (CAN) Mailbox 5 Flag
#define AT91C_CAN_MB6             (0x1 <<  6) // (CAN) Mailbox 6 Flag
#define AT91C_CAN_MB7             (0x1 <<  7) // (CAN) Mailbox 7 Flag
#define AT91C_CAN_MB8             (0x1 <<  8) // (CAN) Mailbox 8 Flag
#define AT91C_CAN_MB9             (0x1 <<  9) // (CAN) Mailbox 9 Flag
#define AT91C_CAN_MB10            (0x1 << 10) // (CAN) Mailbox 10 Flag
#define AT91C_CAN_MB11            (0x1 << 11) // (CAN) Mailbox 11 Flag
#define AT91C_CAN_MB12            (0x1 << 12) // (CAN) Mailbox 12 Flag
#define AT91C_CAN_MB13            (0x1 << 13) // (CAN) Mailbox 13 Flag
#define AT91C_CAN_MB14            (0x1 << 14) // (CAN) Mailbox 14 Flag
#define AT91C_CAN_MB15            (0x1 << 15) // (CAN) Mailbox 15 Flag
#define AT91C_CAN_ERRA            (0x1 << 16) // (CAN) Error Active Mode Flag
#define AT91C_CAN_WARN            (0x1 << 17) // (CAN) Warning Limit Flag
#define AT91C_CAN_ERRP            (0x1 << 18) // (CAN) Error Passive Mode Flag
#define AT91C_CAN_BOFF            (0x1 << 19) // (CAN) Bus Off Mode Flag
#define AT91C_CAN_SLEEP           (0x1 << 20) // (CAN) Sleep Flag
#define AT91C_CAN_WAKEUP          (0x1 << 21) // (CAN) Wakeup Flag
#define AT91C_CAN_TOVF            (0x1 << 22) // (CAN) Timer Overflow Flag
#define AT91C_CAN_TSTP            (0x1 << 23) // (CAN) Timestamp Flag
#define AT91C_CAN_CERR            (0x1 << 24) // (CAN) CRC Error
#define AT91C_CAN_SERR            (0x1 << 25) // (CAN) Stuffing Error
#define AT91C_CAN_AERR            (0x1 << 26) // (CAN) Acknowledgment Error
#define AT91C_CAN_FERR            (0x1 << 27) // (CAN) Form Error
#define AT91C_CAN_BERR            (0x1 << 28) // (CAN) Bit Error
// -------- CAN_IDR : (CAN Offset: 0x8) CAN Interrupt Disable Register --------
// -------- CAN_IMR : (CAN Offset: 0xc) CAN Interrupt Mask Register --------
// -------- CAN_SR : (CAN Offset: 0x10) CAN Status Register --------
#define AT91C_CAN_RBSY            (0x1 << 29) // (CAN) Receiver Busy
#define AT91C_CAN_TBSY            (0x1 << 30) // (CAN) Transmitter Busy
#define AT91C_CAN_OVLY            (0x1 << 31) // (CAN) Overload Busy
// -------- CAN_BR : (CAN Offset: 0x14) CAN Baudrate Register --------
#define AT91C_CAN_PHASE2          (0x7 <<  0) // (CAN) Phase 2 segment
#define AT91C_CAN_PHASE1          (0x7 <<  4) // (CAN) Phase 1 segment
#define AT91C_CAN_PROPAG          (0x7 <<  8) // (CAN) Programmation time segment
#define AT91C_CAN_SYNC            (0x3 << 12) // (CAN) Re-synchronization jump width segment
#define AT91C_CAN_BRP             (0x7F << 16) // (CAN) Baudrate Prescaler
#define AT91C_CAN_SMP             (0x1 << 24) // (CAN) Sampling mode
// -------- CAN_TIM : (CAN Offset: 0x18) CAN Timer Register --------
#define AT91C_CAN_TIMER           (0xFFFF <<  0) // (CAN) Timer field
// -------- CAN_TIMESTP : (CAN Offset: 0x1c) CAN Timestamp Register --------
// -------- CAN_ECR : (CAN Offset: 0x20) CAN Error Counter Register --------
#define AT91C_CAN_REC             (0xFF <<  0) // (CAN) Receive Error Counter
#define AT91C_CAN_TEC             (0xFF << 16) // (CAN) Transmit Error Counter
// -------- CAN_TCR : (CAN Offset: 0x24) CAN Transfer Command Register --------
#define AT91C_CAN_TIMRST          (0x1 << 31) // (CAN) Timer Reset Field
// -------- CAN_ACR : (CAN Offset: 0x28) CAN Abort Command Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR PWMC Channel Interface
// *****************************************************************************
// *** Register offset in AT91S_PWMC_CH structure ***
#define PWMC_CMR        ( 0) // Channel Mode Register
#define PWMC_CDTYR      ( 4) // Channel Duty Cycle Register
#define PWMC_CPRDR      ( 8) // Channel Period Register
#define PWMC_CCNTR      (12) // Channel Counter Register
#define PWMC_CUPDR      (16) // Channel Update Register
#define PWMC_Reserved   (20) // Reserved
// -------- PWMC_CMR : (PWMC_CH Offset: 0x0) PWMC Channel Mode Register --------
#define AT91C_PWMC_CPRE           (0xF <<  0) // (PWMC_CH) Channel Pre-scaler : PWMC_CLKx
#define 	AT91C_PWMC_CPRE_MCK                  (0x0) // (PWMC_CH)
#define 	AT91C_PWMC_CPRE_MCKA                 (0xB) // (PWMC_CH)
#define 	AT91C_PWMC_CPRE_MCKB                 (0xC) // (PWMC_CH)
#define AT91C_PWMC_CALG           (0x1 <<  8) // (PWMC_CH) Channel Alignment
#define AT91C_PWMC_CPOL           (0x1 <<  9) // (PWMC_CH) Channel Polarity
#define AT91C_PWMC_CPD            (0x1 << 10) // (PWMC_CH) Channel Update Period
// -------- PWMC_CDTYR : (PWMC_CH Offset: 0x4) PWMC Channel Duty Cycle Register --------
#define AT91C_PWMC_CDTY           (0x0 <<  0) // (PWMC_CH) Channel Duty Cycle
// -------- PWMC_CPRDR : (PWMC_CH Offset: 0x8) PWMC Channel Period Register --------
#define AT91C_PWMC_CPRD           (0x0 <<  0) // (PWMC_CH) Channel Period
// -------- PWMC_CCNTR : (PWMC_CH Offset: 0xc) PWMC Channel Counter Register --------
#define AT91C_PWMC_CCNT           (0x0 <<  0) // (PWMC_CH) Channel Counter
// -------- PWMC_CUPDR : (PWMC_CH Offset: 0x10) PWMC Channel Update Register --------
#define AT91C_PWMC_CUPD           (0x0 <<  0) // (PWMC_CH) Channel Update

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Pulse Width Modulation Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_PWMC structure ***
#define PWMC_MR         ( 0) // PWMC Mode Register
#define PWMC_ENA        ( 4) // PWMC Enable Register
#define PWMC_DIS        ( 8) // PWMC Disable Register
#define PWMC_SR         (12) // PWMC Status Register
#define PWMC_IER        (16) // PWMC Interrupt Enable Register
#define PWMC_IDR        (20) // PWMC Interrupt Disable Register
#define PWMC_IMR        (24) // PWMC Interrupt Mask Register
#define PWMC_ISR        (28) // PWMC Interrupt Status Register
#define PWMC_VR         (252) // PWMC Version Register
#define PWMC_CH         (512) // PWMC Channel
// -------- PWMC_MR : (PWMC Offset: 0x0) PWMC Mode Register --------
#define AT91C_PWMC_DIVA           (0xFF <<  0) // (PWMC) CLKA divide factor.
#define AT91C_PWMC_PREA           (0xF <<  8) // (PWMC) Divider Input Clock Prescaler A
#define 	AT91C_PWMC_PREA_MCK                  (0x0 <<  8) // (PWMC)
#define AT91C_PWMC_DIVB           (0xFF << 16) // (PWMC) CLKB divide factor.
#define AT91C_PWMC_PREB           (0xF << 24) // (PWMC) Divider Input Clock Prescaler B
#define 	AT91C_PWMC_PREB_MCK                  (0x0 << 24) // (PWMC)
// -------- PWMC_ENA : (PWMC Offset: 0x4) PWMC Enable Register --------
#define AT91C_PWMC_CHID0          (0x1 <<  0) // (PWMC) Channel ID 0
#define AT91C_PWMC_CHID1          (0x1 <<  1) // (PWMC) Channel ID 1
#define AT91C_PWMC_CHID2          (0x1 <<  2) // (PWMC) Channel ID 2
#define AT91C_PWMC_CHID3          (0x1 <<  3) // (PWMC) Channel ID 3
#define AT91C_PWMC_CHID4          (0x1 <<  4) // (PWMC) Channel ID 4
#define AT91C_PWMC_CHID5          (0x1 <<  5) // (PWMC) Channel ID 5
#define AT91C_PWMC_CHID6          (0x1 <<  6) // (PWMC) Channel ID 6
#define AT91C_PWMC_CHID7          (0x1 <<  7) // (PWMC) Channel ID 7
// -------- PWMC_DIS : (PWMC Offset: 0x8) PWMC Disable Register --------
// -------- PWMC_SR : (PWMC Offset: 0xc) PWMC Status Register --------
// -------- PWMC_IER : (PWMC Offset: 0x10) PWMC Interrupt Enable Register --------
// -------- PWMC_IDR : (PWMC Offset: 0x14) PWMC Interrupt Disable Register --------
// -------- PWMC_IMR : (PWMC Offset: 0x18) PWMC Interrupt Mask Register --------
// -------- PWMC_ISR : (PWMC Offset: 0x1c) PWMC Interrupt Status Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Ethernet MAC 10/100
// *****************************************************************************
// *** Register offset in AT91S_EMAC structure ***
#define EMAC_NCR        ( 0) // Network Control Register
#define EMAC_NCFGR      ( 4) // Network Configuration Register
#define EMAC_NSR        ( 8) // Network Status Register
#define EMAC_TSR        (20) // Transmit Status Register
#define EMAC_RBQP       (24) // Receive Buffer Queue Pointer
#define EMAC_TBQP       (28) // Transmit Buffer Queue Pointer
#define EMAC_RSR        (32) // Receive Status Register
#define EMAC_ISR        (36) // Interrupt Status Register
#define EMAC_IER        (40) // Interrupt Enable Register
#define EMAC_IDR        (44) // Interrupt Disable Register
#define EMAC_IMR        (48) // Interrupt Mask Register
#define EMAC_MAN        (52) // PHY Maintenance Register
#define EMAC_PTR        (56) // Pause Time Register
#define EMAC_PFR        (60) // Pause Frames received Register
#define EMAC_FTO        (64) // Frames Transmitted OK Register
#define EMAC_SCF        (68) // Single Collision Frame Register
#define EMAC_MCF        (72) // Multiple Collision Frame Register
#define EMAC_FRO        (76) // Frames Received OK Register
#define EMAC_FCSE       (80) // Frame Check Sequence Error Register
#define EMAC_ALE        (84) // Alignment Error Register
#define EMAC_DTF        (88) // Deferred Transmission Frame Register
#define EMAC_LCOL       (92) // Late Collision Register
#define EMAC_ECOL       (96) // Excessive Collision Register
#define EMAC_TUND       (100) // Transmit Underrun Error Register
#define EMAC_CSE        (104) // Carrier Sense Error Register
#define EMAC_RRE        (108) // Receive Ressource Error Register
#define EMAC_ROV        (112) // Receive Overrun Errors Register
#define EMAC_RSE        (116) // Receive Symbol Errors Register
#define EMAC_ELE        (120) // Excessive Length Errors Register
#define EMAC_RJA        (124) // Receive Jabbers Register
#define EMAC_USF        (128) // Undersize Frames Register
#define EMAC_STE        (132) // SQE Test Error Register
#define EMAC_RLE        (136) // Receive Length Field Mismatch Register
#define EMAC_TPF        (140) // Transmitted Pause Frames Register
#define EMAC_HRB        (144) // Hash Address Bottom[31:0]
#define EMAC_HRT        (148) // Hash Address Top[63:32]
#define EMAC_SA1L       (152) // Specific Address 1 Bottom, First 4 bytes
#define EMAC_SA1H       (156) // Specific Address 1 Top, Last 2 bytes
#define EMAC_SA2L       (160) // Specific Address 2 Bottom, First 4 bytes
#define EMAC_SA2H       (164) // Specific Address 2 Top, Last 2 bytes
#define EMAC_SA3L       (168) // Specific Address 3 Bottom, First 4 bytes
#define EMAC_SA3H       (172) // Specific Address 3 Top, Last 2 bytes
#define EMAC_SA4L       (176) // Specific Address 4 Bottom, First 4 bytes
#define EMAC_SA4H       (180) // Specific Address 4 Top, Last 2 bytes
#define EMAC_TID        (184) // Type ID Checking Register
#define EMAC_TPQ        (188) // Transmit Pause Quantum Register
#define EMAC_USRIO      (192) // USER Input/Output Register
#define EMAC_WOL        (196) // Wake On LAN Register
#define EMAC_REV        (252) // Revision Register
// -------- EMAC_NCR : (EMAC Offset: 0x0)  --------
#define AT91C_EMAC_LB             (0x1 <<  0) // (EMAC) Loopback. Optional. When set, loopback signal is at high level.
#define AT91C_EMAC_LLB            (0x1 <<  1) // (EMAC) Loopback local.
#define AT91C_EMAC_RE             (0x1 <<  2) // (EMAC) Receive enable.
#define AT91C_EMAC_TE             (0x1 <<  3) // (EMAC) Transmit enable.
#define AT91C_EMAC_MPE            (0x1 <<  4) // (EMAC) Management port enable.
#define AT91C_EMAC_CLRSTAT        (0x1 <<  5) // (EMAC) Clear statistics registers.
#define AT91C_EMAC_INCSTAT        (0x1 <<  6) // (EMAC) Increment statistics registers.
#define AT91C_EMAC_WESTAT         (0x1 <<  7) // (EMAC) Write enable for statistics registers.
#define AT91C_EMAC_BP             (0x1 <<  8) // (EMAC) Back pressure.
#define AT91C_EMAC_TSTART         (0x1 <<  9) // (EMAC) Start Transmission.
#define AT91C_EMAC_THALT          (0x1 << 10) // (EMAC) Transmission Halt.
#define AT91C_EMAC_TPFR           (0x1 << 11) // (EMAC) Transmit pause frame
#define AT91C_EMAC_TZQ            (0x1 << 12) // (EMAC) Transmit zero quantum pause frame
// -------- EMAC_NCFGR : (EMAC Offset: 0x4) Network Configuration Register --------
#define AT91C_EMAC_SPD            (0x1 <<  0) // (EMAC) Speed.
#define AT91C_EMAC_FD             (0x1 <<  1) // (EMAC) Full duplex.
#define AT91C_EMAC_JFRAME         (0x1 <<  3) // (EMAC) Jumbo Frames.
#define AT91C_EMAC_CAF            (0x1 <<  4) // (EMAC) Copy all frames.
#define AT91C_EMAC_NBC            (0x1 <<  5) // (EMAC) No broadcast.
#define AT91C_EMAC_MTI            (0x1 <<  6) // (EMAC) Multicast hash event enable
#define AT91C_EMAC_UNI            (0x1 <<  7) // (EMAC) Unicast hash enable.
#define AT91C_EMAC_BIG            (0x1 <<  8) // (EMAC) Receive 1522 bytes.
#define AT91C_EMAC_EAE            (0x1 <<  9) // (EMAC) External address match enable.
#define AT91C_EMAC_CLK            (0x3 << 10) // (EMAC)
#define 	AT91C_EMAC_CLK_HCLK_8               (0x0 << 10) // (EMAC) HCLK divided by 8
#define 	AT91C_EMAC_CLK_HCLK_16              (0x1 << 10) // (EMAC) HCLK divided by 16
#define 	AT91C_EMAC_CLK_HCLK_32              (0x2 << 10) // (EMAC) HCLK divided by 32
#define 	AT91C_EMAC_CLK_HCLK_64              (0x3 << 10) // (EMAC) HCLK divided by 64
#define AT91C_EMAC_RTY            (0x1 << 12) // (EMAC)
#define AT91C_EMAC_PAE            (0x1 << 13) // (EMAC)
#define AT91C_EMAC_RBOF           (0x3 << 14) // (EMAC)
#define 	AT91C_EMAC_RBOF_OFFSET_0             (0x0 << 14) // (EMAC) no offset from start of receive buffer
#define 	AT91C_EMAC_RBOF_OFFSET_1             (0x1 << 14) // (EMAC) one byte offset from start of receive buffer
#define 	AT91C_EMAC_RBOF_OFFSET_2             (0x2 << 14) // (EMAC) two bytes offset from start of receive buffer
#define 	AT91C_EMAC_RBOF_OFFSET_3             (0x3 << 14) // (EMAC) three bytes offset from start of receive buffer
#define AT91C_EMAC_RLCE           (0x1 << 16) // (EMAC) Receive Length field Checking Enable
#define AT91C_EMAC_DRFCS          (0x1 << 17) // (EMAC) Discard Receive FCS
#define AT91C_EMAC_EFRHD          (0x1 << 18) // (EMAC)
#define AT91C_EMAC_IRXFCS         (0x1 << 19) // (EMAC) Ignore RX FCS
// -------- EMAC_NSR : (EMAC Offset: 0x8) Network Status Register --------
#define AT91C_EMAC_LINKR          (0x1 <<  0) // (EMAC)
#define AT91C_EMAC_MDIO           (0x1 <<  1) // (EMAC)
#define AT91C_EMAC_IDLE           (0x1 <<  2) // (EMAC)
// -------- EMAC_TSR : (EMAC Offset: 0x14) Transmit Status Register --------
#define AT91C_EMAC_UBR            (0x1 <<  0) // (EMAC)
#define AT91C_EMAC_COL            (0x1 <<  1) // (EMAC)
#define AT91C_EMAC_RLES           (0x1 <<  2) // (EMAC)
#define AT91C_EMAC_TGO            (0x1 <<  3) // (EMAC) Transmit Go
#define AT91C_EMAC_BEX            (0x1 <<  4) // (EMAC) Buffers exhausted mid frame
#define AT91C_EMAC_COMP           (0x1 <<  5) // (EMAC)
#define AT91C_EMAC_UND            (0x1 <<  6) // (EMAC)
// -------- EMAC_RSR : (EMAC Offset: 0x20) Receive Status Register --------
#define AT91C_EMAC_BNA            (0x1 <<  0) // (EMAC)
#define AT91C_EMAC_REC            (0x1 <<  1) // (EMAC)
#define AT91C_EMAC_OVR            (0x1 <<  2) // (EMAC)
// -------- EMAC_ISR : (EMAC Offset: 0x24) Interrupt Status Register --------
#define AT91C_EMAC_MFD            (0x1 <<  0) // (EMAC)
#define AT91C_EMAC_RCOMP          (0x1 <<  1) // (EMAC)
#define AT91C_EMAC_RXUBR          (0x1 <<  2) // (EMAC)
#define AT91C_EMAC_TXUBR          (0x1 <<  3) // (EMAC)
#define AT91C_EMAC_TUNDR          (0x1 <<  4) // (EMAC)
#define AT91C_EMAC_RLEX           (0x1 <<  5) // (EMAC)
#define AT91C_EMAC_TXERR          (0x1 <<  6) // (EMAC)
#define AT91C_EMAC_TCOMP          (0x1 <<  7) // (EMAC)
#define AT91C_EMAC_LINK           (0x1 <<  9) // (EMAC)
#define AT91C_EMAC_ROVR           (0x1 << 10) // (EMAC)
#define AT91C_EMAC_HRESP          (0x1 << 11) // (EMAC)
#define AT91C_EMAC_PFRE           (0x1 << 12) // (EMAC)
#define AT91C_EMAC_PTZ            (0x1 << 13) // (EMAC)
// -------- EMAC_IER : (EMAC Offset: 0x28) Interrupt Enable Register --------
// -------- EMAC_IDR : (EMAC Offset: 0x2c) Interrupt Disable Register --------
// -------- EMAC_IMR : (EMAC Offset: 0x30) Interrupt Mask Register --------
// -------- EMAC_MAN : (EMAC Offset: 0x34) PHY Maintenance Register --------
#define AT91C_EMAC_DATA           (0xFFFF <<  0) // (EMAC)
#define AT91C_EMAC_CODE           (0x3 << 16) // (EMAC)
#define AT91C_EMAC_REGA           (0x1F << 18) // (EMAC)
#define AT91C_EMAC_PHYA           (0x1F << 23) // (EMAC)
#define AT91C_EMAC_RW             (0x3 << 28) // (EMAC)
#define AT91C_EMAC_SOF            (0x3 << 30) // (EMAC)
// -------- EMAC_USRIO : (EMAC Offset: 0xc0) USER Input Output Register --------
#define AT91C_EMAC_RMII           (0x1 <<  0) // (EMAC) Reduce MII
#define AT91C_EMAC_CLKEN          (0x1 <<  1) // (EMAC) Clock Enable
// -------- EMAC_WOL : (EMAC Offset: 0xc4) Wake On LAN Register --------
#define AT91C_EMAC_IP             (0xFFFF <<  0) // (EMAC) ARP request IP address
#define AT91C_EMAC_MAG            (0x1 << 16) // (EMAC) Magic packet event enable
#define AT91C_EMAC_ARP            (0x1 << 17) // (EMAC) ARP request event enable
#define AT91C_EMAC_SA1            (0x1 << 18) // (EMAC) Specific address register 1 event enable
// -------- EMAC_REV : (EMAC Offset: 0xfc) Revision Register --------
#define AT91C_EMAC_REVREF         (0xFFFF <<  0) // (EMAC)
#define AT91C_EMAC_PARTREF        (0xFFFF << 16) // (EMAC)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR LCD Controller
// *****************************************************************************
// *** Register offset in AT91S_LCDC structure ***
#define LCDC_BA1        ( 0) // DMA Base Address Register 1
#define LCDC_BA2        ( 4) // DMA Base Address Register 2
#define LCDC_FRMP1      ( 8) // DMA Frame Pointer Register 1
#define LCDC_FRMP2      (12) // DMA Frame Pointer Register 2
#define LCDC_FRMA1      (16) // DMA Frame Address Register 1
#define LCDC_FRMA2      (20) // DMA Frame Address Register 2
#define LCDC_FRMCFG     (24) // DMA Frame Configuration Register
#define LCDC_DMACON     (28) // DMA Control Register
#define LCDC_DMA2DCFG   (32) // DMA 2D addressing configuration
#define LCDC_LCDCON1    (2048) // LCD Control 1 Register
#define LCDC_LCDCON2    (2052) // LCD Control 2 Register
#define LCDC_TIM1       (2056) // LCD Timing Config 1 Register
#define LCDC_TIM2       (2060) // LCD Timing Config 2 Register
#define LCDC_LCDFRCFG   (2064) // LCD Frame Config Register
#define LCDC_FIFO       (2068) // LCD FIFO Register
#define LCDC_MVAL       (2072) // LCD Mode Toggle Rate Value Register
#define LCDC_DP1_2      (2076) // Dithering Pattern DP1_2 Register
#define LCDC_DP4_7      (2080) // Dithering Pattern DP4_7 Register
#define LCDC_DP3_5      (2084) // Dithering Pattern DP3_5 Register
#define LCDC_DP2_3      (2088) // Dithering Pattern DP2_3 Register
#define LCDC_DP5_7      (2092) // Dithering Pattern DP5_7 Register
#define LCDC_DP3_4      (2096) // Dithering Pattern DP3_4 Register
#define LCDC_DP4_5      (2100) // Dithering Pattern DP4_5 Register
#define LCDC_DP6_7      (2104) // Dithering Pattern DP6_7 Register
#define LCDC_PWRCON     (2108) // Power Control Register
#define LCDC_CTRSTCON   (2112) // Contrast Control Register
#define LCDC_CTRSTVAL   (2116) // Contrast Value Register
#define LCDC_IER        (2120) // Interrupt Enable Register
#define LCDC_IDR        (2124) // Interrupt Disable Register
#define LCDC_IMR        (2128) // Interrupt Mask Register
#define LCDC_ISR        (2132) // Interrupt Enable Register
#define LCDC_ICR        (2136) // Interrupt Clear Register
#define LCDC_GPR        (2140) // General Purpose Register
#define LCDC_ITR        (2144) // Interrupts Test Register
#define LCDC_IRR        (2148) // Interrupts Raw Status Register
#define LCDC_LUT_ENTRY  (3072) // LUT Entries Register
// -------- LCDC_FRMP1 : (LCDC Offset: 0x8) DMA Frame Pointer 1 Register --------
#define AT91C_LCDC_FRMPT1         (0x3FFFFF <<  0) // (LCDC) Frame Pointer Address 1
// -------- LCDC_FRMP2 : (LCDC Offset: 0xc) DMA Frame Pointer 2 Register --------
#define AT91C_LCDC_FRMPT2         (0x1FFFFF <<  0) // (LCDC) Frame Pointer Address 2
// -------- LCDC_FRMCFG : (LCDC Offset: 0x18) DMA Frame Config Register --------
#define AT91C_LCDC_FRSIZE         (0x3FFFFF <<  0) // (LCDC) FRAME SIZE
#define AT91C_LCDC_BLENGTH        (0xF << 24) // (LCDC) BURST LENGTH
// -------- LCDC_DMACON : (LCDC Offset: 0x1c) DMA Control Register --------
#define AT91C_LCDC_DMAEN          (0x1 <<  0) // (LCDC) DAM Enable
#define AT91C_LCDC_DMARST         (0x1 <<  1) // (LCDC) DMA Reset (WO)
#define AT91C_LCDC_DMABUSY        (0x1 <<  2) // (LCDC) DMA Reset (WO)
#define AT91C_LCDC_DMAUPDT        (0x1 <<  3) // (LCDC) DMA Configuration Update
#define AT91C_LCDC_DMA2DEN        (0x1 <<  4) // (LCDC) 2D Addressing Enable
// -------- LCDC_DMA2DCFG : (LCDC Offset: 0x20) DMA 2D addressing configuration Register --------
#define AT91C_LCDC_ADDRINC        (0xFFFF <<  0) // (LCDC) Number of 32b words that the DMA must jump when going to the next line
#define AT91C_LCDC_PIXELOFF       (0x1F << 24) // (LCDC) Offset (in bits) of the first pixel of the screen in the memory word which contain it
// -------- LCDC_LCDCON1 : (LCDC Offset: 0x800) LCD Control 1 Register --------
#define AT91C_LCDC_BYPASS         (0x1 <<  0) // (LCDC) Bypass lcd_pccklk divider
#define AT91C_LCDC_CLKVAL         (0x1FF << 12) // (LCDC) 9-bit Divider for pixel clock frequency
#define AT91C_LCDC_LINCNT         (0x7FF << 21) // (LCDC) Line Counter (RO)
// -------- LCDC_LCDCON2 : (LCDC Offset: 0x804) LCD Control 2 Register --------
#define AT91C_LCDC_DISTYPE        (0x3 <<  0) // (LCDC) Display Type
#define 	AT91C_LCDC_DISTYPE_STNMONO              (0x0) // (LCDC) STN Mono
#define 	AT91C_LCDC_DISTYPE_STNCOLOR             (0x1) // (LCDC) STN Color
#define 	AT91C_LCDC_DISTYPE_TFT                  (0x2) // (LCDC) TFT
#define AT91C_LCDC_SCANMOD        (0x1 <<  2) // (LCDC) Scan Mode
#define 	AT91C_LCDC_SCANMOD_SINGLESCAN           (0x0 <<  2) // (LCDC) Single Scan
#define 	AT91C_LCDC_SCANMOD_DUALSCAN             (0x1 <<  2) // (LCDC) Dual Scan
#define AT91C_LCDC_IFWIDTH        (0x3 <<  3) // (LCDC) Interface Width
#define 	AT91C_LCDC_IFWIDTH_FOURBITSWIDTH        (0x0 <<  3) // (LCDC) 4 Bits
#define 	AT91C_LCDC_IFWIDTH_EIGTHBITSWIDTH       (0x1 <<  3) // (LCDC) 8 Bits
#define 	AT91C_LCDC_IFWIDTH_SIXTEENBITSWIDTH     (0x2 <<  3) // (LCDC) 16 Bits
#define AT91C_LCDC_PIXELSIZE      (0x7 <<  5) // (LCDC) Bits per pixel
#define 	AT91C_LCDC_PIXELSIZE_ONEBITSPERPIXEL      (0x0 <<  5) // (LCDC) 1 Bits
#define 	AT91C_LCDC_PIXELSIZE_TWOBITSPERPIXEL      (0x1 <<  5) // (LCDC) 2 Bits
#define 	AT91C_LCDC_PIXELSIZE_FOURBITSPERPIXEL     (0x2 <<  5) // (LCDC) 4 Bits
#define 	AT91C_LCDC_PIXELSIZE_EIGTHBITSPERPIXEL    (0x3 <<  5) // (LCDC) 8 Bits
#define 	AT91C_LCDC_PIXELSIZE_SIXTEENBITSPERPIXEL  (0x4 <<  5) // (LCDC) 16 Bits
#define 	AT91C_LCDC_PIXELSIZE_TWENTYFOURBITSPERPIXEL (0x5 <<  5) // (LCDC) 24 Bits
#define AT91C_LCDC_INVVD          (0x1 <<  8) // (LCDC) lcd datas polarity
#define 	AT91C_LCDC_INVVD_NORMALPOL            (0x0 <<  8) // (LCDC) Normal Polarity
#define 	AT91C_LCDC_INVVD_INVERTEDPOL          (0x1 <<  8) // (LCDC) Inverted Polarity
#define AT91C_LCDC_INVFRAME       (0x1 <<  9) // (LCDC) lcd vsync polarity
#define 	AT91C_LCDC_INVFRAME_NORMALPOL            (0x0 <<  9) // (LCDC) Normal Polarity
#define 	AT91C_LCDC_INVFRAME_INVERTEDPOL          (0x1 <<  9) // (LCDC) Inverted Polarity
#define AT91C_LCDC_INVLINE        (0x1 << 10) // (LCDC) lcd hsync polarity
#define 	AT91C_LCDC_INVLINE_NORMALPOL            (0x0 << 10) // (LCDC) Normal Polarity
#define 	AT91C_LCDC_INVLINE_INVERTEDPOL          (0x1 << 10) // (LCDC) Inverted Polarity
#define AT91C_LCDC_INVCLK         (0x1 << 11) // (LCDC) lcd pclk polarity
#define 	AT91C_LCDC_INVCLK_NORMALPOL            (0x0 << 11) // (LCDC) Normal Polarity
#define 	AT91C_LCDC_INVCLK_INVERTEDPOL          (0x1 << 11) // (LCDC) Inverted Polarity
#define AT91C_LCDC_INVDVAL        (0x1 << 12) // (LCDC) lcd dval polarity
#define 	AT91C_LCDC_INVDVAL_NORMALPOL            (0x0 << 12) // (LCDC) Normal Polarity
#define 	AT91C_LCDC_INVDVAL_INVERTEDPOL          (0x1 << 12) // (LCDC) Inverted Polarity
#define AT91C_LCDC_CLKMOD         (0x1 << 15) // (LCDC) lcd pclk Mode
#define 	AT91C_LCDC_CLKMOD_ACTIVEONLYDISP       (0x0 << 15) // (LCDC) Active during display period
#define 	AT91C_LCDC_CLKMOD_ALWAYSACTIVE         (0x1 << 15) // (LCDC) Always Active
#define AT91C_LCDC_MEMOR          (0x1 << 31) // (LCDC) lcd pclk Mode
#define 	AT91C_LCDC_MEMOR_BIGIND               (0x0 << 31) // (LCDC) Big Endian
#define 	AT91C_LCDC_MEMOR_LITTLEIND            (0x1 << 31) // (LCDC) Little Endian
// -------- LCDC_TIM1 : (LCDC Offset: 0x808) LCDC Timing Config 1 Register --------
#define AT91C_LCDC_VFP            (0xFF <<  0) // (LCDC) Vertical Front Porch
#define AT91C_LCDC_VBP            (0xFF <<  8) // (LCDC) Vertical Back Porch
#define AT91C_LCDC_VPW            (0x3F << 16) // (LCDC) Vertical Synchronization Pulse Width
#define AT91C_LCDC_VHDLY          (0xF << 24) // (LCDC) Vertical to Horizontal Delay
// -------- LCDC_TIM2 : (LCDC Offset: 0x80c) LCDC Timing Config 2 Register --------
#define AT91C_LCDC_HBP            (0xFF <<  0) // (LCDC) Horizontal Back Porch
#define AT91C_LCDC_HPW            (0x3F <<  8) // (LCDC) Horizontal Synchronization Pulse Width
#define AT91C_LCDC_HFP            (0x3FF << 22) // (LCDC) Horizontal Front Porch
// -------- LCDC_LCDFRCFG : (LCDC Offset: 0x810) LCD Frame Config Register --------
#define AT91C_LCDC_LINEVAL        (0x7FF <<  0) // (LCDC) Vertical Size of LCD Module
#define AT91C_LCDC_HOZVAL         (0x7FF << 21) // (LCDC) Horizontal Size of LCD Module
// -------- LCDC_FIFO : (LCDC Offset: 0x814) LCD FIFO Register --------
#define AT91C_LCDC_FIFOTH         (0xFFFF <<  0) // (LCDC) FIFO Threshold
// -------- LCDC_MVAL : (LCDC Offset: 0x818) LCD Mode Toggle Rate Value Register --------
#define AT91C_LCDC_MVALUE         (0xFF <<  0) // (LCDC) Toggle Rate Value
#define AT91C_LCDC_MMODE          (0x1 << 31) // (LCDC) Toggle Rate Sel
#define 	AT91C_LCDC_MMODE_EACHFRAME            (0x0 << 31) // (LCDC) Each Frame
#define 	AT91C_LCDC_MMODE_MVALDEFINED          (0x1 << 31) // (LCDC) Defined by MVAL
// -------- LCDC_DP1_2 : (LCDC Offset: 0x81c) Dithering Pattern 1/2 --------
#define AT91C_LCDC_DP1_2_FIELD    (0xFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP4_7 : (LCDC Offset: 0x820) Dithering Pattern 4/7 --------
#define AT91C_LCDC_DP4_7_FIELD    (0xFFFFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP3_5 : (LCDC Offset: 0x824) Dithering Pattern 3/5 --------
#define AT91C_LCDC_DP3_5_FIELD    (0xFFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP2_3 : (LCDC Offset: 0x828) Dithering Pattern 2/3 --------
#define AT91C_LCDC_DP2_3_FIELD    (0xFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP5_7 : (LCDC Offset: 0x82c) Dithering Pattern 5/7 --------
#define AT91C_LCDC_DP5_7_FIELD    (0xFFFFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP3_4 : (LCDC Offset: 0x830) Dithering Pattern 3/4 --------
#define AT91C_LCDC_DP3_4_FIELD    (0xFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP4_5 : (LCDC Offset: 0x834) Dithering Pattern 4/5 --------
#define AT91C_LCDC_DP4_5_FIELD    (0xFFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_DP6_7 : (LCDC Offset: 0x838) Dithering Pattern 6/7 --------
#define AT91C_LCDC_DP6_7_FIELD    (0xFFFFFFF <<  0) // (LCDC) Ratio
// -------- LCDC_PWRCON : (LCDC Offset: 0x83c) LCDC Power Control Register --------
#define AT91C_LCDC_PWR            (0x1 <<  0) // (LCDC) LCD Module Power Control
#define AT91C_LCDC_GUARDT         (0x7F <<  1) // (LCDC) Delay in Frame Period
#define AT91C_LCDC_BUSY           (0x1 << 31) // (LCDC) Read Only : 1 indicates that LCDC is busy
#define 	AT91C_LCDC_BUSY_LCDNOTBUSY           (0x0 << 31) // (LCDC) LCD is Not Busy
#define 	AT91C_LCDC_BUSY_LCDBUSY              (0x1 << 31) // (LCDC) LCD is Busy
// -------- LCDC_CTRSTCON : (LCDC Offset: 0x840) LCDC Contrast Control Register --------
#define AT91C_LCDC_PS             (0x3 <<  0) // (LCDC) LCD Contrast Counter Prescaler
#define 	AT91C_LCDC_PS_NOTDIVIDED           (0x0) // (LCDC) Counter Freq is System Freq.
#define 	AT91C_LCDC_PS_DIVIDEDBYTWO         (0x1) // (LCDC) Counter Freq is System Freq divided by 2.
#define 	AT91C_LCDC_PS_DIVIDEDBYFOUR        (0x2) // (LCDC) Counter Freq is System Freq divided by 4.
#define 	AT91C_LCDC_PS_DIVIDEDBYEIGHT       (0x3) // (LCDC) Counter Freq is System Freq divided by 8.
#define AT91C_LCDC_POL            (0x1 <<  2) // (LCDC) Polarity of output Pulse
#define 	AT91C_LCDC_POL_NEGATIVEPULSE        (0x0 <<  2) // (LCDC) Negative Pulse
#define 	AT91C_LCDC_POL_POSITIVEPULSE        (0x1 <<  2) // (LCDC) Positive Pulse
#define AT91C_LCDC_ENA            (0x1 <<  3) // (LCDC) PWM generator Control
#define 	AT91C_LCDC_ENA_PWMGEMDISABLED       (0x0 <<  3) // (LCDC) PWM Generator Disabled
#define 	AT91C_LCDC_ENA_PWMGEMENABLED        (0x1 <<  3) // (LCDC) PWM Generator Disabled
// -------- LCDC_CTRSTVAL : (LCDC Offset: 0x844) Contrast Value Register --------
#define AT91C_LCDC_CVAL           (0xFF <<  0) // (LCDC) PWM Compare Value
// -------- LCDC_IER : (LCDC Offset: 0x848) LCDC Interrupt Enable Register --------
#define AT91C_LCDC_LNI            (0x1 <<  0) // (LCDC) Line Interrupt
#define AT91C_LCDC_LSTLNI         (0x1 <<  1) // (LCDC) Last Line Interrupt
#define AT91C_LCDC_EOFI           (0x1 <<  2) // (LCDC) End Of Frame Interrupt
#define AT91C_LCDC_UFLWI          (0x1 <<  4) // (LCDC) FIFO Underflow Interrupt
#define AT91C_LCDC_OWRI           (0x1 <<  5) // (LCDC) Over Write Interrupt
#define AT91C_LCDC_MERI           (0x1 <<  6) // (LCDC) Memory Error  Interrupt
// -------- LCDC_IDR : (LCDC Offset: 0x84c) LCDC Interrupt Disable Register --------
// -------- LCDC_IMR : (LCDC Offset: 0x850) LCDC Interrupt Mask Register --------
// -------- LCDC_ISR : (LCDC Offset: 0x854) LCDC Interrupt Status Register --------
// -------- LCDC_ICR : (LCDC Offset: 0x858) LCDC Interrupt Clear Register --------
// -------- LCDC_GPR : (LCDC Offset: 0x85c) LCDC General Purpose Register --------
#define AT91C_LCDC_GPRBUS         (0xFF <<  0) // (LCDC) 8 bits available
// -------- LCDC_ITR : (LCDC Offset: 0x860) Interrupts Test Register --------
// -------- LCDC_IRR : (LCDC Offset: 0x864) Interrupts Raw Status Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR DMA controller from Synopsys
// *****************************************************************************
// *** Register offset in AT91S_DMA structure ***
#define DMA_SAR0        ( 0) // Source Address Register for channel 0
#define DMA_DAR0        ( 8) // Destination Address Register for channel 0
#define DMA_LLP0        (16) // Linked List Pointer Register for channel 0
#define DMA_CTL0l       (24) // Control Register for channel 0 - low
#define DMA_CTL0h       (28) // Control Register for channel 0 - high
#define DMA_SSTAT0      (32) // Source Status Register for channel 0
#define DMA_DSTAT0      (40) // Destination Status Register for channel 0
#define DMA_SSTATAR0    (48) // Source Status Adress Register for channel 0
#define DMA_DSTATAR0    (56) // Destination Status Adress Register for channel 0
#define DMA_CFG0l       (64) // Configuration Register for channel 0 - low
#define DMA_CFG0h       (68) // Configuration Register for channel 0 - high
#define DMA_SGR0        (72) // Source Gather Register for channel 0
#define DMA_DSR0        (80) // Destination Scatter Register for channel 0
#define DMA_SAR1        (88) // Source Address Register for channel 1
#define DMA_DAR1        (96) // Destination Address Register for channel 1
#define DMA_LLP1        (104) // Linked List Pointer Register for channel 1
#define DMA_CTL1l       (112) // Control Register for channel 1 - low
#define DMA_CTL1h       (116) // Control Register for channel 1 - high
#define DMA_SSTAT1      (120) // Source Status Register for channel 1
#define DMA_DSTAT1      (128) // Destination Status Register for channel 1
#define DMA_SSTATAR1    (136) // Source Status Adress Register for channel 1
#define DMA_DSTATAR1    (144) // Destination Status Adress Register for channel 1
#define DMA_CFG1l       (152) // Configuration Register for channel 1 - low
#define DMA_CFG1h       (156) // Configuration Register for channel 1 - high
#define DMA_SGR1        (160) // Source Gather Register for channel 1
#define DMA_DSR1        (168) // Destination Scatter Register for channel 1
#define DMA_RAWTFR      (704) // Raw Status for IntTfr Interrupt
#define DMA_RAWBLOCK    (712) // Raw Status for IntBlock Interrupt
#define DMA_RAWSRCTRAN  (720) // Raw Status for IntSrcTran Interrupt
#define DMA_RAWDSTTRAN  (728) // Raw Status for IntDstTran Interrupt
#define DMA_RAWERR      (736) // Raw Status for IntErr Interrupt
#define DMA_STATUSTFR   (744) // Status for IntTfr Interrupt
#define DMA_STATUSBLOCK (752) // Status for IntBlock Interrupt
#define DMA_STATUSSRCTRAN (760) // Status for IntSrcTran Interrupt
#define DMA_STATUSDSTTRAN (768) // Status for IntDstTran IInterrupt
#define DMA_STATUSERR   (776) // Status for IntErr IInterrupt
#define DMA_MASKTFR     (784) // Mask for IntTfr Interrupt
#define DMA_MASKBLOCK   (792) // Mask for IntBlock Interrupt
#define DMA_MASKSRCTRAN (800) // Mask for IntSrcTran Interrupt
#define DMA_MASKDSTTRAN (808) // Mask for IntDstTran Interrupt
#define DMA_MASKERR     (816) // Mask for IntErr Interrupt
#define DMA_CLEARTFR    (824) // Clear for IntTfr Interrupt
#define DMA_CLEARBLOCK  (832) // Clear for IntBlock Interrupt
#define DMA_CLEARSRCTRAN (840) // Clear for IntSrcTran Interrupt
#define DMA_CLEARDSTTRAN (848) // Clear for IntDstTran IInterrupt
#define DMA_CLEARERR    (856) // Clear for IntErr Interrupt
#define DMA_STATUSINT   (864) // Status for each Interrupt Type
#define DMA_REQSRCREG   (872) // Source Software Transaction Request Register
#define DMA_REQDSTREG   (880) // Destination Software Transaction Request Register
#define DMA_SGLREQSRCREG (888) // Single Source Software Transaction Request Register
#define DMA_SGLREQDSTREG (896) // Single Destination Software Transaction Request Register
#define DMA_LSTREQSRCREG (904) // Last Source Software Transaction Request Register
#define DMA_LSTREQDSTREG (912) // Last Destination Software Transaction Request Register
#define DMA_DMACFGREG   (920) // DW_ahb_dmac Configuration Register
#define DMA_CHENREG     (928) // DW_ahb_dmac Channel Enable Register
#define DMA_DMAIDREG    (936) // DW_ahb_dmac ID Register
#define DMA_DMATESTREG  (944) // DW_ahb_dmac Test Register
#define DMA_VERSIONID   (952) // DW_ahb_dmac Version ID Register
// -------- DMA_SAR : (DMA Offset: 0x0)  --------
#define AT91C_DMA_SADD            (0x0 <<  0) // (DMA) Source Address of DMA Transfer
// -------- DMA_DAR : (DMA Offset: 0x8)  --------
#define AT91C_DMA_DADD            (0x0 <<  0) // (DMA) Destination Address of DMA Transfer
// -------- DMA_LLP : (DMA Offset: 0x10)  --------
#define AT91C_DMA_LOC             (0x0 <<  0) // (DMA) Address of the Next LLI
// -------- DMA_CTLl : (DMA Offset: 0x18)  --------
#define AT91C_DMA_INT_EN          (0x1 <<  0) // (DMA) Interrupt Enable Bit
#define AT91C_DMA_DST_TR_WIDTH    (0x7 <<  1) // (DMA) Destination Transfer Width
#define AT91C_DMA_SRC_TR_WIDTH    (0x7 <<  4) // (DMA) Source Transfer Width
#define AT91C_DMA_DINC            (0x3 <<  7) // (DMA) Destination Address Increment
#define AT91C_DMA_SINC            (0x3 <<  9) // (DMA) Source Address Increment
#define AT91C_DMA_DEST_MSIZE      (0x7 << 11) // (DMA) Destination Burst Transaction Length
#define AT91C_DMA_SRC_MSIZE       (0x7 << 14) // (DMA) Source Burst Transaction Length
#define AT91C_DMA_S_GATH_EN       (0x1 << 17) // (DMA) Source Gather Enable Bit
#define AT91C_DMA_D_SCAT_EN       (0x1 << 18) // (DMA) Destination Scatter Enable Bit
#define AT91C_DMA_TT_FC           (0x7 << 20) // (DMA) Transfer Type and Flow Control
#define AT91C_DMA_DMS             (0x3 << 23) // (DMA) Destination Master Select
#define AT91C_DMA_SMS             (0x3 << 25) // (DMA) Source Master Select
#define AT91C_DMA_LLP_D_EN        (0x1 << 27) // (DMA) Destination Block Chaining Enable
#define AT91C_DMA_LLP_S_EN        (0x1 << 28) // (DMA) Source Block Chaining Enable
// -------- DMA_CTLh : (DMA Offset: 0x1c)  --------
#define AT91C_DMA_BLOCK_TS        (0xFFF <<  0) // (DMA) Block Transfer Size
#define AT91C_DMA_DONE            (0x1 << 12) // (DMA) Done bit
// -------- DMA_CFGl : (DMA Offset: 0x40)  --------
#define AT91C_DMA_CH_PRIOR        (0x7 <<  5) // (DMA) Channel Priority
#define AT91C_DMA_CH_SUSP         (0x1 <<  8) // (DMA) Channel Suspend
#define AT91C_DMA_FIFO_EMPT       (0x1 <<  9) // (DMA) Fifo Empty
#define AT91C_DMA_HS_SEL_DS       (0x1 << 10) // (DMA) Destination Software or Hardware Handshaking Select
#define AT91C_DMA_HS_SEL_SR       (0x1 << 11) // (DMA) Source Software or Hardware Handshaking Select
#define AT91C_DMA_LOCK_CH_L       (0x3 << 12) // (DMA) Channel Lock Level
#define AT91C_DMA_LOCK_B_L        (0x3 << 14) // (DMA) Bus Lock Level
#define AT91C_DMA_LOCK_CH         (0x1 << 16) // (DMA) Channel Lock Bit
#define AT91C_DMA_LOCK_B          (0x1 << 17) // (DMA) Bus Lock Bit
#define AT91C_DMA_DS_HS_POL       (0x1 << 18) // (DMA) Destination Handshaking Interface Polarity
#define AT91C_DMA_SR_HS_POL       (0x1 << 19) // (DMA) Source Handshaking Interface Polarity
#define AT91C_DMA_MAX_ABRST       (0x3FF << 20) // (DMA) Maximum AMBA Burst Length
#define AT91C_DMA_RELOAD_SR       (0x1 << 30) // (DMA) Automatic Source Reload
#define AT91C_DMA_RELOAD_DS       (0x1 << 31) // (DMA) Automatic Destination Reload
// -------- DMA_CFGh : (DMA Offset: 0x44)  --------
#define AT91C_DMA_FCMODE          (0x1 <<  0) // (DMA) Flow Control Mode
#define AT91C_DMA_FIFO_MODE       (0x1 <<  1) // (DMA) Fifo Mode Select
#define AT91C_DMA_PROTCTL         (0x7 <<  2) // (DMA) Protection Control
#define AT91C_DMA_DS_UPD_EN       (0x1 <<  5) // (DMA) Destination Status Update Enable
#define AT91C_DMA_SS_UPD_EN       (0x1 <<  6) // (DMA) Source Status Update Enable
#define AT91C_DMA_SRC_PER         (0xF <<  7) // (DMA) Source Hardware Handshaking Interface
#define AT91C_DMA_DEST_PER        (0xF << 11) // (DMA) Destination Hardware Handshaking Interface
// -------- DMA_SGR : (DMA Offset: 0x48)  --------
#define AT91C_DMA_SGI             (0xFFFFF <<  0) // (DMA) Source Gather Interval
#define AT91C_DMA_SGC             (0xFFF << 20) // (DMA) Source Gather Count
// -------- DMA_DSR : (DMA Offset: 0x50)  --------
#define AT91C_DMA_DSI             (0xFFFFF <<  0) // (DMA) Destination Scatter Interval
#define AT91C_DMA_DSC             (0xFFF << 20) // (DMA) Destination Scatter Count
// -------- DMA_SAR : (DMA Offset: 0x58)  --------
// -------- DMA_DAR : (DMA Offset: 0x60)  --------
// -------- DMA_LLP : (DMA Offset: 0x68)  --------
// -------- DMA_CTLl : (DMA Offset: 0x70)  --------
// -------- DMA_CTLh : (DMA Offset: 0x74)  --------
// -------- DMA_CFGl : (DMA Offset: 0x98)  --------
// -------- DMA_CFGh : (DMA Offset: 0x9c)  --------
// -------- DMA_SGR : (DMA Offset: 0xa0)  --------
// -------- DMA_DSR : (DMA Offset: 0xa8)  --------
// -------- DMA_RAWTFR : (DMA Offset: 0x2c0)  --------
#define AT91C_DMA_RAW             (0x7 <<  0) // (DMA) Raw Interrupt for each Channel
// -------- DMA_RAWBLOCK : (DMA Offset: 0x2c8)  --------
// -------- DMA_RAWSRCTRAN : (DMA Offset: 0x2d0)  --------
// -------- DMA_RAWDSTTRAN : (DMA Offset: 0x2d8)  --------
// -------- DMA_RAWERR : (DMA Offset: 0x2e0)  --------
// -------- DMA_STATUSTFR : (DMA Offset: 0x2e8)  --------
#define AT91C_DMA_STATUS          (0x7 <<  0) // (DMA) Interrupt for each Channel
// -------- DMA_STATUSBLOCK : (DMA Offset: 0x2f0)  --------
// -------- DMA_STATUSSRCTRAN : (DMA Offset: 0x2f8)  --------
// -------- DMA_STATUSDSTTRAN : (DMA Offset: 0x300)  --------
// -------- DMA_STATUSERR : (DMA Offset: 0x308)  --------
// -------- DMA_MASKTFR : (DMA Offset: 0x310)  --------
#define AT91C_DMA_INT_MASK        (0x7 <<  0) // (DMA) Interrupt Mask for each Channel
#define AT91C_DMA_INT_M_WE        (0x7 <<  8) // (DMA) Interrupt Mask Write Enable for each Channel
// -------- DMA_MASKBLOCK : (DMA Offset: 0x318)  --------
// -------- DMA_MASKSRCTRAN : (DMA Offset: 0x320)  --------
// -------- DMA_MASKDSTTRAN : (DMA Offset: 0x328)  --------
// -------- DMA_MASKERR : (DMA Offset: 0x330)  --------
// -------- DMA_CLEARTFR : (DMA Offset: 0x338)  --------
#define AT91C_DMA_CLEAR           (0x7 <<  0) // (DMA) Interrupt Clear for each Channel
// -------- DMA_CLEARBLOCK : (DMA Offset: 0x340)  --------
// -------- DMA_CLEARSRCTRAN : (DMA Offset: 0x348)  --------
// -------- DMA_CLEARDSTTRAN : (DMA Offset: 0x350)  --------
// -------- DMA_CLEARERR : (DMA Offset: 0x358)  --------
// -------- DMA_STATUSINT : (DMA Offset: 0x360)  --------
#define AT91C_DMA_TFR             (0x1 <<  0) // (DMA) OR of the content of StatusTfr Register
#define AT91C_DMA_BLOCK           (0x1 <<  1) // (DMA) OR of the content of StatusBlock Register
#define AT91C_DMA_SRCT            (0x1 <<  2) // (DMA) OR of the content of StatusSrcTran Register
#define AT91C_DMA_DSTT            (0x1 <<  3) // (DMA) OR of the content of StatusDstTran Register
#define AT91C_DMA_ERR             (0x1 <<  4) // (DMA) OR of the content of StatusErr Register
// -------- DMA_REQSRCREG : (DMA Offset: 0x368)  --------
#define AT91C_DMA_SRC_REQ         (0x7 <<  0) // (DMA) Source Request
#define AT91C_DMA_REQ_WE          (0x7 <<  8) // (DMA) Request Write Enable
// -------- DMA_REQDSTREG : (DMA Offset: 0x370)  --------
#define AT91C_DMA_DST_REQ         (0x7 <<  0) // (DMA) Destination Request
// -------- DMA_SGLREQSRCREG : (DMA Offset: 0x378)  --------
#define AT91C_DMA_S_SG_REQ        (0x7 <<  0) // (DMA) Source Single Request
// -------- DMA_SGLREQDSTREG : (DMA Offset: 0x380)  --------
#define AT91C_DMA_D_SG_REQ        (0x7 <<  0) // (DMA) Destination Single Request
// -------- DMA_LSTREQSRCREG : (DMA Offset: 0x388)  --------
#define AT91C_DMA_LSTSRC          (0x7 <<  0) // (DMA) Source Last Transaction Request
#define AT91C_DMA_LSTSR_WE        (0x7 <<  8) // (DMA) Source Last Transaction Request Write Enable
// -------- DMA_LSTREQDSTREG : (DMA Offset: 0x390)  --------
#define AT91C_DMA_LSTDST          (0x7 <<  0) // (DMA) Destination Last Transaction Request
#define AT91C_DMA_LSTDS_WE        (0x7 <<  8) // (DMA) Destination Last Transaction Request Write Enable
// -------- DMA_DMACFGREG : (DMA Offset: 0x398)  --------
#define AT91C_DMA_DMA_EN          (0x7 <<  0) // (DMA) Controller Enable
// -------- DMA_CHENREG : (DMA Offset: 0x3a0)  --------
#define AT91C_DMA_CH_EN           (0x7 <<  0) // (DMA) Channel Enable
#define AT91C_DMA_CH_EN_WE        (0x7 <<  8) // (DMA) Channel Enable Write Enable
// -------- DMA_DMATESTREG : (DMA Offset: 0x3b0)  --------
#define AT91C_DMA_TEST_SLV_IF     (0x1 <<  0) // (DMA) Test Mode for Slave Interface

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB OTG Controller
// *****************************************************************************
// *** Register offset in AT91S_OTG structure ***
#define OTG_InTxFifo    ( 0) // OTG IN TX FIFO. In Host mode, acts as a RX FIFO, in Device mode acts as a TX FIFO (it could be named DevTxFifo0)
#define OTG_DevTxFifo1  (1024) // OTG IN TX FIFO for EP1 in Device mode
#define OTG_DevTxFifo2  (2048) // OTG IN TX FIFO for EP2 in Device mode
#define OTG_DevTxFifo3  (3072) // OTG IN TX FIFO for EP3 in Device mode
#define OTG_DevTxFifo4  (4096) // OTG IN TX FIFO for EP4 in Device mode
#define OTG_DevTxFifo5  (5120) // OTG IN TX FIFO for EP5 in Device mode
#define OTG_OutRxFifo   (16384) // OTG OUT RX FIFO. In Host mode, acts as a TX FIFO, in Device mode, acts as a RX FIFO
#define OTG_DevTxControl0 (17408) // IN EP0 TX Control Reg, Device mode
#define OTG_DevTxStatus0 (17412) // IN EP0 TX Status Reg, Device mode
#define OTG_DevTxFifoSize0 (17416) // IN EP0 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount0 (17420) // IN EP0 TX Transfert Size Reg, Device mode
#define OTG_DevTxControl1 (17424) // IN EP1 TX Control Reg, Device mode
#define OTG_DevTxStatus1 (17428) // IN EP1 TX Status Reg, Device mode
#define OTG_DevTxFifoSize1 (17432) // IN EP1 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount1 (17436) // IN EP1 TX Transfert Size Reg, Device mode
#define OTG_DevTxControl2 (17440) // IN EP2 TX Control Reg, Device mode
#define OTG_DevTxStatus2 (17444) // IN EP2 TX Status Reg, Device mode
#define OTG_DevTxFifoSize2 (17448) // IN EP2 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount2 (17452) // IN EP2 TX Transfert Size Reg, Device mode
#define OTG_DevTxControl3 (17456) // IN EP3 TX Control Reg, Device mode
#define OTG_DevTxStatus3 (17460) // IN EP3 TX Status Reg, Device mode
#define OTG_DevTxFifoSize3 (17464) // IN EP3 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount3 (17468) // IN EP3 TX Transfert Size Reg, Device mode
#define OTG_DevTxControl4 (17472) // IN EP4 TX Control Reg, Device mode
#define OTG_DevTxStatus4 (17476) // IN EP4 TX Status Reg, Device mode
#define OTG_DevTxFifoSize4 (17480) // IN EP4 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount4 (17484) // IN EP4 TX Transfert Size Reg, Device mode
#define OTG_DevTxControl5 (17488) // IN EP5 TX Control Reg, Device mode
#define OTG_DevTxStatus5 (17492) // IN EP5 TX Status Reg, Device mode
#define OTG_DevTxFifoSize5 (17496) // IN EP5 TX FIFO Size Reg, Device mode
#define OTG_DevTxCount5 (17500) // IN EP5 TX Transfert Size Reg, Device mode
#define OTG_DevRxControl0 (17664) // OUT EP0 RX Control Reg, Device mode
#define OTG_DevRxControl1 (17680) // OUT EP1 RX Control Reg, Device mode
#define OTG_DevRxControl2 (17696) // OUT EP2 RX Control Reg, Device mode
#define OTG_DevRxControl3 (17712) // OUT EP3 RX Control Reg, Device mode
#define OTG_DevRxControl4 (17728) // OUT EP4 RX Control Reg, Device mode
#define OTG_DevRxControl5 (17744) // OUT EP5 RX Control Reg, Device mode
#define OTG_DevConfig   (17920) //
#define OTG_DevStatus   (17924) //
#define OTG_DevIntr     (17928) //
#define OTG_DevIntrMask (17932) //
#define OTG_DevRxFifoSize (17936) // OUT EP RX FIFO Size Reg (Device mode)
#define OTG_DevEnpIntrMask (17940) // Global EP Interrupt Enable Reg (Device mode)
#define OTG_DevThreshold (17944) // Threshold Reg (Device mode)
#define OTG_DevRxStatus (17948) // OUT EP RX FIFO Status Reg (Device mode)
#define OTG_DevSetupStatus (17952) // Setup RX FIFO Status Reg (Device mode)
#define OTG_DevEnpIntr  (17956) // Global EP Interrupt Reg (Device mode)
#define OTG_DevFrameNum (17960) // Frame Number Reg (Device mode)
#define OTG_DevSetupDataLow (18176) // Setup Data 1st DWORD (Device mode)
#define OTG_DevSetupDataHigh (18180) // Setup Data 1st DWORD (Device mode)
#define OTG_Biu         (18184) // Slave BIU Delay Count Reg (Device and Host modes)
#define OTG_I2C         (18188) // I2C Reg (Device and Host mode)
#define OTG_DevEpNe0    (18436) // EP0 NE Reg, Device mode
#define OTG_DevEpNe1    (18440) // EP1 NE Reg, Device mode
#define OTG_DevEpNe2    (18444) // EP2 NE Reg, Device mode
#define OTG_DevEpNe3    (18448) // EP3 NE Reg, Device mode
#define OTG_DevEpNe4    (18452) // EP4 NE Reg, Device mode
#define OTG_DevEpNe5    (18456) // EP5 NE Reg, Device mode
#define OTG_HostIntr    (19456) // Interrupt Reg (Host mode)
#define OTG_HostIntrMask (19460) // Interrupt Enable Reg (Host mode)
#define OTG_HostStatus  (19464) // Status Reg (Host mode)
#define OTG_HostFifoControl (19468) // Host Control Reg (Host mode)
#define OTG_HostFifoSize (19472) // FIFO Size Reg (Host mode)
#define OTG_HostThreshold (19476) // Threshold Reg (Host mode)
#define OTG_HostTxCount (19480) // OUT Transfert Size Reg (Host mode)
#define OTG_HostFrameIntvl (20532) // Frame Interval Reg (Host mode)
#define OTG_HostFrameRem (20536) // Frame Remaining Reg (Host mode)
#define OTG_HostFrameNum (20540) // Frame Number Reg (Host mode)
#define OTG_HostRootHubPort0 (20564) // Port Status Change Control Reg (Host mode)
#define OTG_HostToken   (20624) // Token Reg (Host mode)
#define OTG_CtrlStatus  (20628) // Control and Status Reg (Host mode)
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4400) IN EP TX Control Reg, Device mode --------
#define AT91C_TX_SEND_STALL       (0x1 <<  0) // (OTG)
#define AT91C_SEND_NACK           (0x1 <<  1) // (OTG)
#define AT91C_TX_FLUSH_FIFO       (0x1 <<  2) // (OTG)
#define AT91C_TX_FIFO_READY       (0x1 <<  3) // (OTG)
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4404) IN EP TX Status Reg, Device mode --------
#define AT91C_TX_STATUS           (0x1 <<  0) // (OTG) RO
#define AT91C_DATA_SENT           (0x1 <<  1) // (OTG) RW
#define AT91C_BELOW_THRESHOLD     (0x1 <<  2) // (OTG)
#define AT91C_NAK_SENT            (0x1 <<  3) // (OTG)
#define AT91C_DATA_UNDERRUN_ERROR (0x1 <<  4) // (OTG)
#define AT91C_ISO_TX_DONE         (0x1 <<  5) // (OTG)
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4408) IN EP TX FIFO Size Reg, Device mode --------
#define AT91C_DEV_TX_FIFO_SIZE    (0x3FF <<  0) // (OTG) RW
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x440c) IN EP TX Transfert Size Reg, Device mode --------
#define AT91C_TX_TRANSFERT_SIZE   (0x3FF <<  0) // (OTG) RW
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4410) IN EP TX Control Reg, Device mode --------
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4414) IN EP TX Status Reg, Device mode --------
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4418) IN EP TX FIFO Size Reg, Device mode --------
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x441c) IN EP TX Transfert Size Reg, Device mode --------
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4420) IN EP TX Control Reg, Device mode --------
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4424) IN EP TX Status Reg, Device mode --------
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4428) IN EP TX FIFO Size Reg, Device mode --------
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x442c) IN EP TX Transfert Size Reg, Device mode --------
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4430) IN EP TX Control Reg, Device mode --------
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4434) IN EP TX Status Reg, Device mode --------
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4438) IN EP TX FIFO Size Reg, Device mode --------
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x443c) IN EP TX Transfert Size Reg, Device mode --------
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4440) IN EP TX Control Reg, Device mode --------
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4444) IN EP TX Status Reg, Device mode --------
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4448) IN EP TX FIFO Size Reg, Device mode --------
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x444c) IN EP TX Transfert Size Reg, Device mode --------
// -------- OTG_DEV_TX_CONTROL : (OTG Offset: 0x4450) IN EP TX Control Reg, Device mode --------
// -------- OTG_DEV_TX_STATUS : (OTG Offset: 0x4454) IN EP TX Status Reg, Device mode --------
// -------- OTG_DEV_TX_FIFOSIZE : (OTG Offset: 0x4458) IN EP TX FIFO Size Reg, Device mode --------
// -------- OTG_DEV_TX_COUNT : (OTG Offset: 0x445c) IN EP TX Transfert Size Reg, Device mode --------
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4500) OUT EP RX Control Reg, Device mode --------
#define AT91C_RX_SEND_STALL       (0x1 <<  0) // (OTG) RW
#define AT91C_RX_FLUSH_FIFO       (0x1 <<  2) // (OTG) RW
#define AT91C_RX_FIFO_READY       (0x1 <<  3) // (OTG) RW
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4510) OUT EP RX Control Reg, Device mode --------
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4520) OUT EP RX Control Reg, Device mode --------
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4530) OUT EP RX Control Reg, Device mode --------
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4540) OUT EP RX Control Reg, Device mode --------
// -------- OTG_DEV_RX_CONTROL : (OTG Offset: 0x4550) OUT EP RX Control Reg, Device mode --------
// -------- OTG_DEV_CONFIG : (OTG Offset: 0x4600) Device Config Reg, Device mode --------
#define AT91C_SPEED               (0x3 <<  0) // (OTG)
#define AT91C_REMOTE_WAKEUP       (0x1 <<  2) // (OTG)
#define AT91C_SELF_POWERED        (0x1 <<  3) // (OTG)
#define AT91C_SYNC_FRAME          (0x1 <<  4) // (OTG)
#define AT91C_CSR_PRG_SUP         (0x1 <<  5) // (OTG)
#define AT91C_CSR_DONE            (0x1 <<  6) // (OTG)
#define AT91C_SET_DESC_SUP        (0x1 <<  7) // (OTG)
#define AT91C_HST_MODE            (0x1 <<  8) // (OTG)
#define AT91C_SCALE_DOWN          (0x1 <<  9) // (OTG)
#define AT91C_SOFT_DISCONNECT     (0x1 << 10) // (OTG)
#define AT91C_STATUS              (0x1 << 11) // (OTG)
#define AT91C_STATUS_1            (0x1 << 12) // (OTG)
// -------- OTG_DEV_STATUS : (OTG Offset: 0x4604) Device Status Reg, Device mode --------
#define AT91C_CFG                 (0xF <<  0) // (OTG)
#define AT91C_INTF                (0xF <<  4) // (OTG)
#define AT91C_ALT                 (0xF <<  8) // (OTG)
#define AT91C_SUSP                (0x1 << 12) // (OTG)
#define AT91C_TS                  (0x7FF << 21) // (OTG)
// -------- OTG_DEV_INTR : (OTG Offset: 0x4608) Device Interrupt Reg, Device mode --------
#define AT91C_SC_INT              (0x1 <<  0) // (OTG) RW
#define AT91C_SI_INT              (0x1 <<  1) // (OTG) RW
#define AT91C_UR_INT              (0x1 <<  3) // (OTG) RW
#define AT91C_US_INT              (0x1 <<  4) // (OTG) RW
#define AT91C_SOF_INT             (0x1 <<  5) // (OTG) RW
#define AT91C_SETUP_RECEIVED      (0x1 <<  6) // (OTG) RO
#define AT91C_OUT_RECEIVED        (0x1 <<  7) // (OTG) RO
#define AT91C_PORT_INT            (0x1 <<  8) // (OTG) RW
#define AT91C_OTG_INT             (0x1 <<  9) // (OTG) RW
#define AT91C_DEV_I2C_INT         (0x1 << 10) // (OTG) RW
// -------- OTG_DEV_INTR_MASK : (OTG Offset: 0x460c) Device Interrupt Enable Reg, Device mode --------
#define AT91C_SC_INT_ENABLE       (0x1 <<  0) // (OTG) RW
#define AT91C_SI_INT_ENABLE       (0x1 <<  1) // (OTG) RW
#define AT91C_UR_INT_ENABLE       (0x1 <<  3) // (OTG) RW
#define AT91C_US_INT_ENABLE       (0x1 <<  4) // (OTG) RW
#define AT91C_SOF_INT_ENABLE      (0x1 <<  5) // (OTG) RW
#define AT91C_SETUP_INT_ENABLE    (0x1 <<  6) // (OTG) RW
#define AT91C_OUT_RX_FIFO_ENABLE  (0x1 <<  7) // (OTG) RW
#define AT91C_PORT_INT_ENABLE     (0x1 <<  8) // (OTG) RW
#define AT91C_OTG_INT_ENABLE      (0x1 <<  9) // (OTG) RW
#define AT91C_I2C_INT_ENABLE      (0x1 << 10) // (OTG) RW
// -------- OTG_DEV_RX_FIFOSIZE : (OTG Offset: 0x4610)  --------
#define AT91C_DEV_RX_FIFO_SIZE    (0x3FF <<  0) // (OTG)
// -------- OTG_DEV_ENP_INTR_MASK : (OTG Offset: 0x4614)  --------
#define AT91C_IN_EP_INT_ENABLE    (0xFFFF <<  0) // (OTG)
#define AT91C_OUT_EP_INT_ENABLE   (0xFFFF << 16) // (OTG)
// -------- OTG_DEV_THRESHOLD : (OTG Offset: 0x4618)  --------
#define AT91C_TX_THRESHOLD        (0x3FF <<  0) // (OTG)
#define AT91C_RX_THRESHOLD        (0x3FF << 16) // (OTG)
// -------- OTG_DEV_RX_STATUS : (OTG Offset: 0x461c)  --------
#define AT91C_RX_STATUS           (0x1 <<  0) // (OTG)
#define AT91C_RX_STATUS_COMPLETE  (0x1 <<  1) // (OTG)
#define AT91C_ABOVE_THRESHOLD     (0x1 <<  2) // (OTG)
#define AT91C_DATA_OVERRUN_ERROR  (0x1 <<  4) // (OTG)
#define AT91C_RX_ENDPOINT_NUMBER  (0xF << 16) // (OTG)
#define AT91C_RX_TRANSFERT_SIZE   (0x3FF << 22) // (OTG)
// -------- OTG_DEV_SETUP_STATUS : (OTG Offset: 0x4620) Setup Rx FIFO Status Register --------
#define AT91C_SETUP_STATUS        (0x1 <<  0) // (OTG) RO
#define AT91C_SETUP_STATUS_COMPLETE (0x1 <<  1) // (OTG) RW
#define AT91C_SETUP_AFTER_OUT     (0x1 << 15) // (OTG) RO
#define AT91C_SETUP_ENDPOINT_NUMBER (0xF << 16) // (OTG) RO
// -------- OTG_DEV_ENP_INTR : (OTG Offset: 0x4624)  --------
#define AT91C_IN_EP_INT           (0xFFFF <<  0) // (OTG)
#define AT91C_OUT_EP_INT          (0xFFFF << 16) // (OTG)
// -------- OTG_DEV_FRAME_NUM : (OTG Offset: 0x4628)  --------
#define AT91C_FRAME_NUMBER        (0x7FF <<  0) // (OTG)
// -------- OTG_DEV_SETUP_DATA_LOW : (OTG Offset: 0x4700)  --------
#define AT91C_FIRST_SETUP_DATA    (0x0 <<  0) // (OTG)
// -------- OTG_DEV_SETUP_DATA_HIGH : (OTG Offset: 0x4704)  --------
#define AT91C_SECOND_SETUP_DATA   (0x0 <<  0) // (OTG)
// -------- OTG_DEV_BIU : (OTG Offset: 0x4708)  --------
#define AT91C_DELAY_COUNT         (0x3 <<  0) // (OTG)
// -------- OTG_I2C : (OTG Offset: 0x470c)  --------
#define AT91C_I2C_WRITE_READ_DATA (0xFF <<  0) // (OTG)
#define AT91C_I2C_DATA2           (0xFF <<  8) // (OTG)
#define AT91C_I2C_ADDRESS         (0x1F << 16) // (OTG)
#define AT91C_NEW_REGISTER_ADDRESS (0x1 << 28) // (OTG)
#define AT91C_B2V                 (0x1 << 29) // (OTG)
#define AT91C_READ_WRITE_INDICATOR (0x1 << 30) // (OTG)
#define AT91C_I2C_BUSY_DONE_INDICATOR (0x1 << 31) // (OTG)
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x4804)  --------
#define AT91C_NE_ENDPOINT_NUMBER  (0xF <<  0) // (OTG)
#define AT91C_ENDPOINT_DIR        (0x1 <<  4) // (OTG)
#define AT91C_ENDPOINT_TYPE       (0x3 <<  5) // (OTG)
#define AT91C_CONFIG_NUM          (0xF <<  7) // (OTG)
#define AT91C_INTERFACE_NUM       (0xF << 11) // (OTG)
#define AT91C_ALTERNATE_NUM       (0xF << 15) // (OTG)
#define AT91C_MAX_PACKET_SIZE     (0x3FF << 19) // (OTG)
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x4808)  --------
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x480c)  --------
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x4810)  --------
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x4814)  --------
// -------- OTG_DEV_EP_NE : (OTG Offset: 0x4818)  --------
// -------- OTG_HOST_INTR : (OTG Offset: 0x4c00)  --------
#define AT91C_STATUS_INT          (0x1 <<  0) // (OTG)
#define AT91C_ABOVE_THRESHOLD_INT (0x1 <<  1) // (OTG)
#define AT91C_BELOW_THRESHOLD_INT (0x1 <<  2) // (OTG)
#define AT91C_SOF_DUE             (0x1 <<  3) // (OTG)
#define AT91C_HOST_PORT_INT       (0x1 <<  8) // (OTG)
#define AT91C_HOST_OTG_INT        (0x1 <<  9) // (OTG)
#define AT91C_HOST_I2C_INT        (0x1 << 10) // (OTG)
// -------- OTG_HOST_INTR_MASK : (OTG Offset: 0x4c04)  --------
#define AT91C_STATUS_INT_ENABLE   (0x1 <<  0) // (OTG)
#define AT91C_ABOVE_THRESHOLD_INT_ENABLE (0x1 <<  1) // (OTG)
#define AT91C_BELOW_THRESHOLD_INT_ENABLE (0x1 <<  2) // (OTG)
#define AT91C_SOF_DUE_ENABLE      (0x1 <<  3) // (OTG)
#define AT91C_HOST_PORT_INT_ENABLE (0x1 <<  8) // (OTG)
#define AT91C_HOST_OTG_INT_ENABLE (0x1 <<  9) // (OTG)
#define AT91C_HOST_I2C_INT_ENABLE (0x1 << 10) // (OTG)
// -------- OTG_HOST_STATUS : (OTG Offset: 0x4c08)  --------
#define AT91C_RESPONSE_CODE       (0xF <<  0) // (OTG) RO
#define AT91C_HOST_TRANSFERT_SIZE (0x3FF <<  4) // (OTG) RO
// -------- OTG_HOST_FIFO_CONTROL : (OTG Offset: 0x4c0c)  --------
#define AT91C_HOST_FLUSH_FIFO     (0x1 <<  0) // (OTG)
#define AT91C_HOST_SCALE_DOWN     (0x1 <<  9) // (OTG)
// -------- OTG_HOST_FIFOSIZE : (OTG Offset: 0x4c10) FIFO Size Register (Host mode) --------
#define AT91C_HOST_RX_FIFO_SIZE   (0x3FF <<  0) // (OTG) RW
#define AT91C_HOST_TX_FIFO_SIZE   (0x3FF << 16) // (OTG) RW
// -------- OTG_HOST_THRESHOLD : (OTG Offset: 0x4c14) Threshold Register (Host Mode) --------
// -------- OTG_HOST_TX_COUNT : (OTG Offset: 0x4c18)  --------
#define AT91C_TRANSFERT_SIZE      (0x3FF <<  0) // (OTG)
// -------- OTG_HOST_FRAME_INTVL : (OTG Offset: 0x5034)  --------
#define AT91C_FRAME_INTERVAL      (0x3FFF <<  0) // (OTG)
// -------- OTG_HOST_FRAME_REM : (OTG Offset: 0x5038)  --------
#define AT91C_FRAME_REMAINING     (0x3FFF <<  0) // (OTG)
// -------- OTG_HOST_FRAME_NUM : (OTG Offset: 0x503c)  --------
// -------- OTG_HOST_ROOT_HUB_PORT_0 : (OTG Offset: 0x5054) Port Status Change Control Register (Host Mode) --------
#define AT91C_CONNECT_STATUS_CLEAR_PORT (0x1 <<  0) // (OTG) RW
#define AT91C_PORT_ENABLE         (0x1 <<  1) // (OTG)
#define AT91C_PORT_SUSPEND        (0x1 <<  2) // (OTG)
#define AT91C_PORT_OVERCURR       (0x1 <<  3) // (OTG)
#define AT91C_PORT_RESET          (0x1 <<  4) // (OTG)
#define AT91C_PORT_POWER          (0x1 <<  8) // (OTG)
#define AT91C_LS_DEV_ATTACHED_CLEAR_PORT_POWER (0x1 <<  9) // (OTG)
#define AT91C_CONNECT_STATUS_CHANGE (0x1 << 16) // (OTG)
#define AT91C_PORT_ENABLE_CHANGE  (0x1 << 17) // (OTG)
#define AT91C_PORT_SUSPEND_CHANGE (0x1 << 18) // (OTG)
#define AT91C_PORT_OVERCURR_CHANGE (0x1 << 19) // (OTG)
#define AT91C_PORT_RESET_CHANGE   (0x1 << 20) // (OTG)
// -------- OTG_HOST_TOKEN : (OTG Offset: 0x5090)  --------
#define AT91C_TOKEN_ADDRESS       (0x7F <<  0) // (OTG)
#define AT91C_TOKEN_EP_NUM        (0xF <<  7) // (OTG)
#define AT91C_TOKEN_DATA_TOGGLE   (0x3 << 11) // (OTG)
#define AT91C_TOKEN_TYPE          (0xF << 13) // (OTG)
#define AT91C_TOKEN_ISO_TRANSFERT (0x1 << 17) // (OTG)
#define AT91C_TOKEN_TRANSFERT_SPEED (0x3 << 18) // (OTG)
#define AT91C_TOKEN_TRANSFERT_SIZE (0x3FF << 20) // (OTG)
// -------- OTG_CTRL_STATUS : (OTG Offset: 0x5094)  --------
#define AT91C_SESSION_REQ_SUCCESS (0x1 <<  0) // (OTG)
#define AT91C_SESSION_REQ_STATUS_CHANGE (0x1 <<  1) // (OTG)
#define AT91C_HOST_NEG_SUCCESS    (0x1 <<  2) // (OTG)
#define AT91C_HOST_NEG_STATUS_CHANGE (0x1 <<  3) // (OTG)
#define AT91C_SESSION_REQ_DETECTED (0x1 <<  4) // (OTG)
#define AT91C_SESSION_REQ_DETECT_STATUS_CHANGE (0x1 <<  5) // (OTG)
#define AT91C_HOST_NEG_DETECTED   (0x1 <<  6) // (OTG)
#define AT91C_HOST_NEG_DETECT_STATUS_CHANGE (0x1 <<  7) // (OTG)
#define AT91C_CONNECTOR_ID        (0x1 <<  8) // (OTG)
#define AT91C_CONNECTOR_ID_CHANGE (0x1 <<  9) // (OTG)
#define AT91C_CURRENT_HOST_MODE   (0x1 << 10) // (OTG)
#define AT91C_SESSION_REQ         (0x1 << 16) // (OTG)
#define AT91C_HNP_REQ             (0x1 << 17) // (OTG)
#define AT91C_HOST_SET_HNP_ENABLE (0x1 << 18) // (OTG)
#define AT91C_HNP_ENABLE          (0x1 << 19) // (OTG)
#define AT91C_SRP_CAPABLE         (0x1 << 20) // (OTG)
#define AT91C_HNP_CAPABLE         (0x1 << 21) // (OTG)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB Device Interface
// *****************************************************************************
// *** Register offset in AT91S_UDP structure ***
#define UDP_NUM         ( 0) // Frame Number Register
#define UDP_GLBSTATE    ( 4) // Global State Register
#define UDP_FADDR       ( 8) // Function Address Register
#define UDP_IER         (16) // Interrupt Enable Register
#define UDP_IDR         (20) // Interrupt Disable Register
#define UDP_IMR         (24) // Interrupt Mask Register
#define UDP_ISR         (28) // Interrupt Status Register
#define UDP_ICR         (32) // Interrupt Clear Register
#define UDP_RSTEP       (40) // Reset Endpoint Register
#define UDP_CSR         (48) // Endpoint Control and Status Register
#define UDP_FDR         (80) // Endpoint FIFO Data Register
#define UDP_TXVC        (116) // Transceiver Control Register
// -------- UDP_FRM_NUM : (UDP Offset: 0x0) USB Frame Number Register --------
#define AT91C_UDP_FRM_NUM         (0x7FF <<  0) // (UDP) Frame Number as Defined in the Packet Field Formats
#define AT91C_UDP_FRM_ERR         (0x1 << 16) // (UDP) Frame Error
#define AT91C_UDP_FRM_OK          (0x1 << 17) // (UDP) Frame OK
// -------- UDP_GLB_STATE : (UDP Offset: 0x4) USB Global State Register --------
#define AT91C_UDP_FADDEN          (0x1 <<  0) // (UDP) Function Address Enable
#define AT91C_UDP_CONFG           (0x1 <<  1) // (UDP) Configured
#define AT91C_UDP_ESR             (0x1 <<  2) // (UDP) Enable Send Resume
#define AT91C_UDP_RSMINPR         (0x1 <<  3) // (UDP) A Resume Has Been Sent to the Host
#define AT91C_UDP_RMWUPE          (0x1 <<  4) // (UDP) Remote Wake Up Enable
// -------- UDP_FADDR : (UDP Offset: 0x8) USB Function Address Register --------
#define AT91C_UDP_FADD            (0xFF <<  0) // (UDP) Function Address Value
#define AT91C_UDP_FEN             (0x1 <<  8) // (UDP) Function Enable
// -------- UDP_IER : (UDP Offset: 0x10) USB Interrupt Enable Register --------
#define AT91C_UDP_EPINT0          (0x1 <<  0) // (UDP) Endpoint 0 Interrupt
#define AT91C_UDP_EPINT1          (0x1 <<  1) // (UDP) Endpoint 0 Interrupt
#define AT91C_UDP_EPINT2          (0x1 <<  2) // (UDP) Endpoint 2 Interrupt
#define AT91C_UDP_EPINT3          (0x1 <<  3) // (UDP) Endpoint 3 Interrupt
#define AT91C_UDP_EPINT4          (0x1 <<  4) // (UDP) Endpoint 4 Interrupt
#define AT91C_UDP_EPINT5          (0x1 <<  5) // (UDP) Endpoint 5 Interrupt
#define AT91C_UDP_RXSUSP          (0x1 <<  8) // (UDP) USB Suspend Interrupt
#define AT91C_UDP_RXRSM           (0x1 <<  9) // (UDP) USB Resume Interrupt
#define AT91C_UDP_EXTRSM          (0x1 << 10) // (UDP) USB External Resume Interrupt
#define AT91C_UDP_SOFINT          (0x1 << 11) // (UDP) USB Start Of frame Interrupt
#define AT91C_UDP_WAKEUP          (0x1 << 13) // (UDP) USB Resume Interrupt
// -------- UDP_IDR : (UDP Offset: 0x14) USB Interrupt Disable Register --------
// -------- UDP_IMR : (UDP Offset: 0x18) USB Interrupt Mask Register --------
// -------- UDP_ISR : (UDP Offset: 0x1c) USB Interrupt Status Register --------
#define AT91C_UDP_ENDBUSRES       (0x1 << 12) // (UDP) USB End Of Bus Reset Interrupt
// -------- UDP_ICR : (UDP Offset: 0x20) USB Interrupt Clear Register --------
// -------- UDP_RST_EP : (UDP Offset: 0x28) USB Reset Endpoint Register --------
#define AT91C_UDP_EP0             (0x1 <<  0) // (UDP) Reset Endpoint 0
#define AT91C_UDP_EP1             (0x1 <<  1) // (UDP) Reset Endpoint 1
#define AT91C_UDP_EP2             (0x1 <<  2) // (UDP) Reset Endpoint 2
#define AT91C_UDP_EP3             (0x1 <<  3) // (UDP) Reset Endpoint 3
#define AT91C_UDP_EP4             (0x1 <<  4) // (UDP) Reset Endpoint 4
#define AT91C_UDP_EP5             (0x1 <<  5) // (UDP) Reset Endpoint 5
// -------- UDP_CSR : (UDP Offset: 0x30) USB Endpoint Control and Status Register --------
#define AT91C_UDP_TXCOMP          (0x1 <<  0) // (UDP) Generates an IN packet with data previously written in the DPR
#define AT91C_UDP_RX_DATA_BK0     (0x1 <<  1) // (UDP) Receive Data Bank 0
#define AT91C_UDP_RXSETUP         (0x1 <<  2) // (UDP) Sends STALL to the Host (Control endpoints)
#define AT91C_UDP_ISOERROR        (0x1 <<  3) // (UDP) Isochronous error (Isochronous endpoints)
#define AT91C_UDP_TXPKTRDY        (0x1 <<  4) // (UDP) Transmit Packet Ready
#define AT91C_UDP_FORCESTALL      (0x1 <<  5) // (UDP) Force Stall (used by Control, Bulk and Isochronous endpoints).
#define AT91C_UDP_RX_DATA_BK1     (0x1 <<  6) // (UDP) Receive Data Bank 1 (only used by endpoints with ping-pong attributes).
#define AT91C_UDP_DIR             (0x1 <<  7) // (UDP) Transfer Direction
#define AT91C_UDP_EPTYPE          (0x7 <<  8) // (UDP) Endpoint type
#define 	AT91C_UDP_EPTYPE_CTRL                 (0x0 <<  8) // (UDP) Control
#define 	AT91C_UDP_EPTYPE_ISO_OUT              (0x1 <<  8) // (UDP) Isochronous OUT
#define 	AT91C_UDP_EPTYPE_BULK_OUT             (0x2 <<  8) // (UDP) Bulk OUT
#define 	AT91C_UDP_EPTYPE_INT_OUT              (0x3 <<  8) // (UDP) Interrupt OUT
#define 	AT91C_UDP_EPTYPE_ISO_IN               (0x5 <<  8) // (UDP) Isochronous IN
#define 	AT91C_UDP_EPTYPE_BULK_IN              (0x6 <<  8) // (UDP) Bulk IN
#define 	AT91C_UDP_EPTYPE_INT_IN               (0x7 <<  8) // (UDP) Interrupt IN
#define AT91C_UDP_DTGLE           (0x1 << 11) // (UDP) Data Toggle
#define AT91C_UDP_EPEDS           (0x1 << 15) // (UDP) Endpoint Enable Disable
#define AT91C_UDP_RXBYTECNT       (0x7FF << 16) // (UDP) Number Of Bytes Available in the FIFO
// -------- UDP_TXVC : (UDP Offset: 0x74) Transceiver Control Register --------
#define AT91C_UDP_TXVDIS          (0x1 <<  8) // (UDP)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB Host Interface
// *****************************************************************************
// *** Register offset in AT91S_UHP structure ***
#define UHP_HcRevision  ( 0) // Revision
#define UHP_HcControl   ( 4) // Operating modes for the Host Controller
#define UHP_HcCommandStatus ( 8) // Command & status Register
#define UHP_HcInterruptStatus (12) // Interrupt Status Register
#define UHP_HcInterruptEnable (16) // Interrupt Enable Register
#define UHP_HcInterruptDisable (20) // Interrupt Disable Register
#define UHP_HcHCCA      (24) // Pointer to the Host Controller Communication Area
#define UHP_HcPeriodCurrentED (28) // Current Isochronous or Interrupt Endpoint Descriptor
#define UHP_HcControlHeadED (32) // First Endpoint Descriptor of the Control list
#define UHP_HcControlCurrentED (36) // Endpoint Control and Status Register
#define UHP_HcBulkHeadED (40) // First endpoint register of the Bulk list
#define UHP_HcBulkCurrentED (44) // Current endpoint of the Bulk list
#define UHP_HcBulkDoneHead (48) // Last completed transfer descriptor
#define UHP_HcFmInterval (52) // Bit time between 2 consecutive SOFs
#define UHP_HcFmRemaining (56) // Bit time remaining in the current Frame
#define UHP_HcFmNumber  (60) // Frame number
#define UHP_HcPeriodicStart (64) // Periodic Start
#define UHP_HcLSThreshold (68) // LS Threshold
#define UHP_HcRhDescriptorA (72) // Root Hub characteristics A
#define UHP_HcRhDescriptorB (76) // Root Hub characteristics B
#define UHP_HcRhStatus  (80) // Root Hub Status register
#define UHP_HcRhPortStatus (84) // Root Hub Port Status Register

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR GPS engine from Thales
// *****************************************************************************
// *** Register offset in AT91S_GPS structure ***
#define GPS_CORRSWAP32  (540) //
#define GPS_CORRSWAP16  (544) //
#define GPS_CORRSWAPWRITE (548) //
#define GPS_PERIPHCNTRREG (1024) //
#define GPS_CONFIGREG   (1028) //
#define GPS_STATUSREG   (1032) //
#define GPS_ACQTIMEREG  (1036) //
#define GPS_THRESHACQREG (1040) //
#define GPS_SYNCMLREG   (1044) //
#define GPS_SYNCMHREG   (1048) //
#define GPS_CARRNCOREG  (1052) //
#define GPS_CODENCOREG  (1056) //
#define GPS_PROCTIMEREG (1060) //
#define GPS_PROCNCREG   (1064) //
#define GPS_SATREG      (1068) //
#define GPS_DOPSTARTREG (1072) //
#define GPS_DOPENDREG   (1076) //
#define GPS_DOPSTEPREG  (1080) //
#define GPS_SEARCHWINREG (1084) //
#define GPS_INITFIRSTNBREG (1088) //
#define GPS_NAVBITLREG  (1092) //
#define GPS_NAVBITHREG  (1096) //
#define GPS_PROCFIFOTHRESH (1100) //
#define GPS_RESAMPREG   (1104) //
#define GPS_RESPOSREG   (1108) //
#define GPS_RESDOPREG   (1112) //
#define GPS_RESNOISEREG (1116) //
#define GPS_ACQTESTREG  (1120) //
#define GPS_PROCTESTREG (1124) //
#define GPS_VERSIONREG  (1132) // GPS Engine revision register
// -------- GPS_CORRSWAP32 : (GPS Offset: 0x21c)  --------
#define AT91C_GPS_32TOBEDEFINED   (0x0 <<  0) // (GPS) To be defined....
// -------- GPS_CORRSWAP16 : (GPS Offset: 0x220)  --------
// -------- GPS_CORRSWAPWRITE : (GPS Offset: 0x224)  --------
// -------- GPS_PERIPHCNTRREG : (GPS Offset: 0x400)  --------
#define AT91C_GPS_AFMTSEL         (0x0 <<  0) // (GPS)
#define AT91C_GPS_BFMTSEL         (0x0 <<  1) // (GPS)
#define AT91C_GPS_CBUFFEN         (0x0 <<  2) // (GPS)
#define AT91C_GPS_MSTRSYNCSEL     (0x0 <<  3) // (GPS)
#define AT91C_GPS_SYNCSEL         (0x0 <<  4) // (GPS)
#define AT91C_GPS_EDGESEL         (0x0 <<  5) // (GPS)
#define AT91C_GPS_EPSSRCSEL       (0x0 <<  6) // (GPS)
#define AT91C_GPS_CORRSRCSEL      (0x0 <<  7) // (GPS)
#define AT91C_GPS_OUTPUTSRCSEL    (0x0 <<  9) // (GPS)
#define AT91C_GPS_EPSCLKEN        (0x0 << 11) // (GPS)
#define AT91C_GPS_CORRCLKEN       (0x0 << 12) // (GPS)
// -------- GPS_CONFIGREG : (GPS Offset: 0x404)  --------
#define AT91C_GPS_MODESELECT      (0x7 <<  0) // (GPS) Epsilon mode selection
#define AT91C_GPS_ENSYNC          (0x1 <<  3) // (GPS) Enable for input synchro pulse
#define AT91C_GPS_SELABC          (0x3 <<  4) // (GPS) Select a,b,c inputs
#define AT91C_GPS_ACQQUANT        (0x3 <<  6) // (GPS)
#define AT91C_GPS_REALCOMPLEX     (0x1 <<  8) // (GPS) Real to complex control
#define AT91C_GPS_STARTACQMODE    (0x1 <<  9) // (GPS)
#define AT91C_GPS_GPSGLO          (0x1 << 10) // (GPS)
#define AT91C_GPS_TSTOUTSELECT    (0x1F << 11) // (GPS)
#define AT91C_GPS_TSTIQINPUT      (0x1 << 16) // (GPS)
// -------- GPS_STATUSREG : (GPS Offset: 0x408)  --------
#define AT91C_GPS_ACQSTATUS       (0x1 <<  0) // (GPS) ?
#define AT91C_GPS_PROCSTATUS      (0x1 <<  8) // (GPS) ?
// -------- GPS_ACQTIMEREG : (GPS Offset: 0x40c)  --------
// -------- GPS_THRESHACQREG : (GPS Offset: 0x410)  --------
// -------- GPS_SYNCMLREG : (GPS Offset: 0x414)  --------
// -------- GPS_SYNCMHREG : (GPS Offset: 0x418)  --------
// -------- GPS_CARRNCOREG : (GPS Offset: 0x41c)  --------
// -------- GPS_CODENCOREG : (GPS Offset: 0x420)  --------
// -------- GPS_PROCNCREG : (GPS Offset: 0x428)  --------
// -------- GPS_SATREG : (GPS Offset: 0x42c)  --------
// -------- GPS_DOPSTARTREG : (GPS Offset: 0x430)  --------
// -------- GPS_DOPENDREG : (GPS Offset: 0x434)  --------
// -------- GPS_DOPSTEPREG : (GPS Offset: 0x438)  --------
// -------- GPS_SEARCHWINREG : (GPS Offset: 0x43c)  --------
// -------- GPS_INITFIRSTNBREG : (GPS Offset: 0x440)  --------
// -------- GPS_NAVBITLREG : (GPS Offset: 0x444)  --------
// -------- GPS_NAVBITHREG : (GPS Offset: 0x448)  --------
// -------- GPS_PROCFIFOTHRESH : (GPS Offset: 0x44c)  --------
// -------- GPS_RESAMPREG : (GPS Offset: 0x450)  --------
// -------- GPS_RESPOSREG : (GPS Offset: 0x454)  --------
// -------- GPS_RESDOPREG : (GPS Offset: 0x458)  --------
// -------- GPS_RESNOISEREG : (GPS Offset: 0x45c)  --------
// -------- GPS_ACQTESTREG : (GPS Offset: 0x460)  --------
// -------- GPS_PROCTESTREG : (GPS Offset: 0x464)  --------
// -------- GPS_VERSIONREG : (GPS Offset: 0x46c)  --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Trickbox (external) / SIMULATION ONLY
// *****************************************************************************
// *** Register offset in AT91S_TBOX structure ***
#define TBOX_SHMCTRL    ( 0) // SHM Probe Control: 0-> shm probe stopped, 1: shm probe started
#define TBOX_DMAEXTREQ  (2064) // DMA External request lines 3 to 0
#define TBOX_PIOAPUN    (2304) // Spy on PIO PUN inputs
#define TBOX_PIOBPUN    (2308) // Spy on PIO PUN inputs
#define TBOX_PIOCPUN    (2312) // Spy on PIO PUN inputs
#define TBOX_PIODPUN    (2316) // Spy on PIO PUN inputs
#define TBOX_PIOEPUN    (2320) // Spy on PIO PUN inputs
#define TBOX_PIOAENABLEFORCE (2324) // If each bit is 1, the corresponding bit of PIOA is controlled by TBOX_PIOAFORCEVALUE
#define TBOX_PIOAFORCEVALUE (2328) // Value to force on PIOA when bits TBOX_PIOAENABLEFORCE are 1
#define TBOX_PIOBENABLEFORCE (2332) // If each bit is 1, the corresponding bit of PIOB is controlled by TBOX_PIOBFORCEVALUE
#define TBOX_PIOBFORCEVALUE (2336) // Value to force on PIOA when bits TBOX_PIOBENABLEFORCE are 1
#define TBOX_PIOCENABLEFORCE (2340) // If each bit is 1, the corresponding bit of PIOC is controlled by TBOX_PIOCFORCEVALUE
#define TBOX_PIOCFORCEVALUE (2344) // Value to force on PIOA when bits TBOX_PIOCENABLEFORCE are 1
#define TBOX_PIODENABLEFORCE (2348) // If each bit is 1, the corresponding bit of PIOD is controlled by TBOX_PIODFORCEVALUE
#define TBOX_PIODFORCEVALUE (2352) // Value to force on PIOA when bits TBOX_PIODENABLEFORCE are 1
#define TBOX_PIOEENABLEFORCE (2356) // If each bit is 1, the corresponding bit of PIOE is controlled by TBOX_PIOEFORCEVALUE
#define TBOX_PIOEFORCEVALUE (2360) // Value to force on PIOA when bits TBOX_PIOEENABLEFORCE are 1
#define TBOX_PIOA       (2364) // Value Of PIOA
#define TBOX_PIOB       (2368) // Value Of PIOB
#define TBOX_PIOC       (2372) // Value Of PIOC
#define TBOX_PIOD       (2376) // Value Of PIOD
#define TBOX_PIOE       (2380) // Value Of PIOE
#define TBOX_AC97START  (2560) // Start of AC97 test: swith PIO mux to connect PIOs to audio codec model.
#define TBOX_PWMSTART   (2564) // Start of PWM test: Start to count edges on PWM IOs
#define TBOX_PWM1       (2568) // PWM1[4:0]=nb pulses on pb7, PWM1[9:5]=nb pulses on pc28, PWM1[20:16]=nb pulses on pb8, PWM1[25:21]=nb pulses on pc3
#define TBOX_PWM2       (2572) // PWM2[3:0]=nb pulses on pb27, PWM2[7:4]=nb pulses on pc29, PWM2[19:16]=nb pulses on pb29, PWM2[23:20]=nb pulses on pe10
#define TBOX_MAC        (2576) // MAC testbench : bit 0 = rxtrig, bit 1 = clkofftester, bit 2 = err_sig_loops
#define TBOX_USBDEV     (2580) // USB device testbench : bit 0 = flag0, bit 1 = flag1
#define TBOX_KBD        (2584) // Keyboard testbench : bit 0 = keypressed; bits[7:6] = key column; bits[5:4] = key row;
#define TBOX_STOPAPBSPY (2588) // When 1, no more APB SPY messages
#define TBOX_GPSSYNCHRO (2816) // GPS synchronization (Stimulus)
#define TBOX_GPSRAND    (2820) // GPS random data for correlator (Stimulus - Internal Node)
#define TBOX_GPSACQSTATUS (2824) // GPS acquisition status (Probe - Internal Node)
#define TBOX_GPSACQDATA (2828) // GPS acquisition data (Probe - Internal Node)
#define TBOX_GPSSIGFILE (2976) // GPS RFIN/DRFIN driven from files/Samples_GPS.data
#define TBOX_GPSSIGIA   (2980) // GPS DRFIN[1:0] aka SIGI_A (Stimulus)
#define TBOX_GPSSIGQA   (2984) // GPS DRFIN[3:2] aka SIGQ_A (Stimulus)
#define TBOX_GPSSIGIB   (2992) // GPS DRFIN[5:4] aka SIGI_B (Stimulus)
#define TBOX_GPSSIGQB   (2996) // GPS DRFIN[7:6] aka SIGQ_B (Stimulus)
#define TBOX_GPSDUMPRES (3008) // GPS Dump results and errors
// -------- TBOX_DMAEXTREQ : (TBOX Offset: 0x810)  --------
#define AT91C_TBOX_DMAEXTREQ0     (0x1 <<  0) // (TBOX) DMA external request 0
#define AT91C_TBOX_DMAEXTREQ1     (0x1 <<  1) // (TBOX) DMA external request 1
#define AT91C_TBOX_DMAEXTREQ2     (0x1 <<  2) // (TBOX) DMA external request 2
#define AT91C_TBOX_DMAEXTREQ3     (0x1 <<  3) // (TBOX) DMA external request 3

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Advanced  Encryption Standard
// *****************************************************************************
// *** Register offset in AT91S_AES structure ***
#define AES_CR          ( 0) // Control Register
#define AES_MR          ( 4) // Mode Register
#define AES_IER         (16) // Interrupt Enable Register
#define AES_IDR         (20) // Interrupt Disable Register
#define AES_IMR         (24) // Interrupt Mask Register
#define AES_ISR         (28) // Interrupt Status Register
#define AES_KEYWxR      (32) // Key Word x Register
#define AES_IDATAxR     (64) // Input Data x Register
#define AES_ODATAxR     (80) // Output Data x Register
#define AES_IVxR        (96) // Initialization Vector x Register
#define AES_VR          (252) // AES Version Register
#define AES_RPR         (256) // Receive Pointer Register
#define AES_RCR         (260) // Receive Counter Register
#define AES_TPR         (264) // Transmit Pointer Register
#define AES_TCR         (268) // Transmit Counter Register
#define AES_RNPR        (272) // Receive Next Pointer Register
#define AES_RNCR        (276) // Receive Next Counter Register
#define AES_TNPR        (280) // Transmit Next Pointer Register
#define AES_TNCR        (284) // Transmit Next Counter Register
#define AES_PTCR        (288) // PDC Transfer Control Register
#define AES_PTSR        (292) // PDC Transfer Status Register
// -------- AES_CR : (AES Offset: 0x0) Control Register --------
#define AT91C_AES_START           (0x1 <<  0) // (AES) Starts Processing
#define AT91C_AES_SWRST           (0x1 <<  8) // (AES) Software Reset
#define AT91C_AES_LOADSEED        (0x1 << 16) // (AES) Random Number Generator Seed Loading
// -------- AES_MR : (AES Offset: 0x4) Mode Register --------
#define AT91C_AES_CIPHER          (0x1 <<  0) // (AES) Processing Mode
#define AT91C_AES_PROCDLY         (0xF <<  4) // (AES) Processing Delay
#define AT91C_AES_SMOD            (0x3 <<  8) // (AES) Start Mode
#define 	AT91C_AES_SMOD_MANUAL               (0x0 <<  8) // (AES) Manual Mode: The START bit in register AES_CR must be set to begin encryption or decryption.
#define 	AT91C_AES_SMOD_AUTO                 (0x1 <<  8) // (AES) Auto Mode: no action in AES_CR is necessary (cf datasheet).
#define 	AT91C_AES_SMOD_PDC                  (0x2 <<  8) // (AES) PDC Mode (cf datasheet).
#define AT91C_AES_OPMOD           (0x7 << 12) // (AES) Operation Mode
#define 	AT91C_AES_OPMOD_ECB                  (0x0 << 12) // (AES) ECB Electronic CodeBook mode.
#define 	AT91C_AES_OPMOD_CBC                  (0x1 << 12) // (AES) CBC Cipher Block Chaining mode.
#define 	AT91C_AES_OPMOD_OFB                  (0x2 << 12) // (AES) OFB Output Feedback mode.
#define 	AT91C_AES_OPMOD_CFB                  (0x3 << 12) // (AES) CFB Cipher Feedback mode.
#define 	AT91C_AES_OPMOD_CTR                  (0x4 << 12) // (AES) CTR Counter mode.
#define AT91C_AES_LOD             (0x1 << 15) // (AES) Last Output Data Mode
#define AT91C_AES_CFBS            (0x7 << 16) // (AES) Cipher Feedback Data Size
#define 	AT91C_AES_CFBS_128_BIT              (0x0 << 16) // (AES) 128-bit.
#define 	AT91C_AES_CFBS_64_BIT               (0x1 << 16) // (AES) 64-bit.
#define 	AT91C_AES_CFBS_32_BIT               (0x2 << 16) // (AES) 32-bit.
#define 	AT91C_AES_CFBS_16_BIT               (0x3 << 16) // (AES) 16-bit.
#define 	AT91C_AES_CFBS_8_BIT                (0x4 << 16) // (AES) 8-bit.
#define AT91C_AES_CKEY            (0xF << 20) // (AES) Countermeasure Key
#define AT91C_AES_CTYPE           (0x1F << 24) // (AES) Countermeasure Type
#define 	AT91C_AES_CTYPE_TYPE1_EN             (0x1 << 24) // (AES) Countermeasure type 1 is enabled.
#define 	AT91C_AES_CTYPE_TYPE2_EN             (0x2 << 24) // (AES) Countermeasure type 2 is enabled.
#define 	AT91C_AES_CTYPE_TYPE3_EN             (0x4 << 24) // (AES) Countermeasure type 3 is enabled.
#define 	AT91C_AES_CTYPE_TYPE4_EN             (0x8 << 24) // (AES) Countermeasure type 4 is enabled.
#define 	AT91C_AES_CTYPE_TYPE5_EN             (0x10 << 24) // (AES) Countermeasure type 5 is enabled.
// -------- AES_IER : (AES Offset: 0x10) Interrupt Enable Register --------
#define AT91C_AES_DATRDY          (0x1 <<  0) // (AES) DATRDY
#define AT91C_AES_ENDRX           (0x1 <<  1) // (AES) PDC Read Buffer End
#define AT91C_AES_ENDTX           (0x1 <<  2) // (AES) PDC Write Buffer End
#define AT91C_AES_RXBUFF          (0x1 <<  3) // (AES) PDC Read Buffer Full
#define AT91C_AES_TXBUFE          (0x1 <<  4) // (AES) PDC Write Buffer Empty
#define AT91C_AES_URAD            (0x1 <<  8) // (AES) Unspecified Register Access Detection
// -------- AES_IDR : (AES Offset: 0x14) Interrupt Disable Register --------
// -------- AES_IMR : (AES Offset: 0x18) Interrupt Mask Register --------
// -------- AES_ISR : (AES Offset: 0x1c) Interrupt Status Register --------
#define AT91C_AES_URAT            (0x7 << 12) // (AES) Unspecified Register Access Type Status
#define 	AT91C_AES_URAT_IN_DAT_WRITE_DATPROC (0x0 << 12) // (AES) Input data register written during the data processing in PDC mode.
#define 	AT91C_AES_URAT_OUT_DAT_READ_DATPROC (0x1 << 12) // (AES) Output data register read during the data processing.
#define 	AT91C_AES_URAT_MODEREG_WRITE_DATPROC (0x2 << 12) // (AES) Mode register written during the data processing.
#define 	AT91C_AES_URAT_OUT_DAT_READ_SUBKEY  (0x3 << 12) // (AES) Output data register read during the sub-keys generation.
#define 	AT91C_AES_URAT_MODEREG_WRITE_SUBKEY (0x4 << 12) // (AES) Mode register written during the sub-keys generation.
#define 	AT91C_AES_URAT_WO_REG_READ          (0x5 << 12) // (AES) Write-only register read access.

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Error Correction Code controller
// *****************************************************************************
// *** Register offset in AT91S_ECC structure ***
#define ECC_CR          ( 0) //  ECC reset register
#define ECC_MR          ( 4) //  ECC Page size register
#define ECC_SR          ( 8) //  ECC Status register
#define ECC_PR          (12) //  ECC Parity register
#define ECC_NPR         (16) //  ECC Parity N register
#define ECC_VR          (252) //  ECC Version register
// -------- ECC_CR : (ECC Offset: 0x0) ECC reset register --------
#define AT91C_ECC_RST             (0x1 <<  0) // (ECC) ECC reset parity
// -------- ECC_MR : (ECC Offset: 0x4) ECC page size register --------
#define AT91C_ECC_PAGE_SIZE       (0x3 <<  0) // (ECC) Nand Flash page size
// -------- ECC_SR : (ECC Offset: 0x8) ECC status register --------
#define AT91C_ECC_RECERR          (0x1 <<  0) // (ECC) ECC error
#define AT91C_ECC_ECCERR          (0x1 <<  1) // (ECC) ECC single error
#define AT91C_ECC_MULERR          (0x1 <<  2) // (ECC) ECC_MULERR
// -------- ECC_PR : (ECC Offset: 0xc) ECC parity register --------
#define AT91C_ECC_BITADDR         (0xF <<  0) // (ECC) Bit address error
#define AT91C_ECC_WORDADDR        (0xFFF <<  4) // (ECC) address of the failing bit
// -------- ECC_NPR : (ECC Offset: 0x10) ECC N parity register --------
#define AT91C_ECC_NPARITY         (0xFFFF <<  0) // (ECC) ECC parity N
// -------- ECC_VR : (ECC Offset: 0xfc) ECC version register --------
#define AT91C_ECC_VR              (0xF <<  0) // (ECC) ECC version register

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Image Sensor Interface
// *****************************************************************************
// *** Register offset in AT91S_ISI structure ***
#define ISI_CR1         ( 0) // Control Register 1
#define ISI_CR2         ( 4) // Control Register 2
#define ISI_SR          ( 8) // Status Register
#define ISI_IER         (12) // Interrupt Enable Register
#define ISI_IDR         (16) // Interrupt Disable Register
#define ISI_IMR         (20) // Interrupt Mask Register
#define ISI_PSIZE       (32) // Preview Size Register
#define ISI_PDECF       (36) // Preview Decimation Factor Register
#define ISI_PFBD        (40) // Preview Frame Buffer Address Register
#define ISI_CDBA        (44) // Codec Dma Address Register
#define ISI_Y2RSET0     (48) // Color Space Conversion Register
#define ISI_Y2RSET1     (52) // Color Space Conversion Register
#define ISI_R2YSET0     (56) // Color Space Conversion Register
#define ISI_R2YSET1     (60) // Color Space Conversion Register
#define ISI_R2YSET2     (64) // Color Space Conversion Register
// -------- ISI_CR1 : (ISI Offset: 0x0) ISI Control Register 1 --------
#define AT91C_ISI_RST             (0x1 <<  0) // (ISI) Image sensor interface reset
#define AT91C_ISI_DISABLE         (0x1 <<  1) // (ISI) image sensor disable.
#define AT91C_ISI_HSYNC_POL       (0x1 <<  2) // (ISI) Horizontal synchronisation polarity
#define AT91C_ISI_PIXCLK_POL      (0x1 <<  4) // (ISI) Pixel Clock Polarity
#define AT91C_ISI_EMB_SYNC        (0x1 <<  6) // (ISI) Embedded synchronisation
#define AT91C_ISI_CRC_SYNC        (0x1 <<  7) // (ISI) CRC correction
#define AT91C_ISI_FULL            (0x1 << 12) // (ISI) Full mode is allowed
#define AT91C_ISI_THMASK          (0x3 << 13) // (ISI) DMA Burst Mask
#define 	AT91C_ISI_THMASK_4_8_16_BURST         (0x0 << 13) // (ISI) 4,8 and 16 AHB burst are allowed
#define 	AT91C_ISI_THMASK_8_16_BURST           (0x1 << 13) // (ISI) 8 and 16 AHB burst are allowed
#define 	AT91C_ISI_THMASK_16_BURST             (0x2 << 13) // (ISI) Only 16 AHB burst are allowed
#define AT91C_ISI_CODEC_ON        (0x1 << 15) // (ISI) Enable the codec path
#define AT91C_ISI_SLD             (0xFF << 16) // (ISI) Start of Line Delay
#define AT91C_ISI_SFD             (0xFF << 24) // (ISI) Start of frame Delay
// -------- ISI_CR2 : (ISI Offset: 0x4) ISI Control Register 2 --------
#define AT91C_ISI_IM_VSIZE        (0x7FF <<  0) // (ISI) Vertical size of the Image sensor [0..2047]
#define AT91C_ISI_GS_MODE         (0x1 << 11) // (ISI) Grayscale Memory Mode
#define AT91C_ISI_RGB_MODE        (0x3 << 12) // (ISI) RGB mode
#define 	AT91C_ISI_RGB_MODE_RGB_888              (0x0 << 12) // (ISI) RGB 8:8:8 24 bits
#define 	AT91C_ISI_RGB_MODE_RGB_565              (0x1 << 12) // (ISI) RGB 5:6:5 16 bits
#define 	AT91C_ISI_RGB_MODE_RGB_555              (0x2 << 12) // (ISI) RGB 5:5:5 16 bits
#define AT91C_ISI_GRAYSCALE       (0x1 << 13) // (ISI) Grayscale Mode
#define AT91C_ISI_RGB_SWAP        (0x1 << 14) // (ISI) RGB Swap
#define AT91C_ISI_COL_SPACE       (0x1 << 15) // (ISI) Color space for the image data
#define AT91C_ISI_IM_HSIZE        (0x7FF << 16) // (ISI) Horizontal size of the Image sensor [0..2047]
#define 	AT91C_ISI_RGB_MODE_YCC_DEF              (0x0 << 28) // (ISI) Cb(i) Y(i) Cr(i) Y(i+1)
#define 	AT91C_ISI_RGB_MODE_YCC_MOD1             (0x1 << 28) // (ISI) Cr(i) Y(i) Cb(i) Y(i+1)
#define 	AT91C_ISI_RGB_MODE_YCC_MOD2             (0x2 << 28) // (ISI) Y(i) Cb(i) Y(i+1) Cr(i)
#define 	AT91C_ISI_RGB_MODE_YCC_MOD3             (0x3 << 28) // (ISI) Y(i) Cr(i) Y(i+1) Cb(i)
#define AT91C_ISI_RGB_CFG         (0x3 << 30) // (ISI) RGB configuration
#define 	AT91C_ISI_RGB_CFG_RGB_DEF              (0x0 << 30) // (ISI) R/G(MSB)  G(LSB)/B  R/G(MSB)  G(LSB)/B
#define 	AT91C_ISI_RGB_CFG_RGB_MOD1             (0x1 << 30) // (ISI) B/G(MSB)  G(LSB)/R  B/G(MSB)  G(LSB)/R
#define 	AT91C_ISI_RGB_CFG_RGB_MOD2             (0x2 << 30) // (ISI) G(LSB)/R  B/G(MSB)  G(LSB)/R  B/G(MSB)
#define 	AT91C_ISI_RGB_CFG_RGB_MOD3             (0x3 << 30) // (ISI) G(LSB)/B  R/G(MSB)  G(LSB)/B  R/G(MSB)
// -------- ISI_SR : (ISI Offset: 0x8) ISI Status Register --------
#define AT91C_ISI_SOF             (0x1 <<  0) // (ISI) Start of Frame
#define AT91C_ISI_DIS             (0x1 <<  1) // (ISI) Image Sensor Interface disable
#define AT91C_ISI_SOFTRST         (0x1 <<  2) // (ISI) Software Reset
#define AT91C_ISI_CRC_ERR         (0x1 <<  4) // (ISI) CRC synchronisation error
#define AT91C_ISI_FO_C_OVF        (0x1 <<  5) // (ISI) Fifo Codec Overflow
#define AT91C_ISI_FO_P_OVF        (0x1 <<  6) // (ISI) Fifo Preview Overflow
#define AT91C_ISI_FO_P_EMP        (0x1 <<  7) // (ISI) Fifo Preview Empty
#define AT91C_ISI_FO_C_EMP        (0x1 <<  8) // (ISI) Fifo Codec Empty
#define AT91C_ISI_FR_OVR          (0x1 <<  9) // (ISI) Frame rate overun
// -------- ISI_IER : (ISI Offset: 0xc) ISI Interrupt Enable Register --------
// -------- ISI_IDR : (ISI Offset: 0x10) ISI Interrupt Disable Register --------
// -------- ISI_IMR : (ISI Offset: 0x14) ISI Interrupt Mask Register --------
// -------- ISI_PSIZE : (ISI Offset: 0x20) ISI Preview Register --------
#define AT91C_ISI_PREV_VSIZE      (0x3FF <<  0) // (ISI) Vertical size for the preview path
#define AT91C_ISI_PREV_HSIZE      (0x3FF << 16) // (ISI) Horizontal size for the preview path
// -------- ISI_Y2R_SET0 : (ISI Offset: 0x30) Color Space Conversion YCrCb to RGB Register --------
#define AT91C_ISI_Y2R_C0          (0xFF <<  0) // (ISI) Color Space Conversion Matrix Coefficient C0
#define AT91C_ISI_Y2R_C1          (0xFF <<  8) // (ISI) Color Space Conversion Matrix Coefficient C1
#define AT91C_ISI_Y2R_C2          (0xFF << 16) // (ISI) Color Space Conversion Matrix Coefficient C2
#define AT91C_ISI_Y2R_C3          (0xFF << 24) // (ISI) Color Space Conversion Matrix Coefficient C3
// -------- ISI_Y2R_SET1 : (ISI Offset: 0x34) ISI Color Space Conversion YCrCb to RGB set 1 Register --------
#define AT91C_ISI_Y2R_C4          (0x1FF <<  0) // (ISI) Color Space Conversion Matrix Coefficient C4
#define AT91C_ISI_Y2R_YOFF        (0xFF << 12) // (ISI) Color Space Conversion Luninance default offset
#define AT91C_ISI_Y2R_CROFF       (0xFF << 13) // (ISI) Color Space Conversion Red Chrominance default offset
#define AT91C_ISI_Y2R_CBFF        (0xFF << 14) // (ISI) Color Space Conversion Luninance default offset
// -------- ISI_R2Y_SET0 : (ISI Offset: 0x38) Color Space Conversion RGB to YCrCb set 0 register --------
#define AT91C_ISI_R2Y_C0          (0x7F <<  0) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C0
#define AT91C_ISI_R2Y_C1          (0x7F <<  1) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C1
#define AT91C_ISI_R2Y_C2          (0x7F <<  3) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C2
#define AT91C_ISI_R2Y_ROFF        (0x1 <<  4) // (ISI) Color Space Conversion Red component offset
// -------- ISI_R2Y_SET1 : (ISI Offset: 0x3c) Color Space Conversion RGB to YCrCb set 1 register --------
#define AT91C_ISI_R2Y_C3          (0x7F <<  0) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C3
#define AT91C_ISI_R2Y_C4          (0x7F <<  1) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C4
#define AT91C_ISI_R2Y_C5          (0x7F <<  3) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C5
#define AT91C_ISI_R2Y_GOFF        (0x1 <<  4) // (ISI) Color Space Conversion Green component offset
// -------- ISI_R2Y_SET2 : (ISI Offset: 0x40) Color Space Conversion RGB to YCrCb set 2 register --------
#define AT91C_ISI_R2Y_C6          (0x7F <<  0) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C6
#define AT91C_ISI_R2Y_C7          (0x7F <<  1) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C7
#define AT91C_ISI_R2Y_C8          (0x7F <<  3) // (ISI) Color Space Conversion RGB to YCrCb Matrix coefficient C8
#define AT91C_ISI_R2Y_BOFF        (0x1 <<  4) // (ISI) Color Space Conversion Blue component offset

// *****************************************************************************
//               REGISTER ADDRESS DEFINITION FOR AT91SAM9262
// *****************************************************************************
// ========== Register definition for SYS peripheral ==========
#define AT91C_SYS_ECC0            (0xFFFFE000) // (SYS) ECC 0
#define AT91C_SYS_GPBR            (0xFFFFFD60) // (SYS) General Purpose Register
#define AT91C_SYS_ECC1            (0xFFFFE600) // (SYS) ECC 0
// ========== Register definition for EBI0 peripheral ==========
#define AT91C_EBI0_DUMMY          (0xFFFFE200) // (EBI0) Dummy register - Do not use
// ========== Register definition for SDRAMC0 peripheral ==========
#define AT91C_SDRAMC0_ISR         (0xFFFFE220) // (SDRAMC0) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC0_IDR         (0xFFFFE218) // (SDRAMC0) SDRAM Controller Interrupt Disable Register
#define AT91C_SDRAMC0_LPR         (0xFFFFE210) // (SDRAMC0) SDRAM Controller Low Power Register
#define AT91C_SDRAMC0_CR          (0xFFFFE208) // (SDRAMC0) SDRAM Controller Configuration Register
#define AT91C_SDRAMC0_MR          (0xFFFFE200) // (SDRAMC0) SDRAM Controller Mode Register
#define AT91C_SDRAMC0_MDR         (0xFFFFE224) // (SDRAMC0) SDRAM Memory Device Register
#define AT91C_SDRAMC0_IMR         (0xFFFFE21C) // (SDRAMC0) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC0_IER         (0xFFFFE214) // (SDRAMC0) SDRAM Controller Interrupt Enable Register
#define AT91C_SDRAMC0_HSR         (0xFFFFE20C) // (SDRAMC0) SDRAM Controller High Speed Register
#define AT91C_SDRAMC0_TR          (0xFFFFE204) // (SDRAMC0) SDRAM Controller Refresh Timer Register
// ========== Register definition for SMC0 peripheral ==========
#define AT91C_SMC0_SETUP7         (0xFFFFE470) // (SMC0)  Setup Register for CS 7
#define AT91C_SMC0_CYCLE6         (0xFFFFE468) // (SMC0)  Cycle Register for CS 6
#define AT91C_SMC0_SETUP6         (0xFFFFE460) // (SMC0)  Setup Register for CS 6
#define AT91C_SMC0_CYCLE5         (0xFFFFE458) // (SMC0)  Cycle Register for CS 5
#define AT91C_SMC0_SETUP5         (0xFFFFE450) // (SMC0)  Setup Register for CS 5
#define AT91C_SMC0_SETUP2         (0xFFFFE420) // (SMC0)  Setup Register for CS 2
#define AT91C_SMC0_CYCLE1         (0xFFFFE418) // (SMC0)  Cycle Register for CS 1
#define AT91C_SMC0_SETUP1         (0xFFFFE410) // (SMC0)  Setup Register for CS 1
#define AT91C_SMC0_CYCLE0         (0xFFFFE408) // (SMC0)  Cycle Register for CS 0
#define AT91C_SMC0_SETUP0         (0xFFFFE400) // (SMC0)  Setup Register for CS 0
#define AT91C_SMC0_CTRL7          (0xFFFFE47C) // (SMC0)  Control Register for CS 7
#define AT91C_SMC0_PULSE7         (0xFFFFE474) // (SMC0)  Pulse Register for CS 7
#define AT91C_SMC0_CTRL6          (0xFFFFE46C) // (SMC0)  Control Register for CS 6
#define AT91C_SMC0_PULSE6         (0xFFFFE464) // (SMC0)  Pulse Register for CS 6
#define AT91C_SMC0_PULSE3         (0xFFFFE434) // (SMC0)  Pulse Register for CS 3
#define AT91C_SMC0_CTRL2          (0xFFFFE42C) // (SMC0)  Control Register for CS 2
#define AT91C_SMC0_PULSE2         (0xFFFFE424) // (SMC0)  Pulse Register for CS 2
#define AT91C_SMC0_CTRL1          (0xFFFFE41C) // (SMC0)  Control Register for CS 1
#define AT91C_SMC0_PULSE1         (0xFFFFE414) // (SMC0)  Pulse Register for CS 1
#define AT91C_SMC0_CYCLE7         (0xFFFFE478) // (SMC0)  Cycle Register for CS 7
#define AT91C_SMC0_CYCLE2         (0xFFFFE428) // (SMC0)  Cycle Register for CS 2
#define AT91C_SMC0_SETUP3         (0xFFFFE430) // (SMC0)  Setup Register for CS 3
#define AT91C_SMC0_CYCLE3         (0xFFFFE438) // (SMC0)  Cycle Register for CS 3
#define AT91C_SMC0_SETUP4         (0xFFFFE440) // (SMC0)  Setup Register for CS 4
#define AT91C_SMC0_CTRL3          (0xFFFFE43C) // (SMC0)  Control Register for CS 3
#define AT91C_SMC0_PULSE4         (0xFFFFE444) // (SMC0)  Pulse Register for CS 4
#define AT91C_SMC0_CYCLE4         (0xFFFFE448) // (SMC0)  Cycle Register for CS 4
#define AT91C_SMC0_CTRL5          (0xFFFFE45C) // (SMC0)  Control Register for CS 5
#define AT91C_SMC0_PULSE5         (0xFFFFE454) // (SMC0)  Pulse Register for CS 5
#define AT91C_SMC0_CTRL4          (0xFFFFE44C) // (SMC0)  Control Register for CS 4
#define AT91C_SMC0_CTRL0          (0xFFFFE40C) // (SMC0)  Control Register for CS 0
#define AT91C_SMC0_PULSE0         (0xFFFFE404) // (SMC0)  Pulse Register for CS 0
// ========== Register definition for EBI1 peripheral ==========
#define AT91C_EBI1_DUMMY          (0xFFFFE800) // (EBI1) Dummy register - Do not use
// ========== Register definition for SDRAMC1 peripheral ==========
#define AT91C_SDRAMC1_MDR         (0xFFFFE824) // (SDRAMC1) SDRAM Memory Device Register
#define AT91C_SDRAMC1_IMR         (0xFFFFE81C) // (SDRAMC1) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC1_IER         (0xFFFFE814) // (SDRAMC1) SDRAM Controller Interrupt Enable Register
#define AT91C_SDRAMC1_HSR         (0xFFFFE80C) // (SDRAMC1) SDRAM Controller High Speed Register
#define AT91C_SDRAMC1_TR          (0xFFFFE804) // (SDRAMC1) SDRAM Controller Refresh Timer Register
#define AT91C_SDRAMC1_ISR         (0xFFFFE820) // (SDRAMC1) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC1_IDR         (0xFFFFE818) // (SDRAMC1) SDRAM Controller Interrupt Disable Register
#define AT91C_SDRAMC1_LPR         (0xFFFFE810) // (SDRAMC1) SDRAM Controller Low Power Register
#define AT91C_SDRAMC1_CR          (0xFFFFE808) // (SDRAMC1) SDRAM Controller Configuration Register
#define AT91C_SDRAMC1_MR          (0xFFFFE800) // (SDRAMC1) SDRAM Controller Mode Register
// ========== Register definition for SMC1 peripheral ==========
#define AT91C_SMC1_SETUP2         (0xFFFFEA20) // (SMC1)  Setup Register for CS 2
#define AT91C_SMC1_CTRL7          (0xFFFFEA7C) // (SMC1)  Control Register for CS 7
#define AT91C_SMC1_SETUP3         (0xFFFFEA30) // (SMC1)  Setup Register for CS 3
#define AT91C_SMC1_PULSE4         (0xFFFFEA44) // (SMC1)  Pulse Register for CS 4
#define AT91C_SMC1_PULSE3         (0xFFFFEA34) // (SMC1)  Pulse Register for CS 3
#define AT91C_SMC1_CYCLE5         (0xFFFFEA58) // (SMC1)  Cycle Register for CS 5
#define AT91C_SMC1_CYCLE4         (0xFFFFEA48) // (SMC1)  Cycle Register for CS 4
#define AT91C_SMC1_SETUP1         (0xFFFFEA10) // (SMC1)  Setup Register for CS 1
#define AT91C_SMC1_SETUP0         (0xFFFFEA00) // (SMC1)  Setup Register for CS 0
#define AT91C_SMC1_CTRL6          (0xFFFFEA6C) // (SMC1)  Control Register for CS 6
#define AT91C_SMC1_CTRL5          (0xFFFFEA5C) // (SMC1)  Control Register for CS 5
#define AT91C_SMC1_PULSE1         (0xFFFFEA14) // (SMC1)  Pulse Register for CS 1
#define AT91C_SMC1_PULSE0         (0xFFFFEA04) // (SMC1)  Pulse Register for CS 0
#define AT91C_SMC1_SETUP7         (0xFFFFEA70) // (SMC1)  Setup Register for CS 7
#define AT91C_SMC1_CYCLE1         (0xFFFFEA18) // (SMC1)  Cycle Register for CS 1
#define AT91C_SMC1_PULSE2         (0xFFFFEA24) // (SMC1)  Pulse Register for CS 2
#define AT91C_SMC1_CYCLE2         (0xFFFFEA28) // (SMC1)  Cycle Register for CS 2
#define AT91C_SMC1_CYCLE3         (0xFFFFEA38) // (SMC1)  Cycle Register for CS 3
#define AT91C_SMC1_CTRL3          (0xFFFFEA3C) // (SMC1)  Control Register for CS 3
#define AT91C_SMC1_CTRL2          (0xFFFFEA2C) // (SMC1)  Control Register for CS 2
#define AT91C_SMC1_CTRL4          (0xFFFFEA4C) // (SMC1)  Control Register for CS 4
#define AT91C_SMC1_SETUP4         (0xFFFFEA40) // (SMC1)  Setup Register for CS 4
#define AT91C_SMC1_SETUP5         (0xFFFFEA50) // (SMC1)  Setup Register for CS 5
#define AT91C_SMC1_SETUP6         (0xFFFFEA60) // (SMC1)  Setup Register for CS 6
#define AT91C_SMC1_CYCLE0         (0xFFFFEA08) // (SMC1)  Cycle Register for CS 0
#define AT91C_SMC1_PULSE5         (0xFFFFEA54) // (SMC1)  Pulse Register for CS 5
#define AT91C_SMC1_PULSE6         (0xFFFFEA64) // (SMC1)  Pulse Register for CS 6
#define AT91C_SMC1_PULSE7         (0xFFFFEA74) // (SMC1)  Pulse Register for CS 7
#define AT91C_SMC1_CTRL0          (0xFFFFEA0C) // (SMC1)  Control Register for CS 0
#define AT91C_SMC1_CTRL1          (0xFFFFEA1C) // (SMC1)  Control Register for CS 1
#define AT91C_SMC1_CYCLE6         (0xFFFFEA68) // (SMC1)  Cycle Register for CS 6
#define AT91C_SMC1_CYCLE7         (0xFFFFEA78) // (SMC1)  Cycle Register for CS 7
// ========== Register definition for MATRIX peripheral ==========
#define AT91C_MATRIX_MCFG6        (0xFFFFEC18) // (MATRIX)  Master Configuration Register 6
#define AT91C_MATRIX_MCFG7        (0xFFFFEC1C) // (MATRIX)  Master Configuration Register 7
#define AT91C_MATRIX_PRAS1        (0xFFFFEC88) // (MATRIX)  PRAS1
#define AT91C_MATRIX_SCFG0        (0xFFFFEC40) // (MATRIX)  Slave Configuration Register 0
#define AT91C_MATRIX_MCFG8        (0xFFFFEC20) // (MATRIX)  Master Configuration Register 8
#define AT91C_MATRIX_PRBS3        (0xFFFFEC9C) // (MATRIX)  PRBS3
#define AT91C_MATRIX_PRBS1        (0xFFFFEC8C) // (MATRIX)  PRBS1
#define AT91C_MATRIX_SCFG5        (0xFFFFEC54) // (MATRIX)  Slave Configuration Register 5
#define AT91C_MATRIX_SCFG1        (0xFFFFEC44) // (MATRIX)  Slave Configuration Register 1
#define AT91C_MATRIX_PRAS6        (0xFFFFECB0) // (MATRIX)  PRAS6
#define AT91C_MATRIX_PRAS4        (0xFFFFECA0) // (MATRIX)  PRAS4
#define AT91C_MATRIX_SCFG2        (0xFFFFEC48) // (MATRIX)  Slave Configuration Register 2
#define AT91C_MATRIX_SCFG6        (0xFFFFEC58) // (MATRIX)  Slave Configuration Register 6
#define AT91C_MATRIX_MCFG0        (0xFFFFEC00) // (MATRIX)  Master Configuration Register 0
#define AT91C_MATRIX_MCFG4        (0xFFFFEC10) // (MATRIX)  Master Configuration Register 4
#define AT91C_MATRIX_PRBS6        (0xFFFFECB4) // (MATRIX)  PRBS6
#define AT91C_MATRIX_SCFG7        (0xFFFFEC5C) // (MATRIX)  Slave Configuration Register 7
#define AT91C_MATRIX_MCFG5        (0xFFFFEC14) // (MATRIX)  Master Configuration Register 5
#define AT91C_MATRIX_PRAS0        (0xFFFFEC80) // (MATRIX)  PRAS0
#define AT91C_MATRIX_PRAS2        (0xFFFFEC90) // (MATRIX)  PRAS2
#define AT91C_MATRIX_PRBS0        (0xFFFFEC84) // (MATRIX)  PRBS0
#define AT91C_MATRIX_PRBS2        (0xFFFFEC94) // (MATRIX)  PRBS2
#define AT91C_MATRIX_PRBS4        (0xFFFFECA4) // (MATRIX)  PRBS4
#define AT91C_MATRIX_SCFG3        (0xFFFFEC4C) // (MATRIX)  Slave Configuration Register 3
#define AT91C_MATRIX_MCFG1        (0xFFFFEC04) // (MATRIX)  Master Configuration Register 1
#define AT91C_MATRIX_MRCR         (0xFFFFED00) // (MATRIX)  Master Remp Control Register
#define AT91C_MATRIX_PRAS3        (0xFFFFEC98) // (MATRIX)  PRAS3
#define AT91C_MATRIX_PRAS5        (0xFFFFECA8) // (MATRIX)  PRAS5
#define AT91C_MATRIX_PRAS7        (0xFFFFECB8) // (MATRIX)  PRAS7
#define AT91C_MATRIX_SCFG4        (0xFFFFEC50) // (MATRIX)  Slave Configuration Register 4
#define AT91C_MATRIX_MCFG2        (0xFFFFEC08) // (MATRIX)  Master Configuration Register 2
#define AT91C_MATRIX_PRBS5        (0xFFFFECAC) // (MATRIX)  PRBS5
#define AT91C_MATRIX_PRBS7        (0xFFFFECBC) // (MATRIX)  PRBS7
#define AT91C_MATRIX_MCFG3        (0xFFFFEC0C) // (MATRIX)  Master Configuration Register 3
// ========== Register definition for CCFG peripheral ==========
#define AT91C_CCFG_MATRIXVERSION  (0xFFFFEDFC) // (CCFG)  Version Register
#define AT91C_CCFG_EBI1CSA        (0xFFFFED24) // (CCFG)  EBI1 Chip Select Assignement Register
#define AT91C_CCFG_TCMR           (0xFFFFED14) // (CCFG)  TCM configuration
#define AT91C_CCFG_EBI0CSA        (0xFFFFED20) // (CCFG)  EBI0 Chip Select Assignement Register
// ========== Register definition for PDC_DBGU peripheral ==========
#define AT91C_DBGU_PTCR           (0xFFFFEF20) // (PDC_DBGU) PDC Transfer Control Register
#define AT91C_DBGU_TNPR           (0xFFFFEF18) // (PDC_DBGU) Transmit Next Pointer Register
#define AT91C_DBGU_RNPR           (0xFFFFEF10) // (PDC_DBGU) Receive Next Pointer Register
#define AT91C_DBGU_TPR            (0xFFFFEF08) // (PDC_DBGU) Transmit Pointer Register
#define AT91C_DBGU_RPR            (0xFFFFEF00) // (PDC_DBGU) Receive Pointer Register
#define AT91C_DBGU_PTSR           (0xFFFFEF24) // (PDC_DBGU) PDC Transfer Status Register
#define AT91C_DBGU_TNCR           (0xFFFFEF1C) // (PDC_DBGU) Transmit Next Counter Register
#define AT91C_DBGU_RNCR           (0xFFFFEF14) // (PDC_DBGU) Receive Next Counter Register
#define AT91C_DBGU_TCR            (0xFFFFEF0C) // (PDC_DBGU) Transmit Counter Register
#define AT91C_DBGU_RCR            (0xFFFFEF04) // (PDC_DBGU) Receive Counter Register
// ========== Register definition for DBGU peripheral ==========
#define AT91C_DBGU_MR             (0xFFFFEE04) // (DBGU) Mode Register
#define AT91C_DBGU_FNTR           (0xFFFFEE48) // (DBGU) Force NTRST Register
#define AT91C_DBGU_CIDR           (0xFFFFEE40) // (DBGU) Chip ID Register
#define AT91C_DBGU_BRGR           (0xFFFFEE20) // (DBGU) Baud Rate Generator Register
#define AT91C_DBGU_RHR            (0xFFFFEE18) // (DBGU) Receiver Holding Register
#define AT91C_DBGU_IMR            (0xFFFFEE10) // (DBGU) Interrupt Mask Register
#define AT91C_DBGU_IER            (0xFFFFEE08) // (DBGU) Interrupt Enable Register
#define AT91C_DBGU_CR             (0xFFFFEE00) // (DBGU) Control Register
#define AT91C_DBGU_EXID           (0xFFFFEE44) // (DBGU) Chip ID Extension Register
#define AT91C_DBGU_THR            (0xFFFFEE1C) // (DBGU) Transmitter Holding Register
#define AT91C_DBGU_CSR            (0xFFFFEE14) // (DBGU) Channel Status Register
#define AT91C_DBGU_IDR            (0xFFFFEE0C) // (DBGU) Interrupt Disable Register
// ========== Register definition for AIC peripheral ==========
#define AT91C_AIC_ICCR            (0xFFFFF128) // (AIC) Interrupt Clear Command Register
#define AT91C_AIC_IECR            (0xFFFFF120) // (AIC) Interrupt Enable Command Register
#define AT91C_AIC_SMR             (0xFFFFF000) // (AIC) Source Mode Register
#define AT91C_AIC_ISCR            (0xFFFFF12C) // (AIC) Interrupt Set Command Register
#define AT91C_AIC_EOICR           (0xFFFFF130) // (AIC) End of Interrupt Command Register
#define AT91C_AIC_DCR             (0xFFFFF138) // (AIC) Debug Control Register (Protect)
#define AT91C_AIC_FFER            (0xFFFFF140) // (AIC) Fast Forcing Enable Register
#define AT91C_AIC_SVR             (0xFFFFF080) // (AIC) Source Vector Register
#define AT91C_AIC_SPU             (0xFFFFF134) // (AIC) Spurious Vector Register
#define AT91C_AIC_FFDR            (0xFFFFF144) // (AIC) Fast Forcing Disable Register
#define AT91C_AIC_FVR             (0xFFFFF104) // (AIC) FIQ Vector Register
#define AT91C_AIC_FFSR            (0xFFFFF148) // (AIC) Fast Forcing Status Register
#define AT91C_AIC_IMR             (0xFFFFF110) // (AIC) Interrupt Mask Register
#define AT91C_AIC_ISR             (0xFFFFF108) // (AIC) Interrupt Status Register
#define AT91C_AIC_IVR             (0xFFFFF100) // (AIC) IRQ Vector Register
#define AT91C_AIC_IDCR            (0xFFFFF124) // (AIC) Interrupt Disable Command Register
#define AT91C_AIC_CISR            (0xFFFFF114) // (AIC) Core Interrupt Status Register
#define AT91C_AIC_IPR             (0xFFFFF10C) // (AIC) Interrupt Pending Register
// ========== Register definition for PIOA peripheral ==========
#define AT91C_PIOA_BSR            (0xFFFFF274) // (PIOA) Select B Register
#define AT91C_PIOA_IDR            (0xFFFFF244) // (PIOA) Interrupt Disable Register
#define AT91C_PIOA_PDSR           (0xFFFFF23C) // (PIOA) Pin Data Status Register
#define AT91C_PIOA_CODR           (0xFFFFF234) // (PIOA) Clear Output Data Register
#define AT91C_PIOA_IFDR           (0xFFFFF224) // (PIOA) Input Filter Disable Register
#define AT91C_PIOA_OWSR           (0xFFFFF2A8) // (PIOA) Output Write Status Register
#define AT91C_PIOA_OWER           (0xFFFFF2A0) // (PIOA) Output Write Enable Register
#define AT91C_PIOA_MDER           (0xFFFFF250) // (PIOA) Multi-driver Enable Register
#define AT91C_PIOA_IMR            (0xFFFFF248) // (PIOA) Interrupt Mask Register
#define AT91C_PIOA_IER            (0xFFFFF240) // (PIOA) Interrupt Enable Register
#define AT91C_PIOA_ODSR           (0xFFFFF238) // (PIOA) Output Data Status Register
#define AT91C_PIOA_OWDR           (0xFFFFF2A4) // (PIOA) Output Write Disable Register
#define AT91C_PIOA_ISR            (0xFFFFF24C) // (PIOA) Interrupt Status Register
#define AT91C_PIOA_MDDR           (0xFFFFF254) // (PIOA) Multi-driver Disable Register
#define AT91C_PIOA_MDSR           (0xFFFFF258) // (PIOA) Multi-driver Status Register
#define AT91C_PIOA_PER            (0xFFFFF200) // (PIOA) PIO Enable Register
#define AT91C_PIOA_PSR            (0xFFFFF208) // (PIOA) PIO Status Register
#define AT91C_PIOA_PPUER          (0xFFFFF264) // (PIOA) Pull-up Enable Register
#define AT91C_PIOA_ODR            (0xFFFFF214) // (PIOA) Output Disable Registerr
#define AT91C_PIOA_PDR            (0xFFFFF204) // (PIOA) PIO Disable Register
#define AT91C_PIOA_ABSR           (0xFFFFF278) // (PIOA) AB Select Status Register
#define AT91C_PIOA_ASR            (0xFFFFF270) // (PIOA) Select A Register
#define AT91C_PIOA_PPUSR          (0xFFFFF268) // (PIOA) Pull-up Status Register
#define AT91C_PIOA_PPUDR          (0xFFFFF260) // (PIOA) Pull-up Disable Register
#define AT91C_PIOA_SODR           (0xFFFFF230) // (PIOA) Set Output Data Register
#define AT91C_PIOA_IFSR           (0xFFFFF228) // (PIOA) Input Filter Status Register
#define AT91C_PIOA_IFER           (0xFFFFF220) // (PIOA) Input Filter Enable Register
#define AT91C_PIOA_OSR            (0xFFFFF218) // (PIOA) Output Status Register
#define AT91C_PIOA_OER            (0xFFFFF210) // (PIOA) Output Enable Register
// ========== Register definition for PIOB peripheral ==========
#define AT91C_PIOB_IMR            (0xFFFFF448) // (PIOB) Interrupt Mask Register
#define AT91C_PIOB_IER            (0xFFFFF440) // (PIOB) Interrupt Enable Register
#define AT91C_PIOB_OWDR           (0xFFFFF4A4) // (PIOB) Output Write Disable Register
#define AT91C_PIOB_ISR            (0xFFFFF44C) // (PIOB) Interrupt Status Register
#define AT91C_PIOB_PPUDR          (0xFFFFF460) // (PIOB) Pull-up Disable Register
#define AT91C_PIOB_MDSR           (0xFFFFF458) // (PIOB) Multi-driver Status Register
#define AT91C_PIOB_MDER           (0xFFFFF450) // (PIOB) Multi-driver Enable Register
#define AT91C_PIOB_PER            (0xFFFFF400) // (PIOB) PIO Enable Register
#define AT91C_PIOB_PSR            (0xFFFFF408) // (PIOB) PIO Status Register
#define AT91C_PIOB_OER            (0xFFFFF410) // (PIOB) Output Enable Register
#define AT91C_PIOB_BSR            (0xFFFFF474) // (PIOB) Select B Register
#define AT91C_PIOB_PPUER          (0xFFFFF464) // (PIOB) Pull-up Enable Register
#define AT91C_PIOB_MDDR           (0xFFFFF454) // (PIOB) Multi-driver Disable Register
#define AT91C_PIOB_PDR            (0xFFFFF404) // (PIOB) PIO Disable Register
#define AT91C_PIOB_ODR            (0xFFFFF414) // (PIOB) Output Disable Registerr
#define AT91C_PIOB_IFDR           (0xFFFFF424) // (PIOB) Input Filter Disable Register
#define AT91C_PIOB_ABSR           (0xFFFFF478) // (PIOB) AB Select Status Register
#define AT91C_PIOB_ASR            (0xFFFFF470) // (PIOB) Select A Register
#define AT91C_PIOB_PPUSR          (0xFFFFF468) // (PIOB) Pull-up Status Register
#define AT91C_PIOB_ODSR           (0xFFFFF438) // (PIOB) Output Data Status Register
#define AT91C_PIOB_SODR           (0xFFFFF430) // (PIOB) Set Output Data Register
#define AT91C_PIOB_IFSR           (0xFFFFF428) // (PIOB) Input Filter Status Register
#define AT91C_PIOB_IFER           (0xFFFFF420) // (PIOB) Input Filter Enable Register
#define AT91C_PIOB_OSR            (0xFFFFF418) // (PIOB) Output Status Register
#define AT91C_PIOB_IDR            (0xFFFFF444) // (PIOB) Interrupt Disable Register
#define AT91C_PIOB_PDSR           (0xFFFFF43C) // (PIOB) Pin Data Status Register
#define AT91C_PIOB_CODR           (0xFFFFF434) // (PIOB) Clear Output Data Register
#define AT91C_PIOB_OWSR           (0xFFFFF4A8) // (PIOB) Output Write Status Register
#define AT91C_PIOB_OWER           (0xFFFFF4A0) // (PIOB) Output Write Enable Register
// ========== Register definition for PIOC peripheral ==========
#define AT91C_PIOC_OWSR           (0xFFFFF6A8) // (PIOC) Output Write Status Register
#define AT91C_PIOC_PPUSR          (0xFFFFF668) // (PIOC) Pull-up Status Register
#define AT91C_PIOC_PPUDR          (0xFFFFF660) // (PIOC) Pull-up Disable Register
#define AT91C_PIOC_MDSR           (0xFFFFF658) // (PIOC) Multi-driver Status Register
#define AT91C_PIOC_MDER           (0xFFFFF650) // (PIOC) Multi-driver Enable Register
#define AT91C_PIOC_IMR            (0xFFFFF648) // (PIOC) Interrupt Mask Register
#define AT91C_PIOC_OSR            (0xFFFFF618) // (PIOC) Output Status Register
#define AT91C_PIOC_OER            (0xFFFFF610) // (PIOC) Output Enable Register
#define AT91C_PIOC_PSR            (0xFFFFF608) // (PIOC) PIO Status Register
#define AT91C_PIOC_PER            (0xFFFFF600) // (PIOC) PIO Enable Register
#define AT91C_PIOC_BSR            (0xFFFFF674) // (PIOC) Select B Register
#define AT91C_PIOC_PPUER          (0xFFFFF664) // (PIOC) Pull-up Enable Register
#define AT91C_PIOC_IFDR           (0xFFFFF624) // (PIOC) Input Filter Disable Register
#define AT91C_PIOC_ODR            (0xFFFFF614) // (PIOC) Output Disable Registerr
#define AT91C_PIOC_ABSR           (0xFFFFF678) // (PIOC) AB Select Status Register
#define AT91C_PIOC_ASR            (0xFFFFF670) // (PIOC) Select A Register
#define AT91C_PIOC_IFER           (0xFFFFF620) // (PIOC) Input Filter Enable Register
#define AT91C_PIOC_IFSR           (0xFFFFF628) // (PIOC) Input Filter Status Register
#define AT91C_PIOC_SODR           (0xFFFFF630) // (PIOC) Set Output Data Register
#define AT91C_PIOC_ODSR           (0xFFFFF638) // (PIOC) Output Data Status Register
#define AT91C_PIOC_CODR           (0xFFFFF634) // (PIOC) Clear Output Data Register
#define AT91C_PIOC_PDSR           (0xFFFFF63C) // (PIOC) Pin Data Status Register
#define AT91C_PIOC_OWER           (0xFFFFF6A0) // (PIOC) Output Write Enable Register
#define AT91C_PIOC_IER            (0xFFFFF640) // (PIOC) Interrupt Enable Register
#define AT91C_PIOC_OWDR           (0xFFFFF6A4) // (PIOC) Output Write Disable Register
#define AT91C_PIOC_MDDR           (0xFFFFF654) // (PIOC) Multi-driver Disable Register
#define AT91C_PIOC_ISR            (0xFFFFF64C) // (PIOC) Interrupt Status Register
#define AT91C_PIOC_IDR            (0xFFFFF644) // (PIOC) Interrupt Disable Register
#define AT91C_PIOC_PDR            (0xFFFFF604) // (PIOC) PIO Disable Register
// ========== Register definition for PIOD peripheral ==========
#define AT91C_PIOD_IFDR           (0xFFFFF824) // (PIOD) Input Filter Disable Register
#define AT91C_PIOD_ODR            (0xFFFFF814) // (PIOD) Output Disable Registerr
#define AT91C_PIOD_ABSR           (0xFFFFF878) // (PIOD) AB Select Status Register
#define AT91C_PIOD_SODR           (0xFFFFF830) // (PIOD) Set Output Data Register
#define AT91C_PIOD_IFSR           (0xFFFFF828) // (PIOD) Input Filter Status Register
#define AT91C_PIOD_CODR           (0xFFFFF834) // (PIOD) Clear Output Data Register
#define AT91C_PIOD_ODSR           (0xFFFFF838) // (PIOD) Output Data Status Register
#define AT91C_PIOD_IER            (0xFFFFF840) // (PIOD) Interrupt Enable Register
#define AT91C_PIOD_IMR            (0xFFFFF848) // (PIOD) Interrupt Mask Register
#define AT91C_PIOD_OWDR           (0xFFFFF8A4) // (PIOD) Output Write Disable Register
#define AT91C_PIOD_MDDR           (0xFFFFF854) // (PIOD) Multi-driver Disable Register
#define AT91C_PIOD_PDSR           (0xFFFFF83C) // (PIOD) Pin Data Status Register
#define AT91C_PIOD_IDR            (0xFFFFF844) // (PIOD) Interrupt Disable Register
#define AT91C_PIOD_ISR            (0xFFFFF84C) // (PIOD) Interrupt Status Register
#define AT91C_PIOD_PDR            (0xFFFFF804) // (PIOD) PIO Disable Register
#define AT91C_PIOD_OWSR           (0xFFFFF8A8) // (PIOD) Output Write Status Register
#define AT91C_PIOD_OWER           (0xFFFFF8A0) // (PIOD) Output Write Enable Register
#define AT91C_PIOD_ASR            (0xFFFFF870) // (PIOD) Select A Register
#define AT91C_PIOD_PPUSR          (0xFFFFF868) // (PIOD) Pull-up Status Register
#define AT91C_PIOD_PPUDR          (0xFFFFF860) // (PIOD) Pull-up Disable Register
#define AT91C_PIOD_MDSR           (0xFFFFF858) // (PIOD) Multi-driver Status Register
#define AT91C_PIOD_MDER           (0xFFFFF850) // (PIOD) Multi-driver Enable Register
#define AT91C_PIOD_IFER           (0xFFFFF820) // (PIOD) Input Filter Enable Register
#define AT91C_PIOD_OSR            (0xFFFFF818) // (PIOD) Output Status Register
#define AT91C_PIOD_OER            (0xFFFFF810) // (PIOD) Output Enable Register
#define AT91C_PIOD_PSR            (0xFFFFF808) // (PIOD) PIO Status Register
#define AT91C_PIOD_PER            (0xFFFFF800) // (PIOD) PIO Enable Register
#define AT91C_PIOD_BSR            (0xFFFFF874) // (PIOD) Select B Register
#define AT91C_PIOD_PPUER          (0xFFFFF864) // (PIOD) Pull-up Enable Register
// ========== Register definition for PIOE peripheral ==========
#define AT91C_PIOE_PDSR           (0xFFFFFA3C) // (PIOE) Pin Data Status Register
#define AT91C_PIOE_CODR           (0xFFFFFA34) // (PIOE) Clear Output Data Register
#define AT91C_PIOE_OWER           (0xFFFFFAA0) // (PIOE) Output Write Enable Register
#define AT91C_PIOE_MDER           (0xFFFFFA50) // (PIOE) Multi-driver Enable Register
#define AT91C_PIOE_IMR            (0xFFFFFA48) // (PIOE) Interrupt Mask Register
#define AT91C_PIOE_IER            (0xFFFFFA40) // (PIOE) Interrupt Enable Register
#define AT91C_PIOE_ODSR           (0xFFFFFA38) // (PIOE) Output Data Status Register
#define AT91C_PIOE_SODR           (0xFFFFFA30) // (PIOE) Set Output Data Register
#define AT91C_PIOE_PER            (0xFFFFFA00) // (PIOE) PIO Enable Register
#define AT91C_PIOE_OWDR           (0xFFFFFAA4) // (PIOE) Output Write Disable Register
#define AT91C_PIOE_PPUER          (0xFFFFFA64) // (PIOE) Pull-up Enable Register
#define AT91C_PIOE_MDDR           (0xFFFFFA54) // (PIOE) Multi-driver Disable Register
#define AT91C_PIOE_ISR            (0xFFFFFA4C) // (PIOE) Interrupt Status Register
#define AT91C_PIOE_IDR            (0xFFFFFA44) // (PIOE) Interrupt Disable Register
#define AT91C_PIOE_PDR            (0xFFFFFA04) // (PIOE) PIO Disable Register
#define AT91C_PIOE_ODR            (0xFFFFFA14) // (PIOE) Output Disable Registerr
#define AT91C_PIOE_OWSR           (0xFFFFFAA8) // (PIOE) Output Write Status Register
#define AT91C_PIOE_ABSR           (0xFFFFFA78) // (PIOE) AB Select Status Register
#define AT91C_PIOE_ASR            (0xFFFFFA70) // (PIOE) Select A Register
#define AT91C_PIOE_PPUSR          (0xFFFFFA68) // (PIOE) Pull-up Status Register
#define AT91C_PIOE_PPUDR          (0xFFFFFA60) // (PIOE) Pull-up Disable Register
#define AT91C_PIOE_MDSR           (0xFFFFFA58) // (PIOE) Multi-driver Status Register
#define AT91C_PIOE_PSR            (0xFFFFFA08) // (PIOE) PIO Status Register
#define AT91C_PIOE_OER            (0xFFFFFA10) // (PIOE) Output Enable Register
#define AT91C_PIOE_OSR            (0xFFFFFA18) // (PIOE) Output Status Register
#define AT91C_PIOE_IFER           (0xFFFFFA20) // (PIOE) Input Filter Enable Register
#define AT91C_PIOE_BSR            (0xFFFFFA74) // (PIOE) Select B Register
#define AT91C_PIOE_IFDR           (0xFFFFFA24) // (PIOE) Input Filter Disable Register
#define AT91C_PIOE_IFSR           (0xFFFFFA28) // (PIOE) Input Filter Status Register
// ========== Register definition for CKGR peripheral ==========
#define AT91C_CKGR_PLLBR          (0xFFFFFC2C) // (CKGR) PLL B Register
#define AT91C_CKGR_MCFR           (0xFFFFFC24) // (CKGR) Main Clock  Frequency Register
#define AT91C_CKGR_PLLAR          (0xFFFFFC28) // (CKGR) PLL A Register
#define AT91C_CKGR_MOR            (0xFFFFFC20) // (CKGR) Main Oscillator Register
// ========== Register definition for PMC peripheral ==========
#define AT91C_PMC_SCSR            (0xFFFFFC08) // (PMC) System Clock Status Register
#define AT91C_PMC_SCER            (0xFFFFFC00) // (PMC) System Clock Enable Register
#define AT91C_PMC_IMR             (0xFFFFFC6C) // (PMC) Interrupt Mask Register
#define AT91C_PMC_IDR             (0xFFFFFC64) // (PMC) Interrupt Disable Register
#define AT91C_PMC_PCDR            (0xFFFFFC14) // (PMC) Peripheral Clock Disable Register
#define AT91C_PMC_SCDR            (0xFFFFFC04) // (PMC) System Clock Disable Register
#define AT91C_PMC_SR              (0xFFFFFC68) // (PMC) Status Register
#define AT91C_PMC_IER             (0xFFFFFC60) // (PMC) Interrupt Enable Register
#define AT91C_PMC_MCKR            (0xFFFFFC30) // (PMC) Master Clock Register
#define AT91C_PMC_PLLAR           (0xFFFFFC28) // (PMC) PLL A Register
#define AT91C_PMC_MOR             (0xFFFFFC20) // (PMC) Main Oscillator Register
#define AT91C_PMC_PCER            (0xFFFFFC10) // (PMC) Peripheral Clock Enable Register
#define AT91C_PMC_PCSR            (0xFFFFFC18) // (PMC) Peripheral Clock Status Register
#define AT91C_PMC_PLLBR           (0xFFFFFC2C) // (PMC) PLL B Register
#define AT91C_PMC_MCFR            (0xFFFFFC24) // (PMC) Main Clock  Frequency Register
#define AT91C_PMC_PCKR            (0xFFFFFC40) // (PMC) Programmable Clock Register
// ========== Register definition for RSTC peripheral ==========
#define AT91C_RSTC_RSR            (0xFFFFFD04) // (RSTC) Reset Status Register
#define AT91C_RSTC_RMR            (0xFFFFFD08) // (RSTC) Reset Mode Register
#define AT91C_RSTC_RCR            (0xFFFFFD00) // (RSTC) Reset Control Register
// ========== Register definition for SHDWC peripheral ==========
#define AT91C_SHDWC_SHMR          (0xFFFFFD14) // (SHDWC) Shut Down Mode Register
#define AT91C_SHDWC_SHSR          (0xFFFFFD18) // (SHDWC) Shut Down Status Register
#define AT91C_SHDWC_SHCR          (0xFFFFFD10) // (SHDWC) Shut Down Control Register
// ========== Register definition for RTTC0 peripheral ==========
#define AT91C_RTTC0_RTSR          (0xFFFFFD2C) // (RTTC0) Real-time Status Register
#define AT91C_RTTC0_RTAR          (0xFFFFFD24) // (RTTC0) Real-time Alarm Register
#define AT91C_RTTC0_RTVR          (0xFFFFFD28) // (RTTC0) Real-time Value Register
#define AT91C_RTTC0_RTMR          (0xFFFFFD20) // (RTTC0) Real-time Mode Register
// ========== Register definition for RTTC1 peripheral ==========
#define AT91C_RTTC1_RTSR          (0xFFFFFD5C) // (RTTC1) Real-time Status Register
#define AT91C_RTTC1_RTAR          (0xFFFFFD54) // (RTTC1) Real-time Alarm Register
#define AT91C_RTTC1_RTVR          (0xFFFFFD58) // (RTTC1) Real-time Value Register
#define AT91C_RTTC1_RTMR          (0xFFFFFD50) // (RTTC1) Real-time Mode Register
// ========== Register definition for PITC peripheral ==========
#define AT91C_PITC_PIIR           (0xFFFFFD3C) // (PITC) Period Interval Image Register
#define AT91C_PITC_PISR           (0xFFFFFD34) // (PITC) Period Interval Status Register
#define AT91C_PITC_PIVR           (0xFFFFFD38) // (PITC) Period Interval Value Register
#define AT91C_PITC_PIMR           (0xFFFFFD30) // (PITC) Period Interval Mode Register
// ========== Register definition for WDTC peripheral ==========
#define AT91C_WDTC_WDMR           (0xFFFFFD44) // (WDTC) Watchdog Mode Register
#define AT91C_WDTC_WDSR           (0xFFFFFD48) // (WDTC) Watchdog Status Register
#define AT91C_WDTC_WDCR           (0xFFFFFD40) // (WDTC) Watchdog Control Register
// ========== Register definition for TC0 peripheral ==========
#define AT91C_TC0_IDR             (0xFFF7C028) // (TC0) Interrupt Disable Register
#define AT91C_TC0_SR              (0xFFF7C020) // (TC0) Status Register
#define AT91C_TC0_RB              (0xFFF7C018) // (TC0) Register B
#define AT91C_TC0_CV              (0xFFF7C010) // (TC0) Counter Value
#define AT91C_TC0_CCR             (0xFFF7C000) // (TC0) Channel Control Register
#define AT91C_TC0_IMR             (0xFFF7C02C) // (TC0) Interrupt Mask Register
#define AT91C_TC0_IER             (0xFFF7C024) // (TC0) Interrupt Enable Register
#define AT91C_TC0_RC              (0xFFF7C01C) // (TC0) Register C
#define AT91C_TC0_RA              (0xFFF7C014) // (TC0) Register A
#define AT91C_TC0_CMR             (0xFFF7C004) // (TC0) Channel Mode Register (Capture Mode / Waveform Mode)
// ========== Register definition for TC1 peripheral ==========
#define AT91C_TC1_IDR             (0xFFF7C068) // (TC1) Interrupt Disable Register
#define AT91C_TC1_SR              (0xFFF7C060) // (TC1) Status Register
#define AT91C_TC1_RB              (0xFFF7C058) // (TC1) Register B
#define AT91C_TC1_CV              (0xFFF7C050) // (TC1) Counter Value
#define AT91C_TC1_CCR             (0xFFF7C040) // (TC1) Channel Control Register
#define AT91C_TC1_IMR             (0xFFF7C06C) // (TC1) Interrupt Mask Register
#define AT91C_TC1_IER             (0xFFF7C064) // (TC1) Interrupt Enable Register
#define AT91C_TC1_RC              (0xFFF7C05C) // (TC1) Register C
#define AT91C_TC1_RA              (0xFFF7C054) // (TC1) Register A
#define AT91C_TC1_CMR             (0xFFF7C044) // (TC1) Channel Mode Register (Capture Mode / Waveform Mode)
// ========== Register definition for TC2 peripheral ==========
#define AT91C_TC2_IDR             (0xFFF7C0A8) // (TC2) Interrupt Disable Register
#define AT91C_TC2_SR              (0xFFF7C0A0) // (TC2) Status Register
#define AT91C_TC2_RB              (0xFFF7C098) // (TC2) Register B
#define AT91C_TC2_CV              (0xFFF7C090) // (TC2) Counter Value
#define AT91C_TC2_CCR             (0xFFF7C080) // (TC2) Channel Control Register
#define AT91C_TC2_IMR             (0xFFF7C0AC) // (TC2) Interrupt Mask Register
#define AT91C_TC2_IER             (0xFFF7C0A4) // (TC2) Interrupt Enable Register
#define AT91C_TC2_RC              (0xFFF7C09C) // (TC2) Register C
#define AT91C_TC2_RA              (0xFFF7C094) // (TC2) Register A
#define AT91C_TC2_CMR             (0xFFF7C084) // (TC2) Channel Mode Register (Capture Mode / Waveform Mode)
// ========== Register definition for TCB0 peripheral ==========
#define AT91C_TCB0_BCR            (0xFFF7C0C0) // (TCB0) TC Block Control Register
#define AT91C_TCB0_BMR            (0xFFF7C0C4) // (TCB0) TC Block Mode Register
// ========== Register definition for TCB1 peripheral ==========
#define AT91C_TCB1_BCR            (0xFFF7C100) // (TCB1) TC Block Control Register
#define AT91C_TCB1_BMR            (0xFFF7C104) // (TCB1) TC Block Mode Register
// ========== Register definition for TCB2 peripheral ==========
#define AT91C_TCB2_BMR            (0xFFF7C144) // (TCB2) TC Block Mode Register
#define AT91C_TCB2_BCR            (0xFFF7C140) // (TCB2) TC Block Control Register
// ========== Register definition for PDC_MCI0 peripheral ==========
#define AT91C_MCI0_TNCR           (0xFFF8011C) // (PDC_MCI0) Transmit Next Counter Register
#define AT91C_MCI0_RNCR           (0xFFF80114) // (PDC_MCI0) Receive Next Counter Register
#define AT91C_MCI0_TCR            (0xFFF8010C) // (PDC_MCI0) Transmit Counter Register
#define AT91C_MCI0_RCR            (0xFFF80104) // (PDC_MCI0) Receive Counter Register
#define AT91C_MCI0_PTCR           (0xFFF80120) // (PDC_MCI0) PDC Transfer Control Register
#define AT91C_MCI0_TNPR           (0xFFF80118) // (PDC_MCI0) Transmit Next Pointer Register
#define AT91C_MCI0_RPR            (0xFFF80100) // (PDC_MCI0) Receive Pointer Register
#define AT91C_MCI0_TPR            (0xFFF80108) // (PDC_MCI0) Transmit Pointer Register
#define AT91C_MCI0_RNPR           (0xFFF80110) // (PDC_MCI0) Receive Next Pointer Register
#define AT91C_MCI0_PTSR           (0xFFF80124) // (PDC_MCI0) PDC Transfer Status Register
// ========== Register definition for MCI0 peripheral ==========
#define AT91C_MCI0_IMR            (0xFFF8004C) // (MCI0) MCI Interrupt Mask Register
#define AT91C_MCI0_IER            (0xFFF80044) // (MCI0) MCI Interrupt Enable Register
#define AT91C_MCI0_TDR            (0xFFF80034) // (MCI0) MCI Transmit Data Register
#define AT91C_MCI0_CMDR           (0xFFF80014) // (MCI0) MCI Command Register
#define AT91C_MCI0_SDCR           (0xFFF8000C) // (MCI0) MCI SD Card Register
#define AT91C_MCI0_MR             (0xFFF80004) // (MCI0) MCI Mode Register
#define AT91C_MCI0_IDR            (0xFFF80048) // (MCI0) MCI Interrupt Disable Register
#define AT91C_MCI0_SR             (0xFFF80040) // (MCI0) MCI Status Register
#define AT91C_MCI0_RDR            (0xFFF80030) // (MCI0) MCI Receive Data Register
#define AT91C_MCI0_RSPR           (0xFFF80020) // (MCI0) MCI Response Register
#define AT91C_MCI0_ARGR           (0xFFF80010) // (MCI0) MCI Argument Register
#define AT91C_MCI0_DTOR           (0xFFF80008) // (MCI0) MCI Data Timeout Register
#define AT91C_MCI0_CR             (0xFFF80000) // (MCI0) MCI Control Register
// ========== Register definition for PDC_MCI1 peripheral ==========
#define AT91C_MCI1_PTSR           (0xFFF84124) // (PDC_MCI1) PDC Transfer Status Register
#define AT91C_MCI1_TNCR           (0xFFF8411C) // (PDC_MCI1) Transmit Next Counter Register
#define AT91C_MCI1_RNCR           (0xFFF84114) // (PDC_MCI1) Receive Next Counter Register
#define AT91C_MCI1_TCR            (0xFFF8410C) // (PDC_MCI1) Transmit Counter Register
#define AT91C_MCI1_RCR            (0xFFF84104) // (PDC_MCI1) Receive Counter Register
#define AT91C_MCI1_PTCR           (0xFFF84120) // (PDC_MCI1) PDC Transfer Control Register
#define AT91C_MCI1_TNPR           (0xFFF84118) // (PDC_MCI1) Transmit Next Pointer Register
#define AT91C_MCI1_RNPR           (0xFFF84110) // (PDC_MCI1) Receive Next Pointer Register
#define AT91C_MCI1_TPR            (0xFFF84108) // (PDC_MCI1) Transmit Pointer Register
#define AT91C_MCI1_RPR            (0xFFF84100) // (PDC_MCI1) Receive Pointer Register
// ========== Register definition for MCI1 peripheral ==========
#define AT91C_MCI1_CR             (0xFFF84000) // (MCI1) MCI Control Register
#define AT91C_MCI1_IMR            (0xFFF8404C) // (MCI1) MCI Interrupt Mask Register
#define AT91C_MCI1_IER            (0xFFF84044) // (MCI1) MCI Interrupt Enable Register
#define AT91C_MCI1_TDR            (0xFFF84034) // (MCI1) MCI Transmit Data Register
#define AT91C_MCI1_CMDR           (0xFFF84014) // (MCI1) MCI Command Register
#define AT91C_MCI1_SDCR           (0xFFF8400C) // (MCI1) MCI SD Card Register
#define AT91C_MCI1_MR             (0xFFF84004) // (MCI1) MCI Mode Register
#define AT91C_MCI1_IDR            (0xFFF84048) // (MCI1) MCI Interrupt Disable Register
#define AT91C_MCI1_SR             (0xFFF84040) // (MCI1) MCI Status Register
#define AT91C_MCI1_RDR            (0xFFF84030) // (MCI1) MCI Receive Data Register
#define AT91C_MCI1_RSPR           (0xFFF84020) // (MCI1) MCI Response Register
#define AT91C_MCI1_ARGR           (0xFFF84010) // (MCI1) MCI Argument Register
#define AT91C_MCI1_DTOR           (0xFFF84008) // (MCI1) MCI Data Timeout Register
// ========== Register definition for TWI peripheral ==========
#define AT91C_TWI_THR             (0xFFF88034) // (TWI) Transmit Holding Register
#define AT91C_TWI_IMR             (0xFFF8802C) // (TWI) Interrupt Mask Register
#define AT91C_TWI_IER             (0xFFF88024) // (TWI) Interrupt Enable Register
#define AT91C_TWI_IADR            (0xFFF8800C) // (TWI) Internal Address Register
#define AT91C_TWI_MMR             (0xFFF88004) // (TWI) Master Mode Register
#define AT91C_TWI_RHR             (0xFFF88030) // (TWI) Receive Holding Register
#define AT91C_TWI_IDR             (0xFFF88028) // (TWI) Interrupt Disable Register
#define AT91C_TWI_SR              (0xFFF88020) // (TWI) Status Register
#define AT91C_TWI_CWGR            (0xFFF88010) // (TWI) Clock Waveform Generator Register
#define AT91C_TWI_CR              (0xFFF88000) // (TWI) Control Register
// ========== Register definition for PDC_US0 peripheral ==========
#define AT91C_US0_PTSR            (0xFFF8C124) // (PDC_US0) PDC Transfer Status Register
#define AT91C_US0_TNCR            (0xFFF8C11C) // (PDC_US0) Transmit Next Counter Register
#define AT91C_US0_RNCR            (0xFFF8C114) // (PDC_US0) Receive Next Counter Register
#define AT91C_US0_TCR             (0xFFF8C10C) // (PDC_US0) Transmit Counter Register
#define AT91C_US0_RCR             (0xFFF8C104) // (PDC_US0) Receive Counter Register
#define AT91C_US0_PTCR            (0xFFF8C120) // (PDC_US0) PDC Transfer Control Register
#define AT91C_US0_TNPR            (0xFFF8C118) // (PDC_US0) Transmit Next Pointer Register
#define AT91C_US0_RNPR            (0xFFF8C110) // (PDC_US0) Receive Next Pointer Register
#define AT91C_US0_TPR             (0xFFF8C108) // (PDC_US0) Transmit Pointer Register
#define AT91C_US0_RPR             (0xFFF8C100) // (PDC_US0) Receive Pointer Register
// ========== Register definition for US0 peripheral ==========
#define AT91C_US0_TTGR            (0xFFF8C028) // (US0) Transmitter Time-guard Register
#define AT91C_US0_BRGR            (0xFFF8C020) // (US0) Baud Rate Generator Register
#define AT91C_US0_RHR             (0xFFF8C018) // (US0) Receiver Holding Register
#define AT91C_US0_IMR             (0xFFF8C010) // (US0) Interrupt Mask Register
#define AT91C_US0_IER             (0xFFF8C008) // (US0) Interrupt Enable Register
#define AT91C_US0_RTOR            (0xFFF8C024) // (US0) Receiver Time-out Register
#define AT91C_US0_THR             (0xFFF8C01C) // (US0) Transmitter Holding Register
#define AT91C_US0_FIDI            (0xFFF8C040) // (US0) FI_DI_Ratio Register
#define AT91C_US0_CR              (0xFFF8C000) // (US0) Control Register
#define AT91C_US0_IF              (0xFFF8C04C) // (US0) IRDA_FILTER Register
#define AT91C_US0_NER             (0xFFF8C044) // (US0) Nb Errors Register
#define AT91C_US0_MR              (0xFFF8C004) // (US0) Mode Register
#define AT91C_US0_IDR             (0xFFF8C00C) // (US0) Interrupt Disable Register
#define AT91C_US0_CSR             (0xFFF8C014) // (US0) Channel Status Register
// ========== Register definition for PDC_US1 peripheral ==========
#define AT91C_US1_PTSR            (0xFFF90124) // (PDC_US1) PDC Transfer Status Register
#define AT91C_US1_TNCR            (0xFFF9011C) // (PDC_US1) Transmit Next Counter Register
#define AT91C_US1_RNCR            (0xFFF90114) // (PDC_US1) Receive Next Counter Register
#define AT91C_US1_TCR             (0xFFF9010C) // (PDC_US1) Transmit Counter Register
#define AT91C_US1_RCR             (0xFFF90104) // (PDC_US1) Receive Counter Register
#define AT91C_US1_PTCR            (0xFFF90120) // (PDC_US1) PDC Transfer Control Register
#define AT91C_US1_TNPR            (0xFFF90118) // (PDC_US1) Transmit Next Pointer Register
#define AT91C_US1_RNPR            (0xFFF90110) // (PDC_US1) Receive Next Pointer Register
#define AT91C_US1_TPR             (0xFFF90108) // (PDC_US1) Transmit Pointer Register
#define AT91C_US1_RPR             (0xFFF90100) // (PDC_US1) Receive Pointer Register
// ========== Register definition for US1 peripheral ==========
#define AT91C_US1_IF              (0xFFF9004C) // (US1) IRDA_FILTER Register
#define AT91C_US1_NER             (0xFFF90044) // (US1) Nb Errors Register
#define AT91C_US1_FIDI            (0xFFF90040) // (US1) FI_DI_Ratio Register
#define AT91C_US1_IMR             (0xFFF90010) // (US1) Interrupt Mask Register
#define AT91C_US1_IER             (0xFFF90008) // (US1) Interrupt Enable Register
#define AT91C_US1_CR              (0xFFF90000) // (US1) Control Register
#define AT91C_US1_MR              (0xFFF90004) // (US1) Mode Register
#define AT91C_US1_IDR             (0xFFF9000C) // (US1) Interrupt Disable Register
#define AT91C_US1_CSR             (0xFFF90014) // (US1) Channel Status Register
#define AT91C_US1_THR             (0xFFF9001C) // (US1) Transmitter Holding Register
#define AT91C_US1_RTOR            (0xFFF90024) // (US1) Receiver Time-out Register
#define AT91C_US1_RHR             (0xFFF90018) // (US1) Receiver Holding Register
#define AT91C_US1_BRGR            (0xFFF90020) // (US1) Baud Rate Generator Register
#define AT91C_US1_TTGR            (0xFFF90028) // (US1) Transmitter Time-guard Register
// ========== Register definition for PDC_US2 peripheral ==========
#define AT91C_US2_PTCR            (0xFFF94120) // (PDC_US2) PDC Transfer Control Register
#define AT91C_US2_TNPR            (0xFFF94118) // (PDC_US2) Transmit Next Pointer Register
#define AT91C_US2_RNPR            (0xFFF94110) // (PDC_US2) Receive Next Pointer Register
#define AT91C_US2_TPR             (0xFFF94108) // (PDC_US2) Transmit Pointer Register
#define AT91C_US2_RPR             (0xFFF94100) // (PDC_US2) Receive Pointer Register
#define AT91C_US2_PTSR            (0xFFF94124) // (PDC_US2) PDC Transfer Status Register
#define AT91C_US2_TNCR            (0xFFF9411C) // (PDC_US2) Transmit Next Counter Register
#define AT91C_US2_RNCR            (0xFFF94114) // (PDC_US2) Receive Next Counter Register
#define AT91C_US2_TCR             (0xFFF9410C) // (PDC_US2) Transmit Counter Register
#define AT91C_US2_RCR             (0xFFF94104) // (PDC_US2) Receive Counter Register
// ========== Register definition for US2 peripheral ==========
#define AT91C_US2_RTOR            (0xFFF94024) // (US2) Receiver Time-out Register
#define AT91C_US2_THR             (0xFFF9401C) // (US2) Transmitter Holding Register
#define AT91C_US2_CSR             (0xFFF94014) // (US2) Channel Status Register
#define AT91C_US2_FIDI            (0xFFF94040) // (US2) FI_DI_Ratio Register
#define AT91C_US2_TTGR            (0xFFF94028) // (US2) Transmitter Time-guard Register
#define AT91C_US2_MR              (0xFFF94004) // (US2) Mode Register
#define AT91C_US2_IDR             (0xFFF9400C) // (US2) Interrupt Disable Register
#define AT91C_US2_NER             (0xFFF94044) // (US2) Nb Errors Register
#define AT91C_US2_CR              (0xFFF94000) // (US2) Control Register
#define AT91C_US2_IER             (0xFFF94008) // (US2) Interrupt Enable Register
#define AT91C_US2_IMR             (0xFFF94010) // (US2) Interrupt Mask Register
#define AT91C_US2_RHR             (0xFFF94018) // (US2) Receiver Holding Register
#define AT91C_US2_BRGR            (0xFFF94020) // (US2) Baud Rate Generator Register
#define AT91C_US2_IF              (0xFFF9404C) // (US2) IRDA_FILTER Register
// ========== Register definition for PDC_SSC0 peripheral ==========
#define AT91C_SSC0_TCR            (0xFFF9810C) // (PDC_SSC0) Transmit Counter Register
#define AT91C_SSC0_RCR            (0xFFF98104) // (PDC_SSC0) Receive Counter Register
#define AT91C_SSC0_PTCR           (0xFFF98120) // (PDC_SSC0) PDC Transfer Control Register
#define AT91C_SSC0_TNPR           (0xFFF98118) // (PDC_SSC0) Transmit Next Pointer Register
#define AT91C_SSC0_RNPR           (0xFFF98110) // (PDC_SSC0) Receive Next Pointer Register
#define AT91C_SSC0_TPR            (0xFFF98108) // (PDC_SSC0) Transmit Pointer Register
#define AT91C_SSC0_RPR            (0xFFF98100) // (PDC_SSC0) Receive Pointer Register
#define AT91C_SSC0_PTSR           (0xFFF98124) // (PDC_SSC0) PDC Transfer Status Register
#define AT91C_SSC0_RNCR           (0xFFF98114) // (PDC_SSC0) Receive Next Counter Register
#define AT91C_SSC0_TNCR           (0xFFF9811C) // (PDC_SSC0) Transmit Next Counter Register
// ========== Register definition for SSC0 peripheral ==========
#define AT91C_SSC0_IDR            (0xFFF98048) // (SSC0) Interrupt Disable Register
#define AT91C_SSC0_SR             (0xFFF98040) // (SSC0) Status Register
#define AT91C_SSC0_RSHR           (0xFFF98030) // (SSC0) Receive Sync Holding Register
#define AT91C_SSC0_RHR            (0xFFF98020) // (SSC0) Receive Holding Register
#define AT91C_SSC0_TCMR           (0xFFF98018) // (SSC0) Transmit Clock Mode Register
#define AT91C_SSC0_RCMR           (0xFFF98010) // (SSC0) Receive Clock ModeRegister
#define AT91C_SSC0_CR             (0xFFF98000) // (SSC0) Control Register
#define AT91C_SSC0_IMR            (0xFFF9804C) // (SSC0) Interrupt Mask Register
#define AT91C_SSC0_IER            (0xFFF98044) // (SSC0) Interrupt Enable Register
#define AT91C_SSC0_TSHR           (0xFFF98034) // (SSC0) Transmit Sync Holding Register
#define AT91C_SSC0_THR            (0xFFF98024) // (SSC0) Transmit Holding Register
#define AT91C_SSC0_TFMR           (0xFFF9801C) // (SSC0) Transmit Frame Mode Register
#define AT91C_SSC0_RFMR           (0xFFF98014) // (SSC0) Receive Frame Mode Register
#define AT91C_SSC0_CMR            (0xFFF98004) // (SSC0) Clock Mode Register
// ========== Register definition for PDC_SSC1 peripheral ==========
#define AT91C_SSC1_PTCR           (0xFFF9C120) // (PDC_SSC1) PDC Transfer Control Register
#define AT91C_SSC1_TNPR           (0xFFF9C118) // (PDC_SSC1) Transmit Next Pointer Register
#define AT91C_SSC1_RNPR           (0xFFF9C110) // (PDC_SSC1) Receive Next Pointer Register
#define AT91C_SSC1_TPR            (0xFFF9C108) // (PDC_SSC1) Transmit Pointer Register
#define AT91C_SSC1_RPR            (0xFFF9C100) // (PDC_SSC1) Receive Pointer Register
#define AT91C_SSC1_PTSR           (0xFFF9C124) // (PDC_SSC1) PDC Transfer Status Register
#define AT91C_SSC1_TNCR           (0xFFF9C11C) // (PDC_SSC1) Transmit Next Counter Register
#define AT91C_SSC1_RNCR           (0xFFF9C114) // (PDC_SSC1) Receive Next Counter Register
#define AT91C_SSC1_TCR            (0xFFF9C10C) // (PDC_SSC1) Transmit Counter Register
#define AT91C_SSC1_RCR            (0xFFF9C104) // (PDC_SSC1) Receive Counter Register
// ========== Register definition for SSC1 peripheral ==========
#define AT91C_SSC1_IDR            (0xFFF9C048) // (SSC1) Interrupt Disable Register
#define AT91C_SSC1_SR             (0xFFF9C040) // (SSC1) Status Register
#define AT91C_SSC1_RSHR           (0xFFF9C030) // (SSC1) Receive Sync Holding Register
#define AT91C_SSC1_RHR            (0xFFF9C020) // (SSC1) Receive Holding Register
#define AT91C_SSC1_TCMR           (0xFFF9C018) // (SSC1) Transmit Clock Mode Register
#define AT91C_SSC1_RCMR           (0xFFF9C010) // (SSC1) Receive Clock ModeRegister
#define AT91C_SSC1_CR             (0xFFF9C000) // (SSC1) Control Register
#define AT91C_SSC1_IMR            (0xFFF9C04C) // (SSC1) Interrupt Mask Register
#define AT91C_SSC1_IER            (0xFFF9C044) // (SSC1) Interrupt Enable Register
#define AT91C_SSC1_TSHR           (0xFFF9C034) // (SSC1) Transmit Sync Holding Register
#define AT91C_SSC1_THR            (0xFFF9C024) // (SSC1) Transmit Holding Register
#define AT91C_SSC1_TFMR           (0xFFF9C01C) // (SSC1) Transmit Frame Mode Register
#define AT91C_SSC1_RFMR           (0xFFF9C014) // (SSC1) Receive Frame Mode Register
#define AT91C_SSC1_CMR            (0xFFF9C004) // (SSC1) Clock Mode Register
// ========== Register definition for PDC_AC97C peripheral ==========
#define AT91C_AC97C_PTSR          (0xFFFA0124) // (PDC_AC97C) PDC Transfer Status Register
#define AT91C_AC97C_TNCR          (0xFFFA011C) // (PDC_AC97C) Transmit Next Counter Register
#define AT91C_AC97C_RNCR          (0xFFFA0114) // (PDC_AC97C) Receive Next Counter Register
#define AT91C_AC97C_TCR           (0xFFFA010C) // (PDC_AC97C) Transmit Counter Register
#define AT91C_AC97C_RCR           (0xFFFA0104) // (PDC_AC97C) Receive Counter Register
#define AT91C_AC97C_PTCR          (0xFFFA0120) // (PDC_AC97C) PDC Transfer Control Register
#define AT91C_AC97C_TNPR          (0xFFFA0118) // (PDC_AC97C) Transmit Next Pointer Register
#define AT91C_AC97C_RNPR          (0xFFFA0110) // (PDC_AC97C) Receive Next Pointer Register
#define AT91C_AC97C_TPR           (0xFFFA0108) // (PDC_AC97C) Transmit Pointer Register
#define AT91C_AC97C_RPR           (0xFFFA0100) // (PDC_AC97C) Receive Pointer Register
// ========== Register definition for AC97C peripheral ==========
#define AT91C_AC97C_VERSION       (0xFFFA00FC) // (AC97C) Version Register
#define AT91C_AC97C_IMR           (0xFFFA005C) // (AC97C) Interrupt Mask Register
#define AT91C_AC97C_IER           (0xFFFA0054) // (AC97C) Interrupt Enable Register
#define AT91C_AC97C_COMR          (0xFFFA004C) // (AC97C) CODEC Mask Status Register
#define AT91C_AC97C_COTHR         (0xFFFA0044) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_OCA           (0xFFFA0014) // (AC97C) Output Channel Assignement Register
#define AT91C_AC97C_IDR           (0xFFFA0058) // (AC97C) Interrupt Disable Register
#define AT91C_AC97C_CASR          (0xFFFA0028) // (AC97C) Channel A Status Register
#define AT91C_AC97C_CARHR         (0xFFFA0020) // (AC97C) Channel A Receive Holding Register
#define AT91C_AC97C_ICA           (0xFFFA0010) // (AC97C) Input Channel AssignementRegister
#define AT91C_AC97C_MR            (0xFFFA0008) // (AC97C) Mode Register
#define AT91C_AC97C_CATHR         (0xFFFA0024) // (AC97C) Channel A Transmit Holding Register
#define AT91C_AC97C_CAMR          (0xFFFA002C) // (AC97C) Channel A Mode Register
#define AT91C_AC97C_CBTHR         (0xFFFA0034) // (AC97C) Channel B Transmit Holding Register (optional)
#define AT91C_AC97C_CBMR          (0xFFFA003C) // (AC97C) Channel B Mode Register
#define AT91C_AC97C_CBRHR         (0xFFFA0030) // (AC97C) Channel B Receive Holding Register (optional)
#define AT91C_AC97C_CBSR          (0xFFFA0038) // (AC97C) Channel B Status Register
#define AT91C_AC97C_CORHR         (0xFFFA0040) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_COSR          (0xFFFA0048) // (AC97C) CODEC Status Register
#define AT91C_AC97C_SR            (0xFFFA0050) // (AC97C) Status Register
// ========== Register definition for PDC_SPI0 peripheral ==========
#define AT91C_SPI0_PTCR           (0xFFFA4120) // (PDC_SPI0) PDC Transfer Control Register
#define AT91C_SPI0_TNPR           (0xFFFA4118) // (PDC_SPI0) Transmit Next Pointer Register
#define AT91C_SPI0_RNPR           (0xFFFA4110) // (PDC_SPI0) Receive Next Pointer Register
#define AT91C_SPI0_TPR            (0xFFFA4108) // (PDC_SPI0) Transmit Pointer Register
#define AT91C_SPI0_RPR            (0xFFFA4100) // (PDC_SPI0) Receive Pointer Register
#define AT91C_SPI0_PTSR           (0xFFFA4124) // (PDC_SPI0) PDC Transfer Status Register
#define AT91C_SPI0_TNCR           (0xFFFA411C) // (PDC_SPI0) Transmit Next Counter Register
#define AT91C_SPI0_RNCR           (0xFFFA4114) // (PDC_SPI0) Receive Next Counter Register
#define AT91C_SPI0_TCR            (0xFFFA410C) // (PDC_SPI0) Transmit Counter Register
#define AT91C_SPI0_RCR            (0xFFFA4104) // (PDC_SPI0) Receive Counter Register
// ========== Register definition for SPI0 peripheral ==========
#define AT91C_SPI0_SR             (0xFFFA4010) // (SPI0) Status Register
#define AT91C_SPI0_RDR            (0xFFFA4008) // (SPI0) Receive Data Register
#define AT91C_SPI0_CR             (0xFFFA4000) // (SPI0) Control Register
#define AT91C_SPI0_MR             (0xFFFA4004) // (SPI0) Mode Register
#define AT91C_SPI0_TDR            (0xFFFA400C) // (SPI0) Transmit Data Register
#define AT91C_SPI0_IER            (0xFFFA4014) // (SPI0) Interrupt Enable Register
#define AT91C_SPI0_IMR            (0xFFFA401C) // (SPI0) Interrupt Mask Register
#define AT91C_SPI0_CSR            (0xFFFA4030) // (SPI0) Chip Select Register
#define AT91C_SPI0_IDR            (0xFFFA4018) // (SPI0) Interrupt Disable Register
// ========== Register definition for PDC_SPI1 peripheral ==========
#define AT91C_SPI1_PTSR           (0xFFFA8124) // (PDC_SPI1) PDC Transfer Status Register
#define AT91C_SPI1_TNCR           (0xFFFA811C) // (PDC_SPI1) Transmit Next Counter Register
#define AT91C_SPI1_RNCR           (0xFFFA8114) // (PDC_SPI1) Receive Next Counter Register
#define AT91C_SPI1_TCR            (0xFFFA810C) // (PDC_SPI1) Transmit Counter Register
#define AT91C_SPI1_RCR            (0xFFFA8104) // (PDC_SPI1) Receive Counter Register
#define AT91C_SPI1_PTCR           (0xFFFA8120) // (PDC_SPI1) PDC Transfer Control Register
#define AT91C_SPI1_TNPR           (0xFFFA8118) // (PDC_SPI1) Transmit Next Pointer Register
#define AT91C_SPI1_RNPR           (0xFFFA8110) // (PDC_SPI1) Receive Next Pointer Register
#define AT91C_SPI1_TPR            (0xFFFA8108) // (PDC_SPI1) Transmit Pointer Register
#define AT91C_SPI1_RPR            (0xFFFA8100) // (PDC_SPI1) Receive Pointer Register
// ========== Register definition for SPI1 peripheral ==========
#define AT91C_SPI1_CSR            (0xFFFA8030) // (SPI1) Chip Select Register
#define AT91C_SPI1_IDR            (0xFFFA8018) // (SPI1) Interrupt Disable Register
#define AT91C_SPI1_SR             (0xFFFA8010) // (SPI1) Status Register
#define AT91C_SPI1_RDR            (0xFFFA8008) // (SPI1) Receive Data Register
#define AT91C_SPI1_CR             (0xFFFA8000) // (SPI1) Control Register
#define AT91C_SPI1_IMR            (0xFFFA801C) // (SPI1) Interrupt Mask Register
#define AT91C_SPI1_IER            (0xFFFA8014) // (SPI1) Interrupt Enable Register
#define AT91C_SPI1_TDR            (0xFFFA800C) // (SPI1) Transmit Data Register
#define AT91C_SPI1_MR             (0xFFFA8004) // (SPI1) Mode Register
// ========== Register definition for CAN_MB0 peripheral ==========
#define AT91C_CAN_MB0_MDH         (0xFFFAC218) // (CAN_MB0) MailBox Data High Register
#define AT91C_CAN_MB0_MSR         (0xFFFAC210) // (CAN_MB0) MailBox Status Register
#define AT91C_CAN_MB0_MID         (0xFFFAC208) // (CAN_MB0) MailBox ID Register
#define AT91C_CAN_MB0_MMR         (0xFFFAC200) // (CAN_MB0) MailBox Mode Register
#define AT91C_CAN_MB0_MCR         (0xFFFAC21C) // (CAN_MB0) MailBox Control Register
#define AT91C_CAN_MB0_MDL         (0xFFFAC214) // (CAN_MB0) MailBox Data Low Register
#define AT91C_CAN_MB0_MFID        (0xFFFAC20C) // (CAN_MB0) MailBox Family ID Register
#define AT91C_CAN_MB0_MAM         (0xFFFAC204) // (CAN_MB0) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB1 peripheral ==========
#define AT91C_CAN_MB1_MDH         (0xFFFAC238) // (CAN_MB1) MailBox Data High Register
#define AT91C_CAN_MB1_MSR         (0xFFFAC230) // (CAN_MB1) MailBox Status Register
#define AT91C_CAN_MB1_MID         (0xFFFAC228) // (CAN_MB1) MailBox ID Register
#define AT91C_CAN_MB1_MMR         (0xFFFAC220) // (CAN_MB1) MailBox Mode Register
#define AT91C_CAN_MB1_MCR         (0xFFFAC23C) // (CAN_MB1) MailBox Control Register
#define AT91C_CAN_MB1_MDL         (0xFFFAC234) // (CAN_MB1) MailBox Data Low Register
#define AT91C_CAN_MB1_MFID        (0xFFFAC22C) // (CAN_MB1) MailBox Family ID Register
#define AT91C_CAN_MB1_MAM         (0xFFFAC224) // (CAN_MB1) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB2 peripheral ==========
#define AT91C_CAN_MB2_MDH         (0xFFFAC258) // (CAN_MB2) MailBox Data High Register
#define AT91C_CAN_MB2_MSR         (0xFFFAC250) // (CAN_MB2) MailBox Status Register
#define AT91C_CAN_MB2_MID         (0xFFFAC248) // (CAN_MB2) MailBox ID Register
#define AT91C_CAN_MB2_MMR         (0xFFFAC240) // (CAN_MB2) MailBox Mode Register
#define AT91C_CAN_MB2_MCR         (0xFFFAC25C) // (CAN_MB2) MailBox Control Register
#define AT91C_CAN_MB2_MDL         (0xFFFAC254) // (CAN_MB2) MailBox Data Low Register
#define AT91C_CAN_MB2_MFID        (0xFFFAC24C) // (CAN_MB2) MailBox Family ID Register
#define AT91C_CAN_MB2_MAM         (0xFFFAC244) // (CAN_MB2) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB3 peripheral ==========
#define AT91C_CAN_MB3_MDH         (0xFFFAC278) // (CAN_MB3) MailBox Data High Register
#define AT91C_CAN_MB3_MSR         (0xFFFAC270) // (CAN_MB3) MailBox Status Register
#define AT91C_CAN_MB3_MID         (0xFFFAC268) // (CAN_MB3) MailBox ID Register
#define AT91C_CAN_MB3_MMR         (0xFFFAC260) // (CAN_MB3) MailBox Mode Register
#define AT91C_CAN_MB3_MCR         (0xFFFAC27C) // (CAN_MB3) MailBox Control Register
#define AT91C_CAN_MB3_MDL         (0xFFFAC274) // (CAN_MB3) MailBox Data Low Register
#define AT91C_CAN_MB3_MFID        (0xFFFAC26C) // (CAN_MB3) MailBox Family ID Register
#define AT91C_CAN_MB3_MAM         (0xFFFAC264) // (CAN_MB3) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB4 peripheral ==========
#define AT91C_CAN_MB4_MID         (0xFFFAC288) // (CAN_MB4) MailBox ID Register
#define AT91C_CAN_MB4_MMR         (0xFFFAC280) // (CAN_MB4) MailBox Mode Register
#define AT91C_CAN_MB4_MCR         (0xFFFAC29C) // (CAN_MB4) MailBox Control Register
#define AT91C_CAN_MB4_MDL         (0xFFFAC294) // (CAN_MB4) MailBox Data Low Register
#define AT91C_CAN_MB4_MFID        (0xFFFAC28C) // (CAN_MB4) MailBox Family ID Register
#define AT91C_CAN_MB4_MAM         (0xFFFAC284) // (CAN_MB4) MailBox Acceptance Mask Register
#define AT91C_CAN_MB4_MSR         (0xFFFAC290) // (CAN_MB4) MailBox Status Register
#define AT91C_CAN_MB4_MDH         (0xFFFAC298) // (CAN_MB4) MailBox Data High Register
// ========== Register definition for CAN_MB5 peripheral ==========
#define AT91C_CAN_MB5_MCR         (0xFFFAC2BC) // (CAN_MB5) MailBox Control Register
#define AT91C_CAN_MB5_MDL         (0xFFFAC2B4) // (CAN_MB5) MailBox Data Low Register
#define AT91C_CAN_MB5_MFID        (0xFFFAC2AC) // (CAN_MB5) MailBox Family ID Register
#define AT91C_CAN_MB5_MAM         (0xFFFAC2A4) // (CAN_MB5) MailBox Acceptance Mask Register
#define AT91C_CAN_MB5_MDH         (0xFFFAC2B8) // (CAN_MB5) MailBox Data High Register
#define AT91C_CAN_MB5_MSR         (0xFFFAC2B0) // (CAN_MB5) MailBox Status Register
#define AT91C_CAN_MB5_MID         (0xFFFAC2A8) // (CAN_MB5) MailBox ID Register
#define AT91C_CAN_MB5_MMR         (0xFFFAC2A0) // (CAN_MB5) MailBox Mode Register
// ========== Register definition for CAN_MB6 peripheral ==========
#define AT91C_CAN_MB6_MCR         (0xFFFAC2DC) // (CAN_MB6) MailBox Control Register
#define AT91C_CAN_MB6_MDL         (0xFFFAC2D4) // (CAN_MB6) MailBox Data Low Register
#define AT91C_CAN_MB6_MFID        (0xFFFAC2CC) // (CAN_MB6) MailBox Family ID Register
#define AT91C_CAN_MB6_MAM         (0xFFFAC2C4) // (CAN_MB6) MailBox Acceptance Mask Register
#define AT91C_CAN_MB6_MDH         (0xFFFAC2D8) // (CAN_MB6) MailBox Data High Register
#define AT91C_CAN_MB6_MSR         (0xFFFAC2D0) // (CAN_MB6) MailBox Status Register
#define AT91C_CAN_MB6_MID         (0xFFFAC2C8) // (CAN_MB6) MailBox ID Register
#define AT91C_CAN_MB6_MMR         (0xFFFAC2C0) // (CAN_MB6) MailBox Mode Register
// ========== Register definition for CAN_MB7 peripheral ==========
#define AT91C_CAN_MB7_MCR         (0xFFFAC2FC) // (CAN_MB7) MailBox Control Register
#define AT91C_CAN_MB7_MDL         (0xFFFAC2F4) // (CAN_MB7) MailBox Data Low Register
#define AT91C_CAN_MB7_MFID        (0xFFFAC2EC) // (CAN_MB7) MailBox Family ID Register
#define AT91C_CAN_MB7_MAM         (0xFFFAC2E4) // (CAN_MB7) MailBox Acceptance Mask Register
#define AT91C_CAN_MB7_MDH         (0xFFFAC2F8) // (CAN_MB7) MailBox Data High Register
#define AT91C_CAN_MB7_MSR         (0xFFFAC2F0) // (CAN_MB7) MailBox Status Register
#define AT91C_CAN_MB7_MID         (0xFFFAC2E8) // (CAN_MB7) MailBox ID Register
#define AT91C_CAN_MB7_MMR         (0xFFFAC2E0) // (CAN_MB7) MailBox Mode Register
// ========== Register definition for CAN_MB8 peripheral ==========
#define AT91C_CAN_MB8_MCR         (0xFFFAC31C) // (CAN_MB8) MailBox Control Register
#define AT91C_CAN_MB8_MDL         (0xFFFAC314) // (CAN_MB8) MailBox Data Low Register
#define AT91C_CAN_MB8_MFID        (0xFFFAC30C) // (CAN_MB8) MailBox Family ID Register
#define AT91C_CAN_MB8_MAM         (0xFFFAC304) // (CAN_MB8) MailBox Acceptance Mask Register
#define AT91C_CAN_MB8_MMR         (0xFFFAC300) // (CAN_MB8) MailBox Mode Register
#define AT91C_CAN_MB8_MDH         (0xFFFAC318) // (CAN_MB8) MailBox Data High Register
#define AT91C_CAN_MB8_MSR         (0xFFFAC310) // (CAN_MB8) MailBox Status Register
#define AT91C_CAN_MB8_MID         (0xFFFAC308) // (CAN_MB8) MailBox ID Register
// ========== Register definition for CAN_MB9 peripheral ==========
#define AT91C_CAN_MB9_MCR         (0xFFFAC33C) // (CAN_MB9) MailBox Control Register
#define AT91C_CAN_MB9_MDL         (0xFFFAC334) // (CAN_MB9) MailBox Data Low Register
#define AT91C_CAN_MB9_MFID        (0xFFFAC32C) // (CAN_MB9) MailBox Family ID Register
#define AT91C_CAN_MB9_MAM         (0xFFFAC324) // (CAN_MB9) MailBox Acceptance Mask Register
#define AT91C_CAN_MB9_MDH         (0xFFFAC338) // (CAN_MB9) MailBox Data High Register
#define AT91C_CAN_MB9_MSR         (0xFFFAC330) // (CAN_MB9) MailBox Status Register
#define AT91C_CAN_MB9_MID         (0xFFFAC328) // (CAN_MB9) MailBox ID Register
#define AT91C_CAN_MB9_MMR         (0xFFFAC320) // (CAN_MB9) MailBox Mode Register
// ========== Register definition for CAN_MB10 peripheral ==========
#define AT91C_CAN_MB10_MCR        (0xFFFAC35C) // (CAN_MB10) MailBox Control Register
#define AT91C_CAN_MB10_MDL        (0xFFFAC354) // (CAN_MB10) MailBox Data Low Register
#define AT91C_CAN_MB10_MFID       (0xFFFAC34C) // (CAN_MB10) MailBox Family ID Register
#define AT91C_CAN_MB10_MAM        (0xFFFAC344) // (CAN_MB10) MailBox Acceptance Mask Register
#define AT91C_CAN_MB10_MDH        (0xFFFAC358) // (CAN_MB10) MailBox Data High Register
#define AT91C_CAN_MB10_MSR        (0xFFFAC350) // (CAN_MB10) MailBox Status Register
#define AT91C_CAN_MB10_MID        (0xFFFAC348) // (CAN_MB10) MailBox ID Register
#define AT91C_CAN_MB10_MMR        (0xFFFAC340) // (CAN_MB10) MailBox Mode Register
// ========== Register definition for CAN_MB11 peripheral ==========
#define AT91C_CAN_MB11_MDH        (0xFFFAC378) // (CAN_MB11) MailBox Data High Register
#define AT91C_CAN_MB11_MSR        (0xFFFAC370) // (CAN_MB11) MailBox Status Register
#define AT91C_CAN_MB11_MID        (0xFFFAC368) // (CAN_MB11) MailBox ID Register
#define AT91C_CAN_MB11_MMR        (0xFFFAC360) // (CAN_MB11) MailBox Mode Register
#define AT91C_CAN_MB11_MCR        (0xFFFAC37C) // (CAN_MB11) MailBox Control Register
#define AT91C_CAN_MB11_MDL        (0xFFFAC374) // (CAN_MB11) MailBox Data Low Register
#define AT91C_CAN_MB11_MFID       (0xFFFAC36C) // (CAN_MB11) MailBox Family ID Register
#define AT91C_CAN_MB11_MAM        (0xFFFAC364) // (CAN_MB11) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB12 peripheral ==========
#define AT91C_CAN_MB12_MDH        (0xFFFAC398) // (CAN_MB12) MailBox Data High Register
#define AT91C_CAN_MB12_MSR        (0xFFFAC390) // (CAN_MB12) MailBox Status Register
#define AT91C_CAN_MB12_MID        (0xFFFAC388) // (CAN_MB12) MailBox ID Register
#define AT91C_CAN_MB12_MMR        (0xFFFAC380) // (CAN_MB12) MailBox Mode Register
#define AT91C_CAN_MB12_MCR        (0xFFFAC39C) // (CAN_MB12) MailBox Control Register
#define AT91C_CAN_MB12_MDL        (0xFFFAC394) // (CAN_MB12) MailBox Data Low Register
#define AT91C_CAN_MB12_MFID       (0xFFFAC38C) // (CAN_MB12) MailBox Family ID Register
#define AT91C_CAN_MB12_MAM        (0xFFFAC384) // (CAN_MB12) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB13 peripheral ==========
#define AT91C_CAN_MB13_MDH        (0xFFFAC3B8) // (CAN_MB13) MailBox Data High Register
#define AT91C_CAN_MB13_MSR        (0xFFFAC3B0) // (CAN_MB13) MailBox Status Register
#define AT91C_CAN_MB13_MID        (0xFFFAC3A8) // (CAN_MB13) MailBox ID Register
#define AT91C_CAN_MB13_MMR        (0xFFFAC3A0) // (CAN_MB13) MailBox Mode Register
#define AT91C_CAN_MB13_MCR        (0xFFFAC3BC) // (CAN_MB13) MailBox Control Register
#define AT91C_CAN_MB13_MDL        (0xFFFAC3B4) // (CAN_MB13) MailBox Data Low Register
#define AT91C_CAN_MB13_MFID       (0xFFFAC3AC) // (CAN_MB13) MailBox Family ID Register
#define AT91C_CAN_MB13_MAM        (0xFFFAC3A4) // (CAN_MB13) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB14 peripheral ==========
#define AT91C_CAN_MB14_MDH        (0xFFFAC3D8) // (CAN_MB14) MailBox Data High Register
#define AT91C_CAN_MB14_MSR        (0xFFFAC3D0) // (CAN_MB14) MailBox Status Register
#define AT91C_CAN_MB14_MID        (0xFFFAC3C8) // (CAN_MB14) MailBox ID Register
#define AT91C_CAN_MB14_MMR        (0xFFFAC3C0) // (CAN_MB14) MailBox Mode Register
#define AT91C_CAN_MB14_MCR        (0xFFFAC3DC) // (CAN_MB14) MailBox Control Register
#define AT91C_CAN_MB14_MDL        (0xFFFAC3D4) // (CAN_MB14) MailBox Data Low Register
#define AT91C_CAN_MB14_MFID       (0xFFFAC3CC) // (CAN_MB14) MailBox Family ID Register
#define AT91C_CAN_MB14_MAM        (0xFFFAC3C4) // (CAN_MB14) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB15 peripheral ==========
#define AT91C_CAN_MB15_MDH        (0xFFFAC3F8) // (CAN_MB15) MailBox Data High Register
#define AT91C_CAN_MB15_MSR        (0xFFFAC3F0) // (CAN_MB15) MailBox Status Register
#define AT91C_CAN_MB15_MID        (0xFFFAC3E8) // (CAN_MB15) MailBox ID Register
#define AT91C_CAN_MB15_MMR        (0xFFFAC3E0) // (CAN_MB15) MailBox Mode Register
#define AT91C_CAN_MB15_MCR        (0xFFFAC3FC) // (CAN_MB15) MailBox Control Register
#define AT91C_CAN_MB15_MDL        (0xFFFAC3F4) // (CAN_MB15) MailBox Data Low Register
#define AT91C_CAN_MB15_MFID       (0xFFFAC3EC) // (CAN_MB15) MailBox Family ID Register
#define AT91C_CAN_MB15_MAM        (0xFFFAC3E4) // (CAN_MB15) MailBox Acceptance Mask Register
// ========== Register definition for CAN peripheral ==========
#define AT91C_CAN_TCR             (0xFFFAC024) // (CAN) Transfer Command Register
#define AT91C_CAN_IDR             (0xFFFAC008) // (CAN) Interrupt Disable Register
#define AT91C_CAN_MR              (0xFFFAC000) // (CAN) Mode Register
#define AT91C_CAN_VR              (0xFFFAC0FC) // (CAN) Version Register
#define AT91C_CAN_IER             (0xFFFAC004) // (CAN) Interrupt Enable Register
#define AT91C_CAN_IMR             (0xFFFAC00C) // (CAN) Interrupt Mask Register
#define AT91C_CAN_BR              (0xFFFAC014) // (CAN) Baudrate Register
#define AT91C_CAN_TIMESTP         (0xFFFAC01C) // (CAN) Time Stamp Register
#define AT91C_CAN_SR              (0xFFFAC010) // (CAN) Status Register
#define AT91C_CAN_TIM             (0xFFFAC018) // (CAN) Timer Register
#define AT91C_CAN_ECR             (0xFFFAC020) // (CAN) Error Counter Register
#define AT91C_CAN_ACR             (0xFFFAC028) // (CAN) Abort Command Register
// ========== Register definition for PWMC_CH0 peripheral ==========
#define AT91C_PWMC_CH0_Reserved   (0xFFFB8214) // (PWMC_CH0) Reserved
#define AT91C_PWMC_CH0_CCNTR      (0xFFFB820C) // (PWMC_CH0) Channel Counter Register
#define AT91C_PWMC_CH0_CDTYR      (0xFFFB8204) // (PWMC_CH0) Channel Duty Cycle Register
#define AT91C_PWMC_CH0_CUPDR      (0xFFFB8210) // (PWMC_CH0) Channel Update Register
#define AT91C_PWMC_CH0_CPRDR      (0xFFFB8208) // (PWMC_CH0) Channel Period Register
#define AT91C_PWMC_CH0_CMR        (0xFFFB8200) // (PWMC_CH0) Channel Mode Register
// ========== Register definition for PWMC_CH1 peripheral ==========
#define AT91C_PWMC_CH1_Reserved   (0xFFFB8234) // (PWMC_CH1) Reserved
#define AT91C_PWMC_CH1_CCNTR      (0xFFFB822C) // (PWMC_CH1) Channel Counter Register
#define AT91C_PWMC_CH1_CDTYR      (0xFFFB8224) // (PWMC_CH1) Channel Duty Cycle Register
#define AT91C_PWMC_CH1_CUPDR      (0xFFFB8230) // (PWMC_CH1) Channel Update Register
#define AT91C_PWMC_CH1_CPRDR      (0xFFFB8228) // (PWMC_CH1) Channel Period Register
#define AT91C_PWMC_CH1_CMR        (0xFFFB8220) // (PWMC_CH1) Channel Mode Register
// ========== Register definition for PWMC_CH2 peripheral ==========
#define AT91C_PWMC_CH2_Reserved   (0xFFFB8254) // (PWMC_CH2) Reserved
#define AT91C_PWMC_CH2_CCNTR      (0xFFFB824C) // (PWMC_CH2) Channel Counter Register
#define AT91C_PWMC_CH2_CDTYR      (0xFFFB8244) // (PWMC_CH2) Channel Duty Cycle Register
#define AT91C_PWMC_CH2_CUPDR      (0xFFFB8250) // (PWMC_CH2) Channel Update Register
#define AT91C_PWMC_CH2_CPRDR      (0xFFFB8248) // (PWMC_CH2) Channel Period Register
#define AT91C_PWMC_CH2_CMR        (0xFFFB8240) // (PWMC_CH2) Channel Mode Register
// ========== Register definition for PWMC_CH3 peripheral ==========
#define AT91C_PWMC_CH3_CDTYR      (0xFFFB8264) // (PWMC_CH3) Channel Duty Cycle Register
#define AT91C_PWMC_CH3_CUPDR      (0xFFFB8270) // (PWMC_CH3) Channel Update Register
#define AT91C_PWMC_CH3_CPRDR      (0xFFFB8268) // (PWMC_CH3) Channel Period Register
#define AT91C_PWMC_CH3_CMR        (0xFFFB8260) // (PWMC_CH3) Channel Mode Register
#define AT91C_PWMC_CH3_Reserved   (0xFFFB8274) // (PWMC_CH3) Reserved
#define AT91C_PWMC_CH3_CCNTR      (0xFFFB826C) // (PWMC_CH3) Channel Counter Register
// ========== Register definition for PWMC peripheral ==========
#define AT91C_PWMC_IMR            (0xFFFB8018) // (PWMC) PWMC Interrupt Mask Register
#define AT91C_PWMC_IER            (0xFFFB8010) // (PWMC) PWMC Interrupt Enable Register
#define AT91C_PWMC_MR             (0xFFFB8000) // (PWMC) PWMC Mode Register
#define AT91C_PWMC_DIS            (0xFFFB8008) // (PWMC) PWMC Disable Register
#define AT91C_PWMC_VR             (0xFFFB80FC) // (PWMC) PWMC Version Register
#define AT91C_PWMC_IDR            (0xFFFB8014) // (PWMC) PWMC Interrupt Disable Register
#define AT91C_PWMC_ISR            (0xFFFB801C) // (PWMC) PWMC Interrupt Status Register
#define AT91C_PWMC_ENA            (0xFFFB8004) // (PWMC) PWMC Enable Register
#define AT91C_PWMC_SR             (0xFFFB800C) // (PWMC) PWMC Status Register
// ========== Register definition for MACB peripheral ==========
#define AT91C_MACB_RSR            (0xFFFBC020) // (MACB) Receive Status Register
#define AT91C_MACB_MAN            (0xFFFBC034) // (MACB) PHY Maintenance Register
#define AT91C_MACB_ISR            (0xFFFBC024) // (MACB) Interrupt Status Register
#define AT91C_MACB_HRB            (0xFFFBC090) // (MACB) Hash Address Bottom[31:0]
#define AT91C_MACB_MCF            (0xFFFBC048) // (MACB) Multiple Collision Frame Register
#define AT91C_MACB_PTR            (0xFFFBC038) // (MACB) Pause Time Register
#define AT91C_MACB_IER            (0xFFFBC028) // (MACB) Interrupt Enable Register
#define AT91C_MACB_SA2H           (0xFFFBC0A4) // (MACB) Specific Address 2 Top, Last 2 bytes
#define AT91C_MACB_HRT            (0xFFFBC094) // (MACB) Hash Address Top[63:32]
#define AT91C_MACB_LCOL           (0xFFFBC05C) // (MACB) Late Collision Register
#define AT91C_MACB_FRO            (0xFFFBC04C) // (MACB) Frames Received OK Register
#define AT91C_MACB_PFR            (0xFFFBC03C) // (MACB) Pause Frames received Register
#define AT91C_MACB_NCFGR          (0xFFFBC004) // (MACB) Network Configuration Register
#define AT91C_MACB_TID            (0xFFFBC0B8) // (MACB) Type ID Checking Register
#define AT91C_MACB_SA3L           (0xFFFBC0A8) // (MACB) Specific Address 3 Bottom, First 4 bytes
#define AT91C_MACB_FCSE           (0xFFFBC050) // (MACB) Frame Check Sequence Error Register
#define AT91C_MACB_ECOL           (0xFFFBC060) // (MACB) Excessive Collision Register
#define AT91C_MACB_ROV            (0xFFFBC070) // (MACB) Receive Overrun Errors Register
#define AT91C_MACB_NSR            (0xFFFBC008) // (MACB) Network Status Register
#define AT91C_MACB_RBQP           (0xFFFBC018) // (MACB) Receive Buffer Queue Pointer
#define AT91C_MACB_TPQ            (0xFFFBC0BC) // (MACB) Transmit Pause Quantum Register
#define AT91C_MACB_TUND           (0xFFFBC064) // (MACB) Transmit Underrun Error Register
#define AT91C_MACB_RSE            (0xFFFBC074) // (MACB) Receive Symbol Errors Register
#define AT91C_MACB_TBQP           (0xFFFBC01C) // (MACB) Transmit Buffer Queue Pointer
#define AT91C_MACB_ELE            (0xFFFBC078) // (MACB) Excessive Length Errors Register
#define AT91C_MACB_STE            (0xFFFBC084) // (MACB) SQE Test Error Register
#define AT91C_MACB_IDR            (0xFFFBC02C) // (MACB) Interrupt Disable Register
#define AT91C_MACB_SA1L           (0xFFFBC098) // (MACB) Specific Address 1 Bottom, First 4 bytes
#define AT91C_MACB_RLE            (0xFFFBC088) // (MACB) Receive Length Field Mismatch Register
#define AT91C_MACB_IMR            (0xFFFBC030) // (MACB) Interrupt Mask Register
#define AT91C_MACB_FTO            (0xFFFBC040) // (MACB) Frames Transmitted OK Register
#define AT91C_MACB_SA3H           (0xFFFBC0AC) // (MACB) Specific Address 3 Top, Last 2 bytes
#define AT91C_MACB_SA1H           (0xFFFBC09C) // (MACB) Specific Address 1 Top, Last 2 bytes
#define AT91C_MACB_TPF            (0xFFFBC08C) // (MACB) Transmitted Pause Frames Register
#define AT91C_MACB_SCF            (0xFFFBC044) // (MACB) Single Collision Frame Register
#define AT91C_MACB_ALE            (0xFFFBC054) // (MACB) Alignment Error Register
#define AT91C_MACB_USRIO          (0xFFFBC0C0) // (MACB) USER Input/Output Register
#define AT91C_MACB_SA4L           (0xFFFBC0B0) // (MACB) Specific Address 4 Bottom, First 4 bytes
#define AT91C_MACB_SA2L           (0xFFFBC0A0) // (MACB) Specific Address 2 Bottom, First 4 bytes
#define AT91C_MACB_REV            (0xFFFBC0FC) // (MACB) Revision Register
#define AT91C_MACB_CSE            (0xFFFBC068) // (MACB) Carrier Sense Error Register
#define AT91C_MACB_DTF            (0xFFFBC058) // (MACB) Deferred Transmission Frame Register
#define AT91C_MACB_NCR            (0xFFFBC000) // (MACB) Network Control Register
#define AT91C_MACB_WOL            (0xFFFBC0C4) // (MACB) Wake On LAN Register
#define AT91C_MACB_SA4H           (0xFFFBC0B4) // (MACB) Specific Address 4 Top, Last 2 bytes
#define AT91C_MACB_RRE            (0xFFFBC06C) // (MACB) Receive Ressource Error Register
#define AT91C_MACB_RJA            (0xFFFBC07C) // (MACB) Receive Jabbers Register
#define AT91C_MACB_TSR            (0xFFFBC014) // (MACB) Transmit Status Register
#define AT91C_MACB_USF            (0xFFFBC080) // (MACB) Undersize Frames Register
// ========== Register definition for LCDC peripheral ==========
#define AT91C_LCDC_ITR            (0x00700860) // (LCDC) Interrupts Test Register
#define AT91C_LCDC_TIM1           (0x00700808) // (LCDC) LCD Timing Config 1 Register
#define AT91C_LCDC_BA1            (0x00700000) // (LCDC) DMA Base Address Register 1
#define AT91C_LCDC_FIFO           (0x00700814) // (LCDC) LCD FIFO Register
#define AT91C_LCDC_BA2            (0x00700004) // (LCDC) DMA Base Address Register 2
#define AT91C_LCDC_FRMA2          (0x00700014) // (LCDC) DMA Frame Address Register 2
#define AT91C_LCDC_DP2_3          (0x00700828) // (LCDC) Dithering Pattern DP2_3 Register
#define AT91C_LCDC_MVAL           (0x00700818) // (LCDC) LCD Mode Toggle Rate Value Register
#define AT91C_LCDC_FRMCFG         (0x00700018) // (LCDC) DMA Frame Configuration Register
#define AT91C_LCDC_LUT_ENTRY      (0x00700C00) // (LCDC) LUT Entries Register
#define AT91C_LCDC_PWRCON         (0x0070083C) // (LCDC) Power Control Register
#define AT91C_LCDC_DP5_7          (0x0070082C) // (LCDC) Dithering Pattern DP5_7 Register
#define AT91C_LCDC_DP1_2          (0x0070081C) // (LCDC) Dithering Pattern DP1_2 Register
#define AT91C_LCDC_IMR            (0x00700850) // (LCDC) Interrupt Mask Register
#define AT91C_LCDC_CTRSTCON       (0x00700840) // (LCDC) Contrast Control Register
#define AT91C_LCDC_DP3_4          (0x00700830) // (LCDC) Dithering Pattern DP3_4 Register
#define AT91C_LCDC_IRR            (0x00700864) // (LCDC) Interrupts Raw Status Register
#define AT91C_LCDC_ISR            (0x00700854) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_CTRSTVAL       (0x00700844) // (LCDC) Contrast Value Register
#define AT91C_LCDC_TIM2           (0x0070080C) // (LCDC) LCD Timing Config 2 Register
#define AT91C_LCDC_ICR            (0x00700858) // (LCDC) Interrupt Clear Register
#define AT91C_LCDC_LCDFRCFG       (0x00700810) // (LCDC) LCD Frame Config Register
#define AT91C_LCDC_FRMP1          (0x00700008) // (LCDC) DMA Frame Pointer Register 1
#define AT91C_LCDC_DMACON         (0x0070001C) // (LCDC) DMA Control Register
#define AT91C_LCDC_FRMP2          (0x0070000C) // (LCDC) DMA Frame Pointer Register 2
#define AT91C_LCDC_DP4_7          (0x00700820) // (LCDC) Dithering Pattern DP4_7 Register
#define AT91C_LCDC_DMA2DCFG       (0x00700020) // (LCDC) DMA 2D addressing configuration
#define AT91C_LCDC_FRMA1          (0x00700010) // (LCDC) DMA Frame Address Register 1
#define AT91C_LCDC_DP3_5          (0x00700824) // (LCDC) Dithering Pattern DP3_5 Register
#define AT91C_LCDC_DP4_5          (0x00700834) // (LCDC) Dithering Pattern DP4_5 Register
#define AT91C_LCDC_DP6_7          (0x00700838) // (LCDC) Dithering Pattern DP6_7 Register
#define AT91C_LCDC_IER            (0x00700848) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_LCDCON1        (0x00700800) // (LCDC) LCD Control 1 Register
#define AT91C_LCDC_GPR            (0x0070085C) // (LCDC) General Purpose Register
#define AT91C_LCDC_IDR            (0x0070084C) // (LCDC) Interrupt Disable Register
#define AT91C_LCDC_LCDCON2        (0x00700804) // (LCDC) LCD Control 2 Register
// ========== Register definition for DMA peripheral ==========
#define AT91C_DMA_CFG1h           (0x0080009C) // (DMA) Configuration Register for channel 1 - high
#define AT91C_DMA_CFG0h           (0x00800044) // (DMA) Configuration Register for channel 0 - high
#define AT91C_DMA_SGLREQSRCREG    (0x00800378) // (DMA) Single Source Software Transaction Request Register
#define AT91C_DMA_SGR1            (0x008000A0) // (DMA) Source Gather Register for channel 1
#define AT91C_DMA_SAR1            (0x00800058) // (DMA) Source Address Register for channel 1
#define AT91C_DMA_MASKBLOCK       (0x00800318) // (DMA) Mask for IntBlock Interrupt
#define AT91C_DMA_RAWTFR          (0x008002C0) // (DMA) Raw Status for IntTfr Interrupt
#define AT91C_DMA_LSTREQSRCREG    (0x00800388) // (DMA) Last Source Software Transaction Request Register
#define AT91C_DMA_CLEARBLOCK      (0x00800340) // (DMA) Clear for IntBlock Interrupt
#define AT91C_DMA_MASKERR         (0x00800330) // (DMA) Mask for IntErr Interrupt
#define AT91C_DMA_MASKSRCTRAN     (0x00800320) // (DMA) Mask for IntSrcTran Interrupt
#define AT91C_DMA_STATUSTFR       (0x008002E8) // (DMA) Status for IntTfr Interrupt
#define AT91C_DMA_RAWDSTTRAN      (0x008002D8) // (DMA) Raw Status for IntDstTran Interrupt
#define AT91C_DMA_LLP1            (0x00800068) // (DMA) Linked List Pointer Register for channel 1
#define AT91C_DMA_SAR0            (0x00800000) // (DMA) Source Address Register for channel 0
#define AT91C_DMA_LLP0            (0x00800010) // (DMA) Linked List Pointer Register for channel 0
#define AT91C_DMA_SSTAT0          (0x00800020) // (DMA) Source Status Register for channel 0
#define AT91C_DMA_DMATESTREG      (0x008003B0) // (DMA) DW_ahb_dmac Test Register
#define AT91C_DMA_CHENREG         (0x008003A0) // (DMA) DW_ahb_dmac Channel Enable Register
#define AT91C_DMA_REQSRCREG       (0x00800368) // (DMA) Source Software Transaction Request Register
#define AT91C_DMA_CLEARERR        (0x00800358) // (DMA) Clear for IntErr Interrupt
#define AT91C_DMA_CLEARSRCTRAN    (0x00800348) // (DMA) Clear for IntSrcTran Interrupt
#define AT91C_DMA_MASKTFR         (0x00800310) // (DMA) Mask for IntTfr Interrupt
#define AT91C_DMA_STATUSDSTTRAN   (0x00800300) // (DMA) Status for IntDstTran IInterrupt
#define AT91C_DMA_DSTATAR1        (0x00800090) // (DMA) Destination Status Adress Register for channel 1
#define AT91C_DMA_DSTAT1          (0x00800080) // (DMA) Destination Status Register for channel 1
#define AT91C_DMA_SGR0            (0x00800048) // (DMA) Source Gather Register for channel 0
#define AT91C_DMA_DSTATAR0        (0x00800038) // (DMA) Destination Status Adress Register for channel 0
#define AT91C_DMA_DSTAT0          (0x00800028) // (DMA) Destination Status Register for channel 0
#define AT91C_DMA_REQDSTREG       (0x00800370) // (DMA) Destination Software Transaction Request Register
#define AT91C_DMA_DSR1            (0x008000A8) // (DMA) Destination Scatter Register for channel 1
#define AT91C_DMA_DSR0            (0x00800050) // (DMA) Destination Scatter Register for channel 0
#define AT91C_DMA_RAWBLOCK        (0x008002C8) // (DMA) Raw Status for IntBlock Interrupt
#define AT91C_DMA_LSTREQDSTREG    (0x00800390) // (DMA) Last Destination Software Transaction Request Register
#define AT91C_DMA_SGLREQDSTREG    (0x00800380) // (DMA) Single Destination Software Transaction Request Register
#define AT91C_DMA_CLEARTFR        (0x00800338) // (DMA) Clear for IntTfr Interrupt
#define AT91C_DMA_MASKDSTTRAN     (0x00800328) // (DMA) Mask for IntDstTran Interrupt
#define AT91C_DMA_CTL1l           (0x00800070) // (DMA) Control Register for channel 1 - low
#define AT91C_DMA_DAR1            (0x00800060) // (DMA) Destination Address Register for channel 1
#define AT91C_DMA_RAWSRCTRAN      (0x008002D0) // (DMA) Raw Status for IntSrcTran Interrupt
#define AT91C_DMA_RAWERR          (0x008002E0) // (DMA) Raw Status for IntErr Interrupt
#define AT91C_DMA_STATUSBLOCK     (0x008002F0) // (DMA) Status for IntBlock Interrupt
#define AT91C_DMA_CTL0l           (0x00800018) // (DMA) Control Register for channel 0 - low
#define AT91C_DMA_DAR0            (0x00800008) // (DMA) Destination Address Register for channel 0
#define AT91C_DMA_CTL1h           (0x00800074) // (DMA) Control Register for channel 1 - high
#define AT91C_DMA_CTL0h           (0x0080001C) // (DMA) Control Register for channel 0 - high
#define AT91C_DMA_VERSIONID       (0x008003B8) // (DMA) DW_ahb_dmac Version ID Register
#define AT91C_DMA_DMAIDREG        (0x008003A8) // (DMA) DW_ahb_dmac ID Register
#define AT91C_DMA_DMACFGREG       (0x00800398) // (DMA) DW_ahb_dmac Configuration Register
#define AT91C_DMA_STATUSINT       (0x00800360) // (DMA) Status for each Interrupt Type
#define AT91C_DMA_CLEARDSTTRAN    (0x00800350) // (DMA) Clear for IntDstTran IInterrupt
#define AT91C_DMA_SSTAT1          (0x00800078) // (DMA) Source Status Register for channel 1
#define AT91C_DMA_SSTATAR1        (0x00800088) // (DMA) Source Status Adress Register for channel 1
#define AT91C_DMA_CFG1l           (0x00800098) // (DMA) Configuration Register for channel 1 - low
#define AT91C_DMA_STATUSSRCTRAN   (0x008002F8) // (DMA) Status for IntSrcTran Interrupt
#define AT91C_DMA_STATUSERR       (0x00800308) // (DMA) Status for IntErr IInterrupt
#define AT91C_DMA_SSTATAR0        (0x00800030) // (DMA) Source Status Adress Register for channel 0
#define AT91C_DMA_CFG0l           (0x00800040) // (DMA) Configuration Register for channel 0 - low
// ========== Register definition for OTG peripheral ==========
#define AT91C_OTG_DevTxCount0     (0x0090440C) // (OTG) IN EP0 TX Transfert Size Reg, Device mode
#define AT91C_OTG_DevEpNe0        (0x00904804) // (OTG) EP0 NE Reg, Device mode
#define AT91C_OTG_DevTxControl3   (0x00904430) // (OTG) IN EP3 TX Control Reg, Device mode
#define AT91C_OTG_DevTxControl1   (0x00904410) // (OTG) IN EP1 TX Control Reg, Device mode
#define AT91C_OTG_I2C             (0x0090470C) // (OTG) I2C Reg (Device and Host mode)
#define AT91C_OTG_DevEpNe1        (0x00904808) // (OTG) EP1 NE Reg, Device mode
#define AT91C_OTG_DevTxStatus3    (0x00904434) // (OTG) IN EP3 TX Status Reg, Device mode
#define AT91C_OTG_DevTxFifo5      (0x00901400) // (OTG) OTG IN TX FIFO for EP5 in Device mode
#define AT91C_OTG_HostIntr        (0x00904C00) // (OTG) Interrupt Reg (Host mode)
#define AT91C_OTG_DevEpNe2        (0x0090480C) // (OTG) EP2 NE Reg, Device mode
#define AT91C_OTG_DevTxFifoSize5  (0x00904458) // (OTG) IN EP5 TX FIFO Size Reg, Device mode
#define AT91C_OTG_DevTxFifoSize3  (0x00904438) // (OTG) IN EP3 TX FIFO Size Reg, Device mode
#define AT91C_OTG_HostIntrMask    (0x00904C04) // (OTG) Interrupt Enable Reg (Host mode)
#define AT91C_OTG_DevTxCount5     (0x0090445C) // (OTG) IN EP5 TX Transfert Size Reg, Device mode
#define AT91C_OTG_DevRxControl1   (0x00904510) // (OTG) OUT EP1 RX Control Reg, Device mode
#define AT91C_OTG_HostStatus      (0x00904C08) // (OTG) Status Reg (Host mode)
#define AT91C_OTG_DevRxFifoSize   (0x00904610) // (OTG) OUT EP RX FIFO Size Reg (Device mode)
#define AT91C_OTG_HostToken       (0x00905090) // (OTG) Token Reg (Host mode)
#define AT91C_OTG_DevEnpIntrMask  (0x00904614) // (OTG) Global EP Interrupt Enable Reg (Device mode)
#define AT91C_OTG_DevThreshold    (0x00904618) // (OTG) Threshold Reg (Device mode)
#define AT91C_OTG_DevTxStatus1    (0x00904414) // (OTG) IN EP1 TX Status Reg, Device mode
#define AT91C_OTG_DevTxFifo2      (0x00900800) // (OTG) OTG IN TX FIFO for EP2 in Device mode
#define AT91C_OTG_DevTxFifoSize1  (0x00904418) // (OTG) IN EP1 TX FIFO Size Reg, Device mode
#define AT91C_OTG_DevEpNe3        (0x00904810) // (OTG) EP3 NE Reg, Device mode
#define AT91C_OTG_DevTxCount3     (0x0090443C) // (OTG) IN EP3 TX Transfert Size Reg, Device mode
#define AT91C_OTG_OutRxFifo       (0x00904000) // (OTG) OTG OUT RX FIFO. In Host mode, acts as a TX FIFO, in Device mode, acts as a RX FIFO
#define AT91C_OTG_DevEpNe4        (0x00904814) // (OTG) EP4 NE Reg, Device mode
#define AT91C_OTG_DevTxControl4   (0x00904440) // (OTG) IN EP4 TX Control Reg, Device mode
#define AT91C_OTG_HostFifoControl (0x00904C0C) // (OTG) Host Control Reg (Host mode)
#define AT91C_OTG_CtrlStatus      (0x00905094) // (OTG) Control and Status Reg (Host mode)
#define AT91C_OTG_HostFifoSize    (0x00904C10) // (OTG) FIFO Size Reg (Host mode)
#define AT91C_OTG_DevRxStatus     (0x0090461C) // (OTG) OUT EP RX FIFO Status Reg (Device mode)
#define AT91C_OTG_HostRootHubPort0 (0x00905054) // (OTG) Port Status Change Control Reg (Host mode)
#define AT91C_OTG_DevSetupStatus  (0x00904620) // (OTG) Setup RX FIFO Status Reg (Device mode)
#define AT91C_OTG_DevTxCount1     (0x0090441C) // (OTG) IN EP1 TX Transfert Size Reg, Device mode
#define AT91C_OTG_DevTxControl2   (0x00904420) // (OTG) IN EP2 TX Control Reg, Device mode
#define AT91C_OTG_DevTxStatus2    (0x00904424) // (OTG) IN EP2 TX Status Reg, Device mode
#define AT91C_OTG_DevTxStatus4    (0x00904444) // (OTG) IN EP4 TX Status Reg, Device mode
#define AT91C_OTG_DevEpNe5        (0x00904818) // (OTG) EP5 NE Reg, Device mode
#define AT91C_OTG_DevRxControl4   (0x00904540) // (OTG) OUT EP4 RX Control Reg, Device mode
#define AT91C_OTG_DevTxFifo3      (0x00900C00) // (OTG) OTG IN TX FIFO for EP3 in Device mode
#define AT91C_OTG_DevTxFifoSize4  (0x00904448) // (OTG) IN EP4 TX FIFO Size Reg, Device mode
#define AT91C_OTG_DevTxControl0   (0x00904400) // (OTG) IN EP0 TX Control Reg, Device mode
#define AT91C_OTG_DevTxCount4     (0x0090444C) // (OTG) IN EP4 TX Transfert Size Reg, Device mode
#define AT91C_OTG_HostThreshold   (0x00904C14) // (OTG) Threshold Reg (Host mode)
#define AT91C_OTG_DevTxStatus0    (0x00904404) // (OTG) IN EP0 TX Status Reg, Device mode
#define AT91C_OTG_DevRxControl0   (0x00904500) // (OTG) OUT EP0 RX Control Reg, Device mode
#define AT91C_OTG_DevRxControl2   (0x00904520) // (OTG) OUT EP2 RX Control Reg, Device mode
#define AT91C_OTG_HostFrameIntvl  (0x00905034) // (OTG) Frame Interval Reg (Host mode)
#define AT91C_OTG_HostTxCount     (0x00904C18) // (OTG) OUT Transfert Size Reg (Host mode)
#define AT91C_OTG_DevConfig       (0x00904600) // (OTG)
#define AT91C_OTG_DevStatus       (0x00904604) // (OTG)
#define AT91C_OTG_DevEnpIntr      (0x00904624) // (OTG) Global EP Interrupt Reg (Device mode)
#define AT91C_OTG_DevFrameNum     (0x00904628) // (OTG) Frame Number Reg (Device mode)
#define AT91C_OTG_InTxFifo        (0x00900000) // (OTG) OTG IN TX FIFO. In Host mode, acts as a RX FIFO, in Device mode acts as a TX FIFO (it could be named DevTxFifo0)
#define AT91C_OTG_DevSetupDataLow (0x00904700) // (OTG) Setup Data 1st DWORD (Device mode)
#define AT91C_OTG_DevTxFifoSize2  (0x00904428) // (OTG) IN EP2 TX FIFO Size Reg, Device mode
#define AT91C_OTG_DevSetupDataHigh (0x00904704) // (OTG) Setup Data 1st DWORD (Device mode)
#define AT91C_OTG_DevTxCount2     (0x0090442C) // (OTG) IN EP2 TX Transfert Size Reg, Device mode
#define AT91C_OTG_Biu             (0x00904708) // (OTG) Slave BIU Delay Count Reg (Device and Host modes)
#define AT91C_OTG_DevTxControl5   (0x00904450) // (OTG) IN EP5 TX Control Reg, Device mode
#define AT91C_OTG_HostFrameRem    (0x00905038) // (OTG) Frame Remaining Reg (Host mode)
#define AT91C_OTG_DevTxFifo4      (0x00901000) // (OTG) OTG IN TX FIFO for EP4 in Device mode
#define AT91C_OTG_DevTxStatus5    (0x00904454) // (OTG) IN EP5 TX Status Reg, Device mode
#define AT91C_OTG_DevRxControl5   (0x00904550) // (OTG) OUT EP5 RX Control Reg, Device mode
#define AT91C_OTG_HostFrameNum    (0x0090503C) // (OTG) Frame Number Reg (Host mode)
#define AT91C_OTG_DevIntr         (0x00904608) // (OTG)
#define AT91C_OTG_DevIntrMask     (0x0090460C) // (OTG)
#define AT91C_OTG_DevRxControl3   (0x00904530) // (OTG) OUT EP3 RX Control Reg, Device mode
#define AT91C_OTG_DevTxFifoSize0  (0x00904408) // (OTG) IN EP0 TX FIFO Size Reg, Device mode
#define AT91C_OTG_DevTxFifo1      (0x00900400) // (OTG) OTG IN TX FIFO for EP1 in Device mode
// ========== Register definition for UDP peripheral ==========
#define AT91C_UDP_RSTEP           (0xFFF78028) // (UDP) Reset Endpoint Register
#define AT91C_UDP_ICR             (0xFFF78020) // (UDP) Interrupt Clear Register
#define AT91C_UDP_IMR             (0xFFF78018) // (UDP) Interrupt Mask Register
#define AT91C_UDP_IER             (0xFFF78010) // (UDP) Interrupt Enable Register
#define AT91C_UDP_FADDR           (0xFFF78008) // (UDP) Function Address Register
#define AT91C_UDP_TXVC            (0xFFF78074) // (UDP) Transceiver Control Register
#define AT91C_UDP_ISR             (0xFFF7801C) // (UDP) Interrupt Status Register
#define AT91C_UDP_FDR             (0xFFF78050) // (UDP) Endpoint FIFO Data Register
#define AT91C_UDP_NUM             (0xFFF78000) // (UDP) Frame Number Register
#define AT91C_UDP_CSR             (0xFFF78030) // (UDP) Endpoint Control and Status Register
#define AT91C_UDP_GLBSTATE        (0xFFF78004) // (UDP) Global State Register
#define AT91C_UDP_IDR             (0xFFF78014) // (UDP) Interrupt Disable Register
// ========== Register definition for UHP peripheral ==========
#define AT91C_UHP_HcBulkHeadED    (0x00A00028) // (UHP) First endpoint register of the Bulk list
#define AT91C_UHP_HcFmNumber      (0x00A0003C) // (UHP) Frame number
#define AT91C_UHP_HcFmInterval    (0x00A00034) // (UHP) Bit time between 2 consecutive SOFs
#define AT91C_UHP_HcBulkCurrentED (0x00A0002C) // (UHP) Current endpoint of the Bulk list
#define AT91C_UHP_HcRhStatus      (0x00A00050) // (UHP) Root Hub Status register
#define AT91C_UHP_HcRhDescriptorA (0x00A00048) // (UHP) Root Hub characteristics A
#define AT91C_UHP_HcPeriodicStart (0x00A00040) // (UHP) Periodic Start
#define AT91C_UHP_HcFmRemaining   (0x00A00038) // (UHP) Bit time remaining in the current Frame
#define AT91C_UHP_HcBulkDoneHead  (0x00A00030) // (UHP) Last completed transfer descriptor
#define AT91C_UHP_HcRevision      (0x00A00000) // (UHP) Revision
#define AT91C_UHP_HcRhPortStatus  (0x00A00054) // (UHP) Root Hub Port Status Register
#define AT91C_UHP_HcRhDescriptorB (0x00A0004C) // (UHP) Root Hub characteristics B
#define AT91C_UHP_HcLSThreshold   (0x00A00044) // (UHP) LS Threshold
#define AT91C_UHP_HcControl       (0x00A00004) // (UHP) Operating modes for the Host Controller
#define AT91C_UHP_HcInterruptStatus (0x00A0000C) // (UHP) Interrupt Status Register
#define AT91C_UHP_HcInterruptDisable (0x00A00014) // (UHP) Interrupt Disable Register
#define AT91C_UHP_HcCommandStatus (0x00A00008) // (UHP) Command & status Register
#define AT91C_UHP_HcInterruptEnable (0x00A00010) // (UHP) Interrupt Enable Register
#define AT91C_UHP_HcHCCA          (0x00A00018) // (UHP) Pointer to the Host Controller Communication Area
#define AT91C_UHP_HcControlHeadED (0x00A00020) // (UHP) First Endpoint Descriptor of the Control list
#define AT91C_UHP_HcPeriodCurrentED (0x00A0001C) // (UHP) Current Isochronous or Interrupt Endpoint Descriptor
#define AT91C_UHP_HcControlCurrentED (0x00A00024) // (UHP) Endpoint Control and Status Register
// ========== Register definition for GPS peripheral ==========
#define AT91C_GPS_CORRSWAPWRITE   (0xFFFC0224) // (GPS)
#define AT91C_GPS_VERSIONREG      (0xFFFC046C) // (GPS) GPS Engine revision register
#define AT91C_GPS_PROCTESTREG     (0xFFFC0464) // (GPS)
#define AT91C_GPS_RESNOISEREG     (0xFFFC045C) // (GPS)
#define AT91C_GPS_RESPOSREG       (0xFFFC0454) // (GPS)
#define AT91C_GPS_PROCFIFOTHRESH  (0xFFFC044C) // (GPS)
#define AT91C_GPS_NAVBITLREG      (0xFFFC0444) // (GPS)
#define AT91C_GPS_SEARCHWINREG    (0xFFFC043C) // (GPS)
#define AT91C_GPS_DOPENDREG       (0xFFFC0434) // (GPS)
#define AT91C_GPS_SATREG          (0xFFFC042C) // (GPS)
#define AT91C_GPS_PROCTIMEREG     (0xFFFC0424) // (GPS)
#define AT91C_GPS_CARRNCOREG      (0xFFFC041C) // (GPS)
#define AT91C_GPS_CORRSWAP32      (0xFFFC021C) // (GPS)
#define AT91C_GPS_SYNCMLREG       (0xFFFC0414) // (GPS)
#define AT91C_GPS_ACQTIMEREG      (0xFFFC040C) // (GPS)
#define AT91C_GPS_CONFIGREG       (0xFFFC0404) // (GPS)
#define AT91C_GPS_ACQTESTREG      (0xFFFC0460) // (GPS)
#define AT91C_GPS_RESDOPREG       (0xFFFC0458) // (GPS)
#define AT91C_GPS_RESAMPREG       (0xFFFC0450) // (GPS)
#define AT91C_GPS_NAVBITHREG      (0xFFFC0448) // (GPS)
#define AT91C_GPS_INITFIRSTNBREG  (0xFFFC0440) // (GPS)
#define AT91C_GPS_DOPSTEPREG      (0xFFFC0438) // (GPS)
#define AT91C_GPS_DOPSTARTREG     (0xFFFC0430) // (GPS)
#define AT91C_GPS_PROCNCREG       (0xFFFC0428) // (GPS)
#define AT91C_GPS_CODENCOREG      (0xFFFC0420) // (GPS)
#define AT91C_GPS_SYNCMHREG       (0xFFFC0418) // (GPS)
#define AT91C_GPS_CORRSWAP16      (0xFFFC0220) // (GPS)
#define AT91C_GPS_THRESHACQREG    (0xFFFC0410) // (GPS)
#define AT91C_GPS_STATUSREG       (0xFFFC0408) // (GPS)
#define AT91C_GPS_PERIPHCNTRREG   (0xFFFC0400) // (GPS)
// ========== Register definition for TBOX peripheral ==========
#define AT91C_TBOX_GPSSIGQB       (0x70000BB4) // (TBOX) GPS DRFIN[7:6] aka SIGQ_B (Stimulus)
#define AT91C_TBOX_PIOC           (0x70000944) // (TBOX) Value Of PIOC
#define AT91C_TBOX_PIOEENABLEFORCE (0x70000934) // (TBOX) If each bit is 1, the corresponding bit of PIOE is controlled by TBOX_PIOEFORCEVALUE
#define AT91C_TBOX_PWM1           (0x70000A08) // (TBOX) PWM1[4:0]=nb pulses on pb7, PWM1[9:5]=nb pulses on pc28, PWM1[20:16]=nb pulses on pb8, PWM1[25:21]=nb pulses on pc3
#define AT91C_TBOX_PIOD           (0x70000948) // (TBOX) Value Of PIOD
#define AT91C_TBOX_PIOAPUN        (0x70000900) // (TBOX) Spy on PIO PUN inputs
#define AT91C_TBOX_STOPAPBSPY     (0x70000A1C) // (TBOX) When 1, no more APB SPY messages
#define AT91C_TBOX_PWM2           (0x70000A0C) // (TBOX) PWM2[3:0]=nb pulses on pb27, PWM2[7:4]=nb pulses on pc29, PWM2[19:16]=nb pulses on pb29, PWM2[23:20]=nb pulses on pe10
#define AT91C_TBOX_GPSACQDATA     (0x70000B0C) // (TBOX) GPS acquisition data (Probe - Internal Node)
#define AT91C_TBOX_MAC            (0x70000A10) // (TBOX) MAC testbench : bit 0 = rxtrig, bit 1 = clkofftester, bit 2 = err_sig_loops
#define AT91C_TBOX_GPSDUMPRES     (0x70000BC0) // (TBOX) GPS Dump results and errors
#define AT91C_TBOX_GPSSYNCHRO     (0x70000B00) // (TBOX) GPS synchronization (Stimulus)
#define AT91C_TBOX_PIOEPUN        (0x70000910) // (TBOX) Spy on PIO PUN inputs
#define AT91C_TBOX_GPSSIGIA       (0x70000BA4) // (TBOX) GPS DRFIN[1:0] aka SIGI_A (Stimulus)
#define AT91C_TBOX_PIOCENABLEFORCE (0x70000924) // (TBOX) If each bit is 1, the corresponding bit of PIOC is controlled by TBOX_PIOCFORCEVALUE
#define AT91C_TBOX_PIOAENABLEFORCE (0x70000914) // (TBOX) If each bit is 1, the corresponding bit of PIOA is controlled by TBOX_PIOAFORCEVALUE
#define AT91C_TBOX_GPSSIGQA       (0x70000BA8) // (TBOX) GPS DRFIN[3:2] aka SIGQ_A (Stimulus)
#define AT91C_TBOX_PIOEFORCEVALUE (0x70000938) // (TBOX) Value to force on PIOA when bits TBOX_PIOEENABLEFORCE are 1
#define AT91C_TBOX_PIOCFORCEVALUE (0x70000928) // (TBOX) Value to force on PIOA when bits TBOX_PIOCENABLEFORCE are 1
#define AT91C_TBOX_PIOA           (0x7000093C) // (TBOX) Value Of PIOA
#define AT91C_TBOX_PIOE           (0x7000094C) // (TBOX) Value Of PIOE
#define AT91C_TBOX_AC97START      (0x70000A00) // (TBOX) Start of AC97 test: swith PIO mux to connect PIOs to audio codec model.
#define AT91C_TBOX_PWMSTART       (0x70000A04) // (TBOX) Start of PWM test: Start to count edges on PWM IOs
#define AT91C_TBOX_USBDEV         (0x70000A14) // (TBOX) USB device testbench : bit 0 = flag0, bit 1 = flag1
#define AT91C_TBOX_GPSRAND        (0x70000B04) // (TBOX) GPS random data for correlator (Stimulus - Internal Node)
#define AT91C_TBOX_KBD            (0x70000A18) // (TBOX) Keyboard testbench : bit 0 = keypressed; bits[7:6] = key column; bits[5:4] = key row;
#define AT91C_TBOX_GPSACQSTATUS   (0x70000B08) // (TBOX) GPS acquisition status (Probe - Internal Node)
#define AT91C_TBOX_PIOBPUN        (0x70000904) // (TBOX) Spy on PIO PUN inputs
#define AT91C_TBOX_SHMCTRL        (0x70000000) // (TBOX) SHM Probe Control: 0-> shm probe stopped, 1: shm probe started
#define AT91C_TBOX_PIOAFORCEVALUE (0x70000918) // (TBOX) Value to force on PIOA when bits TBOX_PIOAENABLEFORCE are 1
#define AT91C_TBOX_PIOCPUN        (0x70000908) // (TBOX) Spy on PIO PUN inputs
#define AT91C_TBOX_DMAEXTREQ      (0x70000810) // (TBOX) DMA External request lines 3 to 0
#define AT91C_TBOX_PIODPUN        (0x7000090C) // (TBOX) Spy on PIO PUN inputs
#define AT91C_TBOX_PIOBENABLEFORCE (0x7000091C) // (TBOX) If each bit is 1, the corresponding bit of PIOB is controlled by TBOX_PIOBFORCEVALUE
#define AT91C_TBOX_PIODENABLEFORCE (0x7000092C) // (TBOX) If each bit is 1, the corresponding bit of PIOD is controlled by TBOX_PIODFORCEVALUE
#define AT91C_TBOX_GPSSIGIB       (0x70000BB0) // (TBOX) GPS DRFIN[5:4] aka SIGI_B (Stimulus)
#define AT91C_TBOX_GPSSIGFILE     (0x70000BA0) // (TBOX) GPS RFIN/DRFIN driven from files/Samples_GPS.data
#define AT91C_TBOX_PIOBFORCEVALUE (0x70000920) // (TBOX) Value to force on PIOA when bits TBOX_PIOBENABLEFORCE are 1
#define AT91C_TBOX_PIODFORCEVALUE (0x70000930) // (TBOX) Value to force on PIOA when bits TBOX_PIODENABLEFORCE are 1
#define AT91C_TBOX_PIOB           (0x70000940) // (TBOX) Value Of PIOB
// ========== Register definition for AES peripheral ==========
#define AT91C_AES_VR              (0xFFFB00FC) // (AES) AES Version Register
#define AT91C_AES_ISR             (0xFFFB001C) // (AES) Interrupt Status Register
#define AT91C_AES_IDR             (0xFFFB0014) // (AES) Interrupt Disable Register
#define AT91C_AES_IDATAxR         (0xFFFB0040) // (AES) Input Data x Register
#define AT91C_AES_KEYWxR          (0xFFFB0020) // (AES) Key Word x Register
#define AT91C_AES_MR              (0xFFFB0004) // (AES) Mode Register
#define AT91C_AES_IVxR            (0xFFFB0060) // (AES) Initialization Vector x Register
#define AT91C_AES_CR              (0xFFFB0000) // (AES) Control Register
#define AT91C_AES_IER             (0xFFFB0010) // (AES) Interrupt Enable Register
#define AT91C_AES_IMR             (0xFFFB0018) // (AES) Interrupt Mask Register
#define AT91C_AES_ODATAxR         (0xFFFB0050) // (AES) Output Data x Register
// ========== Register definition for PDC_AES peripheral ==========
#define AT91C_AES_TNCR            (0xFFFB011C) // (PDC_AES) Transmit Next Counter Register
#define AT91C_AES_RNCR            (0xFFFB0114) // (PDC_AES) Receive Next Counter Register
#define AT91C_AES_TCR             (0xFFFB010C) // (PDC_AES) Transmit Counter Register
#define AT91C_AES_RCR             (0xFFFB0104) // (PDC_AES) Receive Counter Register
#define AT91C_AES_PTCR            (0xFFFB0120) // (PDC_AES) PDC Transfer Control Register
#define AT91C_AES_TNPR            (0xFFFB0118) // (PDC_AES) Transmit Next Pointer Register
#define AT91C_AES_RNPR            (0xFFFB0110) // (PDC_AES) Receive Next Pointer Register
#define AT91C_AES_PTSR            (0xFFFB0124) // (PDC_AES) PDC Transfer Status Register
#define AT91C_AES_RPR             (0xFFFB0100) // (PDC_AES) Receive Pointer Register
#define AT91C_AES_TPR             (0xFFFB0108) // (PDC_AES) Transmit Pointer Register
// ========== Register definition for HECC0 peripheral ==========
#define AT91C_HECC0_NPR           (0xFFFFE010) // (HECC0)  ECC Parity N register
#define AT91C_HECC0_SR            (0xFFFFE008) // (HECC0)  ECC Status register
#define AT91C_HECC0_CR            (0xFFFFE000) // (HECC0)  ECC reset register
#define AT91C_HECC0_VR            (0xFFFFE0FC) // (HECC0)  ECC Version register
#define AT91C_HECC0_PR            (0xFFFFE00C) // (HECC0)  ECC Parity register
#define AT91C_HECC0_MR            (0xFFFFE004) // (HECC0)  ECC Page size register
// ========== Register definition for HECC1 peripheral ==========
#define AT91C_HECC1_PR            (0xFFFFE60C) // (HECC1)  ECC Parity register
#define AT91C_HECC1_MR            (0xFFFFE604) // (HECC1)  ECC Page size register
#define AT91C_HECC1_NPR           (0xFFFFE610) // (HECC1)  ECC Parity N register
#define AT91C_HECC1_SR            (0xFFFFE608) // (HECC1)  ECC Status register
#define AT91C_HECC1_CR            (0xFFFFE600) // (HECC1)  ECC reset register
#define AT91C_HECC1_VR            (0xFFFFE6FC) // (HECC1)  ECC Version register
// ========== Register definition for HISI peripheral ==========
#define AT91C_HISI_IDR            (0xFFFC4010) // (HISI) Interrupt Disable Register
#define AT91C_HISI_SR             (0xFFFC4008) // (HISI) Status Register
#define AT91C_HISI_CR1            (0xFFFC4000) // (HISI) Control Register 1
#define AT91C_HISI_CDBA           (0xFFFC402C) // (HISI) Codec Dma Address Register
#define AT91C_HISI_PDECF          (0xFFFC4024) // (HISI) Preview Decimation Factor Register
#define AT91C_HISI_IMR            (0xFFFC4014) // (HISI) Interrupt Mask Register
#define AT91C_HISI_IER            (0xFFFC400C) // (HISI) Interrupt Enable Register
#define AT91C_HISI_R2YSET2        (0xFFFC4040) // (HISI) Color Space Conversion Register
#define AT91C_HISI_PSIZE          (0xFFFC4020) // (HISI) Preview Size Register
#define AT91C_HISI_PFBD           (0xFFFC4028) // (HISI) Preview Frame Buffer Address Register
#define AT91C_HISI_Y2RSET0        (0xFFFC4030) // (HISI) Color Space Conversion Register
#define AT91C_HISI_R2YSET0        (0xFFFC4038) // (HISI) Color Space Conversion Register
#define AT91C_HISI_CR2            (0xFFFC4004) // (HISI) Control Register 2
#define AT91C_HISI_Y2RSET1        (0xFFFC4034) // (HISI) Color Space Conversion Register
#define AT91C_HISI_R2YSET1        (0xFFFC403C) // (HISI) Color Space Conversion Register

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91SAM9262
// *****************************************************************************
#define AT91C_PIO_PA0             (1 <<  0) // Pin Controlled by PA0
#define AT91C_PA0_MCI0_DA0        (AT91C_PIO_PA0) //
#define AT91C_PA0_SPI0_MISO       (AT91C_PIO_PA0) //
#define AT91C_PIO_PA1             (1 <<  1) // Pin Controlled by PA1
#define AT91C_PA1_MCI0_CDA        (AT91C_PIO_PA1) //
#define AT91C_PA1_SPI0_MOSI       (AT91C_PIO_PA1) //
#define AT91C_PIO_PA10            (1 << 10) // Pin Controlled by PA10
#define AT91C_PA10_MCI1_DA2       (AT91C_PIO_PA10) //
#define AT91C_PIO_PA11            (1 << 11) // Pin Controlled by PA11
#define AT91C_PA11_MCI1_DA3       (AT91C_PIO_PA11) //
#define AT91C_PIO_PA12            (1 << 12) // Pin Controlled by PA12
#define AT91C_PA12_MCI0_CK        (AT91C_PIO_PA12) //
#define AT91C_PIO_PA13            (1 << 13) // Pin Controlled by PA13
#define AT91C_PA13_CANTX          (AT91C_PIO_PA13) //
#define AT91C_PA13_PCK0           (AT91C_PIO_PA13) //
#define AT91C_PIO_PA14            (1 << 14) // Pin Controlled by PA14
#define AT91C_PA14_CANRX          (AT91C_PIO_PA14) //
#define AT91C_PA14_IRQ0           (AT91C_PIO_PA14) //
#define AT91C_PIO_PA15            (1 << 15) // Pin Controlled by PA15
#define AT91C_PA15_TCLK2          (AT91C_PIO_PA15) //
#define AT91C_PA15_IRQ1           (AT91C_PIO_PA15) //
#define AT91C_PIO_PA16            (1 << 16) // Pin Controlled by PA16
#define AT91C_PA16_MCI0_CDB       (AT91C_PIO_PA16) //
#define AT91C_PA16_EBI1_D16       (AT91C_PIO_PA16) //
#define AT91C_PIO_PA17            (1 << 17) // Pin Controlled by PA17
#define AT91C_PA17_MCI0_DB0       (AT91C_PIO_PA17) //
#define AT91C_PA17_EBI1_D17       (AT91C_PIO_PA17) //
#define AT91C_PIO_PA18            (1 << 18) // Pin Controlled by PA18
#define AT91C_PA18_MCI0_DB1       (AT91C_PIO_PA18) //
#define AT91C_PA18_EBI1_D18       (AT91C_PIO_PA18) //
#define AT91C_PIO_PA19            (1 << 19) // Pin Controlled by PA19
#define AT91C_PA19_MCI0_DB2       (AT91C_PIO_PA19) //
#define AT91C_PA19_EBI1_D19       (AT91C_PIO_PA19) //
#define AT91C_PIO_PA2             (1 <<  2) // Pin Controlled by PA2
#define AT91C_PA2_UNCONNECTED_PA2_A (AT91C_PIO_PA2) //
#define AT91C_PA2_SPI0_SPCK       (AT91C_PIO_PA2) //
#define AT91C_PIO_PA20            (1 << 20) // Pin Controlled by PA20
#define AT91C_PA20_MCI0_DB3       (AT91C_PIO_PA20) //
#define AT91C_PA20_EBI1_D20       (AT91C_PIO_PA20) //
#define AT91C_PIO_PA21            (1 << 21) // Pin Controlled by PA21
#define AT91C_PA21_MCI1_CDB       (AT91C_PIO_PA21) //
#define AT91C_PA21_EBI1_D21       (AT91C_PIO_PA21) //
#define AT91C_PIO_PA22            (1 << 22) // Pin Controlled by PA22
#define AT91C_PA22_MCI1_DB0       (AT91C_PIO_PA22) //
#define AT91C_PA22_EBI1_D22       (AT91C_PIO_PA22) //
#define AT91C_PIO_PA23            (1 << 23) // Pin Controlled by PA23
#define AT91C_PA23_MCI1_DB1       (AT91C_PIO_PA23) //
#define AT91C_PA23_EBI1_D23       (AT91C_PIO_PA23) //
#define AT91C_PIO_PA24            (1 << 24) // Pin Controlled by PA24
#define AT91C_PA24_MCI1_DB2       (AT91C_PIO_PA24) //
#define AT91C_PA24_EBI1_D24       (AT91C_PIO_PA24) //
#define AT91C_PIO_PA25            (1 << 25) // Pin Controlled by PA25
#define AT91C_PA25_MCI1_DB3       (AT91C_PIO_PA25) //
#define AT91C_PA25_EBI1_D25       (AT91C_PIO_PA25) //
#define AT91C_PIO_PA26            (1 << 26) // Pin Controlled by PA26
#define AT91C_PA26_TXD0           (AT91C_PIO_PA26) //
#define AT91C_PA26_EBI1_D26       (AT91C_PIO_PA26) //
#define AT91C_PIO_PA27            (1 << 27) // Pin Controlled by PA27
#define AT91C_PA27_RXD0           (AT91C_PIO_PA27) //
#define AT91C_PA27_EBI1_D27       (AT91C_PIO_PA27) //
#define AT91C_PIO_PA28            (1 << 28) // Pin Controlled by PA28
#define AT91C_PA28_RTS0           (AT91C_PIO_PA28) //
#define AT91C_PA28_EBI1_D28       (AT91C_PIO_PA28) //
#define AT91C_PIO_PA29            (1 << 29) // Pin Controlled by PA29
#define AT91C_PA29_CTS0           (AT91C_PIO_PA29) //
#define AT91C_PA29_EBI1_D29       (AT91C_PIO_PA29) //
#define AT91C_PIO_PA3             (1 <<  3) // Pin Controlled by PA3
#define AT91C_PA3_MCI0_DA1        (AT91C_PIO_PA3) //
#define AT91C_PA3_SPI0_NPCS1      (AT91C_PIO_PA3) //
#define AT91C_PIO_PA30            (1 << 30) // Pin Controlled by PA30
#define AT91C_PA30_SCK0           (AT91C_PIO_PA30) //
#define AT91C_PA30_EBI1_D30       (AT91C_PIO_PA30) //
#define AT91C_PIO_PA31            (1 << 31) // Pin Controlled by PA31
#define AT91C_PA31_DMARQ0         (AT91C_PIO_PA31) //
#define AT91C_PA31_EBI1_D31       (AT91C_PIO_PA31) //
#define AT91C_PIO_PA4             (1 <<  4) // Pin Controlled by PA4
#define AT91C_PA4_MCI0_DA2        (AT91C_PIO_PA4) //
#define AT91C_PA4_SPI0_NPCS2A     (AT91C_PIO_PA4) //
#define AT91C_PIO_PA5             (1 <<  5) // Pin Controlled by PA5
#define AT91C_PA5_MCI0_DA3        (AT91C_PIO_PA5) //
#define AT91C_PA5_SPI0_NPCS0      (AT91C_PIO_PA5) //
#define AT91C_PIO_PA6             (1 <<  6) // Pin Controlled by PA6
#define AT91C_PA6_MCI1_CK         (AT91C_PIO_PA6) //
#define AT91C_PA6_PCK2            (AT91C_PIO_PA6) //
#define AT91C_PIO_PA7             (1 <<  7) // Pin Controlled by PA7
#define AT91C_PA7_MCI1_CDA        (AT91C_PIO_PA7) //
#define AT91C_PIO_PA8             (1 <<  8) // Pin Controlled by PA8
#define AT91C_PA8_MCI1_DA0        (AT91C_PIO_PA8) //
#define AT91C_PIO_PA9             (1 <<  9) // Pin Controlled by PA9
#define AT91C_PA9_MCI1_DA1        (AT91C_PIO_PA9) //
#define AT91C_PIO_PB0             (1 <<  0) // Pin Controlled by PB0
#define AT91C_PB0_AC97FS          (AT91C_PIO_PB0) //
#define AT91C_PB0_TF0             (AT91C_PIO_PB0) //
#define AT91C_PIO_PB1             (1 <<  1) // Pin Controlled by PB1
#define AT91C_PB1_AC97CK          (AT91C_PIO_PB1) //
#define AT91C_PB1_TK0             (AT91C_PIO_PB1) //
#define AT91C_PIO_PB10            (1 << 10) // Pin Controlled by PB10
#define AT91C_PB10_RK1            (AT91C_PIO_PB10) //
#define AT91C_PB10_PCK1           (AT91C_PIO_PB10) //
#define AT91C_PIO_PB11            (1 << 11) // Pin Controlled by PB11
#define AT91C_PB11_RF1            (AT91C_PIO_PB11) //
#define AT91C_PB11_SPI0_NPCS3B    (AT91C_PIO_PB11) //
#define AT91C_PIO_PB12            (1 << 12) // Pin Controlled by PB12
#define AT91C_PB12_SPI1_MISO      (AT91C_PIO_PB12) //
#define AT91C_PIO_PB13            (1 << 13) // Pin Controlled by PB13
#define AT91C_PB13_SPI1_MOSI      (AT91C_PIO_PB13) //
#define AT91C_PIO_PB14            (1 << 14) // Pin Controlled by PB14
#define AT91C_PB14_SPI1_SPCK      (AT91C_PIO_PB14) //
#define AT91C_PIO_PB15            (1 << 15) // Pin Controlled by PB15
#define AT91C_PB15_SPI1_NPCS0     (AT91C_PIO_PB15) //
#define AT91C_PIO_PB16            (1 << 16) // Pin Controlled by PB16
#define AT91C_PB16_SPI1_NPCS1     (AT91C_PIO_PB16) //
#define AT91C_PB16_PCK1           (AT91C_PIO_PB16) //
#define AT91C_PIO_PB17            (1 << 17) // Pin Controlled by PB17
#define AT91C_PB17_SPI1_NPCS2B    (AT91C_PIO_PB17) //
#define AT91C_PB17_TIOA2          (AT91C_PIO_PB17) //
#define AT91C_PIO_PB18            (1 << 18) // Pin Controlled by PB18
#define AT91C_PB18_SPI1_NPCS3B    (AT91C_PIO_PB18) //
#define AT91C_PB18_TIOB2          (AT91C_PIO_PB18) //
#define AT91C_PIO_PB19            (1 << 19) // Pin Controlled by PB19
#define AT91C_PB19_LVRST          (AT91C_PIO_PB19) //
#define AT91C_PIO_PB2             (1 <<  2) // Pin Controlled by PB2
#define AT91C_PB2_AC97TX          (AT91C_PIO_PB2) //
#define AT91C_PB2_TD0             (AT91C_PIO_PB2) //
#define AT91C_PIO_PB20            (1 << 20) // Pin Controlled by PB20
#define AT91C_PB20_CKSYNC         (AT91C_PIO_PB20) //
#define AT91C_PIO_PB21            (1 << 21) // Pin Controlled by PB21
#define AT91C_PB21_PCTL0          (AT91C_PIO_PB21) //
#define AT91C_PIO_PB22            (1 << 22) // Pin Controlled by PB22
#define AT91C_PB22_CKDAT          (AT91C_PIO_PB22) //
#define AT91C_PIO_PB23            (1 << 23) // Pin Controlled by PB23
#define AT91C_PB23_GPSSYNC        (AT91C_PIO_PB23) //
#define AT91C_PIO_PB24            (1 << 24) // Pin Controlled by PB24
#define AT91C_PB24_OTG_SE0_VM     (AT91C_PIO_PB24) //
#define AT91C_PB24_DMARQ3         (AT91C_PIO_PB24) //
#define AT91C_PIO_PB25            (1 << 25) // Pin Controlled by PB25
#define AT91C_PB25_OTG_DAT_VP     (AT91C_PIO_PB25) //
#define AT91C_PIO_PB26            (1 << 26) // Pin Controlled by PB26
#define AT91C_PB26_OTGTP_OE       (AT91C_PIO_PB26) //
#define AT91C_PIO_PB27            (1 << 27) // Pin Controlled by PB27
#define AT91C_PB27_OTGRCV         (AT91C_PIO_PB27) //
#define AT91C_PB27_PWM2           (AT91C_PIO_PB27) //
#define AT91C_PIO_PB28            (1 << 28) // Pin Controlled by PB28
#define AT91C_PB28_OTGSUSPEND     (AT91C_PIO_PB28) //
#define AT91C_PB28_TCLK0          (AT91C_PIO_PB28) //
#define AT91C_PIO_PB29            (1 << 29) // Pin Controlled by PB29
#define AT91C_PB29_OTGINT         (AT91C_PIO_PB29) //
#define AT91C_PB29_PWM3           (AT91C_PIO_PB29) //
#define AT91C_PIO_PB3             (1 <<  3) // Pin Controlled by PB3
#define AT91C_PB3_AC97RX          (AT91C_PIO_PB3) //
#define AT91C_PB3_RD0             (AT91C_PIO_PB3) //
#define AT91C_PIO_PB30            (1 << 30) // Pin Controlled by PB30
#define AT91C_PB30_OTGTWD         (AT91C_PIO_PB30) //
#define AT91C_PIO_PB31            (1 << 31) // Pin Controlled by PB31
#define AT91C_PB31_OTGTWCK        (AT91C_PIO_PB31) //
#define AT91C_PIO_PB4             (1 <<  4) // Pin Controlled by PB4
#define AT91C_PB4_TWD             (AT91C_PIO_PB4) //
#define AT91C_PB4_RK0             (AT91C_PIO_PB4) //
#define AT91C_PIO_PB5             (1 <<  5) // Pin Controlled by PB5
#define AT91C_PB5_TWCK            (AT91C_PIO_PB5) //
#define AT91C_PB5_RF0             (AT91C_PIO_PB5) //
#define AT91C_PIO_PB6             (1 <<  6) // Pin Controlled by PB6
#define AT91C_PB6_TF1             (AT91C_PIO_PB6) //
#define AT91C_PB6_DMARQ1          (AT91C_PIO_PB6) //
#define AT91C_PIO_PB7             (1 <<  7) // Pin Controlled by PB7
#define AT91C_PB7_TK1             (AT91C_PIO_PB7) //
#define AT91C_PB7_PWM0            (AT91C_PIO_PB7) //
#define AT91C_PIO_PB8             (1 <<  8) // Pin Controlled by PB8
#define AT91C_PB8_TD1             (AT91C_PIO_PB8) //
#define AT91C_PB8_PWM1            (AT91C_PIO_PB8) //
#define AT91C_PIO_PB9             (1 <<  9) // Pin Controlled by PB9
#define AT91C_PB9_RD1             (AT91C_PIO_PB9) //
#define AT91C_PB9_LCDCC           (AT91C_PIO_PB9) //
#define AT91C_PIO_PC0             (1 <<  0) // Pin Controlled by PC0
#define AT91C_PC0_LCDVSYNC        (AT91C_PIO_PC0) //
#define AT91C_PIO_PC1             (1 <<  1) // Pin Controlled by PC1
#define AT91C_PC1_LCDHSYNC        (AT91C_PIO_PC1) //
#define AT91C_PIO_PC10            (1 << 10) // Pin Controlled by PC10
#define AT91C_PC10_LCDD6          (AT91C_PIO_PC10) //
#define AT91C_PC10_LCDD11B        (AT91C_PIO_PC10) //
#define AT91C_PIO_PC11            (1 << 11) // Pin Controlled by PC11
#define AT91C_PC11_LCDD7          (AT91C_PIO_PC11) //
#define AT91C_PC11_LCDD12B        (AT91C_PIO_PC11) //
#define AT91C_PIO_PC12            (1 << 12) // Pin Controlled by PC12
#define AT91C_PC12_LCDD8          (AT91C_PIO_PC12) //
#define AT91C_PC12_LCDD13B        (AT91C_PIO_PC12) //
#define AT91C_PIO_PC13            (1 << 13) // Pin Controlled by PC13
#define AT91C_PC13_LCDD9          (AT91C_PIO_PC13) //
#define AT91C_PC13_LCDD14B        (AT91C_PIO_PC13) //
#define AT91C_PIO_PC14            (1 << 14) // Pin Controlled by PC14
#define AT91C_PC14_LCDD10         (AT91C_PIO_PC14) //
#define AT91C_PC14_LCDD15B        (AT91C_PIO_PC14) //
#define AT91C_PIO_PC15            (1 << 15) // Pin Controlled by PC15
#define AT91C_PC15_LCDD11         (AT91C_PIO_PC15) //
#define AT91C_PC15_LCDD19B        (AT91C_PIO_PC15) //
#define AT91C_PIO_PC16            (1 << 16) // Pin Controlled by PC16
#define AT91C_PC16_LCDD12         (AT91C_PIO_PC16) //
#define AT91C_PC16_LCDD20B        (AT91C_PIO_PC16) //
#define AT91C_PIO_PC17            (1 << 17) // Pin Controlled by PC17
#define AT91C_PC17_LCDD13         (AT91C_PIO_PC17) //
#define AT91C_PC17_LCDD21B        (AT91C_PIO_PC17) //
#define AT91C_PIO_PC18            (1 << 18) // Pin Controlled by PC18
#define AT91C_PC18_LCDD14         (AT91C_PIO_PC18) //
#define AT91C_PC18_LCDD22B        (AT91C_PIO_PC18) //
#define AT91C_PIO_PC19            (1 << 19) // Pin Controlled by PC19
#define AT91C_PC19_LCDD15         (AT91C_PIO_PC19) //
#define AT91C_PC19_LCDD23B        (AT91C_PIO_PC19) //
#define AT91C_PIO_PC2             (1 <<  2) // Pin Controlled by PC2
#define AT91C_PC2_LCDDOTCK        (AT91C_PIO_PC2) //
#define AT91C_PIO_PC20            (1 << 20) // Pin Controlled by PC20
#define AT91C_PC20_LCDD16         (AT91C_PIO_PC20) //
#define AT91C_PC20_ETX2           (AT91C_PIO_PC20) //
#define AT91C_PIO_PC21            (1 << 21) // Pin Controlled by PC21
#define AT91C_PC21_LCDD17         (AT91C_PIO_PC21) //
#define AT91C_PC21_ETX3           (AT91C_PIO_PC21) //
#define AT91C_PIO_PC22            (1 << 22) // Pin Controlled by PC22
#define AT91C_PC22_LCDD18         (AT91C_PIO_PC22) //
#define AT91C_PC22_ERX2           (AT91C_PIO_PC22) //
#define AT91C_PIO_PC23            (1 << 23) // Pin Controlled by PC23
#define AT91C_PC23_LCDD19         (AT91C_PIO_PC23) //
#define AT91C_PC23_ERX3           (AT91C_PIO_PC23) //
#define AT91C_PIO_PC24            (1 << 24) // Pin Controlled by PC24
#define AT91C_PC24_LCDD20         (AT91C_PIO_PC24) //
#define AT91C_PC24_ETXER          (AT91C_PIO_PC24) //
#define AT91C_PIO_PC25            (1 << 25) // Pin Controlled by PC25
#define AT91C_PC25_LCDD21         (AT91C_PIO_PC25) //
#define AT91C_PC25_ERXDV          (AT91C_PIO_PC25) //
#define AT91C_PIO_PC26            (1 << 26) // Pin Controlled by PC26
#define AT91C_PC26_LCDD22         (AT91C_PIO_PC26) //
#define AT91C_PC26_ECOL           (AT91C_PIO_PC26) //
#define AT91C_PIO_PC27            (1 << 27) // Pin Controlled by PC27
#define AT91C_PC27_LCDD23         (AT91C_PIO_PC27) //
#define AT91C_PC27_ERXCK          (AT91C_PIO_PC27) //
#define AT91C_PIO_PC28            (1 << 28) // Pin Controlled by PC28
#define AT91C_PC28_PWM0           (AT91C_PIO_PC28) //
#define AT91C_PC28_TCLK1          (AT91C_PIO_PC28) //
#define AT91C_PIO_PC29            (1 << 29) // Pin Controlled by PC29
#define AT91C_PC29_PCK0           (AT91C_PIO_PC29) //
#define AT91C_PC29_PWM2           (AT91C_PIO_PC29) //
#define AT91C_PIO_PC3             (1 <<  3) // Pin Controlled by PC3
#define AT91C_PC3_LCDEN           (AT91C_PIO_PC3) //
#define AT91C_PC3_PWM1            (AT91C_PIO_PC3) //
#define AT91C_PIO_PC30            (1 << 30) // Pin Controlled by PC30
#define AT91C_PC30_DRXD           (AT91C_PIO_PC30) //
#define AT91C_PIO_PC31            (1 << 31) // Pin Controlled by PC31
#define AT91C_PC31_DTXD           (AT91C_PIO_PC31) //
#define AT91C_PIO_PC4             (1 <<  4) // Pin Controlled by PC4
#define AT91C_PC4_LCDD0           (AT91C_PIO_PC4) //
#define AT91C_PC4_LCDD3B          (AT91C_PIO_PC4) //
#define AT91C_PIO_PC5             (1 <<  5) // Pin Controlled by PC5
#define AT91C_PC5_LCDD1           (AT91C_PIO_PC5) //
#define AT91C_PC5_LCDD4B          (AT91C_PIO_PC5) //
#define AT91C_PIO_PC6             (1 <<  6) // Pin Controlled by PC6
#define AT91C_PC6_LCDD2           (AT91C_PIO_PC6) //
#define AT91C_PC6_LCDD5B          (AT91C_PIO_PC6) //
#define AT91C_PIO_PC7             (1 <<  7) // Pin Controlled by PC7
#define AT91C_PC7_LCDD3           (AT91C_PIO_PC7) //
#define AT91C_PC7_LCDD6B          (AT91C_PIO_PC7) //
#define AT91C_PIO_PC8             (1 <<  8) // Pin Controlled by PC8
#define AT91C_PC8_LCDD4           (AT91C_PIO_PC8) //
#define AT91C_PC8_LCDD7B          (AT91C_PIO_PC8) //
#define AT91C_PIO_PC9             (1 <<  9) // Pin Controlled by PC9
#define AT91C_PC9_LCDD5           (AT91C_PIO_PC9) //
#define AT91C_PC9_LCDD10B         (AT91C_PIO_PC9) //
#define AT91C_PIO_PD0             (1 <<  0) // Pin Controlled by PD0
#define AT91C_PD0_TXD1            (AT91C_PIO_PD0) //
#define AT91C_PD0_SPI0_NPCS2D     (AT91C_PIO_PD0) //
#define AT91C_PIO_PD1             (1 <<  1) // Pin Controlled by PD1
#define AT91C_PD1_RXD1            (AT91C_PIO_PD1) //
#define AT91C_PD1_SPI0_NPCS3D     (AT91C_PIO_PD1) //
#define AT91C_PIO_PD10            (1 << 10) // Pin Controlled by PD10
#define AT91C_PD10_UNCONNECTED_PD10_A (AT91C_PIO_PD10) //
#define AT91C_PD10_SCK1           (AT91C_PIO_PD10) //
#define AT91C_PIO_PD11            (1 << 11) // Pin Controlled by PD11
#define AT91C_PD11_EBI0_NCS2      (AT91C_PIO_PD11) //
#define AT91C_PD11_TSYNC          (AT91C_PIO_PD11) //
#define AT91C_PIO_PD12            (1 << 12) // Pin Controlled by PD12
#define AT91C_PD12_EBI0_A23       (AT91C_PIO_PD12) //
#define AT91C_PD12_TCLK           (AT91C_PIO_PD12) //
#define AT91C_PIO_PD13            (1 << 13) // Pin Controlled by PD13
#define AT91C_PD13_EBI0_A24       (AT91C_PIO_PD13) //
#define AT91C_PD13_TPS0           (AT91C_PIO_PD13) //
#define AT91C_PIO_PD14            (1 << 14) // Pin Controlled by PD14
#define AT91C_PD14_EBI0_A25_CFNRW (AT91C_PIO_PD14) //
#define AT91C_PD14_TPS1           (AT91C_PIO_PD14) //
#define AT91C_PIO_PD15            (1 << 15) // Pin Controlled by PD15
#define AT91C_PD15_EBI0_NCS3_NANDCS (AT91C_PIO_PD15) //
#define AT91C_PD15_TPS2           (AT91C_PIO_PD15) //
#define AT91C_PIO_PD16            (1 << 16) // Pin Controlled by PD16
#define AT91C_PD16_EBI0_D16       (AT91C_PIO_PD16) //
#define AT91C_PD16_TPK0           (AT91C_PIO_PD16) //
#define AT91C_PIO_PD17            (1 << 17) // Pin Controlled by PD17
#define AT91C_PD17_EBI0_D17       (AT91C_PIO_PD17) //
#define AT91C_PD17_TPK1           (AT91C_PIO_PD17) //
#define AT91C_PIO_PD18            (1 << 18) // Pin Controlled by PD18
#define AT91C_PD18_EBI0_D18       (AT91C_PIO_PD18) //
#define AT91C_PD18_TPK2           (AT91C_PIO_PD18) //
#define AT91C_PIO_PD19            (1 << 19) // Pin Controlled by PD19
#define AT91C_PD19_EBI0_D19       (AT91C_PIO_PD19) //
#define AT91C_PD19_TPK3           (AT91C_PIO_PD19) //
#define AT91C_PIO_PD2             (1 <<  2) // Pin Controlled by PD2
#define AT91C_PD2_TXD2            (AT91C_PIO_PD2) //
#define AT91C_PD2_SPI1_NPCS2D     (AT91C_PIO_PD2) //
#define AT91C_PIO_PD20            (1 << 20) // Pin Controlled by PD20
#define AT91C_PD20_EBI0_D20       (AT91C_PIO_PD20) //
#define AT91C_PD20_TPK4           (AT91C_PIO_PD20) //
#define AT91C_PIO_PD21            (1 << 21) // Pin Controlled by PD21
#define AT91C_PD21_EBI0_D21       (AT91C_PIO_PD21) //
#define AT91C_PD21_TPK5           (AT91C_PIO_PD21) //
#define AT91C_PIO_PD22            (1 << 22) // Pin Controlled by PD22
#define AT91C_PD22_EBI0_D22       (AT91C_PIO_PD22) //
#define AT91C_PD22_TPK6           (AT91C_PIO_PD22) //
#define AT91C_PIO_PD23            (1 << 23) // Pin Controlled by PD23
#define AT91C_PD23_EBI0_D23       (AT91C_PIO_PD23) //
#define AT91C_PD23_TPK7           (AT91C_PIO_PD23) //
#define AT91C_PIO_PD24            (1 << 24) // Pin Controlled by PD24
#define AT91C_PD24_EBI0_D24       (AT91C_PIO_PD24) //
#define AT91C_PD24_TPK8           (AT91C_PIO_PD24) //
#define AT91C_PIO_PD25            (1 << 25) // Pin Controlled by PD25
#define AT91C_PD25_EBI0_D25       (AT91C_PIO_PD25) //
#define AT91C_PD25_TPK9           (AT91C_PIO_PD25) //
#define AT91C_PIO_PD26            (1 << 26) // Pin Controlled by PD26
#define AT91C_PD26_EBI0_D26       (AT91C_PIO_PD26) //
#define AT91C_PD26_TPK10          (AT91C_PIO_PD26) //
#define AT91C_PIO_PD27            (1 << 27) // Pin Controlled by PD27
#define AT91C_PD27_EBI0_D27       (AT91C_PIO_PD27) //
#define AT91C_PD27_TPK11          (AT91C_PIO_PD27) //
#define AT91C_PIO_PD28            (1 << 28) // Pin Controlled by PD28
#define AT91C_PD28_EBI0_D28       (AT91C_PIO_PD28) //
#define AT91C_PD28_TPK12          (AT91C_PIO_PD28) //
#define AT91C_PIO_PD29            (1 << 29) // Pin Controlled by PD29
#define AT91C_PD29_EBI0_D29       (AT91C_PIO_PD29) //
#define AT91C_PD29_TPK13          (AT91C_PIO_PD29) //
#define AT91C_PIO_PD3             (1 <<  3) // Pin Controlled by PD3
#define AT91C_PD3_RXD2            (AT91C_PIO_PD3) //
#define AT91C_PD3_SPI1_NPCS3D     (AT91C_PIO_PD3) //
#define AT91C_PIO_PD30            (1 << 30) // Pin Controlled by PD30
#define AT91C_PD30_EBI0_D30       (AT91C_PIO_PD30) //
#define AT91C_PD30_TPK14          (AT91C_PIO_PD30) //
#define AT91C_PIO_PD31            (1 << 31) // Pin Controlled by PD31
#define AT91C_PD31_EBI0_D31       (AT91C_PIO_PD31) //
#define AT91C_PD31_TPK15          (AT91C_PIO_PD31) //
#define AT91C_PIO_PD4             (1 <<  4) // Pin Controlled by PD4
#define AT91C_PD4_FIQ             (AT91C_PIO_PD4) //
#define AT91C_PD4_DMARQ2          (AT91C_PIO_PD4) //
#define AT91C_PIO_PD5             (1 <<  5) // Pin Controlled by PD5
#define AT91C_PD5_EBI0_NWAIT      (AT91C_PIO_PD5) //
#define AT91C_PD5_RTS2            (AT91C_PIO_PD5) //
#define AT91C_PIO_PD6             (1 <<  6) // Pin Controlled by PD6
#define AT91C_PD6_EBI0_NCS4_CFCS0 (AT91C_PIO_PD6) //
#define AT91C_PD6_CTS2            (AT91C_PIO_PD6) //
#define AT91C_PIO_PD7             (1 <<  7) // Pin Controlled by PD7
#define AT91C_PD7_EBI0_NCS5_CFCS1 (AT91C_PIO_PD7) //
#define AT91C_PD7_RTS1            (AT91C_PIO_PD7) //
#define AT91C_PIO_PD8             (1 <<  8) // Pin Controlled by PD8
#define AT91C_PD8_EBI0_CFCE1      (AT91C_PIO_PD8) //
#define AT91C_PD8_CTS1            (AT91C_PIO_PD8) //
#define AT91C_PIO_PD9             (1 <<  9) // Pin Controlled by PD9
#define AT91C_PD9_EBI0_CFCE2      (AT91C_PIO_PD9) //
#define AT91C_PD9_SCK2            (AT91C_PIO_PD9) //
#define AT91C_PIO_PE0             (1 <<  0) // Pin Controlled by PE0
#define AT91C_PE0_ISI_D0          (AT91C_PIO_PE0) //
#define AT91C_PE0_DRFIN0          (AT91C_PIO_PE0) //  GPS Digital RF Input sigi_a0
#define AT91C_PIO_PE1             (1 <<  1) // Pin Controlled by PE1
#define AT91C_PE1_ISI_D1          (AT91C_PIO_PE1) //
#define AT91C_PE1_DRFIN1          (AT91C_PIO_PE1) //  GPS Digital RF Input sigi_a1
#define AT91C_PIO_PE10            (1 << 10) // Pin Controlled by PE10
#define AT91C_PE10_ISI_VSYNC      (AT91C_PIO_PE10) //
#define AT91C_PE10_PWM3           (AT91C_PIO_PE10) //
#define AT91C_PIO_PE11            (1 << 11) // Pin Controlled by PE11
#define AT91C_PE11_ISI_MCK        (AT91C_PIO_PE11) //
#define AT91C_PE11_PCK3           (AT91C_PIO_PE11) //
#define AT91C_PIO_PE12            (1 << 12) // Pin Controlled by PE12
#define AT91C_PE12_KBDR0          (AT91C_PIO_PE12) //
#define AT91C_PE12_ISI_D8         (AT91C_PIO_PE12) //
#define AT91C_PIO_PE13            (1 << 13) // Pin Controlled by PE13
#define AT91C_PE13_KBDR1          (AT91C_PIO_PE13) //
#define AT91C_PE13_ISI_D9         (AT91C_PIO_PE13) //
#define AT91C_PIO_PE14            (1 << 14) // Pin Controlled by PE14
#define AT91C_PE14_KBDR2          (AT91C_PIO_PE14) //
#define AT91C_PE14_ISI_D10        (AT91C_PIO_PE14) //
#define AT91C_PIO_PE15            (1 << 15) // Pin Controlled by PE15
#define AT91C_PE15_KBDR3          (AT91C_PIO_PE15) //
#define AT91C_PE15_ISI_D11        (AT91C_PIO_PE15) //
#define AT91C_PIO_PE16            (1 << 16) // Pin Controlled by PE16
#define AT91C_PE16_KBDR4          (AT91C_PIO_PE16) //
#define AT91C_PIO_PE17            (1 << 17) // Pin Controlled by PE17
#define AT91C_PE17_KBDC0          (AT91C_PIO_PE17) //
#define AT91C_PIO_PE18            (1 << 18) // Pin Controlled by PE18
#define AT91C_PE18_KBDC1          (AT91C_PIO_PE18) //
#define AT91C_PE18_TIOA0          (AT91C_PIO_PE18) //
#define AT91C_PIO_PE19            (1 << 19) // Pin Controlled by PE19
#define AT91C_PE19_KBDC2          (AT91C_PIO_PE19) //
#define AT91C_PE19_TIOB0          (AT91C_PIO_PE19) //
#define AT91C_PIO_PE2             (1 <<  2) // Pin Controlled by PE2
#define AT91C_PE2_ISI_D2          (AT91C_PIO_PE2) //
#define AT91C_PE2_DRFIN2          (AT91C_PIO_PE2) //  GPS Digital RF Input sigq_a0
#define AT91C_PIO_PE20            (1 << 20) // Pin Controlled by PE20
#define AT91C_PE20_KBDC3          (AT91C_PIO_PE20) //
#define AT91C_PE20_EBI1_NWAIT     (AT91C_PIO_PE20) //
#define AT91C_PIO_PE21            (1 << 21) // Pin Controlled by PE21
#define AT91C_PE21_ETXCK          (AT91C_PIO_PE21) //
#define AT91C_PE21_EBI1_NANDWE    (AT91C_PIO_PE21) //
#define AT91C_PIO_PE22            (1 << 22) // Pin Controlled by PE22
#define AT91C_PE22_ECRS           (AT91C_PIO_PE22) //
#define AT91C_PE22_EBI1_NCS2_NANDCS (AT91C_PIO_PE22) //
#define AT91C_PIO_PE23            (1 << 23) // Pin Controlled by PE23
#define AT91C_PE23_ETX0           (AT91C_PIO_PE23) //
#define AT91C_PE23_EBI1_NANDOE    (AT91C_PIO_PE23) //
#define AT91C_PIO_PE24            (1 << 24) // Pin Controlled by PE24
#define AT91C_PE24_ETX1           (AT91C_PIO_PE24) //
#define AT91C_PE24_EBI1_NWR3_NBS3 (AT91C_PIO_PE24) //
#define AT91C_PIO_PE25            (1 << 25) // Pin Controlled by PE25
#define AT91C_PE25_ERX0           (AT91C_PIO_PE25) //
#define AT91C_PE25_EBI1_NCS1_SDCS (AT91C_PIO_PE25) //
#define AT91C_PIO_PE26            (1 << 26) // Pin Controlled by PE26
#define AT91C_PE26_ERX1           (AT91C_PIO_PE26) //
#define AT91C_PIO_PE27            (1 << 27) // Pin Controlled by PE27
#define AT91C_PE27_ERXER          (AT91C_PIO_PE27) //
#define AT91C_PE27_EBI1_SDCKE     (AT91C_PIO_PE27) //
#define AT91C_PIO_PE28            (1 << 28) // Pin Controlled by PE28
#define AT91C_PE28_ETXEN          (AT91C_PIO_PE28) //
#define AT91C_PE28_EBI1_RAS       (AT91C_PIO_PE28) //
#define AT91C_PIO_PE29            (1 << 29) // Pin Controlled by PE29
#define AT91C_PE29_EMDC           (AT91C_PIO_PE29) //
#define AT91C_PE29_EBI1_CAS       (AT91C_PIO_PE29) //
#define AT91C_PIO_PE3             (1 <<  3) // Pin Controlled by PE3
#define AT91C_PE3_ISI_D3          (AT91C_PIO_PE3) //
#define AT91C_PE3_DRFIN3          (AT91C_PIO_PE3) //  GPS Digital RF Input sigq_a1
#define AT91C_PIO_PE30            (1 << 30) // Pin Controlled by PE30
#define AT91C_PE30_EMDIO          (AT91C_PIO_PE30) //
#define AT91C_PE30_EBI1_SDWE      (AT91C_PIO_PE30) //
#define AT91C_PIO_PE31            (1 << 31) // Pin Controlled by PE31
#define AT91C_PE31_EF100          (AT91C_PIO_PE31) //
#define AT91C_PE31_EBI1_SDA10     (AT91C_PIO_PE31) //
#define AT91C_PIO_PE4             (1 <<  4) // Pin Controlled by PE4
#define AT91C_PE4_ISI_D4          (AT91C_PIO_PE4) //
#define AT91C_PE4_DRFIN4          (AT91C_PIO_PE4) //  GPS Digital RF Input sigi_b0
#define AT91C_PIO_PE5             (1 <<  5) // Pin Controlled by PE5
#define AT91C_PE5_ISI_D5          (AT91C_PIO_PE5) //
#define AT91C_PE5_DRFIN5          (AT91C_PIO_PE5) //  GPS Digital RF Input sigi_b1
#define AT91C_PIO_PE6             (1 <<  6) // Pin Controlled by PE6
#define AT91C_PE6_ISI_D6          (AT91C_PIO_PE6) //
#define AT91C_PE6_DRFIN6          (AT91C_PIO_PE6) //  GPS Digital RF Input sigq_b0
#define AT91C_PIO_PE7             (1 <<  7) // Pin Controlled by PE7
#define AT91C_PE7_ISI_D7          (AT91C_PIO_PE7) //
#define AT91C_PE7_DRFIN7          (AT91C_PIO_PE7) //  GPS Digital RF Input sigq_b1
#define AT91C_PIO_PE8             (1 <<  8) // Pin Controlled by PE8
#define AT91C_PE8_ISI_PCK         (AT91C_PIO_PE8) //
#define AT91C_PE8_TIOA1           (AT91C_PIO_PE8) //
#define AT91C_PIO_PE9             (1 <<  9) // Pin Controlled by PE9
#define AT91C_PE9_ISI_HSYNC       (AT91C_PIO_PE9) //
#define AT91C_PE9_TIOB1           (AT91C_PIO_PE9) //

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91SAM9262
// *****************************************************************************
#define AT91C_ID_FIQ              ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS              ( 1) // System Controller
#define AT91C_ID_PIOA             ( 2) // Parallel IO Controller A
#define AT91C_ID_PIOB             ( 3) // Parallel IO Controller B
#define AT91C_ID_PIOCDE           ( 4) // Parallel IO Controller C, Parallel IO Controller D, Parallel IO Controller E
#define AT91C_ID_AES              ( 5) // Advanced Encryption Standard
#define AT91C_ID_US0              ( 7) // USART 0
#define AT91C_ID_US1              ( 8) // USART 1
#define AT91C_ID_US2              ( 9) // USART 2
#define AT91C_ID_MCI0             (10) // Multimedia Card Interface 0
#define AT91C_ID_MCI1             (11) // Multimedia Card Interface 1
#define AT91C_ID_CAN              (12) // CAN Controller
#define AT91C_ID_TWI              (13) // Two-Wire Interface
#define AT91C_ID_SPI0             (14) // Serial Peripheral Interface 0
#define AT91C_ID_SPI1             (15) // Serial Peripheral Interface 1
#define AT91C_ID_SSC0             (16) // Serial Synchronous Controller 0
#define AT91C_ID_SSC1             (17) // Serial Synchronous Controller 1
#define AT91C_ID_AC97C            (18) // AC97 Controller
#define AT91C_ID_TC012            (19) // Timer Counter 0, Timer Counter 1, Timer Counter 2
#define AT91C_ID_PWMC             (20) // PWM Controller
#define AT91C_ID_EMAC             (21) // Ethernet Mac
#define AT91C_ID_GPS              (22) // GPS engine
#define AT91C_ID_UDP              (24) // USB Device Port
#define AT91C_ID_HISI             (25) // Image Sensor Interface
#define AT91C_ID_LCDC             (26) // LCD Controller
#define AT91C_ID_DMA              (27) // DMA Controller
#define AT91C_ID_OTG              (28) // USB OTG Controller
#define AT91C_ID_UHP              (29) // USB Host Port
#define AT91C_ID_IRQ0             (30) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ID_IRQ1             (31) // Advanced Interrupt Controller (IRQ1)
#define AT91C_ALL_INT             (0xFF7FFFBF) // ALL VALID INTERRUPTS

// *****************************************************************************
//               BASE ADDRESS DEFINITIONS FOR AT91SAM9262
// *****************************************************************************
#define AT91C_BASE_SYS            (0xFFFFE000) // (SYS) Base Address
#define AT91C_BASE_EBI0           (0xFFFFE200) // (EBI0) Base Address
#define AT91C_BASE_SDRAMC0        (0xFFFFE200) // (SDRAMC0) Base Address
#define AT91C_BASE_SMC0           (0xFFFFE400) // (SMC0) Base Address
#define AT91C_BASE_EBI1           (0xFFFFE800) // (EBI1) Base Address
#define AT91C_BASE_SDRAMC1        (0xFFFFE800) // (SDRAMC1) Base Address
#define AT91C_BASE_SMC1           (0xFFFFEA00) // (SMC1) Base Address
#define AT91C_BASE_MATRIX         (0xFFFFEC00) // (MATRIX) Base Address
#define AT91C_BASE_CCFG           (0xFFFFED10) // (CCFG) Base Address
#define AT91C_BASE_PDC_DBGU       (0xFFFFEF00) // (PDC_DBGU) Base Address
#define AT91C_BASE_DBGU           (0xFFFFEE00) // (DBGU) Base Address
#define AT91C_BASE_AIC            (0xFFFFF000) // (AIC) Base Address
#define AT91C_BASE_PIOA           (0xFFFFF200) // (PIOA) Base Address
#define AT91C_BASE_PIOB           (0xFFFFF400) // (PIOB) Base Address
#define AT91C_BASE_PIOC           (0xFFFFF600) // (PIOC) Base Address
#define AT91C_BASE_PIOD           (0xFFFFF800) // (PIOD) Base Address
#define AT91C_BASE_PIOE           (0xFFFFFA00) // (PIOE) Base Address
#define AT91C_BASE_CKGR           (0xFFFFFC20) // (CKGR) Base Address
#define AT91C_BASE_PMC            (0xFFFFFC00) // (PMC) Base Address
#define AT91C_BASE_RSTC           (0xFFFFFD00) // (RSTC) Base Address
#define AT91C_BASE_SHDWC          (0xFFFFFD10) // (SHDWC) Base Address
#define AT91C_BASE_RTTC0          (0xFFFFFD20) // (RTTC0) Base Address
#define AT91C_BASE_RTTC1          (0xFFFFFD50) // (RTTC1) Base Address
#define AT91C_BASE_PITC           (0xFFFFFD30) // (PITC) Base Address
#define AT91C_BASE_WDTC           (0xFFFFFD40) // (WDTC) Base Address
#define AT91C_BASE_TC0            (0xFFF7C000) // (TC0) Base Address
#define AT91C_BASE_TC1            (0xFFF7C040) // (TC1) Base Address
#define AT91C_BASE_TC2            (0xFFF7C080) // (TC2) Base Address
#define AT91C_BASE_TCB0           (0xFFF7C000) // (TCB0) Base Address
#define AT91C_BASE_TCB1           (0xFFF7C040) // (TCB1) Base Address
#define AT91C_BASE_TCB2           (0xFFF7C080) // (TCB2) Base Address
#define AT91C_BASE_PDC_MCI0       (0xFFF80100) // (PDC_MCI0) Base Address
#define AT91C_BASE_MCI0           (0xFFF80000) // (MCI0) Base Address
#define AT91C_BASE_PDC_MCI1       (0xFFF84100) // (PDC_MCI1) Base Address
#define AT91C_BASE_MCI1           (0xFFF84000) // (MCI1) Base Address
#define AT91C_BASE_TWI            (0xFFF88000) // (TWI) Base Address
#define AT91C_BASE_PDC_US0        (0xFFF8C100) // (PDC_US0) Base Address
#define AT91C_BASE_US0            (0xFFF8C000) // (US0) Base Address
#define AT91C_BASE_PDC_US1        (0xFFF90100) // (PDC_US1) Base Address
#define AT91C_BASE_US1            (0xFFF90000) // (US1) Base Address
#define AT91C_BASE_PDC_US2        (0xFFF94100) // (PDC_US2) Base Address
#define AT91C_BASE_US2            (0xFFF94000) // (US2) Base Address
#define AT91C_BASE_PDC_SSC0       (0xFFF98100) // (PDC_SSC0) Base Address
#define AT91C_BASE_SSC0           (0xFFF98000) // (SSC0) Base Address
#define AT91C_BASE_PDC_SSC1       (0xFFF9C100) // (PDC_SSC1) Base Address
#define AT91C_BASE_SSC1           (0xFFF9C000) // (SSC1) Base Address
#define AT91C_BASE_PDC_AC97C      (0xFFFA0100) // (PDC_AC97C) Base Address
#define AT91C_BASE_AC97C          (0xFFFA0000) // (AC97C) Base Address
#define AT91C_BASE_PDC_SPI0       (0xFFFA4100) // (PDC_SPI0) Base Address
#define AT91C_BASE_SPI0           (0xFFFA4000) // (SPI0) Base Address
#define AT91C_BASE_PDC_SPI1       (0xFFFA8100) // (PDC_SPI1) Base Address
#define AT91C_BASE_SPI1           (0xFFFA8000) // (SPI1) Base Address
#define AT91C_BASE_CAN_MB0        (0xFFFAC200) // (CAN_MB0) Base Address
#define AT91C_BASE_CAN_MB1        (0xFFFAC220) // (CAN_MB1) Base Address
#define AT91C_BASE_CAN_MB2        (0xFFFAC240) // (CAN_MB2) Base Address
#define AT91C_BASE_CAN_MB3        (0xFFFAC260) // (CAN_MB3) Base Address
#define AT91C_BASE_CAN_MB4        (0xFFFAC280) // (CAN_MB4) Base Address
#define AT91C_BASE_CAN_MB5        (0xFFFAC2A0) // (CAN_MB5) Base Address
#define AT91C_BASE_CAN_MB6        (0xFFFAC2C0) // (CAN_MB6) Base Address
#define AT91C_BASE_CAN_MB7        (0xFFFAC2E0) // (CAN_MB7) Base Address
#define AT91C_BASE_CAN_MB8        (0xFFFAC300) // (CAN_MB8) Base Address
#define AT91C_BASE_CAN_MB9        (0xFFFAC320) // (CAN_MB9) Base Address
#define AT91C_BASE_CAN_MB10       (0xFFFAC340) // (CAN_MB10) Base Address
#define AT91C_BASE_CAN_MB11       (0xFFFAC360) // (CAN_MB11) Base Address
#define AT91C_BASE_CAN_MB12       (0xFFFAC380) // (CAN_MB12) Base Address
#define AT91C_BASE_CAN_MB13       (0xFFFAC3A0) // (CAN_MB13) Base Address
#define AT91C_BASE_CAN_MB14       (0xFFFAC3C0) // (CAN_MB14) Base Address
#define AT91C_BASE_CAN_MB15       (0xFFFAC3E0) // (CAN_MB15) Base Address
#define AT91C_BASE_CAN            (0xFFFAC000) // (CAN) Base Address
#define AT91C_BASE_PWMC_CH0       (0xFFFB8200) // (PWMC_CH0) Base Address
#define AT91C_BASE_PWMC_CH1       (0xFFFB8220) // (PWMC_CH1) Base Address
#define AT91C_BASE_PWMC_CH2       (0xFFFB8240) // (PWMC_CH2) Base Address
#define AT91C_BASE_PWMC_CH3       (0xFFFB8260) // (PWMC_CH3) Base Address
#define AT91C_BASE_PWMC           (0xFFFB8000) // (PWMC) Base Address
#define AT91C_BASE_MACB           (0xFFFBC000) // (MACB) Base Address
#define AT91C_BASE_LCDC           (0x00700000) // (LCDC) Base Address
#define AT91C_BASE_DMA            (0x00800000) // (DMA) Base Address
#define AT91C_BASE_OTG            (0x00900000) // (OTG) Base Address
#define AT91C_BASE_UDP            (0xFFF78000) // (UDP) Base Address
#define AT91C_BASE_UHP            (0x00A00000) // (UHP) Base Address
#define AT91C_BASE_GPS            (0xFFFC0000) // (GPS) Base Address
#define AT91C_BASE_TBOX           (0x70000000) // (TBOX) Base Address
#define AT91C_BASE_AES            (0xFFFB0000) // (AES) Base Address
#define AT91C_BASE_PDC_AES        (0xFFFB0100) // (PDC_AES) Base Address
#define AT91C_BASE_HECC0          (0xFFFFE000) // (HECC0) Base Address
#define AT91C_BASE_HECC1          (0xFFFFE600) // (HECC1) Base Address
#define AT91C_BASE_HISI           (0xFFFC4000) // (HISI) Base Address

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91SAM9262
// *****************************************************************************
// ITCM
#define AT91C_ITCM 	              (0x00100000) // Maximum ITCM Area base address
#define AT91C_ITCM_SIZE	          (0x00010000) // Maximum ITCM Area size in byte (64 Kbytes)
// DTCM
#define AT91C_DTCM 	              (0x00200000) // Maximum DTCM Area base address
#define AT91C_DTCM_SIZE	          (0x00010000) // Maximum DTCM Area size in byte (64 Kbytes)
// IRAM
#define AT91C_IRAM 	              (0x00300000) // Maximum Internal SRAM base address
#define AT91C_IRAM_SIZE	          (0x00014000) // Maximum Internal SRAM size in byte (80 Kbytes)
// IRAM_MIN
#define AT91C_IRAM_MIN	           (0x00300000) // Minimum Internal RAM base address
#define AT91C_IRAM_MIN_SIZE	      (0x00004000) // Minimum Internal RAM size in byte (16 Kbytes)
// IROM
#define AT91C_IROM 	              (0x00400000) // Internal ROM base address
#define AT91C_IROM_SIZE	          (0x00020000) // Internal ROM size in byte (128 Kbytes)
// GPS_TCM
#define AT91C_GPS_TCM	            (0x00500000) // RAM connected to GPS base address
#define AT91C_GPS_TCM_SIZE	       (0x00004000) // RAM connected to GPS size in byte (16 Kbytes)
// EBI0_CS0
#define AT91C_EBI0_CS0	           (0x10000000) // EBI0 Chip Select 0 base address
#define AT91C_EBI0_CS0_SIZE	      (0x10000000) // EBI0 Chip Select 0 size in byte (262144 Kbytes)
// EBI0_CS1
#define AT91C_EBI0_CS1	           (0x20000000) // EBI0 Chip Select 1 base address
#define AT91C_EBI0_CS1_SIZE	      (0x10000000) // EBI0 Chip Select 1 size in byte (262144 Kbytes)
// EBI0_SDRAM
#define AT91C_EBI0_SDRAM	         (0x20000000) // SDRAM on EBI0 Chip Select 1 base address
#define AT91C_EBI0_SDRAM_SIZE	    (0x10000000) // SDRAM on EBI0 Chip Select 1 size in byte (262144 Kbytes)
// EBI0_SDRAM_16BIT
#define AT91C_EBI0_SDRAM_16BIT	   (0x20000000) // SDRAM on EBI0 Chip Select 1 base address
#define AT91C_EBI0_SDRAM_16BIT_SIZE	 (0x02000000) // SDRAM on EBI0 Chip Select 1 size in byte (32768 Kbytes)
// EBI0_SDRAM_32BIT
#define AT91C_EBI0_SDRAM_32BIT	   (0x20000000) // SDRAM on EBI0 Chip Select 1 base address
#define AT91C_EBI0_SDRAM_32BIT_SIZE	 (0x04000000) // SDRAM on EBI0 Chip Select 1 size in byte (65536 Kbytes)
// EBI0_CS2
#define AT91C_EBI0_CS2	           (0x30000000) // EBI0 Chip Select 2 base address
#define AT91C_EBI0_CS2_SIZE	      (0x10000000) // EBI0 Chip Select 2 size in byte (262144 Kbytes)
// EBI0_CS3
#define AT91C_EBI0_CS3	           (0x40000000) // EBI0 Chip Select 3 base address
#define AT91C_EBI0_CS3_SIZE	      (0x10000000) // EBI0 Chip Select 3 size in byte (262144 Kbytes)
// EBI0_SM
#define AT91C_EBI0_SM	            (0x40000000) // SmartMedia on EBI0 Chip Select 3 base address
#define AT91C_EBI0_SM_SIZE	       (0x10000000) // SmartMedia on EBI0 Chip Select 3 size in byte (262144 Kbytes)
// EBI0_CS4
#define AT91C_EBI0_CS4	           (0x50000000) // EBI0 Chip Select 4 base address
#define AT91C_EBI0_CS4_SIZE	      (0x10000000) // EBI0 Chip Select 4 size in byte (262144 Kbytes)
// EBI0_CF0
#define AT91C_EBI0_CF0	           (0x50000000) // CompactFlash 0 on EBI0 Chip Select 4 base address
#define AT91C_EBI0_CF0_SIZE	      (0x10000000) // CompactFlash 0 on EBI0 Chip Select 4 size in byte (262144 Kbytes)
// EBI0_CS5
#define AT91C_EBI0_CS5	           (0x60000000) // EBI0 Chip Select 5 base address
#define AT91C_EBI0_CS5_SIZE	      (0x10000000) // EBI0 Chip Select 5 size in byte (262144 Kbytes)
// EBI0_CF1
#define AT91C_EBI0_CF1	           (0x60000000) // CompactFlash 1 on EBI0Chip Select 5 base address
#define AT91C_EBI0_CF1_SIZE	      (0x10000000) // CompactFlash 1 on EBI0Chip Select 5 size in byte (262144 Kbytes)
// EBI1_CS0
#define AT91C_EBI1_CS0	           (0x70000000) // EBI1 Chip Select 0 base address
#define AT91C_EBI1_CS0_SIZE	      (0x10000000) // EBI1 Chip Select 0 size in byte (262144 Kbytes)
// EBI1_CS1
#define AT91C_EBI1_CS1	           (0x80000000) // EBI1 Chip Select 1 base address
#define AT91C_EBI1_CS1_SIZE	      (0x10000000) // EBI1 Chip Select 1 size in byte (262144 Kbytes)
// EBI1_SDRAM_16BIT
#define AT91C_EBI1_SDRAM_16BIT	   (0x80000000) // SDRAM on EBI1 Chip Select 1 base address
#define AT91C_EBI1_SDRAM_16BIT_SIZE	 (0x02000000) // SDRAM on EBI1 Chip Select 1 size in byte (32768 Kbytes)
// EBI1_SDRAM_32BIT
#define AT91C_EBI1_SDRAM_32BIT	   (0x80000000) // SDRAM on EBI1 Chip Select 1 base address
#define AT91C_EBI1_SDRAM_32BIT_SIZE	 (0x04000000) // SDRAM on EBI1 Chip Select 1 size in byte (65536 Kbytes)
// EBI1_CS2
#define AT91C_EBI1_CS2	           (0x90000000) // EBI1 Chip Select 2 base address
#define AT91C_EBI1_CS2_SIZE	      (0x10000000) // EBI1 Chip Select 2 size in byte (262144 Kbytes)
// EBI1_SM
#define AT91C_EBI1_SM	            (0x90000000) // SmartMedia on EBI1 Chip Select 2 base address
#define AT91C_EBI1_SM_SIZE	       (0x10000000) // SmartMedia on EBI1 Chip Select 2 size in byte (262144 Kbytes)

#define AT91C_NR_PIO               (32 * 5)

#endif /* AT91SAM9263_INC_H */

