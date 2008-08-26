//  ----------------------------------------------------------------------------
//          ATMEL Microcontroller Software Support  -  ROUSSET  -
//  ----------------------------------------------------------------------------
//  DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
//  DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  ----------------------------------------------------------------------------
// File Name           : AT91SAM9RL64.h
// Object              : AT91SAM9RL64 definitions
// Generated           : AT91 SW Application Group  11/13/2006 (17:21:42)
//
// CVS Reference       : /AT91SAM9RL64.pl/1.19/Mon Nov 13 16:21:11 2006//
// CVS Reference       : /SYS_SAM9RL64.pl/1.2/Mon Nov 13 16:23:12 2006//
// CVS Reference       : /HMATRIX1_SAM9RL64.pl/1.1/Wed Sep 13 15:29:30 2006//
// CVS Reference       : /CCR_SAM9RL64.pl/1.1/Wed Sep 13 15:29:30 2006//
// CVS Reference       : /PMC_CAP9.pl/1.2/Thu Oct 26 11:26:44 2006//
// CVS Reference       : /EBI_SAM9260.pl/1.1/Fri Sep 30 11:12:14 2005//
// CVS Reference       : /HSDRAMC1_6100A.pl/1.2/Mon Aug 09 09:52:25 2004//
// CVS Reference       : /HSMC3_6105A.pl/1.4/Tue Nov 16 08:16:23 2004//
// CVS Reference       : /HECC_6143A.pl/1.1/Wed Feb 09 16:16:57 2005//
// CVS Reference       : /AIC_6075A.pl/1.1/Mon Jul 12 16:04:01 2004//
// CVS Reference       : /PDC_6074C.pl/1.2/Thu Feb 03 08:02:11 2005//
// CVS Reference       : /DBGU_6059D.pl/1.1/Mon Jan 31 12:54:41 2005//
// CVS Reference       : /PIO_6057A.pl/1.2/Thu Feb 03 09:29:42 2005//
// CVS Reference       : /RSTC_6098A.pl/1.3/Thu Nov 04 12:57:00 2004//
// CVS Reference       : /SHDWC_6122A.pl/1.3/Wed Oct 06 13:16:58 2004//
// CVS Reference       : /RTTC_6081A.pl/1.2/Thu Nov 04 12:57:22 2004//
// CVS Reference       : /PITC_6079A.pl/1.2/Thu Nov 04 12:56:22 2004//
// CVS Reference       : /WDTC_6080A.pl/1.3/Thu Nov 04 12:58:52 2004//
// CVS Reference       : /TC_6082A.pl/1.7/Wed Mar 09 15:31:51 2005//
// CVS Reference       : /MCI_6101E.pl/1.1/Fri Jun 03 12:20:23 2005//
// CVS Reference       : /TWI_6061B.pl/1.2/Tue Sep 12 12:35:28 2006//
// CVS Reference       : /US_6089J.pl/1.2/Wed Oct 11 12:26:02 2006//
// CVS Reference       : /SSC_6078B.pl/1.1/Wed Jul 13 14:25:46 2005//
// CVS Reference       : /SPI_6088D.pl/1.3/Fri May 20 13:23:02 2005//
// CVS Reference       : /AC97C_XXXX.pl/1.3/Tue Feb 22 16:08:27 2005//
// CVS Reference       : /PWM_6044D.pl/1.2/Tue May 10 11:39:09 2005//
// CVS Reference       : /LCDC_6063A.pl/1.3/Fri Dec 09 09:59:26 2005//
// CVS Reference       : /HDMA_SAM9RL64.pl/1.2/Wed Sep 06 15:25:21 2006//
// CVS Reference       : /UDPHS_SAM9265.pl/1.8/Tue Sep 12 12:35:29 2006//
// CVS Reference       : /TSC_XXXX.pl/1.1/Wed Oct 11 13:03:27 2006//
// CVS Reference       : /RTC_1245D.pl/1.3/Fri Sep 17 13:01:31 2004//
//  ----------------------------------------------------------------------------

// Hardware register definition

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR System Peripherals
// *****************************************************************************
// -------- GPBR : (SYS Offset: 0x3d50) GPBR General Purpose Register --------
#define AT91C_GPBR_GPRV           (0x0 <<  0) // (SYS) General Purpose Register Value
// -------- SLCKSEL : (SYS Offset: 0x3d60) Slow Clock Selection Register --------
#define AT91C_SLCKSEL_RCEN        (0x1 <<  0) // (SYS) Enable Internal RC Oscillator
#define AT91C_SLCKSEL_OSC32EN     (0x1 <<  1) // (SYS) Enable External Oscillator
#define AT91C_SLCKSEL_OSC32BYP    (0x1 <<  2) // (SYS) Bypass External Oscillator
#define AT91C_SLCKSEL_OSCSEL      (0x1 <<  3) // (SYS) OSC Selection

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR External Bus Interface
// *****************************************************************************
// *** Register offset in AT91S_EBI structure ***
#define EBI_DUMMY       ( 0) // Dummy register - Do not use

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
//              SOFTWARE API DEFINITION  FOR AHB Matrix Interface
// *****************************************************************************
// *** Register offset in AT91S_MATRIX structure ***
#define MATRIX_MCFG0    ( 0) //  Master Configuration Register 0 : rom
#define MATRIX_MCFG1    ( 4) //  Master Configuration Register 1 ; htcm
#define MATRIX_MCFG2    ( 8) //  Master Configuration Register 2 : lcdc
#define MATRIX_MCFG3    (12) //  Master Configuration Register 3 : usb_dev_hs
#define MATRIX_MCFG4    (16) //  Master Configuration Register 4 : ebi
#define MATRIX_MCFG5    (20) //  Master Configuration Register 5 : bridge
#define MATRIX_MCFG6    (24) //  Master Configuration Register 6
#define MATRIX_MCFG7    (28) //  Master Configuration Register 7
#define MATRIX_MCFG8    (32) //  Master Configuration Register 8
#define MATRIX_SCFG0    (64) //  Slave Configuration Register 0 : rom
#define MATRIX_SCFG1    (68) //  Slave Configuration Register 1 : htcm
#define MATRIX_SCFG2    (72) //  Slave Configuration Register 2 : lcdc
#define MATRIX_SCFG3    (76) //  Slave Configuration Register 3 : usb_dev_hs
#define MATRIX_SCFG4    (80) //  Slave Configuration Register 4 ; ebi
#define MATRIX_SCFG5    (84) //  Slave Configuration Register 5 : bridge
#define MATRIX_SCFG6    (88) //  Slave Configuration Register 6
#define MATRIX_SCFG7    (92) //  Slave Configuration Register 7
#define MATRIX_PRAS0    (128) //  PRAS0 : rom
#define MATRIX_PRBS0    (132) //  PRBS0 : rom
#define MATRIX_PRAS1    (136) //  PRAS1 : htcm
#define MATRIX_PRBS1    (140) //  PRBS1 : htcm
#define MATRIX_PRAS2    (144) //  PRAS2 : lcdc
#define MATRIX_PRBS2    (148) //  PRBS2 : lcdc
#define MATRIX_PRAS3    (152) //  PRAS3 : usb_dev_hs
#define MATRIX_PRBS3    (156) //  PRBS3 : usb_dev_hs
#define MATRIX_PRAS4    (160) //  PRAS4 : ebi
#define MATRIX_PRBS4    (164) //  PRBS4 : ebi
#define MATRIX_PRAS5    (168) //  PRAS5 : bridge
#define MATRIX_PRBS5    (172) //  PRBS5 : bridge
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
#define CCFG_UDPHS      (12) //  USB Device HS configuration
#define CCFG_EBICSA     (16) //  EBI Chip Select Assignement Register
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
// -------- CCFG_UDPHS : (CCFG Offset: 0xc) USB Device HS configuration --------
#define AT91C_CCFG_DONT_USE_UTMI_LOCK (0x1 <<  0) // (CCFG)
#define 	AT91C_CCFG_DONT_USE_UTMI_LOCK_DONT_USE_LOCK        (0x0) // (CCFG)
// -------- CCFG_EBICSA : (CCFG Offset: 0x10) EBI Chip Select Assignement Register --------
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
#define AT91C_EBI_SUPPLY          (0x1 << 16) // (CCFG) EBI supply set to 1.8

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
#define 	AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE  (0x0 <<  5) // (AIC) Internal Sources Code Label Level Sensitive
#define 	AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED   (0x1 <<  5) // (AIC) Internal Sources Code Label Edge triggered
#define 	AT91C_AIC_SRCTYPE_EXT_HIGH_LEVEL       (0x2 <<  5) // (AIC) External Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE    (0x3 <<  5) // (AIC) External Sources Code Label Positive Edge triggered
// -------- AIC_CISR : (AIC Offset: 0x114) AIC Core Interrupt Status Register --------
#define AT91C_AIC_NFIQ            (0x1 <<  0) // (AIC) NFIQ Status
#define AT91C_AIC_NIRQ            (0x1 <<  1) // (AIC) NIRQ Status
// -------- AIC_DCR : (AIC Offset: 0x138) AIC Debug Control Register (Protect) --------
#define AT91C_AIC_DCR_PROT        (0x1 <<  0) // (AIC) Protection Mode
#define AT91C_AIC_DCR_GMSK        (0x1 <<  1) // (AIC) General Mask

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
//              SOFTWARE API DEFINITION  FOR Power Management Controler
// *****************************************************************************
// *** Register offset in AT91S_PMC structure ***
#define PMC_SCER        ( 0) // System Clock Enable Register
#define PMC_SCDR        ( 4) // System Clock Disable Register
#define PMC_SCSR        ( 8) // System Clock Status Register
#define PMC_PCER        (16) // Peripheral Clock Enable Register
#define PMC_PCDR        (20) // Peripheral Clock Disable Register
#define PMC_PCSR        (24) // Peripheral Clock Status Register
#define PMC_UCKR        (28) // UTMI Clock Configuration Register
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
// -------- CKGR_UCKR : (PMC Offset: 0x1c) UTMI Clock Configuration Register --------
#define AT91C_CKGR_UPLLEN         (0x1 << 16) // (PMC) UTMI PLL Enable
#define 	AT91C_CKGR_UPLLEN_DISABLED             (0x0 << 16) // (PMC) The UTMI PLL is disabled
#define 	AT91C_CKGR_UPLLEN_ENABLED              (0x1 << 16) // (PMC) The UTMI PLL is enabled
#define AT91C_CKGR_PLLCOUNT       (0xF << 20) // (PMC) UTMI Oscillator Start-up Time
#define AT91C_CKGR_BIASEN         (0x1 << 24) // (PMC) UTMI BIAS Enable
#define 	AT91C_CKGR_BIASEN_DISABLED             (0x0 << 24) // (PMC) The UTMI BIAS is disabled
#define 	AT91C_CKGR_BIASEN_ENABLED              (0x1 << 24) // (PMC) The UTMI BIAS is enabled
#define AT91C_CKGR_BIASCOUNT      (0xF << 28) // (PMC) UTMI BIAS Start-up Time
// -------- CKGR_MOR : (PMC Offset: 0x20) Main Oscillator Register --------
#define AT91C_CKGR_MOSCEN         (0x1 <<  0) // (PMC) Main Oscillator Enable
#define AT91C_CKGR_OSCBYPASS      (0x1 <<  1) // (PMC) Main Oscillator Bypass
#define AT91C_CKGR_OSCOUNT        (0xFF <<  8) // (PMC) Main Oscillator Start-up Time
// -------- CKGR_MCFR : (PMC Offset: 0x24) Main Clock Frequency Register --------
#define AT91C_CKGR_MAINF          (0xFFFF <<  0) // (PMC) Main Clock Frequency
#define AT91C_CKGR_MAINRDY        (0x1 << 16) // (PMC) Main Clock Ready
// -------- CKGR_PLLAR : (PMC Offset: 0x28) PLL A Register --------
#define AT91C_CKGR_DIVA           (0xFF <<  0) // (PMC) Divider A Selected
#define 	AT91C_CKGR_DIVA_0                    (0x0) // (PMC) Divider A output is 0
#define 	AT91C_CKGR_DIVA_BYPASS               (0x1) // (PMC) Divider A is bypassed
#define AT91C_CKGR_PLLACOUNT      (0x3F <<  8) // (PMC) PLL A Counter
#define AT91C_CKGR_OUTA           (0x3 << 14) // (PMC) PLL A Output Frequency Range
#define 	AT91C_CKGR_OUTA_0                    (0x0 << 14) // (PMC) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_1                    (0x1 << 14) // (PMC) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_2                    (0x2 << 14) // (PMC) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_3                    (0x3 << 14) // (PMC) Please refer to the PLLA datasheet
#define AT91C_CKGR_MULA           (0x7FF << 16) // (PMC) PLL A Multiplier
#define AT91C_CKGR_SRCA           (0x1 << 29) // (PMC)
// -------- CKGR_PLLBR : (PMC Offset: 0x2c) PLL B Register --------
#define AT91C_CKGR_DIVB           (0xFF <<  0) // (PMC) Divider B Selected
#define 	AT91C_CKGR_DIVB_0                    (0x0) // (PMC) Divider B output is 0
#define 	AT91C_CKGR_DIVB_BYPASS               (0x1) // (PMC) Divider B is bypassed
#define AT91C_CKGR_PLLBCOUNT      (0x3F <<  8) // (PMC) PLL B Counter
#define AT91C_CKGR_OUTB           (0x3 << 14) // (PMC) PLL B Output Frequency Range
#define 	AT91C_CKGR_OUTB_0                    (0x0 << 14) // (PMC) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_1                    (0x1 << 14) // (PMC) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_2                    (0x2 << 14) // (PMC) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_3                    (0x3 << 14) // (PMC) Please refer to the PLLB datasheet
#define AT91C_CKGR_MULB           (0x7FF << 16) // (PMC) PLL B Multiplier
#define AT91C_CKGR_USBDIV         (0x3 << 28) // (PMC) Divider for USB Clocks
#define 	AT91C_CKGR_USBDIV_0                    (0x0 << 28) // (PMC) Divider output is PLL clock output
#define 	AT91C_CKGR_USBDIV_1                    (0x1 << 28) // (PMC) Divider output is PLL clock output divided by 2
#define 	AT91C_CKGR_USBDIV_2                    (0x2 << 28) // (PMC) Divider output is PLL clock output divided by 4
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
#define 	AT91C_PMC_MDIV_4                    (0x2 <<  8) // (PMC) The processor clock is four times faster than the master clock
// -------- PMC_PCKR : (PMC Offset: 0x40) Programmable Clock Register --------
// -------- PMC_IER : (PMC Offset: 0x60) PMC Interrupt Enable Register --------
#define AT91C_PMC_MOSCS           (0x1 <<  0) // (PMC) MOSC Status/Enable/Disable/Mask
#define AT91C_PMC_LOCKA           (0x1 <<  1) // (PMC) PLL A Status/Enable/Disable/Mask
#define AT91C_PMC_LOCKB           (0x1 <<  2) // (PMC) PLL B Status/Enable/Disable/Mask
#define AT91C_PMC_MCKRDY          (0x1 <<  3) // (PMC) Master Clock Status/Enable/Disable/Mask
#define AT91C_PMC_LOCKU           (0x1 <<  6) // (PMC) PLL UTMI Status/Enable/Disable/Mask
#define AT91C_PMC_PCK0RDY         (0x1 <<  8) // (PMC) PCK0_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK1RDY         (0x1 <<  9) // (PMC) PCK1_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK2RDY         (0x1 << 10) // (PMC) PCK2_RDY Status/Enable/Disable/Mask
#define AT91C_PMC_PCK3RDY         (0x1 << 11) // (PMC) PCK3_RDY Status/Enable/Disable/Mask
// -------- PMC_IDR : (PMC Offset: 0x64) PMC Interrupt Disable Register --------
// -------- PMC_SR : (PMC Offset: 0x68) PMC Status Register --------
// -------- PMC_IMR : (PMC Offset: 0x6c) PMC Interrupt Mask Register --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Clock Generator Controler
// *****************************************************************************
// *** Register offset in AT91S_CKGR structure ***
#define CKGR_UCKR       ( 0) // UTMI Clock Configuration Register
#define CKGR_MOR        ( 4) // Main Oscillator Register
#define CKGR_MCFR       ( 8) // Main Clock  Frequency Register
#define CKGR_PLLAR      (12) // PLL A Register
#define CKGR_PLLBR      (16) // PLL B Register
// -------- CKGR_UCKR : (CKGR Offset: 0x0) UTMI Clock Configuration Register --------
// -------- CKGR_MOR : (CKGR Offset: 0x4) Main Oscillator Register --------
// -------- CKGR_MCFR : (CKGR Offset: 0x8) Main Clock Frequency Register --------
// -------- CKGR_PLLAR : (CKGR Offset: 0xc) PLL A Register --------
// -------- CKGR_PLLBR : (CKGR Offset: 0x10) PLL B Register --------

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
#define AT91C_SHDWC_WKMODE1       (0x3 <<  8) // (SHDWC) Wake Up 1 Mode Selection
#define 	AT91C_SHDWC_WKMODE1_NONE                 (0x0 <<  8) // (SHDWC) None. No detection is performed on the wake up input.
#define 	AT91C_SHDWC_WKMODE1_HIGH                 (0x1 <<  8) // (SHDWC) High Level.
#define 	AT91C_SHDWC_WKMODE1_LOW                  (0x2 <<  8) // (SHDWC) Low Level.
#define 	AT91C_SHDWC_WKMODE1_ANYLEVEL             (0x3 <<  8) // (SHDWC) Any level change.
#define AT91C_SHDWC_CPTWK1        (0xF << 12) // (SHDWC) Counter On Wake Up 1
#define AT91C_SHDWC_RTTWKEN       (0x1 << 16) // (SHDWC) Real Time Timer Wake Up Enable
#define AT91C_SHDWC_RTCWKEN       (0x1 << 17) // (SHDWC) Real Time Clock Wake Up Enable
// -------- SHDWC_SHSR : (SHDWC Offset: 0x8) Shut Down Status Register --------
#define AT91C_SHDWC_WAKEUP0       (0x1 <<  0) // (SHDWC) Wake Up 0 Status
#define AT91C_SHDWC_WAKEUP1       (0x1 <<  1) // (SHDWC) Wake Up 1 Status
#define AT91C_SHDWC_FWKUP         (0x1 <<  2) // (SHDWC) Force Wake Up Status
#define AT91C_SHDWC_RTTWK         (0x1 << 16) // (SHDWC) Real Time Timer wake Up
#define AT91C_SHDWC_RTCWK         (0x1 << 17) // (SHDWC) Real Time Clock wake Up

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
//              SOFTWARE API DEFINITION  FOR Real-time Clock Alarm and Parallel Load Interface
// *****************************************************************************
// *** Register offset in AT91S_RTC structure ***
#define RTC_CR          ( 0) // Control Register
#define RTC_MR          ( 4) // Mode Register
#define RTC_TIMR        ( 8) // Time Register
#define RTC_CALR        (12) // Calendar Register
#define RTC_TIMALR      (16) // Time Alarm Register
#define RTC_CALALR      (20) // Calendar Alarm Register
#define RTC_SR          (24) // Status Register
#define RTC_SCCR        (28) // Status Clear Command Register
#define RTC_IER         (32) // Interrupt Enable Register
#define RTC_IDR         (36) // Interrupt Disable Register
#define RTC_IMR         (40) // Interrupt Mask Register
#define RTC_VER         (44) // Valid Entry Register
// -------- RTC_CR : (RTC Offset: 0x0) RTC Control Register --------
#define AT91C_RTC_UPDTIM          (0x1 <<  0) // (RTC) Update Request Time Register
#define AT91C_RTC_UPDCAL          (0x1 <<  1) // (RTC) Update Request Calendar Register
#define AT91C_RTC_TIMEVSEL        (0x3 <<  8) // (RTC) Time Event Selection
#define 	AT91C_RTC_TIMEVSEL_MINUTE               (0x0 <<  8) // (RTC) Minute change.
#define 	AT91C_RTC_TIMEVSEL_HOUR                 (0x1 <<  8) // (RTC) Hour change.
#define 	AT91C_RTC_TIMEVSEL_DAY24                (0x2 <<  8) // (RTC) Every day at midnight.
#define 	AT91C_RTC_TIMEVSEL_DAY12                (0x3 <<  8) // (RTC) Every day at noon.
#define AT91C_RTC_CALEVSEL        (0x3 << 16) // (RTC) Calendar Event Selection
#define 	AT91C_RTC_CALEVSEL_WEEK                 (0x0 << 16) // (RTC) Week change (every Monday at time 00:00:00).
#define 	AT91C_RTC_CALEVSEL_MONTH                (0x1 << 16) // (RTC) Month change (every 01 of each month at time 00:00:00).
#define 	AT91C_RTC_CALEVSEL_YEAR                 (0x2 << 16) // (RTC) Year change (every January 1 at time 00:00:00).
// -------- RTC_MR : (RTC Offset: 0x4) RTC Mode Register --------
#define AT91C_RTC_HRMOD           (0x1 <<  0) // (RTC) 12-24 hour Mode
// -------- RTC_TIMR : (RTC Offset: 0x8) RTC Time Register --------
#define AT91C_RTC_SEC             (0x7F <<  0) // (RTC) Current Second
#define AT91C_RTC_MIN             (0x7F <<  8) // (RTC) Current Minute
#define AT91C_RTC_HOUR            (0x3F << 16) // (RTC) Current Hour
#define AT91C_RTC_AMPM            (0x1 << 22) // (RTC) Ante Meridiem, Post Meridiem Indicator
// -------- RTC_CALR : (RTC Offset: 0xc) RTC Calendar Register --------
#define AT91C_RTC_CENT            (0x3F <<  0) // (RTC) Current Century
#define AT91C_RTC_YEAR            (0xFF <<  8) // (RTC) Current Year
#define AT91C_RTC_MONTH           (0x1F << 16) // (RTC) Current Month
#define AT91C_RTC_DAY             (0x7 << 21) // (RTC) Current Day
#define AT91C_RTC_DATE            (0x3F << 24) // (RTC) Current Date
// -------- RTC_TIMALR : (RTC Offset: 0x10) RTC Time Alarm Register --------
#define AT91C_RTC_SECEN           (0x1 <<  7) // (RTC) Second Alarm Enable
#define AT91C_RTC_MINEN           (0x1 << 15) // (RTC) Minute Alarm
#define AT91C_RTC_HOUREN          (0x1 << 23) // (RTC) Current Hour
// -------- RTC_CALALR : (RTC Offset: 0x14) RTC Calendar Alarm Register --------
#define AT91C_RTC_MONTHEN         (0x1 << 23) // (RTC) Month Alarm Enable
#define AT91C_RTC_DATEEN          (0x1 << 31) // (RTC) Date Alarm Enable
// -------- RTC_SR : (RTC Offset: 0x18) RTC Status Register --------
#define AT91C_RTC_ACKUPD          (0x1 <<  0) // (RTC) Acknowledge for Update
#define AT91C_RTC_ALARM           (0x1 <<  1) // (RTC) Alarm Flag
#define AT91C_RTC_SECEV           (0x1 <<  2) // (RTC) Second Event
#define AT91C_RTC_TIMEV           (0x1 <<  3) // (RTC) Time Event
#define AT91C_RTC_CALEV           (0x1 <<  4) // (RTC) Calendar event
// -------- RTC_SCCR : (RTC Offset: 0x1c) RTC Status Clear Command Register --------
// -------- RTC_IER : (RTC Offset: 0x20) RTC Interrupt Enable Register --------
// -------- RTC_IDR : (RTC Offset: 0x24) RTC Interrupt Disable Register --------
// -------- RTC_IMR : (RTC Offset: 0x28) RTC Interrupt Mask Register --------
// -------- RTC_VER : (RTC Offset: 0x2c) RTC Valid Entry Register --------
#define AT91C_RTC_NVTIM           (0x1 <<  0) // (RTC) Non valid Time
#define AT91C_RTC_NVCAL           (0x1 <<  1) // (RTC) Non valid Calendar
#define AT91C_RTC_NVTIMALR        (0x1 <<  2) // (RTC) Non valid time Alarm
#define AT91C_RTC_NVCALALR        (0x1 <<  3) // (RTC) Nonvalid Calendar Alarm

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
#define MCI_BLKR        (24) // MCI Block Register
#define MCI_RSPR        (32) // MCI Response Register
#define MCI_RDR         (48) // MCI Receive Data Register
#define MCI_TDR         (52) // MCI Transmit Data Register
#define MCI_SR          (64) // MCI Status Register
#define MCI_IER         (68) // MCI Interrupt Enable Register
#define MCI_IDR         (72) // MCI Interrupt Disable Register
#define MCI_IMR         (76) // MCI Interrupt Mask Register
#define MCI_VR          (252) // MCI Version Register
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
#define AT91C_MCI_RDPROOF         (0x1 << 11) // (MCI) Read Proof Enable
#define AT91C_MCI_WRPROOF         (0x1 << 12) // (MCI) Write Proof Enable
#define AT91C_MCI_PDCFBYTE        (0x1 << 13) // (MCI) PDC Force Byte Transfer
#define AT91C_MCI_PDCPADV         (0x1 << 14) // (MCI) PDC Padding Value
#define AT91C_MCI_PDCMODE         (0x1 << 15) // (MCI) PDC Oriented Mode
#define AT91C_MCI_BLKLEN          (0xFFFF << 16) // (MCI) Data Block Length
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
#define AT91C_MCI_SCDSEL          (0x3 <<  0) // (MCI) SD Card Selector
#define AT91C_MCI_SCDBUS          (0x1 <<  7) // (MCI) SDCard/SDIO Bus Width
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
#define AT91C_MCI_TRTYP           (0x7 << 19) // (MCI) Transfer Type
#define 	AT91C_MCI_TRTYP_BLOCK                (0x0 << 19) // (MCI) MMC/SDCard Single Block Transfer type
#define 	AT91C_MCI_TRTYP_MULTIPLE             (0x1 << 19) // (MCI) MMC/SDCard Multiple Block transfer type
#define 	AT91C_MCI_TRTYP_STREAM               (0x2 << 19) // (MCI) MMC Stream transfer type
#define 	AT91C_MCI_TRTYP_SDIO_BYTE            (0x4 << 19) // (MCI) SDIO Byte transfer type
#define 	AT91C_MCI_TRTYP_SDIO_BLOCK           (0x5 << 19) // (MCI) SDIO Block transfer type
#define AT91C_MCI_IOSPCMD         (0x3 << 24) // (MCI) SDIO Special Command
#define 	AT91C_MCI_IOSPCMD_NONE                 (0x0 << 24) // (MCI) NOT a special command
#define 	AT91C_MCI_IOSPCMD_SUSPEND              (0x1 << 24) // (MCI) SDIO Suspend Command
#define 	AT91C_MCI_IOSPCMD_RESUME               (0x2 << 24) // (MCI) SDIO Resume Command
// -------- MCI_BLKR : (MCI Offset: 0x18) MCI Block Register --------
#define AT91C_MCI_BCNT            (0xFFFF <<  0) // (MCI) MMC/SDIO Block Count / SDIO Byte Count
// -------- MCI_SR : (MCI Offset: 0x40) MCI Status Register --------
#define AT91C_MCI_CMDRDY          (0x1 <<  0) // (MCI) Command Ready flag
#define AT91C_MCI_RXRDY           (0x1 <<  1) // (MCI) RX Ready flag
#define AT91C_MCI_TXRDY           (0x1 <<  2) // (MCI) TX Ready flag
#define AT91C_MCI_BLKE            (0x1 <<  3) // (MCI) Data Block Transfer Ended flag
#define AT91C_MCI_DTIP            (0x1 <<  4) // (MCI) Data Transfer in Progress flag
#define AT91C_MCI_NOTBUSY         (0x1 <<  5) // (MCI) Data Line Not Busy flag
#define AT91C_MCI_ENDRX           (0x1 <<  6) // (MCI) End of RX Buffer flag
#define AT91C_MCI_ENDTX           (0x1 <<  7) // (MCI) End of TX Buffer flag
#define AT91C_MCI_SDIOIRQA        (0x1 <<  8) // (MCI) SDIO Interrupt for Slot A
#define AT91C_MCI_SDIOIRQB        (0x1 <<  9) // (MCI) SDIO Interrupt for Slot B
#define AT91C_MCI_SDIOIRQC        (0x1 << 10) // (MCI) SDIO Interrupt for Slot C
#define AT91C_MCI_SDIOIRQD        (0x1 << 11) // (MCI) SDIO Interrupt for Slot D
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
#define TWI_SMR         ( 8) // Slave Mode Register
#define TWI_IADR        (12) // Internal Address Register
#define TWI_CWGR        (16) // Clock Waveform Generator Register
#define TWI_SR          (32) // Status Register
#define TWI_IER         (36) // Interrupt Enable Register
#define TWI_IDR         (40) // Interrupt Disable Register
#define TWI_IMR         (44) // Interrupt Mask Register
#define TWI_RHR         (48) // Receive Holding Register
#define TWI_THR         (52) // Transmit Holding Register
#define TWI_RPR         (256) // Receive Pointer Register
#define TWI_RCR         (260) // Receive Counter Register
#define TWI_TPR         (264) // Transmit Pointer Register
#define TWI_TCR         (268) // Transmit Counter Register
#define TWI_RNPR        (272) // Receive Next Pointer Register
#define TWI_RNCR        (276) // Receive Next Counter Register
#define TWI_TNPR        (280) // Transmit Next Pointer Register
#define TWI_TNCR        (284) // Transmit Next Counter Register
#define TWI_PTCR        (288) // PDC Transfer Control Register
#define TWI_PTSR        (292) // PDC Transfer Status Register
// -------- TWI_CR : (TWI Offset: 0x0) TWI Control Register --------
#define AT91C_TWI_START           (0x1 <<  0) // (TWI) Send a START Condition
#define AT91C_TWI_STOP            (0x1 <<  1) // (TWI) Send a STOP Condition
#define AT91C_TWI_MSEN            (0x1 <<  2) // (TWI) TWI Master Transfer Enabled
#define AT91C_TWI_MSDIS           (0x1 <<  3) // (TWI) TWI Master Transfer Disabled
#define AT91C_TWI_SVEN            (0x1 <<  4) // (TWI) TWI Slave mode Enabled
#define AT91C_TWI_SVDIS           (0x1 <<  5) // (TWI) TWI Slave mode Disabled
#define AT91C_TWI_SWRST           (0x1 <<  7) // (TWI) Software Reset
// -------- TWI_MMR : (TWI Offset: 0x4) TWI Master Mode Register --------
#define AT91C_TWI_IADRSZ          (0x3 <<  8) // (TWI) Internal Device Address Size
#define 	AT91C_TWI_IADRSZ_NO                   (0x0 <<  8) // (TWI) No internal device address
#define 	AT91C_TWI_IADRSZ_1_BYTE               (0x1 <<  8) // (TWI) One-byte internal device address
#define 	AT91C_TWI_IADRSZ_2_BYTE               (0x2 <<  8) // (TWI) Two-byte internal device address
#define 	AT91C_TWI_IADRSZ_3_BYTE               (0x3 <<  8) // (TWI) Three-byte internal device address
#define AT91C_TWI_MREAD           (0x1 << 12) // (TWI) Master Read Direction
#define AT91C_TWI_DADR            (0x7F << 16) // (TWI) Device Address
// -------- TWI_SMR : (TWI Offset: 0x8) TWI Slave Mode Register --------
#define AT91C_TWI_SADR            (0x7F << 16) // (TWI) Slave Address
// -------- TWI_CWGR : (TWI Offset: 0x10) TWI Clock Waveform Generator Register --------
#define AT91C_TWI_CLDIV           (0xFF <<  0) // (TWI) Clock Low Divider
#define AT91C_TWI_CHDIV           (0xFF <<  8) // (TWI) Clock High Divider
#define AT91C_TWI_CKDIV           (0x7 << 16) // (TWI) Clock Divider
// -------- TWI_SR : (TWI Offset: 0x20) TWI Status Register --------
#define AT91C_TWI_TXCOMP_SLAVE    (0x1 <<  0) // (TWI) Transmission Completed
#define AT91C_TWI_TXCOMP_MASTER   (0x1 <<  0) // (TWI) Transmission Completed
#define AT91C_TWI_RXRDY           (0x1 <<  1) // (TWI) Receive holding register ReaDY
#define AT91C_TWI_TXRDY_MASTER    (0x1 <<  2) // (TWI) Transmit holding register ReaDY
#define AT91C_TWI_TXRDY_SLAVE     (0x1 <<  2) // (TWI) Transmit holding register ReaDY
#define AT91C_TWI_SVREAD          (0x1 <<  3) // (TWI) Slave READ (used only in Slave mode)
#define AT91C_TWI_SVACC           (0x1 <<  4) // (TWI) Slave ACCess (used only in Slave mode)
#define AT91C_TWI_GACC            (0x1 <<  5) // (TWI) General Call ACcess (used only in Slave mode)
#define AT91C_TWI_OVRE            (0x1 <<  6) // (TWI) Overrun Error (used only in Master and Multi-master mode)
#define AT91C_TWI_NACK_MASTER     (0x1 <<  8) // (TWI) Not Acknowledged
#define AT91C_TWI_NACK_SLAVE      (0x1 <<  8) // (TWI) Not Acknowledged
#define AT91C_TWI_ARBLST_MULTI_MASTER (0x1 <<  9) // (TWI) Arbitration Lost (used only in Multimaster mode)
#define AT91C_TWI_SCLWS           (0x1 << 10) // (TWI) Clock Wait State (used only in Slave mode)
#define AT91C_TWI_EOSACC          (0x1 << 11) // (TWI) End Of Slave ACCess (used only in Slave mode)
#define AT91C_TWI_ENDRX           (0x1 << 12) // (TWI) End of Receiver Transfer
#define AT91C_TWI_ENDTX           (0x1 << 13) // (TWI) End of Receiver Transfer
#define AT91C_TWI_RXBUFF          (0x1 << 14) // (TWI) RXBUFF Interrupt
#define AT91C_TWI_TXBUFE          (0x1 << 15) // (TWI) TXBUFE Interrupt
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
#define US_MAN          (80) // Manchester Encoder Decoder Register
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
#define AT91C_US_VAR_SYNC         (0x1 << 22) // (USART) Variable synchronization of command/data sync Start Frame Delimiter
#define AT91C_US_MAX_ITER         (0x1 << 24) // (USART) Number of Repetitions
#define AT91C_US_FILTER           (0x1 << 28) // (USART) Receive Line Filter
#define AT91C_US_MANMODE          (0x1 << 29) // (USART) Manchester Encoder/Decoder Enable
#define AT91C_US_MODSYNC          (0x1 << 30) // (USART) Manchester Synchronization mode
#define AT91C_US_ONEBIT           (0x1 << 31) // (USART) Start Frame Delimiter selector
// -------- US_IER : (USART Offset: 0x8) Debug Unit Interrupt Enable Register --------
#define AT91C_US_RXBRK            (0x1 <<  2) // (USART) Break Received/End of Break
#define AT91C_US_TIMEOUT          (0x1 <<  8) // (USART) Receiver Time-out
#define AT91C_US_ITERATION        (0x1 << 10) // (USART) Max number of Repetitions Reached
#define AT91C_US_NACK             (0x1 << 13) // (USART) Non Acknowledge
#define AT91C_US_RIIC             (0x1 << 16) // (USART) Ring INdicator Input Change Flag
#define AT91C_US_DSRIC            (0x1 << 17) // (USART) Data Set Ready Input Change Flag
#define AT91C_US_DCDIC            (0x1 << 18) // (USART) Data Carrier Flag
#define AT91C_US_CTSIC            (0x1 << 19) // (USART) Clear To Send Input Change Flag
#define AT91C_US_MANE             (0x1 << 20) // (USART) Manchester Error Interrupt
// -------- US_IDR : (USART Offset: 0xc) Debug Unit Interrupt Disable Register --------
// -------- US_IMR : (USART Offset: 0x10) Debug Unit Interrupt Mask Register --------
// -------- US_CSR : (USART Offset: 0x14) Debug Unit Channel Status Register --------
#define AT91C_US_RI               (0x1 << 20) // (USART) Image of RI Input
#define AT91C_US_DSR              (0x1 << 21) // (USART) Image of DSR Input
#define AT91C_US_DCD              (0x1 << 22) // (USART) Image of DCD Input
#define AT91C_US_CTS              (0x1 << 23) // (USART) Image of CTS Input
#define AT91C_US_MANERR           (0x1 << 24) // (USART) Manchester Error
// -------- US_MAN : (USART Offset: 0x50) Manchester Encoder Decoder Register --------
#define AT91C_US_TX_PL            (0xF <<  0) // (USART) Transmitter Preamble Length
#define AT91C_US_TX_PP            (0x3 <<  8) // (USART) Transmitter Preamble Pattern
#define 	AT91C_US_TX_PP_ALL_ONE              (0x0 <<  8) // (USART) ALL_ONE
#define 	AT91C_US_TX_PP_ALL_ZERO             (0x1 <<  8) // (USART) ALL_ZERO
#define 	AT91C_US_TX_PP_ZERO_ONE             (0x2 <<  8) // (USART) ZERO_ONE
#define 	AT91C_US_TX_PP_ONE_ZERO             (0x3 <<  8) // (USART) ONE_ZERO
#define AT91C_US_TX_MPOL          (0x1 << 12) // (USART) Transmitter Manchester Polarity
#define AT91C_US_RX_PL            (0xF << 16) // (USART) Receiver Preamble Length
#define AT91C_US_RX_PP            (0x3 << 24) // (USART) Receiver Preamble Pattern detected
#define 	AT91C_US_RX_PP_ALL_ONE              (0x0 << 24) // (USART) ALL_ONE
#define 	AT91C_US_RX_PP_ALL_ZERO             (0x1 << 24) // (USART) ALL_ZERO
#define 	AT91C_US_RX_PP_ZERO_ONE             (0x2 << 24) // (USART) ZERO_ONE
#define 	AT91C_US_RX_PP_ONE_ZERO             (0x3 << 24) // (USART) ONE_ZERO
#define AT91C_US_RX_MPOL          (0x1 << 28) // (USART) Receiver Manchester Polarity
#define AT91C_US_DRIFT            (0x1 << 30) // (USART) Drift compensation

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
//              SOFTWARE API DEFINITION  FOR
// *****************************************************************************
// *** Register offset in AT91S_TSC structure ***
#define TSC_RPR         (256) // Receive Pointer Register
#define TSC_RCR         (260) // Receive Counter Register
#define TSC_TPR         (264) // Transmit Pointer Register
#define TSC_TCR         (268) // Transmit Counter Register
#define TSC_RNPR        (272) // Receive Next Pointer Register
#define TSC_RNCR        (276) // Receive Next Counter Register
#define TSC_TNPR        (280) // Transmit Next Pointer Register
#define TSC_TNCR        (284) // Transmit Next Counter Register
#define TSC_PTCR        (288) // PDC Transfer Control Register
#define TSC_PTSR        (292) // PDC Transfer Status Register

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR UDPHS Enpoint FIFO data register
// *****************************************************************************
// *** Register offset in AT91S_UDPHS_EPTFIFO structure ***
#define UDPHS_READEPT0  ( 0) // FIFO Endpoint Data Register 0
#define UDPHS_READEPT1  (65536) // FIFO Endpoint Data Register 1
#define UDPHS_READEPT2  (131072) // FIFO Endpoint Data Register 2
#define UDPHS_READEPT3  (196608) // FIFO Endpoint Data Register 3
#define UDPHS_READEPT4  (262144) // FIFO Endpoint Data Register 4
#define UDPHS_READEPT5  (327680) // FIFO Endpoint Data Register 5
#define UDPHS_READEPT6  (393216) // FIFO Endpoint Data Register 6
#define UDPHS_READEPT7  (458752) // FIFO Endpoint Data Register 7
#define UDPHS_READEPT8  (524288) // FIFO Endpoint Data Register 8
#define UDPHS_READEPT9  (589824) // FIFO Endpoint Data Register 9
#define UDPHS_READEPTA  (655360) // FIFO Endpoint Data Register 10
#define UDPHS_READEPTB  (720896) // FIFO Endpoint Data Register 11
#define UDPHS_READEPTC  (786432) // FIFO Endpoint Data Register 12
#define UDPHS_READEPTD  (851968) // FIFO Endpoint Data Register 13
#define UDPHS_READEPTE  (917504) // FIFO Endpoint Data Register 14
#define UDPHS_READEPTF  (983040) // FIFO Endpoint Data Register 15

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR UDPHS Endpoint struct
// *****************************************************************************
// *** Register offset in AT91S_UDPHS_EPT structure ***
#define UDPHS_EPTCFG    ( 0) // UDPHS Endpoint Config Register
#define UDPHS_EPTCTLENB ( 4) // UDPHS Endpoint Control Enable Register
#define UDPHS_EPTCTLDIS ( 8) // UDPHS Endpoint Control Disable Register
#define UDPHS_EPTCTL    (12) // UDPHS Endpoint Control Register
#define UDPHS_EPTSETSTA (20) // UDPHS Endpoint Set Status Register
#define UDPHS_EPTCLRSTA (24) // UDPHS Endpoint Clear Status Register
#define UDPHS_EPTSTA    (28) // UDPHS Endpoint Status Register
// -------- UDPHS_EPTCFG : (UDPHS_EPT Offset: 0x0) UDPHS Endpoint Config Register --------
#define AT91C_UDPHS_EPT_SIZE      (0x7 <<  0) // (UDPHS_EPT) Endpoint Size
#define 	AT91C_UDPHS_EPT_SIZE_8                    (0x0) // (UDPHS_EPT)    8 bytes
#define 	AT91C_UDPHS_EPT_SIZE_16                   (0x1) // (UDPHS_EPT)   16 bytes
#define 	AT91C_UDPHS_EPT_SIZE_32                   (0x2) // (UDPHS_EPT)   32 bytes
#define 	AT91C_UDPHS_EPT_SIZE_64                   (0x3) // (UDPHS_EPT)   64 bytes
#define 	AT91C_UDPHS_EPT_SIZE_128                  (0x4) // (UDPHS_EPT)  128 bytes
#define 	AT91C_UDPHS_EPT_SIZE_256                  (0x5) // (UDPHS_EPT)  256 bytes
#define 	AT91C_UDPHS_EPT_SIZE_512                  (0x6) // (UDPHS_EPT)  512 bytes
#define 	AT91C_UDPHS_EPT_SIZE_1024                 (0x7) // (UDPHS_EPT) 1024 bytes
#define AT91C_UDPHS_EPT_DIR       (0x1 <<  3) // (UDPHS_EPT) Endpoint Direction 0:OUT, 1:IN
#define 	AT91C_UDPHS_EPT_DIR_OUT                  (0x0 <<  3) // (UDPHS_EPT) Direction OUT
#define 	AT91C_UDPHS_EPT_DIR_IN                   (0x1 <<  3) // (UDPHS_EPT) Direction IN
#define AT91C_UDPHS_EPT_TYPE      (0x3 <<  4) // (UDPHS_EPT) Endpoint Type
#define 	AT91C_UDPHS_EPT_TYPE_CTL_EPT              (0x0 <<  4) // (UDPHS_EPT) Control endpoint
#define 	AT91C_UDPHS_EPT_TYPE_ISO_EPT              (0x1 <<  4) // (UDPHS_EPT) Isochronous endpoint
#define 	AT91C_UDPHS_EPT_TYPE_BUL_EPT              (0x2 <<  4) // (UDPHS_EPT) Bulk endpoint
#define 	AT91C_UDPHS_EPT_TYPE_INT_EPT              (0x3 <<  4) // (UDPHS_EPT) Interrupt endpoint
#define AT91C_UDPHS_BK_NUMBER     (0x3 <<  6) // (UDPHS_EPT) Number of Banks
#define 	AT91C_UDPHS_BK_NUMBER_0                    (0x0 <<  6) // (UDPHS_EPT) Zero Bank, the EndPoint is not mapped in memory
#define 	AT91C_UDPHS_BK_NUMBER_1                    (0x1 <<  6) // (UDPHS_EPT) One Bank (Bank0)
#define 	AT91C_UDPHS_BK_NUMBER_2                    (0x2 <<  6) // (UDPHS_EPT) Double bank (Ping-Pong : Bank0 / Bank1)
#define 	AT91C_UDPHS_BK_NUMBER_3                    (0x3 <<  6) // (UDPHS_EPT) Triple Bank (Bank0 / Bank1 / Bank2)
#define AT91C_UDPHS_NB_TRANS      (0x3 <<  8) // (UDPHS_EPT) Number Of Transaction per Micro-Frame (High-Bandwidth iso only)
#define AT91C_UDPHS_EPT_MAPD      (0x1 << 31) // (UDPHS_EPT) Endpoint Mapped (read only
// -------- UDPHS_EPTCTLENB : (UDPHS_EPT Offset: 0x4) UDPHS Endpoint Control Enable Register --------
#define AT91C_UDPHS_EPT_ENABL     (0x1 <<  0) // (UDPHS_EPT) Endpoint Enable
#define AT91C_UDPHS_AUTO_VALID    (0x1 <<  1) // (UDPHS_EPT) Packet Auto-Valid Enable/Disable
#define AT91C_UDPHS_INTDIS_DMA    (0x1 <<  3) // (UDPHS_EPT) Endpoint Interrupts DMA Request Enable/Disable
#define AT91C_UDPHS_NYET_DIS      (0x1 <<  4) // (UDPHS_EPT) NYET Enable/Disable
#define AT91C_UDPHS_DATAX_RX      (0x1 <<  6) // (UDPHS_EPT) DATAx Interrupt Enable/Disable
#define AT91C_UDPHS_MDATA_RX      (0x1 <<  7) // (UDPHS_EPT) MDATA Interrupt Enabled/Disable
#define AT91C_UDPHS_ERR_OVFLW     (0x1 <<  8) // (UDPHS_EPT) OverFlow Error Interrupt Enable/Disable/Status
#define AT91C_UDPHS_RX_BK_RDY     (0x1 <<  9) // (UDPHS_EPT) Received OUT Data
#define AT91C_UDPHS_TX_COMPLT     (0x1 << 10) // (UDPHS_EPT) Transmitted IN Data Complete Interrupt Enable/Disable or Transmitted IN Data Complete (clear)
#define AT91C_UDPHS_TX_PK_RDY     (0x1 << 11) // (UDPHS_EPT) TX Packet Ready Interrupt Enable/Disable
#define AT91C_UDPHS_ERR_TRANS     (0x1 << 11) // (UDPHS_EPT) Transaction Error Interrupt Enable/Disable
#define AT91C_UDPHS_RX_SETUP      (0x1 << 12) // (UDPHS_EPT) Received SETUP Interrupt Enable/Disable
#define AT91C_UDPHS_ERR_FL_ISO    (0x1 << 12) // (UDPHS_EPT) Error Flow Clear/Interrupt Enable/Disable
#define AT91C_UDPHS_STALL_SNT     (0x1 << 13) // (UDPHS_EPT) Stall Sent Clear
#define AT91C_UDPHS_ERR_CRISO     (0x1 << 13) // (UDPHS_EPT) CRC error / Error NB Trans / Interrupt Enable/Disable
#define AT91C_UDPHS_NAK_IN        (0x1 << 14) // (UDPHS_EPT) NAKIN ERROR FLUSH / Clear / Interrupt Enable/Disable
#define AT91C_UDPHS_NAK_OUT       (0x1 << 15) // (UDPHS_EPT) NAKOUT / Clear / Interrupt Enable/Disable
#define AT91C_UDPHS_BUSY_BANK     (0x1 << 18) // (UDPHS_EPT) Busy Bank Interrupt Enable/Disable
#define AT91C_UDPHS_SHRT_PCKT     (0x1 << 31) // (UDPHS_EPT) Short Packet / Interrupt Enable/Disable
// -------- UDPHS_EPTCTLDIS : (UDPHS_EPT Offset: 0x8) UDPHS Endpoint Control Disable Register --------
#define AT91C_UDPHS_EPT_DISABL    (0x1 <<  0) // (UDPHS_EPT) Endpoint Disable
// -------- UDPHS_EPTCTL : (UDPHS_EPT Offset: 0xc) UDPHS Endpoint Control Register --------
// -------- UDPHS_EPTSETSTA : (UDPHS_EPT Offset: 0x14) UDPHS Endpoint Set Status Register --------
#define AT91C_UDPHS_FRCESTALL     (0x1 <<  5) // (UDPHS_EPT) Stall Handshake Request Set/Clear/Status
#define AT91C_UDPHS_KILL_BANK     (0x1 <<  9) // (UDPHS_EPT) KILL Bank
// -------- UDPHS_EPTCLRSTA : (UDPHS_EPT Offset: 0x18) UDPHS Endpoint Clear Status Register --------
#define AT91C_UDPHS_TOGGLESQ      (0x1 <<  6) // (UDPHS_EPT) Data Toggle Clear
// -------- UDPHS_EPTSTA : (UDPHS_EPT Offset: 0x1c) UDPHS Endpoint Status Register --------
#define AT91C_UDPHS_TOGGLESQ_STA  (0x3 <<  6) // (UDPHS_EPT) Toggle Sequencing
#define 	AT91C_UDPHS_TOGGLESQ_STA_00                   (0x0 <<  6) // (UDPHS_EPT) Data0
#define 	AT91C_UDPHS_TOGGLESQ_STA_01                   (0x1 <<  6) // (UDPHS_EPT) Data1
#define 	AT91C_UDPHS_TOGGLESQ_STA_10                   (0x2 <<  6) // (UDPHS_EPT) Data2 (only for High-Bandwidth Isochronous EndPoint)
#define 	AT91C_UDPHS_TOGGLESQ_STA_11                   (0x3 <<  6) // (UDPHS_EPT) MData (only for High-Bandwidth Isochronous EndPoint)
#define AT91C_UDPHS_CURRENT_BANK  (0x3 << 16) // (UDPHS_EPT)
#define 	AT91C_UDPHS_CURRENT_BANK_00                   (0x0 << 16) // (UDPHS_EPT) Bank 0
#define 	AT91C_UDPHS_CURRENT_BANK_01                   (0x1 << 16) // (UDPHS_EPT) Bank 1
#define 	AT91C_UDPHS_CURRENT_BANK_10                   (0x2 << 16) // (UDPHS_EPT) Bank 2
#define 	AT91C_UDPHS_CURRENT_BANK_11                   (0x3 << 16) // (UDPHS_EPT) Invalid
#define AT91C_UDPHS_CONTROL_DIR   (0x3 << 16) // (UDPHS_EPT)
#define 	AT91C_UDPHS_CONTROL_DIR_00                   (0x0 << 16) // (UDPHS_EPT) Bank 0
#define 	AT91C_UDPHS_CONTROL_DIR_01                   (0x1 << 16) // (UDPHS_EPT) Bank 1
#define 	AT91C_UDPHS_CONTROL_DIR_10                   (0x2 << 16) // (UDPHS_EPT) Bank 2
#define 	AT91C_UDPHS_CONTROL_DIR_11                   (0x3 << 16) // (UDPHS_EPT) Invalid
#define AT91C_UDPHS_BUSY_BANK_STA (0x3 << 18) // (UDPHS_EPT) Busy Bank Number
#define 	AT91C_UDPHS_BUSY_BANK_STA_00                   (0x0 << 18) // (UDPHS_EPT) All banks are free
#define 	AT91C_UDPHS_BUSY_BANK_STA_01                   (0x1 << 18) // (UDPHS_EPT) 1 busy bank
#define 	AT91C_UDPHS_BUSY_BANK_STA_10                   (0x2 << 18) // (UDPHS_EPT) 2 busy banks
#define 	AT91C_UDPHS_BUSY_BANK_STA_11                   (0x3 << 18) // (UDPHS_EPT) 3 busy banks
#define AT91C_UDPHS_BYTE_COUNT    (0x7FF << 20) // (UDPHS_EPT) UDPHS Byte Count

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR UDPHS DMA struct
// *****************************************************************************
// *** Register offset in AT91S_UDPHS_DMA structure ***
#define UDPHS_DMANXTDSC ( 0) // UDPHS DMA Channel Next Descriptor Address
#define UDPHS_DMAADDRESS ( 4) // UDPHS DMA Channel Address Register
#define UDPHS_DMACONTROL ( 8) // UDPHS DMA Channel Control Register
#define UDPHS_DMASTATUS (12) // UDPHS DMA Channel Status Register
// -------- UDPHS_DMANXTDSC : (UDPHS_DMA Offset: 0x0) UDPHS DMA Next Descriptor Address Register --------
#define AT91C_UDPHS_NXT_DSC_ADD   (0xFFFFFFF <<  4) // (UDPHS_DMA) next Channel Descriptor
// -------- UDPHS_DMAADDRESS : (UDPHS_DMA Offset: 0x4) UDPHS DMA Channel Address Register --------
#define AT91C_UDPHS_BUFF_ADD      (0x0 <<  0) // (UDPHS_DMA) starting address of a DMA Channel transfer
// -------- UDPHS_DMACONTROL : (UDPHS_DMA Offset: 0x8) UDPHS DMA Channel Control Register --------
#define AT91C_UDPHS_CHANN_ENB     (0x1 <<  0) // (UDPHS_DMA) Channel Enabled
#define AT91C_UDPHS_LDNXT_DSC     (0x1 <<  1) // (UDPHS_DMA) Load Next Channel Transfer Descriptor Enable
#define AT91C_UDPHS_END_TR_EN     (0x1 <<  2) // (UDPHS_DMA) Buffer Close Input Enable
#define AT91C_UDPHS_END_B_EN      (0x1 <<  3) // (UDPHS_DMA) End of DMA Buffer Packet Validation
#define AT91C_UDPHS_END_TR_IT     (0x1 <<  4) // (UDPHS_DMA) End Of Transfer Interrupt Enable
#define AT91C_UDPHS_END_BUFFIT    (0x1 <<  5) // (UDPHS_DMA) End Of Channel Buffer Interrupt Enable
#define AT91C_UDPHS_DESC_LD_IT    (0x1 <<  6) // (UDPHS_DMA) Descriptor Loaded Interrupt Enable
#define AT91C_UDPHS_BURST_LCK     (0x1 <<  7) // (UDPHS_DMA) Burst Lock Enable
#define AT91C_UDPHS_BUFF_LENGTH   (0xFFFF << 16) // (UDPHS_DMA) Buffer Byte Length (write only)
// -------- UDPHS_DMASTATUS : (UDPHS_DMA Offset: 0xc) UDPHS DMA Channelx Status Register --------
#define AT91C_UDPHS_CHANN_ACT     (0x1 <<  1) // (UDPHS_DMA)
#define AT91C_UDPHS_END_TR_ST     (0x1 <<  4) // (UDPHS_DMA)
#define AT91C_UDPHS_END_BF_ST     (0x1 <<  5) // (UDPHS_DMA)
#define AT91C_UDPHS_DESC_LDST     (0x1 <<  6) // (UDPHS_DMA)
#define AT91C_UDPHS_BUFF_COUNT    (0xFFFF << 16) // (UDPHS_DMA)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR UDPHS High Speed Device Interface
// *****************************************************************************
// *** Register offset in AT91S_UDPHS structure ***
#define UDPHS_CTRL      ( 0) // UDPHS Control Register
#define UDPHS_FNUM      ( 4) // UDPHS Frame Number Register
#define UDPHS_IEN       (16) // UDPHS Interrupt Enable Register
#define UDPHS_INTSTA    (20) // UDPHS Interrupt Status Register
#define UDPHS_CLRINT    (24) // UDPHS Clear Interrupt Register
#define UDPHS_EPTRST    (28) // UDPHS Endpoints Reset Register
#define UDPHS_TSTSOFCNT (208) // UDPHS Test SOF Counter Register
#define UDPHS_TSTCNTA   (212) // UDPHS Test A Counter Register
#define UDPHS_TSTCNTB   (216) // UDPHS Test B Counter Register
#define UDPHS_TSTMODREG (220) // UDPHS Test Mode Register
#define UDPHS_TST       (224) // UDPHS Test Register
#define UDPHS_RIPPADDRSIZE (236) // UDPHS PADDRSIZE Register
#define UDPHS_RIPNAME1  (240) // UDPHS Name1 Register
#define UDPHS_RIPNAME2  (244) // UDPHS Name2 Register
#define UDPHS_IPFEATURES (248) // UDPHS Features Register
#define UDPHS_IPVERSION (252) // UDPHS Version Register
#define UDPHS_EPT       (256) // UDPHS Endpoint struct
#define UDPHS_DMA       (768) // UDPHS DMA channel struct (not use [0])
// -------- UDPHS_CTRL : (UDPHS Offset: 0x0) UDPHS Control Register --------
#define AT91C_UDPHS_DEV_ADDR      (0x7F <<  0) // (UDPHS) UDPHS Address
#define AT91C_UDPHS_FADDR_EN      (0x1 <<  7) // (UDPHS) Function Address Enable
#define AT91C_UDPHS_EN_UDPHS      (0x1 <<  8) // (UDPHS) UDPHS Enable
#define AT91C_UDPHS_DETACH        (0x1 <<  9) // (UDPHS) Detach Command
#define AT91C_UDPHS_REWAKEUP      (0x1 << 10) // (UDPHS) Send Remote Wake Up
#define AT91C_UDPHS_PULLD_DIS     (0x1 << 11) // (UDPHS) PullDown Disable
// -------- UDPHS_FNUM : (UDPHS Offset: 0x4) UDPHS Frame Number Register --------
#define AT91C_UDPHS_MICRO_FRAME_NUM (0x7 <<  0) // (UDPHS) Micro Frame Number
#define AT91C_UDPHS_FRAME_NUMBER  (0x7FF <<  3) // (UDPHS) Frame Number as defined in the Packet Field Formats
#define AT91C_UDPHS_FNUM_ERR      (0x1 << 31) // (UDPHS) Frame Number CRC Error
// -------- UDPHS_IEN : (UDPHS Offset: 0x10) UDPHS Interrupt Enable Register --------
#define AT91C_UDPHS_DET_SUSPD     (0x1 <<  1) // (UDPHS) Suspend Interrupt Enable/Clear/Status
#define AT91C_UDPHS_MICRO_SOF     (0x1 <<  2) // (UDPHS) Micro-SOF Interrupt Enable/Clear/Status
#define AT91C_UDPHS_IEN_SOF       (0x1 <<  3) // (UDPHS) SOF Interrupt Enable/Clear/Status
#define AT91C_UDPHS_ENDRESET      (0x1 <<  4) // (UDPHS) End Of Reset Interrupt Enable/Clear/Status
#define AT91C_UDPHS_WAKE_UP       (0x1 <<  5) // (UDPHS) Wake Up CPU Interrupt Enable/Clear/Status
#define AT91C_UDPHS_ENDOFRSM      (0x1 <<  6) // (UDPHS) End Of Resume Interrupt Enable/Clear/Status
#define AT91C_UDPHS_UPSTR_RES     (0x1 <<  7) // (UDPHS) Upstream Resume Interrupt Enable/Clear/Status
#define AT91C_UDPHS_EPT_INT_0     (0x1 <<  8) // (UDPHS) Endpoint 0 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_1     (0x1 <<  9) // (UDPHS) Endpoint 1 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_2     (0x1 << 10) // (UDPHS) Endpoint 2 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_3     (0x1 << 11) // (UDPHS) Endpoint 3 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_4     (0x1 << 12) // (UDPHS) Endpoint 4 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_5     (0x1 << 13) // (UDPHS) Endpoint 5 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_6     (0x1 << 14) // (UDPHS) Endpoint 6 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_7     (0x1 << 15) // (UDPHS) Endpoint 7 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_8     (0x1 << 16) // (UDPHS) Endpoint 8 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_9     (0x1 << 17) // (UDPHS) Endpoint 9 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_10    (0x1 << 18) // (UDPHS) Endpoint 10 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_11    (0x1 << 19) // (UDPHS) Endpoint 11 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_12    (0x1 << 20) // (UDPHS) Endpoint 12 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_13    (0x1 << 21) // (UDPHS) Endpoint 13 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_14    (0x1 << 22) // (UDPHS) Endpoint 14 Interrupt Enable/Status
#define AT91C_UDPHS_EPT_INT_15    (0x1 << 23) // (UDPHS) Endpoint 15 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_1     (0x1 << 25) // (UDPHS) DMA Channel 1 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_2     (0x1 << 26) // (UDPHS) DMA Channel 2 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_3     (0x1 << 27) // (UDPHS) DMA Channel 3 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_4     (0x1 << 28) // (UDPHS) DMA Channel 4 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_5     (0x1 << 29) // (UDPHS) DMA Channel 5 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_6     (0x1 << 30) // (UDPHS) DMA Channel 6 Interrupt Enable/Status
#define AT91C_UDPHS_DMA_INT_7     (0x1 << 31) // (UDPHS) DMA Channel 7 Interrupt Enable/Status
// -------- UDPHS_INTSTA : (UDPHS Offset: 0x14) UDPHS Interrupt Status Register --------
#define AT91C_UDPHS_SPEED         (0x1 <<  0) // (UDPHS) Speed Status
// -------- UDPHS_CLRINT : (UDPHS Offset: 0x18) UDPHS Clear Interrupt Register --------
// -------- UDPHS_EPTRST : (UDPHS Offset: 0x1c) UDPHS Endpoints Reset Register --------
#define AT91C_UDPHS_RST_EPT_0     (0x1 <<  0) // (UDPHS) Endpoint Reset 0
#define AT91C_UDPHS_RST_EPT_1     (0x1 <<  1) // (UDPHS) Endpoint Reset 1
#define AT91C_UDPHS_RST_EPT_2     (0x1 <<  2) // (UDPHS) Endpoint Reset 2
#define AT91C_UDPHS_RST_EPT_3     (0x1 <<  3) // (UDPHS) Endpoint Reset 3
#define AT91C_UDPHS_RST_EPT_4     (0x1 <<  4) // (UDPHS) Endpoint Reset 4
#define AT91C_UDPHS_RST_EPT_5     (0x1 <<  5) // (UDPHS) Endpoint Reset 5
#define AT91C_UDPHS_RST_EPT_6     (0x1 <<  6) // (UDPHS) Endpoint Reset 6
#define AT91C_UDPHS_RST_EPT_7     (0x1 <<  7) // (UDPHS) Endpoint Reset 7
#define AT91C_UDPHS_RST_EPT_8     (0x1 <<  8) // (UDPHS) Endpoint Reset 8
#define AT91C_UDPHS_RST_EPT_9     (0x1 <<  9) // (UDPHS) Endpoint Reset 9
#define AT91C_UDPHS_RST_EPT_10    (0x1 << 10) // (UDPHS) Endpoint Reset 10
#define AT91C_UDPHS_RST_EPT_11    (0x1 << 11) // (UDPHS) Endpoint Reset 11
#define AT91C_UDPHS_RST_EPT_12    (0x1 << 12) // (UDPHS) Endpoint Reset 12
#define AT91C_UDPHS_RST_EPT_13    (0x1 << 13) // (UDPHS) Endpoint Reset 13
#define AT91C_UDPHS_RST_EPT_14    (0x1 << 14) // (UDPHS) Endpoint Reset 14
#define AT91C_UDPHS_RST_EPT_15    (0x1 << 15) // (UDPHS) Endpoint Reset 15
// -------- UDPHS_TSTSOFCNT : (UDPHS Offset: 0xd0) UDPHS Test SOF Counter Register --------
#define AT91C_UDPHS_SOFCNTMAX     (0x3 <<  0) // (UDPHS) SOF Counter Max Value
#define AT91C_UDPHS_SOFCTLOAD     (0x1 <<  7) // (UDPHS) SOF Counter Load
// -------- UDPHS_TSTCNTA : (UDPHS Offset: 0xd4) UDPHS Test A Counter Register --------
#define AT91C_UDPHS_CNTAMAX       (0x7FFF <<  0) // (UDPHS) A Counter Max Value
#define AT91C_UDPHS_CNTALOAD      (0x1 << 15) // (UDPHS) A Counter Load
// -------- UDPHS_TSTCNTB : (UDPHS Offset: 0xd8) UDPHS Test B Counter Register --------
#define AT91C_UDPHS_CNTBMAX       (0x7FFF <<  0) // (UDPHS) B Counter Max Value
#define AT91C_UDPHS_CNTBLOAD      (0x1 << 15) // (UDPHS) B Counter Load
// -------- UDPHS_TSTMODREG : (UDPHS Offset: 0xdc) UDPHS Test Mode Register --------
#define AT91C_UDPHS_TSTMODE       (0x1F <<  1) // (UDPHS) UDPHS Core TestModeReg
// -------- UDPHS_TST : (UDPHS Offset: 0xe0) UDPHS Test Register --------
#define AT91C_UDPHS_SPEED_CFG     (0x3 <<  0) // (UDPHS) Speed Configuration
#define 	AT91C_UDPHS_SPEED_CFG_NM                   (0x0) // (UDPHS) Normal Mode
#define 	AT91C_UDPHS_SPEED_CFG_RS                   (0x1) // (UDPHS) Reserved
#define 	AT91C_UDPHS_SPEED_CFG_HS                   (0x2) // (UDPHS) Force High Speed
#define 	AT91C_UDPHS_SPEED_CFG_FS                   (0x3) // (UDPHS) Force Full-Speed
#define AT91C_UDPHS_TST_J         (0x1 <<  2) // (UDPHS) TestJMode
#define AT91C_UDPHS_TST_K         (0x1 <<  3) // (UDPHS) TestKMode
#define AT91C_UDPHS_TST_PKT       (0x1 <<  4) // (UDPHS) TestPacketMode
#define AT91C_UDPHS_OPMODE2       (0x1 <<  5) // (UDPHS) OpMode2
// -------- UDPHS_RIPPADDRSIZE : (UDPHS Offset: 0xec) UDPHS PADDRSIZE Register --------
#define AT91C_UDPHS_IPPADDRSIZE   (0x0 <<  0) // (UDPHS) 2^UDPHSDEV_PADDR_SIZE
// -------- UDPHS_RIPNAME1 : (UDPHS Offset: 0xf0) UDPHS Name Register --------
#define AT91C_UDPHS_IPNAME1       (0x0 <<  0) // (UDPHS) ASCII string HUSB
// -------- UDPHS_RIPNAME2 : (UDPHS Offset: 0xf4) UDPHS Name Register --------
#define AT91C_UDPHS_IPNAME2       (0x0 <<  0) // (UDPHS) ASCII string 2DEV
// -------- UDPHS_IPFEATURES : (UDPHS Offset: 0xf8) UDPHS Features Register --------
#define AT91C_UDPHS_EPT_NBR_MAX   (0xF <<  0) // (UDPHS) Max Number of Endpoints
#define AT91C_UDPHS_DMA_CHANNEL_NBR (0x7 <<  4) // (UDPHS) Number of DMA Channels
#define AT91C_UDPHS_DMA_B_SIZ     (0x1 <<  7) // (UDPHS) DMA Buffer Size
#define AT91C_UDPHS_DMA_FIFO_WORD_DEPTH (0xF <<  8) // (UDPHS) DMA FIFO Depth in words
#define AT91C_UDPHS_FIFO_MAX_SIZE (0x7 << 12) // (UDPHS) DPRAM size
#define AT91C_UDPHS_BW_DPRAM      (0x1 << 15) // (UDPHS) DPRAM byte write capability
#define AT91C_UDPHS_DATAB16_8     (0x1 << 16) // (UDPHS) UTMI DataBus16_8
#define AT91C_UDPHS_ISO_EPT_1     (0x1 << 17) // (UDPHS) Endpoint 1 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_2     (0x1 << 18) // (UDPHS) Endpoint 2 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_3     (0x1 << 19) // (UDPHS) Endpoint 3 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_4     (0x1 << 20) // (UDPHS) Endpoint 4 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_5     (0x1 << 21) // (UDPHS) Endpoint 5 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_6     (0x1 << 22) // (UDPHS) Endpoint 6 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_7     (0x1 << 23) // (UDPHS) Endpoint 7 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_8     (0x1 << 24) // (UDPHS) Endpoint 8 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_9     (0x1 << 25) // (UDPHS) Endpoint 9 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_10    (0x1 << 26) // (UDPHS) Endpoint 10 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_11    (0x1 << 27) // (UDPHS) Endpoint 11 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_12    (0x1 << 28) // (UDPHS) Endpoint 12 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_13    (0x1 << 29) // (UDPHS) Endpoint 13 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_14    (0x1 << 30) // (UDPHS) Endpoint 14 High Bandwidth Isochronous Capability
#define AT91C_UDPHS_ISO_EPT_15    (0x1 << 31) // (UDPHS) Endpoint 15 High Bandwidth Isochronous Capability
// -------- UDPHS_IPVERSION : (UDPHS Offset: 0xfc) UDPHS Version Register --------
#define AT91C_UDPHS_VERSION_NUM   (0xFFFF <<  0) // (UDPHS) Give the IP version
#define AT91C_UDPHS_METAL_FIX_NUM (0x7 << 16) // (UDPHS) Give the number of metal fixes

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
//              SOFTWARE API DEFINITION  FOR HDMA Channel structure
// *****************************************************************************
// *** Register offset in AT91S_HDMA_CH structure ***
#define HDMA_SADDR      ( 0) // HDMA Channel Source Address Register
#define HDMA_DADDR      ( 4) // HDMA Channel Destination Address Register
#define HDMA_DSCR       ( 8) // HDMA Channel Descriptor Address Register
#define HDMA_CTRLA      (12) // HDMA Channel Control A Register
#define HDMA_CTRLB      (16) // HDMA Channel Control B Register
#define HDMA_CFG        (20) // HDMA Channel Configuration Register
#define HDMA_SPIP       (24) // HDMA Channel Source Picture in Picture Configuration Register
#define HDMA_DPIP       (28) // HDMA Channel Destination Picture in Picture Configuration Register
#define HDMA_BDSCR      (32) // HDMA Reserved
#define HDMA_CADDR      (36) // HDMA Reserved
// -------- HDMA_SADDR : (HDMA_CH Offset: 0x0)  --------
#define AT91C_SADDR               (0x0 <<  0) // (HDMA_CH)
// -------- HDMA_DADDR : (HDMA_CH Offset: 0x4)  --------
#define AT91C_DADDR               (0x0 <<  0) // (HDMA_CH)
// -------- HDMA_DSCR : (HDMA_CH Offset: 0x8)  --------
#define AT91C_DSCR_IF             (0x3 <<  0) // (HDMA_CH)
#define AT91C_DSCR                (0x3FFFFFFF <<  2) // (HDMA_CH)
// -------- HDMA_CTRLA : (HDMA_CH Offset: 0xc)  --------
#define AT91C_BTSIZE              (0xFFF <<  0) // (HDMA_CH)
#define AT91C_FC                  (0x7 << 12) // (HDMA_CH)
#define AT91C_AUTO                (0x1 << 15) // (HDMA_CH)
#define AT91C_SCSIZE              (0x7 << 16) // (HDMA_CH)
#define AT91C_DCSIZE              (0x7 << 20) // (HDMA_CH)
#define AT91C_SRC_WIDTH           (0x3 << 24) // (HDMA_CH)
#define AT91C_DST_WIDTH           (0x3 << 28) // (HDMA_CH)
// -------- HDMA_CTRLB : (HDMA_CH Offset: 0x10)  --------
#define AT91C_SIF                 (0x3 <<  0) // (HDMA_CH)
#define AT91C_DIF                 (0x3 <<  4) // (HDMA_CH)
#define AT91C_SRC_PIP             (0x1 <<  8) // (HDMA_CH)
#define AT91C_DST_PIP             (0x1 << 12) // (HDMA_CH)
#define AT91C_SRC_DSCR            (0x1 << 16) // (HDMA_CH)
#define AT91C_DST_DSCR            (0x1 << 20) // (HDMA_CH)
#define AT91C_SRC_INCR            (0x3 << 24) // (HDMA_CH)
#define AT91C_DST_INCR            (0x3 << 28) // (HDMA_CH)
// -------- HDMA_CFG : (HDMA_CH Offset: 0x14)  --------
#define AT91C_SRC_PER             (0xF <<  0) // (HDMA_CH)
#define AT91C_DST_PER             (0xF <<  4) // (HDMA_CH)
#define AT91C_SRC_REP             (0x1 <<  8) // (HDMA_CH)
#define AT91C_SRC_H2SEL           (0x1 <<  9) // (HDMA_CH)
#define AT91C_DST_REP             (0x1 << 12) // (HDMA_CH)
#define AT91C_DST_H2SEL           (0x1 << 13) // (HDMA_CH)
#define AT91C_LOCK_IF             (0x1 << 20) // (HDMA_CH)
#define AT91C_LOCK_B              (0x1 << 21) // (HDMA_CH)
#define AT91C_LOCK_IF_L           (0x1 << 22) // (HDMA_CH)
#define AT91C_AHB_PROT            (0x7 << 24) // (HDMA_CH)
// -------- HDMA_SPIP : (HDMA_CH Offset: 0x18)  --------
#define AT91C_SPIP_HOLE           (0xFFFF <<  0) // (HDMA_CH)
#define AT91C_SPIP_BOUNDARY       (0x3FF << 16) // (HDMA_CH)
// -------- HDMA_DPIP : (HDMA_CH Offset: 0x1c)  --------
#define AT91C_DPIP_HOLE           (0xFFFF <<  0) // (HDMA_CH)
#define AT91C_DPIP_BOUNDARY       (0x3FF << 16) // (HDMA_CH)
// -------- HDMA_BDSCR : (HDMA_CH Offset: 0x20)  --------
// -------- HDMA_CADDR : (HDMA_CH Offset: 0x24)  --------

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR HDMA controller
// *****************************************************************************
// *** Register offset in AT91S_HDMA structure ***
#define HDMA_GCFG       ( 0) // HDMA Global Configuration Register
#define HDMA_EN         ( 4) // HDMA Controller Enable Register
#define HDMA_SREQ       ( 8) // HDMA Software Single Request Register
#define HDMA_BREQ       (12) // HDMA Software Chunk Transfer Request Register
#define HDMA_LAST       (16) // HDMA Software Last Transfer Flag Register
#define HDMA_SYNC       (20) // HDMA Request Synchronization Register
#define HDMA_EBCIER     (24) // HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Enable register
#define HDMA_EBCIDR     (28) // HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Disable register
#define HDMA_EBCIMR     (32) // HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Mask Register
#define HDMA_EBCISR     (36) // HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Status Register
#define HDMA_CHER       (40) // HDMA Channel Handler Enable Register
#define HDMA_CHDR       (44) // HDMA Channel Handler Disable Register
#define HDMA_CHSR       (48) // HDMA Channel Handler Status Register
#define HDMA_RSVD0      (52) // HDMA Reserved
#define HDMA_RSVD1      (56) // HDMA Reserved
#define HDMA_CH         (60) // HDMA Channel structure
// -------- HDMA_GCFG : (HDMA Offset: 0x0)  --------
#define AT91C_IF0_BIGEND          (0x1 <<  0) // (HDMA)
#define AT91C_IF1_BIGEND          (0x1 <<  1) // (HDMA)
#define AT91C_IF2_BIGEND          (0x1 <<  2) // (HDMA)
#define AT91C_IF3_BIGEND          (0x1 <<  3) // (HDMA)
#define AT91C_ARB_CFG             (0x1 <<  4) // (HDMA)
// -------- HDMA_EN : (HDMA Offset: 0x4)  --------
#define AT91C_HDMA_ENABLE         (0x1 <<  0) // (HDMA)
// -------- HDMA_SREQ : (HDMA Offset: 0x8)  --------
#define AT91C_SOFT_SREQ           (0xFFFF <<  0) // (HDMA)
// -------- HDMA_BREQ : (HDMA Offset: 0xc)  --------
#define AT91C_SOFT_BREQ           (0xFFFF <<  0) // (HDMA)
// -------- HDMA_LAST : (HDMA Offset: 0x10)  --------
#define AT91C_SOFT_LAST           (0xFFFF <<  0) // (HDMA)
// -------- HDMA_SYNC : (HDMA Offset: 0x14)  --------
#define AT91C_SYNC_REQ            (0xFFFF <<  0) // (HDMA)
// -------- HDMA_EBCIER : (HDMA Offset: 0x18)  --------
#define AT91C_BTC                 (0xFF <<  0) // (HDMA)
#define AT91C_CBTC                (0xFF <<  8) // (HDMA)
#define AT91C_ERR                 (0xFF << 16) // (HDMA)
// -------- HDMA_EBCIDR : (HDMA Offset: 0x1c)  --------
// -------- HDMA_EBCIMR : (HDMA Offset: 0x20)  --------
// -------- HDMA_EBCISR : (HDMA Offset: 0x24)  --------
// -------- HDMA_CHER : (HDMA Offset: 0x28)  --------
#define AT91C_ENABLE              (0xFF <<  0) // (HDMA)
#define AT91C_SUSPEND             (0xFF <<  8) // (HDMA)
#define AT91C_KEEPON              (0xFF << 24) // (HDMA)
// -------- HDMA_CHDR : (HDMA Offset: 0x2c)  --------
#define AT91C_RESUME              (0xFF <<  8) // (HDMA)
// -------- HDMA_CHSR : (HDMA Offset: 0x30)  --------
#define AT91C_STALLED             (0xFF << 14) // (HDMA)
#define AT91C_EMPTY               (0xFF << 16) // (HDMA)
// -------- HDMA_RSVD : (HDMA Offset: 0x34)  --------
// -------- HDMA_RSVD : (HDMA Offset: 0x38)  --------

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
//               REGISTER ADDRESS DEFINITION FOR AT91SAM9RL64
// *****************************************************************************
// ========== Register definition for SYS peripheral ==========
#define AT91C_SYS_SLCKSEL         (0xFFFFFD60) // (SYS) Slow Clock Selection Register
#define AT91C_SYS_GPBR            (0xFFFFFD50) // (SYS) General Purpose Register
// ========== Register definition for EBI peripheral ==========
#define AT91C_EBI_DUMMY           (0xFFFFE800) // (EBI) Dummy register - Do not use
// ========== Register definition for SDRAMC peripheral ==========
#define AT91C_SDRAMC_ISR          (0xFFFFEA20) // (SDRAMC) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC_IDR          (0xFFFFEA18) // (SDRAMC) SDRAM Controller Interrupt Disable Register
#define AT91C_SDRAMC_MR           (0xFFFFEA00) // (SDRAMC) SDRAM Controller Mode Register
#define AT91C_SDRAMC_CR           (0xFFFFEA08) // (SDRAMC) SDRAM Controller Configuration Register
#define AT91C_SDRAMC_LPR          (0xFFFFEA10) // (SDRAMC) SDRAM Controller Low Power Register
#define AT91C_SDRAMC_MDR          (0xFFFFEA24) // (SDRAMC) SDRAM Memory Device Register
#define AT91C_SDRAMC_TR           (0xFFFFEA04) // (SDRAMC) SDRAM Controller Refresh Timer Register
#define AT91C_SDRAMC_HSR          (0xFFFFEA0C) // (SDRAMC) SDRAM Controller High Speed Register
#define AT91C_SDRAMC_IER          (0xFFFFEA14) // (SDRAMC) SDRAM Controller Interrupt Enable Register
#define AT91C_SDRAMC_IMR          (0xFFFFEA1C) // (SDRAMC) SDRAM Controller Interrupt Mask Register
// ========== Register definition for SMC peripheral ==========
#define AT91C_SMC_CYCLE1          (0xFFFFEC18) // (SMC)  Cycle Register for CS 1
#define AT91C_SMC_CTRL7           (0xFFFFEC7C) // (SMC)  Control Register for CS 7
#define AT91C_SMC_CTRL2           (0xFFFFEC2C) // (SMC)  Control Register for CS 2
#define AT91C_SMC_PULSE2          (0xFFFFEC24) // (SMC)  Pulse Register for CS 2
#define AT91C_SMC_CTRL1           (0xFFFFEC1C) // (SMC)  Control Register for CS 1
#define AT91C_SMC_SETUP4          (0xFFFFEC40) // (SMC)  Setup Register for CS 4
#define AT91C_SMC_CYCLE3          (0xFFFFEC38) // (SMC)  Cycle Register for CS 3
#define AT91C_SMC_SETUP3          (0xFFFFEC30) // (SMC)  Setup Register for CS 3
#define AT91C_SMC_CYCLE2          (0xFFFFEC28) // (SMC)  Cycle Register for CS 2
#define AT91C_SMC_SETUP2          (0xFFFFEC20) // (SMC)  Setup Register for CS 2
#define AT91C_SMC_PULSE5          (0xFFFFEC54) // (SMC)  Pulse Register for CS 5
#define AT91C_SMC_CTRL4           (0xFFFFEC4C) // (SMC)  Control Register for CS 4
#define AT91C_SMC_PULSE4          (0xFFFFEC44) // (SMC)  Pulse Register for CS 4
#define AT91C_SMC_CTRL3           (0xFFFFEC3C) // (SMC)  Control Register for CS 3
#define AT91C_SMC_PULSE3          (0xFFFFEC34) // (SMC)  Pulse Register for CS 3
#define AT91C_SMC_PULSE0          (0xFFFFEC04) // (SMC)  Pulse Register for CS 0
#define AT91C_SMC_CYCLE4          (0xFFFFEC48) // (SMC)  Cycle Register for CS 4
#define AT91C_SMC_SETUP5          (0xFFFFEC50) // (SMC)  Setup Register for CS 5
#define AT91C_SMC_CYCLE5          (0xFFFFEC58) // (SMC)  Cycle Register for CS 5
#define AT91C_SMC_SETUP6          (0xFFFFEC60) // (SMC)  Setup Register for CS 6
#define AT91C_SMC_CYCLE6          (0xFFFFEC68) // (SMC)  Cycle Register for CS 6
#define AT91C_SMC_SETUP0          (0xFFFFEC00) // (SMC)  Setup Register for CS 0
#define AT91C_SMC_CYCLE0          (0xFFFFEC08) // (SMC)  Cycle Register for CS 0
#define AT91C_SMC_SETUP1          (0xFFFFEC10) // (SMC)  Setup Register for CS 1
#define AT91C_SMC_CTRL5           (0xFFFFEC5C) // (SMC)  Control Register for CS 5
#define AT91C_SMC_PULSE6          (0xFFFFEC64) // (SMC)  Pulse Register for CS 6
#define AT91C_SMC_CTRL6           (0xFFFFEC6C) // (SMC)  Control Register for CS 6
#define AT91C_SMC_PULSE7          (0xFFFFEC74) // (SMC)  Pulse Register for CS 7
#define AT91C_SMC_CTRL0           (0xFFFFEC0C) // (SMC)  Control Register for CS 0
#define AT91C_SMC_PULSE1          (0xFFFFEC14) // (SMC)  Pulse Register for CS 1
#define AT91C_SMC_SETUP7          (0xFFFFEC70) // (SMC)  Setup Register for CS 7
#define AT91C_SMC_CYCLE7          (0xFFFFEC78) // (SMC)  Cycle Register for CS 7
// ========== Register definition for MATRIX peripheral ==========
#define AT91C_MATRIX_MCFG1        (0xFFFFEE04) // (MATRIX)  Master Configuration Register 1 ; htcm
#define AT91C_MATRIX_MRCR         (0xFFFFEF00) // (MATRIX)  Master Remp Control Register
#define AT91C_MATRIX_PRAS7        (0xFFFFEEB8) // (MATRIX)  PRAS7
#define AT91C_MATRIX_PRAS5        (0xFFFFEEA8) // (MATRIX)  PRAS5 : bridge
#define AT91C_MATRIX_SCFG4        (0xFFFFEE50) // (MATRIX)  Slave Configuration Register 4 ; ebi
#define AT91C_MATRIX_MCFG2        (0xFFFFEE08) // (MATRIX)  Master Configuration Register 2 : lcdc
#define AT91C_MATRIX_PRBS7        (0xFFFFEEBC) // (MATRIX)  PRBS7
#define AT91C_MATRIX_MCFG6        (0xFFFFEE18) // (MATRIX)  Master Configuration Register 6
#define AT91C_MATRIX_PRBS0        (0xFFFFEE84) // (MATRIX)  PRBS0 : rom
#define AT91C_MATRIX_MCFG7        (0xFFFFEE1C) // (MATRIX)  Master Configuration Register 7
#define AT91C_MATRIX_PRAS3        (0xFFFFEE98) // (MATRIX)  PRAS3 : usb_dev_hs
#define AT91C_MATRIX_PRAS1        (0xFFFFEE88) // (MATRIX)  PRAS1 : htcm
#define AT91C_MATRIX_SCFG0        (0xFFFFEE40) // (MATRIX)  Slave Configuration Register 0 : rom
#define AT91C_MATRIX_PRBS5        (0xFFFFEEAC) // (MATRIX)  PRBS5 : bridge
#define AT91C_MATRIX_PRBS3        (0xFFFFEE9C) // (MATRIX)  PRBS3 : usb_dev_hs
#define AT91C_MATRIX_PRBS1        (0xFFFFEE8C) // (MATRIX)  PRBS1 : htcm
#define AT91C_MATRIX_SCFG5        (0xFFFFEE54) // (MATRIX)  Slave Configuration Register 5 : bridge
#define AT91C_MATRIX_SCFG1        (0xFFFFEE44) // (MATRIX)  Slave Configuration Register 1 : htcm
#define AT91C_MATRIX_PRAS4        (0xFFFFEEA0) // (MATRIX)  PRAS4 : ebi
#define AT91C_MATRIX_PRAS6        (0xFFFFEEB0) // (MATRIX)  PRAS6
#define AT91C_MATRIX_SCFG6        (0xFFFFEE58) // (MATRIX)  Slave Configuration Register 6
#define AT91C_MATRIX_MCFG0        (0xFFFFEE00) // (MATRIX)  Master Configuration Register 0 : rom
#define AT91C_MATRIX_PRBS6        (0xFFFFEEB4) // (MATRIX)  PRBS6
#define AT91C_MATRIX_MCFG3        (0xFFFFEE0C) // (MATRIX)  Master Configuration Register 3 : usb_dev_hs
#define AT91C_MATRIX_MCFG8        (0xFFFFEE20) // (MATRIX)  Master Configuration Register 8
#define AT91C_MATRIX_MCFG4        (0xFFFFEE10) // (MATRIX)  Master Configuration Register 4 : ebi
#define AT91C_MATRIX_MCFG5        (0xFFFFEE14) // (MATRIX)  Master Configuration Register 5 : bridge
#define AT91C_MATRIX_PRAS0        (0xFFFFEE80) // (MATRIX)  PRAS0 : rom
#define AT91C_MATRIX_PRAS2        (0xFFFFEE90) // (MATRIX)  PRAS2 : lcdc
#define AT91C_MATRIX_SCFG2        (0xFFFFEE48) // (MATRIX)  Slave Configuration Register 2 : lcdc
#define AT91C_MATRIX_PRBS2        (0xFFFFEE94) // (MATRIX)  PRBS2 : lcdc
#define AT91C_MATRIX_PRBS4        (0xFFFFEEA4) // (MATRIX)  PRBS4 : ebi
#define AT91C_MATRIX_SCFG3        (0xFFFFEE4C) // (MATRIX)  Slave Configuration Register 3 : usb_dev_hs
#define AT91C_MATRIX_SCFG7        (0xFFFFEE5C) // (MATRIX)  Slave Configuration Register 7
// ========== Register definition for CCFG peripheral ==========
#define AT91C_CCFG_MATRIXVERSION  (0xFFFFEFFC) // (CCFG)  Version Register
#define AT91C_CCFG_EBICSA         (0xFFFFEF20) // (CCFG)  EBI Chip Select Assignement Register
#define AT91C_CCFG_UDPHS          (0xFFFFEF1C) // (CCFG)  USB Device HS configuration
#define AT91C_CCFG_TCMR           (0xFFFFEF14) // (CCFG)  TCM configuration
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
// ========== Register definition for PDC_DBGU peripheral ==========
#define AT91C_DBGU_TNCR           (0xFFFFF31C) // (PDC_DBGU) Transmit Next Counter Register
#define AT91C_DBGU_RNCR           (0xFFFFF314) // (PDC_DBGU) Receive Next Counter Register
#define AT91C_DBGU_PTCR           (0xFFFFF320) // (PDC_DBGU) PDC Transfer Control Register
#define AT91C_DBGU_PTSR           (0xFFFFF324) // (PDC_DBGU) PDC Transfer Status Register
#define AT91C_DBGU_RCR            (0xFFFFF304) // (PDC_DBGU) Receive Counter Register
#define AT91C_DBGU_TCR            (0xFFFFF30C) // (PDC_DBGU) Transmit Counter Register
#define AT91C_DBGU_RPR            (0xFFFFF300) // (PDC_DBGU) Receive Pointer Register
#define AT91C_DBGU_TPR            (0xFFFFF308) // (PDC_DBGU) Transmit Pointer Register
#define AT91C_DBGU_RNPR           (0xFFFFF310) // (PDC_DBGU) Receive Next Pointer Register
#define AT91C_DBGU_TNPR           (0xFFFFF318) // (PDC_DBGU) Transmit Next Pointer Register
// ========== Register definition for DBGU peripheral ==========
#define AT91C_DBGU_EXID           (0xFFFFF244) // (DBGU) Chip ID Extension Register
#define AT91C_DBGU_THR            (0xFFFFF21C) // (DBGU) Transmitter Holding Register
#define AT91C_DBGU_CSR            (0xFFFFF214) // (DBGU) Channel Status Register
#define AT91C_DBGU_IDR            (0xFFFFF20C) // (DBGU) Interrupt Disable Register
#define AT91C_DBGU_MR             (0xFFFFF204) // (DBGU) Mode Register
#define AT91C_DBGU_FNTR           (0xFFFFF248) // (DBGU) Force NTRST Register
#define AT91C_DBGU_CIDR           (0xFFFFF240) // (DBGU) Chip ID Register
#define AT91C_DBGU_BRGR           (0xFFFFF220) // (DBGU) Baud Rate Generator Register
#define AT91C_DBGU_RHR            (0xFFFFF218) // (DBGU) Receiver Holding Register
#define AT91C_DBGU_IMR            (0xFFFFF210) // (DBGU) Interrupt Mask Register
#define AT91C_DBGU_IER            (0xFFFFF208) // (DBGU) Interrupt Enable Register
#define AT91C_DBGU_CR             (0xFFFFF200) // (DBGU) Control Register
// ========== Register definition for PIOA peripheral ==========
#define AT91C_PIOA_IMR            (0xFFFFF448) // (PIOA) Interrupt Mask Register
#define AT91C_PIOA_IER            (0xFFFFF440) // (PIOA) Interrupt Enable Register
#define AT91C_PIOA_OWDR           (0xFFFFF4A4) // (PIOA) Output Write Disable Register
#define AT91C_PIOA_ISR            (0xFFFFF44C) // (PIOA) Interrupt Status Register
#define AT91C_PIOA_PPUDR          (0xFFFFF460) // (PIOA) Pull-up Disable Register
#define AT91C_PIOA_MDSR           (0xFFFFF458) // (PIOA) Multi-driver Status Register
#define AT91C_PIOA_MDER           (0xFFFFF450) // (PIOA) Multi-driver Enable Register
#define AT91C_PIOA_PER            (0xFFFFF400) // (PIOA) PIO Enable Register
#define AT91C_PIOA_PSR            (0xFFFFF408) // (PIOA) PIO Status Register
#define AT91C_PIOA_OER            (0xFFFFF410) // (PIOA) Output Enable Register
#define AT91C_PIOA_BSR            (0xFFFFF474) // (PIOA) Select B Register
#define AT91C_PIOA_PPUER          (0xFFFFF464) // (PIOA) Pull-up Enable Register
#define AT91C_PIOA_MDDR           (0xFFFFF454) // (PIOA) Multi-driver Disable Register
#define AT91C_PIOA_PDR            (0xFFFFF404) // (PIOA) PIO Disable Register
#define AT91C_PIOA_ODR            (0xFFFFF414) // (PIOA) Output Disable Registerr
#define AT91C_PIOA_IFDR           (0xFFFFF424) // (PIOA) Input Filter Disable Register
#define AT91C_PIOA_ABSR           (0xFFFFF478) // (PIOA) AB Select Status Register
#define AT91C_PIOA_ASR            (0xFFFFF470) // (PIOA) Select A Register
#define AT91C_PIOA_PPUSR          (0xFFFFF468) // (PIOA) Pull-up Status Register
#define AT91C_PIOA_ODSR           (0xFFFFF438) // (PIOA) Output Data Status Register
#define AT91C_PIOA_SODR           (0xFFFFF430) // (PIOA) Set Output Data Register
#define AT91C_PIOA_IFSR           (0xFFFFF428) // (PIOA) Input Filter Status Register
#define AT91C_PIOA_IFER           (0xFFFFF420) // (PIOA) Input Filter Enable Register
#define AT91C_PIOA_OSR            (0xFFFFF418) // (PIOA) Output Status Register
#define AT91C_PIOA_IDR            (0xFFFFF444) // (PIOA) Interrupt Disable Register
#define AT91C_PIOA_PDSR           (0xFFFFF43C) // (PIOA) Pin Data Status Register
#define AT91C_PIOA_CODR           (0xFFFFF434) // (PIOA) Clear Output Data Register
#define AT91C_PIOA_OWSR           (0xFFFFF4A8) // (PIOA) Output Write Status Register
#define AT91C_PIOA_OWER           (0xFFFFF4A0) // (PIOA) Output Write Enable Register
// ========== Register definition for PIOB peripheral ==========
#define AT91C_PIOB_OWSR           (0xFFFFF6A8) // (PIOB) Output Write Status Register
#define AT91C_PIOB_PPUSR          (0xFFFFF668) // (PIOB) Pull-up Status Register
#define AT91C_PIOB_PPUDR          (0xFFFFF660) // (PIOB) Pull-up Disable Register
#define AT91C_PIOB_MDSR           (0xFFFFF658) // (PIOB) Multi-driver Status Register
#define AT91C_PIOB_MDER           (0xFFFFF650) // (PIOB) Multi-driver Enable Register
#define AT91C_PIOB_IMR            (0xFFFFF648) // (PIOB) Interrupt Mask Register
#define AT91C_PIOB_OSR            (0xFFFFF618) // (PIOB) Output Status Register
#define AT91C_PIOB_OER            (0xFFFFF610) // (PIOB) Output Enable Register
#define AT91C_PIOB_PSR            (0xFFFFF608) // (PIOB) PIO Status Register
#define AT91C_PIOB_PER            (0xFFFFF600) // (PIOB) PIO Enable Register
#define AT91C_PIOB_BSR            (0xFFFFF674) // (PIOB) Select B Register
#define AT91C_PIOB_PPUER          (0xFFFFF664) // (PIOB) Pull-up Enable Register
#define AT91C_PIOB_IFDR           (0xFFFFF624) // (PIOB) Input Filter Disable Register
#define AT91C_PIOB_ODR            (0xFFFFF614) // (PIOB) Output Disable Registerr
#define AT91C_PIOB_ABSR           (0xFFFFF678) // (PIOB) AB Select Status Register
#define AT91C_PIOB_ASR            (0xFFFFF670) // (PIOB) Select A Register
#define AT91C_PIOB_IFER           (0xFFFFF620) // (PIOB) Input Filter Enable Register
#define AT91C_PIOB_IFSR           (0xFFFFF628) // (PIOB) Input Filter Status Register
#define AT91C_PIOB_SODR           (0xFFFFF630) // (PIOB) Set Output Data Register
#define AT91C_PIOB_ODSR           (0xFFFFF638) // (PIOB) Output Data Status Register
#define AT91C_PIOB_CODR           (0xFFFFF634) // (PIOB) Clear Output Data Register
#define AT91C_PIOB_PDSR           (0xFFFFF63C) // (PIOB) Pin Data Status Register
#define AT91C_PIOB_OWER           (0xFFFFF6A0) // (PIOB) Output Write Enable Register
#define AT91C_PIOB_IER            (0xFFFFF640) // (PIOB) Interrupt Enable Register
#define AT91C_PIOB_OWDR           (0xFFFFF6A4) // (PIOB) Output Write Disable Register
#define AT91C_PIOB_MDDR           (0xFFFFF654) // (PIOB) Multi-driver Disable Register
#define AT91C_PIOB_ISR            (0xFFFFF64C) // (PIOB) Interrupt Status Register
#define AT91C_PIOB_IDR            (0xFFFFF644) // (PIOB) Interrupt Disable Register
#define AT91C_PIOB_PDR            (0xFFFFF604) // (PIOB) PIO Disable Register
// ========== Register definition for PIOC peripheral ==========
#define AT91C_PIOC_IFDR           (0xFFFFF824) // (PIOC) Input Filter Disable Register
#define AT91C_PIOC_ODR            (0xFFFFF814) // (PIOC) Output Disable Registerr
#define AT91C_PIOC_ABSR           (0xFFFFF878) // (PIOC) AB Select Status Register
#define AT91C_PIOC_SODR           (0xFFFFF830) // (PIOC) Set Output Data Register
#define AT91C_PIOC_IFSR           (0xFFFFF828) // (PIOC) Input Filter Status Register
#define AT91C_PIOC_CODR           (0xFFFFF834) // (PIOC) Clear Output Data Register
#define AT91C_PIOC_ODSR           (0xFFFFF838) // (PIOC) Output Data Status Register
#define AT91C_PIOC_IER            (0xFFFFF840) // (PIOC) Interrupt Enable Register
#define AT91C_PIOC_IMR            (0xFFFFF848) // (PIOC) Interrupt Mask Register
#define AT91C_PIOC_OWDR           (0xFFFFF8A4) // (PIOC) Output Write Disable Register
#define AT91C_PIOC_MDDR           (0xFFFFF854) // (PIOC) Multi-driver Disable Register
#define AT91C_PIOC_PDSR           (0xFFFFF83C) // (PIOC) Pin Data Status Register
#define AT91C_PIOC_IDR            (0xFFFFF844) // (PIOC) Interrupt Disable Register
#define AT91C_PIOC_ISR            (0xFFFFF84C) // (PIOC) Interrupt Status Register
#define AT91C_PIOC_PDR            (0xFFFFF804) // (PIOC) PIO Disable Register
#define AT91C_PIOC_OWSR           (0xFFFFF8A8) // (PIOC) Output Write Status Register
#define AT91C_PIOC_OWER           (0xFFFFF8A0) // (PIOC) Output Write Enable Register
#define AT91C_PIOC_ASR            (0xFFFFF870) // (PIOC) Select A Register
#define AT91C_PIOC_PPUSR          (0xFFFFF868) // (PIOC) Pull-up Status Register
#define AT91C_PIOC_PPUDR          (0xFFFFF860) // (PIOC) Pull-up Disable Register
#define AT91C_PIOC_MDSR           (0xFFFFF858) // (PIOC) Multi-driver Status Register
#define AT91C_PIOC_MDER           (0xFFFFF850) // (PIOC) Multi-driver Enable Register
#define AT91C_PIOC_IFER           (0xFFFFF820) // (PIOC) Input Filter Enable Register
#define AT91C_PIOC_OSR            (0xFFFFF818) // (PIOC) Output Status Register
#define AT91C_PIOC_OER            (0xFFFFF810) // (PIOC) Output Enable Register
#define AT91C_PIOC_PSR            (0xFFFFF808) // (PIOC) PIO Status Register
#define AT91C_PIOC_PER            (0xFFFFF800) // (PIOC) PIO Enable Register
#define AT91C_PIOC_BSR            (0xFFFFF874) // (PIOC) Select B Register
#define AT91C_PIOC_PPUER          (0xFFFFF864) // (PIOC) Pull-up Enable Register
// ========== Register definition for PIOD peripheral ==========
#define AT91C_PIOD_PDSR           (0xFFFFFA3C) // (PIOD) Pin Data Status Register
#define AT91C_PIOD_CODR           (0xFFFFFA34) // (PIOD) Clear Output Data Register
#define AT91C_PIOD_OWER           (0xFFFFFAA0) // (PIOD) Output Write Enable Register
#define AT91C_PIOD_MDER           (0xFFFFFA50) // (PIOD) Multi-driver Enable Register
#define AT91C_PIOD_IMR            (0xFFFFFA48) // (PIOD) Interrupt Mask Register
#define AT91C_PIOD_IER            (0xFFFFFA40) // (PIOD) Interrupt Enable Register
#define AT91C_PIOD_ODSR           (0xFFFFFA38) // (PIOD) Output Data Status Register
#define AT91C_PIOD_SODR           (0xFFFFFA30) // (PIOD) Set Output Data Register
#define AT91C_PIOD_PER            (0xFFFFFA00) // (PIOD) PIO Enable Register
#define AT91C_PIOD_OWDR           (0xFFFFFAA4) // (PIOD) Output Write Disable Register
#define AT91C_PIOD_PPUER          (0xFFFFFA64) // (PIOD) Pull-up Enable Register
#define AT91C_PIOD_MDDR           (0xFFFFFA54) // (PIOD) Multi-driver Disable Register
#define AT91C_PIOD_ISR            (0xFFFFFA4C) // (PIOD) Interrupt Status Register
#define AT91C_PIOD_IDR            (0xFFFFFA44) // (PIOD) Interrupt Disable Register
#define AT91C_PIOD_PDR            (0xFFFFFA04) // (PIOD) PIO Disable Register
#define AT91C_PIOD_ODR            (0xFFFFFA14) // (PIOD) Output Disable Registerr
#define AT91C_PIOD_OWSR           (0xFFFFFAA8) // (PIOD) Output Write Status Register
#define AT91C_PIOD_ABSR           (0xFFFFFA78) // (PIOD) AB Select Status Register
#define AT91C_PIOD_ASR            (0xFFFFFA70) // (PIOD) Select A Register
#define AT91C_PIOD_PPUSR          (0xFFFFFA68) // (PIOD) Pull-up Status Register
#define AT91C_PIOD_PPUDR          (0xFFFFFA60) // (PIOD) Pull-up Disable Register
#define AT91C_PIOD_MDSR           (0xFFFFFA58) // (PIOD) Multi-driver Status Register
#define AT91C_PIOD_PSR            (0xFFFFFA08) // (PIOD) PIO Status Register
#define AT91C_PIOD_OER            (0xFFFFFA10) // (PIOD) Output Enable Register
#define AT91C_PIOD_OSR            (0xFFFFFA18) // (PIOD) Output Status Register
#define AT91C_PIOD_IFER           (0xFFFFFA20) // (PIOD) Input Filter Enable Register
#define AT91C_PIOD_BSR            (0xFFFFFA74) // (PIOD) Select B Register
#define AT91C_PIOD_IFDR           (0xFFFFFA24) // (PIOD) Input Filter Disable Register
#define AT91C_PIOD_IFSR           (0xFFFFFA28) // (PIOD) Input Filter Status Register
// ========== Register definition for PMC peripheral ==========
#define AT91C_PMC_SCSR            (0xFFFFFC08) // (PMC) System Clock Status Register
#define AT91C_PMC_SCER            (0xFFFFFC00) // (PMC) System Clock Enable Register
#define AT91C_PMC_IMR             (0xFFFFFC6C) // (PMC) Interrupt Mask Register
#define AT91C_PMC_IDR             (0xFFFFFC64) // (PMC) Interrupt Disable Register
#define AT91C_PMC_UCKR            (0xFFFFFC1C) // (PMC) UTMI Clock Configuration Register
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
// ========== Register definition for CKGR peripheral ==========
#define AT91C_CKGR_PLLBR          (0xFFFFFC2C) // (CKGR) PLL B Register
#define AT91C_CKGR_MCFR           (0xFFFFFC24) // (CKGR) Main Clock  Frequency Register
#define AT91C_CKGR_UCKR           (0xFFFFFC1C) // (CKGR) UTMI Clock Configuration Register
#define AT91C_CKGR_PLLAR          (0xFFFFFC28) // (CKGR) PLL A Register
#define AT91C_CKGR_MOR            (0xFFFFFC20) // (CKGR) Main Oscillator Register
// ========== Register definition for RSTC peripheral ==========
#define AT91C_RSTC_RSR            (0xFFFFFD04) // (RSTC) Reset Status Register
#define AT91C_RSTC_RMR            (0xFFFFFD08) // (RSTC) Reset Mode Register
#define AT91C_RSTC_RCR            (0xFFFFFD00) // (RSTC) Reset Control Register
// ========== Register definition for SHDWC peripheral ==========
#define AT91C_SHDWC_SHMR          (0xFFFFFD14) // (SHDWC) Shut Down Mode Register
#define AT91C_SHDWC_SHSR          (0xFFFFFD18) // (SHDWC) Shut Down Status Register
#define AT91C_SHDWC_SHCR          (0xFFFFFD10) // (SHDWC) Shut Down Control Register
// ========== Register definition for RTTC peripheral ==========
#define AT91C_RTTC_RTSR           (0xFFFFFD2C) // (RTTC) Real-time Status Register
#define AT91C_RTTC_RTAR           (0xFFFFFD24) // (RTTC) Real-time Alarm Register
#define AT91C_RTTC_RTVR           (0xFFFFFD28) // (RTTC) Real-time Value Register
#define AT91C_RTTC_RTMR           (0xFFFFFD20) // (RTTC) Real-time Mode Register
// ========== Register definition for PITC peripheral ==========
#define AT91C_PITC_PIIR           (0xFFFFFD3C) // (PITC) Period Interval Image Register
#define AT91C_PITC_PISR           (0xFFFFFD34) // (PITC) Period Interval Status Register
#define AT91C_PITC_PIVR           (0xFFFFFD38) // (PITC) Period Interval Value Register
#define AT91C_PITC_PIMR           (0xFFFFFD30) // (PITC) Period Interval Mode Register
// ========== Register definition for WDTC peripheral ==========
#define AT91C_WDTC_WDMR           (0xFFFFFD44) // (WDTC) Watchdog Mode Register
#define AT91C_WDTC_WDSR           (0xFFFFFD48) // (WDTC) Watchdog Status Register
#define AT91C_WDTC_WDCR           (0xFFFFFD40) // (WDTC) Watchdog Control Register
// ========== Register definition for RTC peripheral ==========
#define AT91C_RTC_IMR             (0xFFFFFE28) // (RTC) Interrupt Mask Register
#define AT91C_RTC_IER             (0xFFFFFE20) // (RTC) Interrupt Enable Register
#define AT91C_RTC_SR              (0xFFFFFE18) // (RTC) Status Register
#define AT91C_RTC_TIMALR          (0xFFFFFE10) // (RTC) Time Alarm Register
#define AT91C_RTC_TIMR            (0xFFFFFE08) // (RTC) Time Register
#define AT91C_RTC_CR              (0xFFFFFE00) // (RTC) Control Register
#define AT91C_RTC_VER             (0xFFFFFE2C) // (RTC) Valid Entry Register
#define AT91C_RTC_IDR             (0xFFFFFE24) // (RTC) Interrupt Disable Register
#define AT91C_RTC_SCCR            (0xFFFFFE1C) // (RTC) Status Clear Command Register
#define AT91C_RTC_CALALR          (0xFFFFFE14) // (RTC) Calendar Alarm Register
#define AT91C_RTC_CALR            (0xFFFFFE0C) // (RTC) Calendar Register
#define AT91C_RTC_MR              (0xFFFFFE04) // (RTC) Mode Register
// ========== Register definition for TC0 peripheral ==========
#define AT91C_TC0_IMR             (0xFFFA002C) // (TC0) Interrupt Mask Register
#define AT91C_TC0_IER             (0xFFFA0024) // (TC0) Interrupt Enable Register
#define AT91C_TC0_RC              (0xFFFA001C) // (TC0) Register C
#define AT91C_TC0_RA              (0xFFFA0014) // (TC0) Register A
#define AT91C_TC0_CMR             (0xFFFA0004) // (TC0) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC0_IDR             (0xFFFA0028) // (TC0) Interrupt Disable Register
#define AT91C_TC0_SR              (0xFFFA0020) // (TC0) Status Register
#define AT91C_TC0_RB              (0xFFFA0018) // (TC0) Register B
#define AT91C_TC0_CV              (0xFFFA0010) // (TC0) Counter Value
#define AT91C_TC0_CCR             (0xFFFA0000) // (TC0) Channel Control Register
// ========== Register definition for TC1 peripheral ==========
#define AT91C_TC1_IMR             (0xFFFA006C) // (TC1) Interrupt Mask Register
#define AT91C_TC1_IER             (0xFFFA0064) // (TC1) Interrupt Enable Register
#define AT91C_TC1_RC              (0xFFFA005C) // (TC1) Register C
#define AT91C_TC1_RA              (0xFFFA0054) // (TC1) Register A
#define AT91C_TC1_CMR             (0xFFFA0044) // (TC1) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC1_IDR             (0xFFFA0068) // (TC1) Interrupt Disable Register
#define AT91C_TC1_SR              (0xFFFA0060) // (TC1) Status Register
#define AT91C_TC1_RB              (0xFFFA0058) // (TC1) Register B
#define AT91C_TC1_CV              (0xFFFA0050) // (TC1) Counter Value
#define AT91C_TC1_CCR             (0xFFFA0040) // (TC1) Channel Control Register
// ========== Register definition for TC2 peripheral ==========
#define AT91C_TC2_IMR             (0xFFFA00AC) // (TC2) Interrupt Mask Register
#define AT91C_TC2_IER             (0xFFFA00A4) // (TC2) Interrupt Enable Register
#define AT91C_TC2_RC              (0xFFFA009C) // (TC2) Register C
#define AT91C_TC2_RA              (0xFFFA0094) // (TC2) Register A
#define AT91C_TC2_CMR             (0xFFFA0084) // (TC2) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC2_IDR             (0xFFFA00A8) // (TC2) Interrupt Disable Register
#define AT91C_TC2_SR              (0xFFFA00A0) // (TC2) Status Register
#define AT91C_TC2_RB              (0xFFFA0098) // (TC2) Register B
#define AT91C_TC2_CV              (0xFFFA0090) // (TC2) Counter Value
#define AT91C_TC2_CCR             (0xFFFA0080) // (TC2) Channel Control Register
// ========== Register definition for TCB0 peripheral ==========
#define AT91C_TCB0_BMR            (0xFFFA00C4) // (TCB0) TC Block Mode Register
#define AT91C_TCB0_BCR            (0xFFFA00C0) // (TCB0) TC Block Control Register
// ========== Register definition for TCB1 peripheral ==========
#define AT91C_TCB1_BMR            (0xFFFA0104) // (TCB1) TC Block Mode Register
#define AT91C_TCB1_BCR            (0xFFFA0100) // (TCB1) TC Block Control Register
// ========== Register definition for TCB2 peripheral ==========
#define AT91C_TCB2_BCR            (0xFFFA0140) // (TCB2) TC Block Control Register
#define AT91C_TCB2_BMR            (0xFFFA0144) // (TCB2) TC Block Mode Register
// ========== Register definition for PDC_MCI peripheral ==========
#define AT91C_MCI_PTCR            (0xFFFA4120) // (PDC_MCI) PDC Transfer Control Register
#define AT91C_MCI_TNPR            (0xFFFA4118) // (PDC_MCI) Transmit Next Pointer Register
#define AT91C_MCI_RNPR            (0xFFFA4110) // (PDC_MCI) Receive Next Pointer Register
#define AT91C_MCI_TPR             (0xFFFA4108) // (PDC_MCI) Transmit Pointer Register
#define AT91C_MCI_RPR             (0xFFFA4100) // (PDC_MCI) Receive Pointer Register
#define AT91C_MCI_PTSR            (0xFFFA4124) // (PDC_MCI) PDC Transfer Status Register
#define AT91C_MCI_TNCR            (0xFFFA411C) // (PDC_MCI) Transmit Next Counter Register
#define AT91C_MCI_RNCR            (0xFFFA4114) // (PDC_MCI) Receive Next Counter Register
#define AT91C_MCI_TCR             (0xFFFA410C) // (PDC_MCI) Transmit Counter Register
#define AT91C_MCI_RCR             (0xFFFA4104) // (PDC_MCI) Receive Counter Register
// ========== Register definition for MCI peripheral ==========
#define AT91C_MCI_IMR             (0xFFFA404C) // (MCI) MCI Interrupt Mask Register
#define AT91C_MCI_IER             (0xFFFA4044) // (MCI) MCI Interrupt Enable Register
#define AT91C_MCI_TDR             (0xFFFA4034) // (MCI) MCI Transmit Data Register
#define AT91C_MCI_IDR             (0xFFFA4048) // (MCI) MCI Interrupt Disable Register
#define AT91C_MCI_SR              (0xFFFA4040) // (MCI) MCI Status Register
#define AT91C_MCI_ARGR            (0xFFFA4010) // (MCI) MCI Argument Register
#define AT91C_MCI_DTOR            (0xFFFA4008) // (MCI) MCI Data Timeout Register
#define AT91C_MCI_CR              (0xFFFA4000) // (MCI) MCI Control Register
#define AT91C_MCI_VR              (0xFFFA40FC) // (MCI) MCI Version Register
#define AT91C_MCI_MR              (0xFFFA4004) // (MCI) MCI Mode Register
#define AT91C_MCI_SDCR            (0xFFFA400C) // (MCI) MCI SD Card Register
#define AT91C_MCI_CMDR            (0xFFFA4014) // (MCI) MCI Command Register
#define AT91C_MCI_RDR             (0xFFFA4030) // (MCI) MCI Receive Data Register
#define AT91C_MCI_BLKR            (0xFFFA4018) // (MCI) MCI Block Register
#define AT91C_MCI_RSPR            (0xFFFA4020) // (MCI) MCI Response Register
// ========== Register definition for PDC_TWI0 peripheral ==========
#define AT91C_TWI0_PTSR           (0xFFFA8124) // (PDC_TWI0) PDC Transfer Status Register
#define AT91C_TWI0_TNCR           (0xFFFA811C) // (PDC_TWI0) Transmit Next Counter Register
#define AT91C_TWI0_RNCR           (0xFFFA8114) // (PDC_TWI0) Receive Next Counter Register
#define AT91C_TWI0_TCR            (0xFFFA810C) // (PDC_TWI0) Transmit Counter Register
#define AT91C_TWI0_RCR            (0xFFFA8104) // (PDC_TWI0) Receive Counter Register
#define AT91C_TWI0_PTCR           (0xFFFA8120) // (PDC_TWI0) PDC Transfer Control Register
#define AT91C_TWI0_TNPR           (0xFFFA8118) // (PDC_TWI0) Transmit Next Pointer Register
#define AT91C_TWI0_RNPR           (0xFFFA8110) // (PDC_TWI0) Receive Next Pointer Register
#define AT91C_TWI0_TPR            (0xFFFA8108) // (PDC_TWI0) Transmit Pointer Register
#define AT91C_TWI0_RPR            (0xFFFA8100) // (PDC_TWI0) Receive Pointer Register
// ========== Register definition for TWI0 peripheral ==========
#define AT91C_TWI0_SR             (0xFFFA8020) // (TWI0) Status Register
#define AT91C_TWI0_CWGR           (0xFFFA8010) // (TWI0) Clock Waveform Generator Register
#define AT91C_TWI0_SMR            (0xFFFA8008) // (TWI0) Slave Mode Register
#define AT91C_TWI0_CR             (0xFFFA8000) // (TWI0) Control Register
#define AT91C_TWI0_THR            (0xFFFA8034) // (TWI0) Transmit Holding Register
#define AT91C_TWI0_IMR            (0xFFFA802C) // (TWI0) Interrupt Mask Register
#define AT91C_TWI0_IER            (0xFFFA8024) // (TWI0) Interrupt Enable Register
#define AT91C_TWI0_IDR            (0xFFFA8028) // (TWI0) Interrupt Disable Register
#define AT91C_TWI0_RHR            (0xFFFA8030) // (TWI0) Receive Holding Register
#define AT91C_TWI0_MMR            (0xFFFA8004) // (TWI0) Master Mode Register
#define AT91C_TWI0_IADR           (0xFFFA800C) // (TWI0) Internal Address Register
// ========== Register definition for TWI1 peripheral ==========
#define AT91C_TWI1_THR            (0xFFFAC034) // (TWI1) Transmit Holding Register
#define AT91C_TWI1_IMR            (0xFFFAC02C) // (TWI1) Interrupt Mask Register
#define AT91C_TWI1_IER            (0xFFFAC024) // (TWI1) Interrupt Enable Register
#define AT91C_TWI1_IADR           (0xFFFAC00C) // (TWI1) Internal Address Register
#define AT91C_TWI1_MMR            (0xFFFAC004) // (TWI1) Master Mode Register
#define AT91C_TWI1_RHR            (0xFFFAC030) // (TWI1) Receive Holding Register
#define AT91C_TWI1_IDR            (0xFFFAC028) // (TWI1) Interrupt Disable Register
#define AT91C_TWI1_SR             (0xFFFAC020) // (TWI1) Status Register
#define AT91C_TWI1_CWGR           (0xFFFAC010) // (TWI1) Clock Waveform Generator Register
#define AT91C_TWI1_SMR            (0xFFFAC008) // (TWI1) Slave Mode Register
#define AT91C_TWI1_CR             (0xFFFAC000) // (TWI1) Control Register
// ========== Register definition for PDC_US0 peripheral ==========
#define AT91C_US0_TNCR            (0xFFFB011C) // (PDC_US0) Transmit Next Counter Register
#define AT91C_US0_RNCR            (0xFFFB0114) // (PDC_US0) Receive Next Counter Register
#define AT91C_US0_TCR             (0xFFFB010C) // (PDC_US0) Transmit Counter Register
#define AT91C_US0_RCR             (0xFFFB0104) // (PDC_US0) Receive Counter Register
#define AT91C_US0_PTCR            (0xFFFB0120) // (PDC_US0) PDC Transfer Control Register
#define AT91C_US0_TNPR            (0xFFFB0118) // (PDC_US0) Transmit Next Pointer Register
#define AT91C_US0_RNPR            (0xFFFB0110) // (PDC_US0) Receive Next Pointer Register
#define AT91C_US0_PTSR            (0xFFFB0124) // (PDC_US0) PDC Transfer Status Register
#define AT91C_US0_RPR             (0xFFFB0100) // (PDC_US0) Receive Pointer Register
#define AT91C_US0_TPR             (0xFFFB0108) // (PDC_US0) Transmit Pointer Register
// ========== Register definition for US0 peripheral ==========
#define AT91C_US0_IF              (0xFFFB004C) // (US0) IRDA_FILTER Register
#define AT91C_US0_NER             (0xFFFB0044) // (US0) Nb Errors Register
#define AT91C_US0_RTOR            (0xFFFB0024) // (US0) Receiver Time-out Register
#define AT91C_US0_THR             (0xFFFB001C) // (US0) Transmitter Holding Register
#define AT91C_US0_CSR             (0xFFFB0014) // (US0) Channel Status Register
#define AT91C_US0_IDR             (0xFFFB000C) // (US0) Interrupt Disable Register
#define AT91C_US0_MR              (0xFFFB0004) // (US0) Mode Register
#define AT91C_US0_MAN             (0xFFFB0050) // (US0) Manchester Encoder Decoder Register
#define AT91C_US0_FIDI            (0xFFFB0040) // (US0) FI_DI_Ratio Register
#define AT91C_US0_TTGR            (0xFFFB0028) // (US0) Transmitter Time-guard Register
#define AT91C_US0_BRGR            (0xFFFB0020) // (US0) Baud Rate Generator Register
#define AT91C_US0_RHR             (0xFFFB0018) // (US0) Receiver Holding Register
#define AT91C_US0_IMR             (0xFFFB0010) // (US0) Interrupt Mask Register
#define AT91C_US0_IER             (0xFFFB0008) // (US0) Interrupt Enable Register
#define AT91C_US0_CR              (0xFFFB0000) // (US0) Control Register
// ========== Register definition for PDC_US1 peripheral ==========
#define AT91C_US1_PTCR            (0xFFFB4120) // (PDC_US1) PDC Transfer Control Register
#define AT91C_US1_TNPR            (0xFFFB4118) // (PDC_US1) Transmit Next Pointer Register
#define AT91C_US1_RNPR            (0xFFFB4110) // (PDC_US1) Receive Next Pointer Register
#define AT91C_US1_TPR             (0xFFFB4108) // (PDC_US1) Transmit Pointer Register
#define AT91C_US1_RPR             (0xFFFB4100) // (PDC_US1) Receive Pointer Register
#define AT91C_US1_PTSR            (0xFFFB4124) // (PDC_US1) PDC Transfer Status Register
#define AT91C_US1_TNCR            (0xFFFB411C) // (PDC_US1) Transmit Next Counter Register
#define AT91C_US1_RNCR            (0xFFFB4114) // (PDC_US1) Receive Next Counter Register
#define AT91C_US1_TCR             (0xFFFB410C) // (PDC_US1) Transmit Counter Register
#define AT91C_US1_RCR             (0xFFFB4104) // (PDC_US1) Receive Counter Register
// ========== Register definition for US1 peripheral ==========
#define AT91C_US1_MAN             (0xFFFB4050) // (US1) Manchester Encoder Decoder Register
#define AT91C_US1_FIDI            (0xFFFB4040) // (US1) FI_DI_Ratio Register
#define AT91C_US1_TTGR            (0xFFFB4028) // (US1) Transmitter Time-guard Register
#define AT91C_US1_BRGR            (0xFFFB4020) // (US1) Baud Rate Generator Register
#define AT91C_US1_RHR             (0xFFFB4018) // (US1) Receiver Holding Register
#define AT91C_US1_IMR             (0xFFFB4010) // (US1) Interrupt Mask Register
#define AT91C_US1_IER             (0xFFFB4008) // (US1) Interrupt Enable Register
#define AT91C_US1_CR              (0xFFFB4000) // (US1) Control Register
#define AT91C_US1_IF              (0xFFFB404C) // (US1) IRDA_FILTER Register
#define AT91C_US1_NER             (0xFFFB4044) // (US1) Nb Errors Register
#define AT91C_US1_RTOR            (0xFFFB4024) // (US1) Receiver Time-out Register
#define AT91C_US1_THR             (0xFFFB401C) // (US1) Transmitter Holding Register
#define AT91C_US1_CSR             (0xFFFB4014) // (US1) Channel Status Register
#define AT91C_US1_IDR             (0xFFFB400C) // (US1) Interrupt Disable Register
#define AT91C_US1_MR              (0xFFFB4004) // (US1) Mode Register
// ========== Register definition for PDC_US2 peripheral ==========
#define AT91C_US2_PTSR            (0xFFFB8124) // (PDC_US2) PDC Transfer Status Register
#define AT91C_US2_TNCR            (0xFFFB811C) // (PDC_US2) Transmit Next Counter Register
#define AT91C_US2_RNCR            (0xFFFB8114) // (PDC_US2) Receive Next Counter Register
#define AT91C_US2_TCR             (0xFFFB810C) // (PDC_US2) Transmit Counter Register
#define AT91C_US2_RCR             (0xFFFB8104) // (PDC_US2) Receive Counter Register
#define AT91C_US2_PTCR            (0xFFFB8120) // (PDC_US2) PDC Transfer Control Register
#define AT91C_US2_TNPR            (0xFFFB8118) // (PDC_US2) Transmit Next Pointer Register
#define AT91C_US2_RNPR            (0xFFFB8110) // (PDC_US2) Receive Next Pointer Register
#define AT91C_US2_TPR             (0xFFFB8108) // (PDC_US2) Transmit Pointer Register
#define AT91C_US2_RPR             (0xFFFB8100) // (PDC_US2) Receive Pointer Register
// ========== Register definition for US2 peripheral ==========
#define AT91C_US2_MAN             (0xFFFB8050) // (US2) Manchester Encoder Decoder Register
#define AT91C_US2_FIDI            (0xFFFB8040) // (US2) FI_DI_Ratio Register
#define AT91C_US2_TTGR            (0xFFFB8028) // (US2) Transmitter Time-guard Register
#define AT91C_US2_BRGR            (0xFFFB8020) // (US2) Baud Rate Generator Register
#define AT91C_US2_RHR             (0xFFFB8018) // (US2) Receiver Holding Register
#define AT91C_US2_IMR             (0xFFFB8010) // (US2) Interrupt Mask Register
#define AT91C_US2_IER             (0xFFFB8008) // (US2) Interrupt Enable Register
#define AT91C_US2_CR              (0xFFFB8000) // (US2) Control Register
#define AT91C_US2_IF              (0xFFFB804C) // (US2) IRDA_FILTER Register
#define AT91C_US2_NER             (0xFFFB8044) // (US2) Nb Errors Register
#define AT91C_US2_RTOR            (0xFFFB8024) // (US2) Receiver Time-out Register
#define AT91C_US2_THR             (0xFFFB801C) // (US2) Transmitter Holding Register
#define AT91C_US2_CSR             (0xFFFB8014) // (US2) Channel Status Register
#define AT91C_US2_IDR             (0xFFFB800C) // (US2) Interrupt Disable Register
#define AT91C_US2_MR              (0xFFFB8004) // (US2) Mode Register
// ========== Register definition for PDC_US3 peripheral ==========
#define AT91C_US3_PTSR            (0xFFFBC124) // (PDC_US3) PDC Transfer Status Register
#define AT91C_US3_TNCR            (0xFFFBC11C) // (PDC_US3) Transmit Next Counter Register
#define AT91C_US3_RNCR            (0xFFFBC114) // (PDC_US3) Receive Next Counter Register
#define AT91C_US3_TCR             (0xFFFBC10C) // (PDC_US3) Transmit Counter Register
#define AT91C_US3_RCR             (0xFFFBC104) // (PDC_US3) Receive Counter Register
#define AT91C_US3_PTCR            (0xFFFBC120) // (PDC_US3) PDC Transfer Control Register
#define AT91C_US3_TNPR            (0xFFFBC118) // (PDC_US3) Transmit Next Pointer Register
#define AT91C_US3_RNPR            (0xFFFBC110) // (PDC_US3) Receive Next Pointer Register
#define AT91C_US3_TPR             (0xFFFBC108) // (PDC_US3) Transmit Pointer Register
#define AT91C_US3_RPR             (0xFFFBC100) // (PDC_US3) Receive Pointer Register
// ========== Register definition for US3 peripheral ==========
#define AT91C_US3_MAN             (0xFFFBC050) // (US3) Manchester Encoder Decoder Register
#define AT91C_US3_BRGR            (0xFFFBC020) // (US3) Baud Rate Generator Register
#define AT91C_US3_RHR             (0xFFFBC018) // (US3) Receiver Holding Register
#define AT91C_US3_IMR             (0xFFFBC010) // (US3) Interrupt Mask Register
#define AT91C_US3_IER             (0xFFFBC008) // (US3) Interrupt Enable Register
#define AT91C_US3_CR              (0xFFFBC000) // (US3) Control Register
#define AT91C_US3_RTOR            (0xFFFBC024) // (US3) Receiver Time-out Register
#define AT91C_US3_THR             (0xFFFBC01C) // (US3) Transmitter Holding Register
#define AT91C_US3_CSR             (0xFFFBC014) // (US3) Channel Status Register
#define AT91C_US3_FIDI            (0xFFFBC040) // (US3) FI_DI_Ratio Register
#define AT91C_US3_TTGR            (0xFFFBC028) // (US3) Transmitter Time-guard Register
#define AT91C_US3_IF              (0xFFFBC04C) // (US3) IRDA_FILTER Register
#define AT91C_US3_NER             (0xFFFBC044) // (US3) Nb Errors Register
#define AT91C_US3_MR              (0xFFFBC004) // (US3) Mode Register
#define AT91C_US3_IDR             (0xFFFBC00C) // (US3) Interrupt Disable Register
// ========== Register definition for PDC_SSC0 peripheral ==========
#define AT91C_SSC0_PTCR           (0xFFFC0120) // (PDC_SSC0) PDC Transfer Control Register
#define AT91C_SSC0_TNPR           (0xFFFC0118) // (PDC_SSC0) Transmit Next Pointer Register
#define AT91C_SSC0_RNPR           (0xFFFC0110) // (PDC_SSC0) Receive Next Pointer Register
#define AT91C_SSC0_TPR            (0xFFFC0108) // (PDC_SSC0) Transmit Pointer Register
#define AT91C_SSC0_RPR            (0xFFFC0100) // (PDC_SSC0) Receive Pointer Register
#define AT91C_SSC0_PTSR           (0xFFFC0124) // (PDC_SSC0) PDC Transfer Status Register
#define AT91C_SSC0_TNCR           (0xFFFC011C) // (PDC_SSC0) Transmit Next Counter Register
#define AT91C_SSC0_RNCR           (0xFFFC0114) // (PDC_SSC0) Receive Next Counter Register
#define AT91C_SSC0_TCR            (0xFFFC010C) // (PDC_SSC0) Transmit Counter Register
#define AT91C_SSC0_RCR            (0xFFFC0104) // (PDC_SSC0) Receive Counter Register
// ========== Register definition for SSC0 peripheral ==========
#define AT91C_SSC0_RSHR           (0xFFFC0030) // (SSC0) Receive Sync Holding Register
#define AT91C_SSC0_RHR            (0xFFFC0020) // (SSC0) Receive Holding Register
#define AT91C_SSC0_TCMR           (0xFFFC0018) // (SSC0) Transmit Clock Mode Register
#define AT91C_SSC0_RCMR           (0xFFFC0010) // (SSC0) Receive Clock ModeRegister
#define AT91C_SSC0_IER            (0xFFFC0044) // (SSC0) Interrupt Enable Register
#define AT91C_SSC0_TSHR           (0xFFFC0034) // (SSC0) Transmit Sync Holding Register
#define AT91C_SSC0_THR            (0xFFFC0024) // (SSC0) Transmit Holding Register
#define AT91C_SSC0_IDR            (0xFFFC0048) // (SSC0) Interrupt Disable Register
#define AT91C_SSC0_SR             (0xFFFC0040) // (SSC0) Status Register
#define AT91C_SSC0_CR             (0xFFFC0000) // (SSC0) Control Register
#define AT91C_SSC0_IMR            (0xFFFC004C) // (SSC0) Interrupt Mask Register
#define AT91C_SSC0_CMR            (0xFFFC0004) // (SSC0) Clock Mode Register
#define AT91C_SSC0_RFMR           (0xFFFC0014) // (SSC0) Receive Frame Mode Register
#define AT91C_SSC0_TFMR           (0xFFFC001C) // (SSC0) Transmit Frame Mode Register
// ========== Register definition for PDC_SSC1 peripheral ==========
#define AT91C_SSC1_PTSR           (0xFFFC4124) // (PDC_SSC1) PDC Transfer Status Register
#define AT91C_SSC1_TNCR           (0xFFFC411C) // (PDC_SSC1) Transmit Next Counter Register
#define AT91C_SSC1_RNCR           (0xFFFC4114) // (PDC_SSC1) Receive Next Counter Register
#define AT91C_SSC1_TCR            (0xFFFC410C) // (PDC_SSC1) Transmit Counter Register
#define AT91C_SSC1_RCR            (0xFFFC4104) // (PDC_SSC1) Receive Counter Register
#define AT91C_SSC1_PTCR           (0xFFFC4120) // (PDC_SSC1) PDC Transfer Control Register
#define AT91C_SSC1_TNPR           (0xFFFC4118) // (PDC_SSC1) Transmit Next Pointer Register
#define AT91C_SSC1_RNPR           (0xFFFC4110) // (PDC_SSC1) Receive Next Pointer Register
#define AT91C_SSC1_TPR            (0xFFFC4108) // (PDC_SSC1) Transmit Pointer Register
#define AT91C_SSC1_RPR            (0xFFFC4100) // (PDC_SSC1) Receive Pointer Register
// ========== Register definition for SSC1 peripheral ==========
#define AT91C_SSC1_IDR            (0xFFFC4048) // (SSC1) Interrupt Disable Register
#define AT91C_SSC1_TCMR           (0xFFFC4018) // (SSC1) Transmit Clock Mode Register
#define AT91C_SSC1_RCMR           (0xFFFC4010) // (SSC1) Receive Clock ModeRegister
#define AT91C_SSC1_CR             (0xFFFC4000) // (SSC1) Control Register
#define AT91C_SSC1_THR            (0xFFFC4024) // (SSC1) Transmit Holding Register
#define AT91C_SSC1_TFMR           (0xFFFC401C) // (SSC1) Transmit Frame Mode Register
#define AT91C_SSC1_RFMR           (0xFFFC4014) // (SSC1) Receive Frame Mode Register
#define AT91C_SSC1_SR             (0xFFFC4040) // (SSC1) Status Register
#define AT91C_SSC1_RHR            (0xFFFC4020) // (SSC1) Receive Holding Register
#define AT91C_SSC1_RSHR           (0xFFFC4030) // (SSC1) Receive Sync Holding Register
#define AT91C_SSC1_IMR            (0xFFFC404C) // (SSC1) Interrupt Mask Register
#define AT91C_SSC1_IER            (0xFFFC4044) // (SSC1) Interrupt Enable Register
#define AT91C_SSC1_CMR            (0xFFFC4004) // (SSC1) Clock Mode Register
#define AT91C_SSC1_TSHR           (0xFFFC4034) // (SSC1) Transmit Sync Holding Register
// ========== Register definition for PWMC_CH0 peripheral ==========
#define AT91C_PWMC_CH0_CUPDR      (0xFFFC8210) // (PWMC_CH0) Channel Update Register
#define AT91C_PWMC_CH0_CPRDR      (0xFFFC8208) // (PWMC_CH0) Channel Period Register
#define AT91C_PWMC_CH0_CMR        (0xFFFC8200) // (PWMC_CH0) Channel Mode Register
#define AT91C_PWMC_CH0_Reserved   (0xFFFC8214) // (PWMC_CH0) Reserved
#define AT91C_PWMC_CH0_CCNTR      (0xFFFC820C) // (PWMC_CH0) Channel Counter Register
#define AT91C_PWMC_CH0_CDTYR      (0xFFFC8204) // (PWMC_CH0) Channel Duty Cycle Register
// ========== Register definition for PWMC_CH1 peripheral ==========
#define AT91C_PWMC_CH1_CUPDR      (0xFFFC8230) // (PWMC_CH1) Channel Update Register
#define AT91C_PWMC_CH1_CPRDR      (0xFFFC8228) // (PWMC_CH1) Channel Period Register
#define AT91C_PWMC_CH1_CMR        (0xFFFC8220) // (PWMC_CH1) Channel Mode Register
#define AT91C_PWMC_CH1_Reserved   (0xFFFC8234) // (PWMC_CH1) Reserved
#define AT91C_PWMC_CH1_CCNTR      (0xFFFC822C) // (PWMC_CH1) Channel Counter Register
#define AT91C_PWMC_CH1_CDTYR      (0xFFFC8224) // (PWMC_CH1) Channel Duty Cycle Register
// ========== Register definition for PWMC_CH2 peripheral ==========
#define AT91C_PWMC_CH2_CUPDR      (0xFFFC8250) // (PWMC_CH2) Channel Update Register
#define AT91C_PWMC_CH2_CPRDR      (0xFFFC8248) // (PWMC_CH2) Channel Period Register
#define AT91C_PWMC_CH2_CMR        (0xFFFC8240) // (PWMC_CH2) Channel Mode Register
#define AT91C_PWMC_CH2_Reserved   (0xFFFC8254) // (PWMC_CH2) Reserved
#define AT91C_PWMC_CH2_CCNTR      (0xFFFC824C) // (PWMC_CH2) Channel Counter Register
#define AT91C_PWMC_CH2_CDTYR      (0xFFFC8244) // (PWMC_CH2) Channel Duty Cycle Register
// ========== Register definition for PWMC_CH3 peripheral ==========
#define AT91C_PWMC_CH3_CUPDR      (0xFFFC8270) // (PWMC_CH3) Channel Update Register
#define AT91C_PWMC_CH3_CPRDR      (0xFFFC8268) // (PWMC_CH3) Channel Period Register
#define AT91C_PWMC_CH3_CMR        (0xFFFC8260) // (PWMC_CH3) Channel Mode Register
#define AT91C_PWMC_CH3_Reserved   (0xFFFC8274) // (PWMC_CH3) Reserved
#define AT91C_PWMC_CH3_CCNTR      (0xFFFC826C) // (PWMC_CH3) Channel Counter Register
#define AT91C_PWMC_CH3_CDTYR      (0xFFFC8264) // (PWMC_CH3) Channel Duty Cycle Register
// ========== Register definition for PWMC peripheral ==========
#define AT91C_PWMC_VR             (0xFFFC80FC) // (PWMC) PWMC Version Register
#define AT91C_PWMC_IMR            (0xFFFC8018) // (PWMC) PWMC Interrupt Mask Register
#define AT91C_PWMC_IER            (0xFFFC8010) // (PWMC) PWMC Interrupt Enable Register
#define AT91C_PWMC_DIS            (0xFFFC8008) // (PWMC) PWMC Disable Register
#define AT91C_PWMC_MR             (0xFFFC8000) // (PWMC) PWMC Mode Register
#define AT91C_PWMC_ISR            (0xFFFC801C) // (PWMC) PWMC Interrupt Status Register
#define AT91C_PWMC_IDR            (0xFFFC8014) // (PWMC) PWMC Interrupt Disable Register
#define AT91C_PWMC_SR             (0xFFFC800C) // (PWMC) PWMC Status Register
#define AT91C_PWMC_ENA            (0xFFFC8004) // (PWMC) PWMC Enable Register
// ========== Register definition for PDC_SPI peripheral ==========
#define AT91C_SPI_PTSR            (0xFFFCC124) // (PDC_SPI) PDC Transfer Status Register
#define AT91C_SPI_TNCR            (0xFFFCC11C) // (PDC_SPI) Transmit Next Counter Register
#define AT91C_SPI_RNCR            (0xFFFCC114) // (PDC_SPI) Receive Next Counter Register
#define AT91C_SPI_TCR             (0xFFFCC10C) // (PDC_SPI) Transmit Counter Register
#define AT91C_SPI_RCR             (0xFFFCC104) // (PDC_SPI) Receive Counter Register
#define AT91C_SPI_PTCR            (0xFFFCC120) // (PDC_SPI) PDC Transfer Control Register
#define AT91C_SPI_TNPR            (0xFFFCC118) // (PDC_SPI) Transmit Next Pointer Register
#define AT91C_SPI_RNPR            (0xFFFCC110) // (PDC_SPI) Receive Next Pointer Register
#define AT91C_SPI_TPR             (0xFFFCC108) // (PDC_SPI) Transmit Pointer Register
#define AT91C_SPI_RPR             (0xFFFCC100) // (PDC_SPI) Receive Pointer Register
// ========== Register definition for SPI peripheral ==========
#define AT91C_SPI_IMR             (0xFFFCC01C) // (SPI) Interrupt Mask Register
#define AT91C_SPI_IER             (0xFFFCC014) // (SPI) Interrupt Enable Register
#define AT91C_SPI_TDR             (0xFFFCC00C) // (SPI) Transmit Data Register
#define AT91C_SPI_MR              (0xFFFCC004) // (SPI) Mode Register
#define AT91C_SPI_CSR             (0xFFFCC030) // (SPI) Chip Select Register
#define AT91C_SPI_IDR             (0xFFFCC018) // (SPI) Interrupt Disable Register
#define AT91C_SPI_SR              (0xFFFCC010) // (SPI) Status Register
#define AT91C_SPI_RDR             (0xFFFCC008) // (SPI) Receive Data Register
#define AT91C_SPI_CR              (0xFFFCC000) // (SPI) Control Register
// ========== Register definition for PDC_TSC peripheral ==========
#define AT91C_TSC_PTCR            (0xFFFD0120) // (PDC_TSC) PDC Transfer Control Register
#define AT91C_TSC_TNPR            (0xFFFD0118) // (PDC_TSC) Transmit Next Pointer Register
#define AT91C_TSC_RNPR            (0xFFFD0110) // (PDC_TSC) Receive Next Pointer Register
#define AT91C_TSC_TPR             (0xFFFD0108) // (PDC_TSC) Transmit Pointer Register
#define AT91C_TSC_RPR             (0xFFFD0100) // (PDC_TSC) Receive Pointer Register
#define AT91C_TSC_PTSR            (0xFFFD0124) // (PDC_TSC) PDC Transfer Status Register
#define AT91C_TSC_TNCR            (0xFFFD011C) // (PDC_TSC) Transmit Next Counter Register
#define AT91C_TSC_RNCR            (0xFFFD0114) // (PDC_TSC) Receive Next Counter Register
#define AT91C_TSC_TCR             (0xFFFD010C) // (PDC_TSC) Transmit Counter Register
#define AT91C_TSC_RCR             (0xFFFD0104) // (PDC_TSC) Receive Counter Register
// ========== Register definition for TSC peripheral ==========
// ========== Register definition for UDPHS_EPTFIFO peripheral ==========
#define AT91C_UDPHS_EPTFIFO_READEPT5 (0x00650000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 5
#define AT91C_UDPHS_EPTFIFO_READEPT4 (0x00640000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 4
#define AT91C_UDPHS_EPTFIFO_READEPT2 (0x00620000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 2
#define AT91C_UDPHS_EPTFIFO_READEPTC (0x006C0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 12
#define AT91C_UDPHS_EPTFIFO_READEPT6 (0x00660000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 6
#define AT91C_UDPHS_EPTFIFO_READEPT3 (0x00630000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 3
#define AT91C_UDPHS_EPTFIFO_READEPTD (0x006D0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 13
#define AT91C_UDPHS_EPTFIFO_READEPTB (0x006B0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 11
#define AT91C_UDPHS_EPTFIFO_READEPT7 (0x00670000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 7
#define AT91C_UDPHS_EPTFIFO_READEPT0 (0x00600000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 0
#define AT91C_UDPHS_EPTFIFO_READEPTF (0x006F0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 15
#define AT91C_UDPHS_EPTFIFO_READEPTE (0x006E0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 14
#define AT91C_UDPHS_EPTFIFO_READEPTA (0x006A0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 10
#define AT91C_UDPHS_EPTFIFO_READEPT9 (0x00690000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 9
#define AT91C_UDPHS_EPTFIFO_READEPT8 (0x00680000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 8
#define AT91C_UDPHS_EPTFIFO_READEPT1 (0x00610000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 1
// ========== Register definition for UDPHS_EPT_0 peripheral ==========
#define AT91C_UDPHS_EPT_0_EPTCLRSTA (0xFFFD4118) // (UDPHS_EPT_0) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_0_EPTCTLDIS (0xFFFD4108) // (UDPHS_EPT_0) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_0_EPTCFG  (0xFFFD4100) // (UDPHS_EPT_0) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_0_EPTSTA  (0xFFFD411C) // (UDPHS_EPT_0) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_0_EPTSETSTA (0xFFFD4114) // (UDPHS_EPT_0) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_0_EPTCTL  (0xFFFD410C) // (UDPHS_EPT_0) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_0_EPTCTLENB (0xFFFD4104) // (UDPHS_EPT_0) UDPHS Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_1 peripheral ==========
#define AT91C_UDPHS_EPT_1_EPTCLRSTA (0xFFFD4138) // (UDPHS_EPT_1) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_1_EPTCTLDIS (0xFFFD4128) // (UDPHS_EPT_1) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_1_EPTCFG  (0xFFFD4120) // (UDPHS_EPT_1) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_1_EPTSTA  (0xFFFD413C) // (UDPHS_EPT_1) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_1_EPTSETSTA (0xFFFD4134) // (UDPHS_EPT_1) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_1_EPTCTL  (0xFFFD412C) // (UDPHS_EPT_1) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_1_EPTCTLENB (0xFFFD4124) // (UDPHS_EPT_1) UDPHS Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_2 peripheral ==========
#define AT91C_UDPHS_EPT_2_EPTCLRSTA (0xFFFD4158) // (UDPHS_EPT_2) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_2_EPTCTLDIS (0xFFFD4148) // (UDPHS_EPT_2) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_2_EPTCFG  (0xFFFD4140) // (UDPHS_EPT_2) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_2_EPTSTA  (0xFFFD415C) // (UDPHS_EPT_2) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_2_EPTSETSTA (0xFFFD4154) // (UDPHS_EPT_2) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_2_EPTCTL  (0xFFFD414C) // (UDPHS_EPT_2) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_2_EPTCTLENB (0xFFFD4144) // (UDPHS_EPT_2) UDPHS Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_3 peripheral ==========
#define AT91C_UDPHS_EPT_3_EPTCLRSTA (0xFFFD4178) // (UDPHS_EPT_3) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_3_EPTCTLDIS (0xFFFD4168) // (UDPHS_EPT_3) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_3_EPTCFG  (0xFFFD4160) // (UDPHS_EPT_3) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_3_EPTSTA  (0xFFFD417C) // (UDPHS_EPT_3) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_3_EPTSETSTA (0xFFFD4174) // (UDPHS_EPT_3) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_3_EPTCTL  (0xFFFD416C) // (UDPHS_EPT_3) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_3_EPTCTLENB (0xFFFD4164) // (UDPHS_EPT_3) UDPHS Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_4 peripheral ==========
#define AT91C_UDPHS_EPT_4_EPTCLRSTA (0xFFFD4198) // (UDPHS_EPT_4) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_4_EPTCTLDIS (0xFFFD4188) // (UDPHS_EPT_4) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_4_EPTCFG  (0xFFFD4180) // (UDPHS_EPT_4) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_4_EPTSTA  (0xFFFD419C) // (UDPHS_EPT_4) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_4_EPTSETSTA (0xFFFD4194) // (UDPHS_EPT_4) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_4_EPTCTL  (0xFFFD418C) // (UDPHS_EPT_4) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_4_EPTCTLENB (0xFFFD4184) // (UDPHS_EPT_4) UDPHS Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_5 peripheral ==========
#define AT91C_UDPHS_EPT_5_EPTCTLDIS (0xFFFD41A8) // (UDPHS_EPT_5) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_5_EPTCFG  (0xFFFD41A0) // (UDPHS_EPT_5) UDPHS Endpoint Config Register
#define AT91C_UDPHS_EPT_5_EPTSTA  (0xFFFD41BC) // (UDPHS_EPT_5) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_5_EPTSETSTA (0xFFFD41B4) // (UDPHS_EPT_5) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_5_EPTCTL  (0xFFFD41AC) // (UDPHS_EPT_5) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_5_EPTCTLENB (0xFFFD41A4) // (UDPHS_EPT_5) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_5_EPTCLRSTA (0xFFFD41B8) // (UDPHS_EPT_5) UDPHS Endpoint Clear Status Register
// ========== Register definition for UDPHS_EPT_6 peripheral ==========
#define AT91C_UDPHS_EPT_6_EPTSTA  (0xFFFD41DC) // (UDPHS_EPT_6) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_6_EPTSETSTA (0xFFFD41D4) // (UDPHS_EPT_6) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_6_EPTCTL  (0xFFFD41CC) // (UDPHS_EPT_6) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_6_EPTCTLENB (0xFFFD41C4) // (UDPHS_EPT_6) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_6_EPTCLRSTA (0xFFFD41D8) // (UDPHS_EPT_6) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_6_EPTCTLDIS (0xFFFD41C8) // (UDPHS_EPT_6) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_6_EPTCFG  (0xFFFD41C0) // (UDPHS_EPT_6) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_7 peripheral ==========
#define AT91C_UDPHS_EPT_7_EPTSTA  (0xFFFD41FC) // (UDPHS_EPT_7) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_7_EPTSETSTA (0xFFFD41F4) // (UDPHS_EPT_7) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_7_EPTCTL  (0xFFFD41EC) // (UDPHS_EPT_7) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_7_EPTCTLENB (0xFFFD41E4) // (UDPHS_EPT_7) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_7_EPTCLRSTA (0xFFFD41F8) // (UDPHS_EPT_7) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_7_EPTCTLDIS (0xFFFD41E8) // (UDPHS_EPT_7) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_7_EPTCFG  (0xFFFD41E0) // (UDPHS_EPT_7) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_8 peripheral ==========
#define AT91C_UDPHS_EPT_8_EPTSTA  (0xFFFD421C) // (UDPHS_EPT_8) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_8_EPTSETSTA (0xFFFD4214) // (UDPHS_EPT_8) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_8_EPTCTL  (0xFFFD420C) // (UDPHS_EPT_8) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_8_EPTCTLENB (0xFFFD4204) // (UDPHS_EPT_8) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_8_EPTCLRSTA (0xFFFD4218) // (UDPHS_EPT_8) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_8_EPTCTLDIS (0xFFFD4208) // (UDPHS_EPT_8) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_8_EPTCFG  (0xFFFD4200) // (UDPHS_EPT_8) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_9 peripheral ==========
#define AT91C_UDPHS_EPT_9_EPTSTA  (0xFFFD423C) // (UDPHS_EPT_9) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_9_EPTSETSTA (0xFFFD4234) // (UDPHS_EPT_9) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_9_EPTCTL  (0xFFFD422C) // (UDPHS_EPT_9) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_9_EPTCTLENB (0xFFFD4224) // (UDPHS_EPT_9) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_9_EPTCLRSTA (0xFFFD4238) // (UDPHS_EPT_9) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_9_EPTCTLDIS (0xFFFD4228) // (UDPHS_EPT_9) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_9_EPTCFG  (0xFFFD4220) // (UDPHS_EPT_9) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_10 peripheral ==========
#define AT91C_UDPHS_EPT_10_EPTSTA (0xFFFD425C) // (UDPHS_EPT_10) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_10_EPTSETSTA (0xFFFD4254) // (UDPHS_EPT_10) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_10_EPTCTL (0xFFFD424C) // (UDPHS_EPT_10) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_10_EPTCTLENB (0xFFFD4244) // (UDPHS_EPT_10) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_10_EPTCLRSTA (0xFFFD4258) // (UDPHS_EPT_10) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_10_EPTCTLDIS (0xFFFD4248) // (UDPHS_EPT_10) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_10_EPTCFG (0xFFFD4240) // (UDPHS_EPT_10) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_11 peripheral ==========
#define AT91C_UDPHS_EPT_11_EPTSTA (0xFFFD427C) // (UDPHS_EPT_11) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_11_EPTSETSTA (0xFFFD4274) // (UDPHS_EPT_11) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_11_EPTCTL (0xFFFD426C) // (UDPHS_EPT_11) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_11_EPTCTLENB (0xFFFD4264) // (UDPHS_EPT_11) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_11_EPTCLRSTA (0xFFFD4278) // (UDPHS_EPT_11) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_11_EPTCTLDIS (0xFFFD4268) // (UDPHS_EPT_11) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_11_EPTCFG (0xFFFD4260) // (UDPHS_EPT_11) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_12 peripheral ==========
#define AT91C_UDPHS_EPT_12_EPTSTA (0xFFFD429C) // (UDPHS_EPT_12) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_12_EPTSETSTA (0xFFFD4294) // (UDPHS_EPT_12) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_12_EPTCTL (0xFFFD428C) // (UDPHS_EPT_12) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_12_EPTCTLENB (0xFFFD4284) // (UDPHS_EPT_12) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_12_EPTCLRSTA (0xFFFD4298) // (UDPHS_EPT_12) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_12_EPTCTLDIS (0xFFFD4288) // (UDPHS_EPT_12) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_12_EPTCFG (0xFFFD4280) // (UDPHS_EPT_12) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_13 peripheral ==========
#define AT91C_UDPHS_EPT_13_EPTSTA (0xFFFD42BC) // (UDPHS_EPT_13) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_13_EPTSETSTA (0xFFFD42B4) // (UDPHS_EPT_13) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_13_EPTCTL (0xFFFD42AC) // (UDPHS_EPT_13) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_13_EPTCTLENB (0xFFFD42A4) // (UDPHS_EPT_13) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_13_EPTCLRSTA (0xFFFD42B8) // (UDPHS_EPT_13) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_13_EPTCTLDIS (0xFFFD42A8) // (UDPHS_EPT_13) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_13_EPTCFG (0xFFFD42A0) // (UDPHS_EPT_13) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_14 peripheral ==========
#define AT91C_UDPHS_EPT_14_EPTSTA (0xFFFD42DC) // (UDPHS_EPT_14) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_14_EPTSETSTA (0xFFFD42D4) // (UDPHS_EPT_14) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_14_EPTCTLENB (0xFFFD42C4) // (UDPHS_EPT_14) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_14_EPTCTL (0xFFFD42CC) // (UDPHS_EPT_14) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_14_EPTCLRSTA (0xFFFD42D8) // (UDPHS_EPT_14) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_14_EPTCTLDIS (0xFFFD42C8) // (UDPHS_EPT_14) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_14_EPTCFG (0xFFFD42C0) // (UDPHS_EPT_14) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_EPT_15 peripheral ==========
#define AT91C_UDPHS_EPT_15_EPTSTA (0xFFFD42FC) // (UDPHS_EPT_15) UDPHS Endpoint Status Register
#define AT91C_UDPHS_EPT_15_EPTSETSTA (0xFFFD42F4) // (UDPHS_EPT_15) UDPHS Endpoint Set Status Register
#define AT91C_UDPHS_EPT_15_EPTCTL (0xFFFD42EC) // (UDPHS_EPT_15) UDPHS Endpoint Control Register
#define AT91C_UDPHS_EPT_15_EPTCTLENB (0xFFFD42E4) // (UDPHS_EPT_15) UDPHS Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_15_EPTCLRSTA (0xFFFD42F8) // (UDPHS_EPT_15) UDPHS Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_15_EPTCTLDIS (0xFFFD42E8) // (UDPHS_EPT_15) UDPHS Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_15_EPTCFG (0xFFFD42E0) // (UDPHS_EPT_15) UDPHS Endpoint Config Register
// ========== Register definition for UDPHS_DMA_1 peripheral ==========
#define AT91C_UDPHS_DMA_1_DMASTATUS (0xFFFD431C) // (UDPHS_DMA_1) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_1_DMAADDRESS (0xFFFD4314) // (UDPHS_DMA_1) UDPHS DMA Channel Address Register
#define AT91C_UDPHS_DMA_1_DMACONTROL (0xFFFD4318) // (UDPHS_DMA_1) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_1_DMANXTDSC (0xFFFD4310) // (UDPHS_DMA_1) UDPHS DMA Channel Next Descriptor Address
// ========== Register definition for UDPHS_DMA_2 peripheral ==========
#define AT91C_UDPHS_DMA_2_DMASTATUS (0xFFFD432C) // (UDPHS_DMA_2) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_2_DMAADDRESS (0xFFFD4324) // (UDPHS_DMA_2) UDPHS DMA Channel Address Register
#define AT91C_UDPHS_DMA_2_DMACONTROL (0xFFFD4328) // (UDPHS_DMA_2) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_2_DMANXTDSC (0xFFFD4320) // (UDPHS_DMA_2) UDPHS DMA Channel Next Descriptor Address
// ========== Register definition for UDPHS_DMA_3 peripheral ==========
#define AT91C_UDPHS_DMA_3_DMAADDRESS (0xFFFD4334) // (UDPHS_DMA_3) UDPHS DMA Channel Address Register
#define AT91C_UDPHS_DMA_3_DMACONTROL (0xFFFD4338) // (UDPHS_DMA_3) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_3_DMANXTDSC (0xFFFD4330) // (UDPHS_DMA_3) UDPHS DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_3_DMASTATUS (0xFFFD433C) // (UDPHS_DMA_3) UDPHS DMA Channel Status Register
// ========== Register definition for UDPHS_DMA_4 peripheral ==========
#define AT91C_UDPHS_DMA_4_DMACONTROL (0xFFFD4348) // (UDPHS_DMA_4) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_4_DMANXTDSC (0xFFFD4340) // (UDPHS_DMA_4) UDPHS DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_4_DMASTATUS (0xFFFD434C) // (UDPHS_DMA_4) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_4_DMAADDRESS (0xFFFD4344) // (UDPHS_DMA_4) UDPHS DMA Channel Address Register
// ========== Register definition for UDPHS_DMA_5 peripheral ==========
#define AT91C_UDPHS_DMA_5_DMACONTROL (0xFFFD4358) // (UDPHS_DMA_5) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_5_DMANXTDSC (0xFFFD4350) // (UDPHS_DMA_5) UDPHS DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_5_DMASTATUS (0xFFFD435C) // (UDPHS_DMA_5) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_5_DMAADDRESS (0xFFFD4354) // (UDPHS_DMA_5) UDPHS DMA Channel Address Register
// ========== Register definition for UDPHS_DMA_6 peripheral ==========
#define AT91C_UDPHS_DMA_6_DMACONTROL (0xFFFD4368) // (UDPHS_DMA_6) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_6_DMANXTDSC (0xFFFD4360) // (UDPHS_DMA_6) UDPHS DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_6_DMASTATUS (0xFFFD436C) // (UDPHS_DMA_6) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_6_DMAADDRESS (0xFFFD4364) // (UDPHS_DMA_6) UDPHS DMA Channel Address Register
// ========== Register definition for UDPHS_DMA_7 peripheral ==========
#define AT91C_UDPHS_DMA_7_DMACONTROL (0xFFFD4378) // (UDPHS_DMA_7) UDPHS DMA Channel Control Register
#define AT91C_UDPHS_DMA_7_DMANXTDSC (0xFFFD4370) // (UDPHS_DMA_7) UDPHS DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_7_DMASTATUS (0xFFFD437C) // (UDPHS_DMA_7) UDPHS DMA Channel Status Register
#define AT91C_UDPHS_DMA_7_DMAADDRESS (0xFFFD4374) // (UDPHS_DMA_7) UDPHS DMA Channel Address Register
// ========== Register definition for UDPHS peripheral ==========
#define AT91C_UDPHS_INTSTA        (0xFFFD4014) // (UDPHS) UDPHS Interrupt Status Register
#define AT91C_UDPHS_FNUM          (0xFFFD4004) // (UDPHS) UDPHS Frame Number Register
#define AT91C_UDPHS_TSTSOFCNT     (0xFFFD40D0) // (UDPHS) UDPHS Test SOF Counter Register
#define AT91C_UDPHS_CLRINT        (0xFFFD4018) // (UDPHS) UDPHS Clear Interrupt Register
#define AT91C_UDPHS_IEN           (0xFFFD4010) // (UDPHS) UDPHS Interrupt Enable Register
#define AT91C_UDPHS_TSTMODREG     (0xFFFD40DC) // (UDPHS) UDPHS Test Mode Register
#define AT91C_UDPHS_TSTCNTA       (0xFFFD40D4) // (UDPHS) UDPHS Test A Counter Register
#define AT91C_UDPHS_IPFEATURES    (0xFFFD40F8) // (UDPHS) UDPHS Features Register
#define AT91C_UDPHS_RIPNAME1      (0xFFFD40F0) // (UDPHS) UDPHS Name1 Register
#define AT91C_UDPHS_TST           (0xFFFD40E0) // (UDPHS) UDPHS Test Register
#define AT91C_UDPHS_TSTCNTB       (0xFFFD40D8) // (UDPHS) UDPHS Test B Counter Register
#define AT91C_UDPHS_CTRL          (0xFFFD4000) // (UDPHS) UDPHS Control Register
#define AT91C_UDPHS_IPVERSION     (0xFFFD40FC) // (UDPHS) UDPHS Version Register
#define AT91C_UDPHS_RIPNAME2      (0xFFFD40F4) // (UDPHS) UDPHS Name2 Register
#define AT91C_UDPHS_RIPPADDRSIZE  (0xFFFD40EC) // (UDPHS) UDPHS PADDRSIZE Register
#define AT91C_UDPHS_EPTRST        (0xFFFD401C) // (UDPHS) UDPHS Endpoints Reset Register
// ========== Register definition for PDC_AC97C peripheral ==========
#define AT91C_AC97C_PTCR          (0xFFFD8120) // (PDC_AC97C) PDC Transfer Control Register
#define AT91C_AC97C_TNPR          (0xFFFD8118) // (PDC_AC97C) Transmit Next Pointer Register
#define AT91C_AC97C_RNPR          (0xFFFD8110) // (PDC_AC97C) Receive Next Pointer Register
#define AT91C_AC97C_TPR           (0xFFFD8108) // (PDC_AC97C) Transmit Pointer Register
#define AT91C_AC97C_RPR           (0xFFFD8100) // (PDC_AC97C) Receive Pointer Register
#define AT91C_AC97C_PTSR          (0xFFFD8124) // (PDC_AC97C) PDC Transfer Status Register
#define AT91C_AC97C_TNCR          (0xFFFD811C) // (PDC_AC97C) Transmit Next Counter Register
#define AT91C_AC97C_RNCR          (0xFFFD8114) // (PDC_AC97C) Receive Next Counter Register
#define AT91C_AC97C_TCR           (0xFFFD810C) // (PDC_AC97C) Transmit Counter Register
#define AT91C_AC97C_RCR           (0xFFFD8104) // (PDC_AC97C) Receive Counter Register
// ========== Register definition for AC97C peripheral ==========
#define AT91C_AC97C_CAMR          (0xFFFD802C) // (AC97C) Channel A Mode Register
#define AT91C_AC97C_CORHR         (0xFFFD8040) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_CBSR          (0xFFFD8038) // (AC97C) Channel B Status Register
#define AT91C_AC97C_CBRHR         (0xFFFD8030) // (AC97C) Channel B Receive Holding Register (optional)
#define AT91C_AC97C_CBTHR         (0xFFFD8034) // (AC97C) Channel B Transmit Holding Register (optional)
#define AT91C_AC97C_CBMR          (0xFFFD803C) // (AC97C) Channel B Mode Register
#define AT91C_AC97C_COTHR         (0xFFFD8044) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_COMR          (0xFFFD804C) // (AC97C) CODEC Mask Status Register
#define AT91C_AC97C_IER           (0xFFFD8054) // (AC97C) Interrupt Enable Register
#define AT91C_AC97C_COSR          (0xFFFD8048) // (AC97C) CODEC Status Register
#define AT91C_AC97C_SR            (0xFFFD8050) // (AC97C) Status Register
#define AT91C_AC97C_IDR           (0xFFFD8058) // (AC97C) Interrupt Disable Register
#define AT91C_AC97C_MR            (0xFFFD8008) // (AC97C) Mode Register
#define AT91C_AC97C_ICA           (0xFFFD8010) // (AC97C) Input Channel AssignementRegister
#define AT91C_AC97C_VERSION       (0xFFFD80FC) // (AC97C) Version Register
#define AT91C_AC97C_IMR           (0xFFFD805C) // (AC97C) Interrupt Mask Register
#define AT91C_AC97C_CATHR         (0xFFFD8024) // (AC97C) Channel A Transmit Holding Register
#define AT91C_AC97C_OCA           (0xFFFD8014) // (AC97C) Output Channel Assignement Register
#define AT91C_AC97C_CASR          (0xFFFD8028) // (AC97C) Channel A Status Register
#define AT91C_AC97C_CARHR         (0xFFFD8020) // (AC97C) Channel A Receive Holding Register
// ========== Register definition for LCDC peripheral ==========
#define AT91C_LCDC_LUT_ENTRY      (0x00500C00) // (LCDC) LUT Entries Register
#define AT91C_LCDC_CTRSTVAL       (0x00500844) // (LCDC) Contrast Value Register
#define AT91C_LCDC_PWRCON         (0x0050083C) // (LCDC) Power Control Register
#define AT91C_LCDC_DP4_5          (0x00500834) // (LCDC) Dithering Pattern DP4_5 Register
#define AT91C_LCDC_DP5_7          (0x0050082C) // (LCDC) Dithering Pattern DP5_7 Register
#define AT91C_LCDC_DP3_5          (0x00500824) // (LCDC) Dithering Pattern DP3_5 Register
#define AT91C_LCDC_DP1_2          (0x0050081C) // (LCDC) Dithering Pattern DP1_2 Register
#define AT91C_LCDC_FIFO           (0x00500814) // (LCDC) LCD FIFO Register
#define AT91C_LCDC_TIM2           (0x0050080C) // (LCDC) LCD Timing Config 2 Register
#define AT91C_LCDC_LCDCON2        (0x00500804) // (LCDC) LCD Control 2 Register
#define AT91C_LCDC_DMACON         (0x0050001C) // (LCDC) DMA Control Register
#define AT91C_LCDC_FRMA2          (0x00500014) // (LCDC) DMA Frame Address Register 2
#define AT91C_LCDC_ITR            (0x00500860) // (LCDC) Interrupts Test Register
#define AT91C_LCDC_ICR            (0x00500858) // (LCDC) Interrupt Clear Register
#define AT91C_LCDC_IMR            (0x00500850) // (LCDC) Interrupt Mask Register
#define AT91C_LCDC_IER            (0x00500848) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_FRMP2          (0x0050000C) // (LCDC) DMA Frame Pointer Register 2
#define AT91C_LCDC_BA2            (0x00500004) // (LCDC) DMA Base Address Register 2
#define AT91C_LCDC_CTRSTCON       (0x00500840) // (LCDC) Contrast Control Register
#define AT91C_LCDC_DP6_7          (0x00500838) // (LCDC) Dithering Pattern DP6_7 Register
#define AT91C_LCDC_DP3_4          (0x00500830) // (LCDC) Dithering Pattern DP3_4 Register
#define AT91C_LCDC_DP2_3          (0x00500828) // (LCDC) Dithering Pattern DP2_3 Register
#define AT91C_LCDC_DP4_7          (0x00500820) // (LCDC) Dithering Pattern DP4_7 Register
#define AT91C_LCDC_MVAL           (0x00500818) // (LCDC) LCD Mode Toggle Rate Value Register
#define AT91C_LCDC_LCDFRCFG       (0x00500810) // (LCDC) LCD Frame Config Register
#define AT91C_LCDC_TIM1           (0x00500808) // (LCDC) LCD Timing Config 1 Register
#define AT91C_LCDC_LCDCON1        (0x00500800) // (LCDC) LCD Control 1 Register
#define AT91C_LCDC_DMA2DCFG       (0x00500020) // (LCDC) DMA 2D addressing configuration
#define AT91C_LCDC_FRMCFG         (0x00500018) // (LCDC) DMA Frame Configuration Register
#define AT91C_LCDC_IRR            (0x00500864) // (LCDC) Interrupts Raw Status Register
#define AT91C_LCDC_GPR            (0x0050085C) // (LCDC) General Purpose Register
#define AT91C_LCDC_ISR            (0x00500854) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_IDR            (0x0050084C) // (LCDC) Interrupt Disable Register
#define AT91C_LCDC_FRMA1          (0x00500010) // (LCDC) DMA Frame Address Register 1
#define AT91C_LCDC_FRMP1          (0x00500008) // (LCDC) DMA Frame Pointer Register 1
#define AT91C_LCDC_BA1            (0x00500000) // (LCDC) DMA Base Address Register 1
// ========== Register definition for LCDC_16B_TFT peripheral ==========
#define AT91C_TFT_LUT_ENTRY       (0x00500C00) // (LCDC_16B_TFT) LUT Entries Register
#define AT91C_TFT_CTRSTVAL        (0x00500844) // (LCDC_16B_TFT) Contrast Value Register
#define AT91C_TFT_PWRCON          (0x0050083C) // (LCDC_16B_TFT) Power Control Register
#define AT91C_TFT_DP4_5           (0x00500834) // (LCDC_16B_TFT) Dithering Pattern DP4_5 Register
#define AT91C_TFT_DP5_7           (0x0050082C) // (LCDC_16B_TFT) Dithering Pattern DP5_7 Register
#define AT91C_TFT_DP3_5           (0x00500824) // (LCDC_16B_TFT) Dithering Pattern DP3_5 Register
#define AT91C_TFT_DP1_2           (0x0050081C) // (LCDC_16B_TFT) Dithering Pattern DP1_2 Register
#define AT91C_TFT_FIFO            (0x00500814) // (LCDC_16B_TFT) LCD FIFO Register
#define AT91C_TFT_TIM2            (0x0050080C) // (LCDC_16B_TFT) LCD Timing Config 2 Register
#define AT91C_TFT_LCDCON2         (0x00500804) // (LCDC_16B_TFT) LCD Control 2 Register
#define AT91C_TFT_DMACON          (0x0050001C) // (LCDC_16B_TFT) DMA Control Register
#define AT91C_TFT_FRMA2           (0x00500014) // (LCDC_16B_TFT) DMA Frame Address Register 2
#define AT91C_TFT_ITR             (0x00500860) // (LCDC_16B_TFT) Interrupts Test Register
#define AT91C_TFT_ICR             (0x00500858) // (LCDC_16B_TFT) Interrupt Clear Register
#define AT91C_TFT_IMR             (0x00500850) // (LCDC_16B_TFT) Interrupt Mask Register
#define AT91C_TFT_IER             (0x00500848) // (LCDC_16B_TFT) Interrupt Enable Register
#define AT91C_TFT_FRMP2           (0x0050000C) // (LCDC_16B_TFT) DMA Frame Pointer Register 2
#define AT91C_TFT_BA2             (0x00500004) // (LCDC_16B_TFT) DMA Base Address Register 2
#define AT91C_TFT_CTRSTCON        (0x00500840) // (LCDC_16B_TFT) Contrast Control Register
#define AT91C_TFT_DP6_7           (0x00500838) // (LCDC_16B_TFT) Dithering Pattern DP6_7 Register
#define AT91C_TFT_DP3_4           (0x00500830) // (LCDC_16B_TFT) Dithering Pattern DP3_4 Register
#define AT91C_TFT_DP2_3           (0x00500828) // (LCDC_16B_TFT) Dithering Pattern DP2_3 Register
#define AT91C_TFT_DP4_7           (0x00500820) // (LCDC_16B_TFT) Dithering Pattern DP4_7 Register
#define AT91C_TFT_MVAL            (0x00500818) // (LCDC_16B_TFT) LCD Mode Toggle Rate Value Register
#define AT91C_TFT_LCDFRCFG        (0x00500810) // (LCDC_16B_TFT) LCD Frame Config Register
#define AT91C_TFT_TIM1            (0x00500808) // (LCDC_16B_TFT) LCD Timing Config 1 Register
#define AT91C_TFT_LCDCON1         (0x00500800) // (LCDC_16B_TFT) LCD Control 1 Register
#define AT91C_TFT_DMA2DCFG        (0x00500020) // (LCDC_16B_TFT) DMA 2D addressing configuration
#define AT91C_TFT_FRMCFG          (0x00500018) // (LCDC_16B_TFT) DMA Frame Configuration Register
#define AT91C_TFT_IRR             (0x00500864) // (LCDC_16B_TFT) Interrupts Raw Status Register
#define AT91C_TFT_GPR             (0x0050085C) // (LCDC_16B_TFT) General Purpose Register
#define AT91C_TFT_ISR             (0x00500854) // (LCDC_16B_TFT) Interrupt Enable Register
#define AT91C_TFT_IDR             (0x0050084C) // (LCDC_16B_TFT) Interrupt Disable Register
#define AT91C_TFT_FRMA1           (0x00500010) // (LCDC_16B_TFT) DMA Frame Address Register 1
#define AT91C_TFT_FRMP1           (0x00500008) // (LCDC_16B_TFT) DMA Frame Pointer Register 1
#define AT91C_TFT_BA1             (0x00500000) // (LCDC_16B_TFT) DMA Base Address Register 1
// ========== Register definition for HDMA_CH_0 peripheral ==========
#define AT91C_HDMA_CH_0_CADDR     (0xFFFFE660) // (HDMA_CH_0) HDMA Reserved
#define AT91C_HDMA_CH_0_DPIP      (0xFFFFE658) // (HDMA_CH_0) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_0_CFG       (0xFFFFE650) // (HDMA_CH_0) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_0_CTRLA     (0xFFFFE648) // (HDMA_CH_0) HDMA Channel Control A Register
#define AT91C_HDMA_CH_0_DADDR     (0xFFFFE640) // (HDMA_CH_0) HDMA Channel Destination Address Register
#define AT91C_HDMA_CH_0_BDSCR     (0xFFFFE65C) // (HDMA_CH_0) HDMA Reserved
#define AT91C_HDMA_CH_0_SPIP      (0xFFFFE654) // (HDMA_CH_0) HDMA Channel Source Picture in Picture Configuration Register
#define AT91C_HDMA_CH_0_CTRLB     (0xFFFFE64C) // (HDMA_CH_0) HDMA Channel Control B Register
#define AT91C_HDMA_CH_0_DSCR      (0xFFFFE644) // (HDMA_CH_0) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_0_SADDR     (0xFFFFE63C) // (HDMA_CH_0) HDMA Channel Source Address Register
// ========== Register definition for HDMA_CH_1 peripheral ==========
#define AT91C_HDMA_CH_1_CADDR     (0xFFFFE688) // (HDMA_CH_1) HDMA Reserved
#define AT91C_HDMA_CH_1_DPIP      (0xFFFFE680) // (HDMA_CH_1) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_1_CFG       (0xFFFFE678) // (HDMA_CH_1) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_1_CTRLA     (0xFFFFE670) // (HDMA_CH_1) HDMA Channel Control A Register
#define AT91C_HDMA_CH_1_DADDR     (0xFFFFE668) // (HDMA_CH_1) HDMA Channel Destination Address Register
#define AT91C_HDMA_CH_1_BDSCR     (0xFFFFE684) // (HDMA_CH_1) HDMA Reserved
#define AT91C_HDMA_CH_1_SPIP      (0xFFFFE67C) // (HDMA_CH_1) HDMA Channel Source Picture in Picture Configuration Register
#define AT91C_HDMA_CH_1_CTRLB     (0xFFFFE674) // (HDMA_CH_1) HDMA Channel Control B Register
#define AT91C_HDMA_CH_1_DSCR      (0xFFFFE66C) // (HDMA_CH_1) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_1_SADDR     (0xFFFFE664) // (HDMA_CH_1) HDMA Channel Source Address Register
// ========== Register definition for HDMA peripheral ==========
#define AT91C_HDMA_RSVD0          (0xFFFFE634) // (HDMA) HDMA Reserved
#define AT91C_HDMA_CHDR           (0xFFFFE62C) // (HDMA) HDMA Channel Handler Disable Register
#define AT91C_HDMA_EBCISR         (0xFFFFE624) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Status Register
#define AT91C_HDMA_EBCIDR         (0xFFFFE61C) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Disable register
#define AT91C_HDMA_SYNC           (0xFFFFE614) // (HDMA) HDMA Request Synchronization Register
#define AT91C_HDMA_BREQ           (0xFFFFE60C) // (HDMA) HDMA Software Chunk Transfer Request Register
#define AT91C_HDMA_EN             (0xFFFFE604) // (HDMA) HDMA Controller Enable Register
#define AT91C_HDMA_RSVD1          (0xFFFFE638) // (HDMA) HDMA Reserved
#define AT91C_HDMA_CHSR           (0xFFFFE630) // (HDMA) HDMA Channel Handler Status Register
#define AT91C_HDMA_CHER           (0xFFFFE628) // (HDMA) HDMA Channel Handler Enable Register
#define AT91C_HDMA_EBCIMR         (0xFFFFE620) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Mask Register
#define AT91C_HDMA_EBCIER         (0xFFFFE618) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Enable register
#define AT91C_HDMA_LAST           (0xFFFFE610) // (HDMA) HDMA Software Last Transfer Flag Register
#define AT91C_HDMA_SREQ           (0xFFFFE608) // (HDMA) HDMA Software Single Request Register
#define AT91C_HDMA_GCFG           (0xFFFFE600) // (HDMA) HDMA Global Configuration Register
// ========== Register definition for HECC peripheral ==========
#define AT91C_HECC_PR             (0xFFFFE80C) // (HECC)  ECC Parity register
#define AT91C_HECC_MR             (0xFFFFE804) // (HECC)  ECC Page size register
#define AT91C_HECC_NPR            (0xFFFFE810) // (HECC)  ECC Parity N register
#define AT91C_HECC_SR             (0xFFFFE808) // (HECC)  ECC Status register
#define AT91C_HECC_CR             (0xFFFFE800) // (HECC)  ECC reset register
#define AT91C_HECC_VR             (0xFFFFE8FC) // (HECC)  ECC Version register

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91SAM9RL64
// *****************************************************************************
#define AT91C_PIO_PA0             (1 <<  0) // Pin Controlled by PA0
#define AT91C_PA0_MC_DA0          (AT91C_PIO_PA0) //
#define AT91C_PIO_PA1             (1 <<  1) // Pin Controlled by PA1
#define AT91C_PA1_MC_CDA          (AT91C_PIO_PA1) //
#define AT91C_PIO_PA10            (1 << 10) // Pin Controlled by PA10
#define AT91C_PA10_CTS0           (AT91C_PIO_PA10) //
#define AT91C_PA10_RK0            (AT91C_PIO_PA10) //
#define AT91C_PIO_PA11            (1 << 11) // Pin Controlled by PA11
#define AT91C_PA11_TXD1           (AT91C_PIO_PA11) //
#define AT91C_PIO_PA12            (1 << 12) // Pin Controlled by PA12
#define AT91C_PA12_RXD1           (AT91C_PIO_PA12) //
#define AT91C_PIO_PA13            (1 << 13) // Pin Controlled by PA13
#define AT91C_PA13_TXD2           (AT91C_PIO_PA13) //
#define AT91C_PA13_TD1            (AT91C_PIO_PA13) //
#define AT91C_PIO_PA14            (1 << 14) // Pin Controlled by PA14
#define AT91C_PA14_RXD2           (AT91C_PIO_PA14) //
#define AT91C_PA14_RD1            (AT91C_PIO_PA14) //
#define AT91C_PIO_PA15            (1 << 15) // Pin Controlled by PA15
#define AT91C_PA15_TD0            (AT91C_PIO_PA15) //
#define AT91C_PIO_PA16            (1 << 16) // Pin Controlled by PA16
#define AT91C_PA16_RD0            (AT91C_PIO_PA16) //
#define AT91C_PIO_PA17            (1 << 17) // Pin Controlled by PA17
#define AT91C_PA17_AD0            (AT91C_PIO_PA17) //
#define AT91C_PIO_PA18            (1 << 18) // Pin Controlled by PA18
#define AT91C_PA18_AD1            (AT91C_PIO_PA18) //
#define AT91C_PA18_RTS1           (AT91C_PIO_PA18) //
#define AT91C_PIO_PA19            (1 << 19) // Pin Controlled by PA19
#define AT91C_PA19_AD2            (AT91C_PIO_PA19) //
#define AT91C_PA19_CTS1           (AT91C_PIO_PA19) //
#define AT91C_PIO_PA2             (1 <<  2) // Pin Controlled by PA2
#define AT91C_PA2_MC_CK           (AT91C_PIO_PA2) //
#define AT91C_PIO_PA20            (1 << 20) // Pin Controlled by PA20
#define AT91C_PA20_AD3            (AT91C_PIO_PA20) //
#define AT91C_PA20_SCK3           (AT91C_PIO_PA20) //
#define AT91C_PIO_PA21            (1 << 21) // Pin Controlled by PA21
#define AT91C_PA21_DRXD           (AT91C_PIO_PA21) //
#define AT91C_PIO_PA22            (1 << 22) // Pin Controlled by PA22
#define AT91C_PA22_DTXD           (AT91C_PIO_PA22) //
#define AT91C_PA22_RF0            (AT91C_PIO_PA22) //
#define AT91C_PIO_PA23            (1 << 23) // Pin Controlled by PA23
#define AT91C_PA23_TWD0           (AT91C_PIO_PA23) //
#define AT91C_PIO_PA24            (1 << 24) // Pin Controlled by PA24
#define AT91C_PA24_TWCK0          (AT91C_PIO_PA24) //
#define AT91C_PIO_PA25            (1 << 25) // Pin Controlled by PA25
#define AT91C_PA25_MISO           (AT91C_PIO_PA25) //
#define AT91C_PIO_PA26            (1 << 26) // Pin Controlled by PA26
#define AT91C_PA26_MOSI           (AT91C_PIO_PA26) //
#define AT91C_PIO_PA27            (1 << 27) // Pin Controlled by PA27
#define AT91C_PA27_SPCK           (AT91C_PIO_PA27) //
#define AT91C_PIO_PA28            (1 << 28) // Pin Controlled by PA28
#define AT91C_PA28_NPCS0          (AT91C_PIO_PA28) //
#define AT91C_PIO_PA29            (1 << 29) // Pin Controlled by PA29
#define AT91C_PA29_RTS2           (AT91C_PIO_PA29) //
#define AT91C_PA29_TF1            (AT91C_PIO_PA29) //
#define AT91C_PIO_PA3             (1 <<  3) // Pin Controlled by PA3
#define AT91C_PA3_MC_DA1          (AT91C_PIO_PA3) //
#define AT91C_PA3_TCLK0           (AT91C_PIO_PA3) //
#define AT91C_PIO_PA30            (1 << 30) // Pin Controlled by PA30
#define AT91C_PA30_CTS2           (AT91C_PIO_PA30) //
#define AT91C_PA30_TK1            (AT91C_PIO_PA30) //
#define AT91C_PIO_PA31            (1 << 31) // Pin Controlled by PA31
#define AT91C_PA31_NWAIT          (AT91C_PIO_PA31) //
#define AT91C_PA31_IRQ            (AT91C_PIO_PA31) //
#define AT91C_PIO_PA4             (1 <<  4) // Pin Controlled by PA4
#define AT91C_PA4_MC_DA2          (AT91C_PIO_PA4) //
#define AT91C_PA4_TIOA0           (AT91C_PIO_PA4) //
#define AT91C_PIO_PA5             (1 <<  5) // Pin Controlled by PA5
#define AT91C_PA5_MC_DA3          (AT91C_PIO_PA5) //
#define AT91C_PA5_TIOB0           (AT91C_PIO_PA5) //
#define AT91C_PIO_PA6             (1 <<  6) // Pin Controlled by PA6
#define AT91C_PA6_TXD0            (AT91C_PIO_PA6) //
#define AT91C_PIO_PA7             (1 <<  7) // Pin Controlled by PA7
#define AT91C_PA7_RXD0            (AT91C_PIO_PA7) //
#define AT91C_PIO_PA8             (1 <<  8) // Pin Controlled by PA8
#define AT91C_PA8_SCK0            (AT91C_PIO_PA8) //
#define AT91C_PA8_RF1             (AT91C_PIO_PA8) //
#define AT91C_PIO_PA9             (1 <<  9) // Pin Controlled by PA9
#define AT91C_PA9_RTS0            (AT91C_PIO_PA9) //
#define AT91C_PA9_RK1             (AT91C_PIO_PA9) //
#define AT91C_PIO_PB0             (1 <<  0) // Pin Controlled by PB0
#define AT91C_PB0_TXD3            (AT91C_PIO_PB0) //
#define AT91C_PIO_PB1             (1 <<  1) // Pin Controlled by PB1
#define AT91C_PB1_RXD3            (AT91C_PIO_PB1) //
#define AT91C_PIO_PB10            (1 << 10) // Pin Controlled by PB10
#define AT91C_PB10_A25_CFRNW      (AT91C_PIO_PB10) //
#define AT91C_PB10_FIQ            (AT91C_PIO_PB10) //
#define AT91C_PIO_PB11            (1 << 11) // Pin Controlled by PB11
#define AT91C_PB11_A18            (AT91C_PIO_PB11) //
#define AT91C_PIO_PB12            (1 << 12) // Pin Controlled by PB12
#define AT91C_PB12_A19            (AT91C_PIO_PB12) //
#define AT91C_PIO_PB13            (1 << 13) // Pin Controlled by PB13
#define AT91C_PB13_A20            (AT91C_PIO_PB13) //
#define AT91C_PIO_PB14            (1 << 14) // Pin Controlled by PB14
#define AT91C_PB14_A23            (AT91C_PIO_PB14) //
#define AT91C_PB14_PCK0           (AT91C_PIO_PB14) //
#define AT91C_PIO_PB15            (1 << 15) // Pin Controlled by PB15
#define AT91C_PB15_A24            (AT91C_PIO_PB15) //
#define AT91C_PB15_ADTRG          (AT91C_PIO_PB15) //
#define AT91C_PIO_PB16            (1 << 16) // Pin Controlled by PB16
#define AT91C_PB16_D16            (AT91C_PIO_PB16) //
#define AT91C_PIO_PB17            (1 << 17) // Pin Controlled by PB17
#define AT91C_PB17_D17            (AT91C_PIO_PB17) //
#define AT91C_PIO_PB18            (1 << 18) // Pin Controlled by PB18
#define AT91C_PB18_D18            (AT91C_PIO_PB18) //
#define AT91C_PIO_PB19            (1 << 19) // Pin Controlled by PB19
#define AT91C_PB19_D19            (AT91C_PIO_PB19) //
#define AT91C_PIO_PB2             (1 <<  2) // Pin Controlled by PB2
#define AT91C_PB2_A21_NANDALE     (AT91C_PIO_PB2) //
#define AT91C_PIO_PB20            (1 << 20) // Pin Controlled by PB20
#define AT91C_PB20_D20            (AT91C_PIO_PB20) //
#define AT91C_PIO_PB21            (1 << 21) // Pin Controlled by PB21
#define AT91C_PB21_D21            (AT91C_PIO_PB21) //
#define AT91C_PIO_PB22            (1 << 22) // Pin Controlled by PB22
#define AT91C_PB22_D22            (AT91C_PIO_PB22) //
#define AT91C_PIO_PB23            (1 << 23) // Pin Controlled by PB23
#define AT91C_PB23_D23            (AT91C_PIO_PB23) //
#define AT91C_PIO_PB24            (1 << 24) // Pin Controlled by PB24
#define AT91C_PB24_D24            (AT91C_PIO_PB24) //
#define AT91C_PIO_PB25            (1 << 25) // Pin Controlled by PB25
#define AT91C_PB25_D25            (AT91C_PIO_PB25) //
#define AT91C_PIO_PB26            (1 << 26) // Pin Controlled by PB26
#define AT91C_PB26_D26            (AT91C_PIO_PB26) //
#define AT91C_PIO_PB27            (1 << 27) // Pin Controlled by PB27
#define AT91C_PB27_D27            (AT91C_PIO_PB27) //
#define AT91C_PIO_PB28            (1 << 28) // Pin Controlled by PB28
#define AT91C_PB28_D28            (AT91C_PIO_PB28) //
#define AT91C_PIO_PB29            (1 << 29) // Pin Controlled by PB29
#define AT91C_PB29_D29            (AT91C_PIO_PB29) //
#define AT91C_PIO_PB3             (1 <<  3) // Pin Controlled by PB3
#define AT91C_PB3_A22_NANDCLE     (AT91C_PIO_PB3) //
#define AT91C_PIO_PB30            (1 << 30) // Pin Controlled by PB30
#define AT91C_PB30_D30            (AT91C_PIO_PB30) //
#define AT91C_PIO_PB31            (1 << 31) // Pin Controlled by PB31
#define AT91C_PB31_D31            (AT91C_PIO_PB31) //
#define AT91C_PIO_PB4             (1 <<  4) // Pin Controlled by PB4
#define AT91C_PB4_NANDOE          (AT91C_PIO_PB4) //
#define AT91C_PIO_PB5             (1 <<  5) // Pin Controlled by PB5
#define AT91C_PB5_NANDWE          (AT91C_PIO_PB5) //
#define AT91C_PIO_PB6             (1 <<  6) // Pin Controlled by PB6
#define AT91C_PB6_NCS3_NANDCS     (AT91C_PIO_PB6) //
#define AT91C_PIO_PB7             (1 <<  7) // Pin Controlled by PB7
#define AT91C_PB7_NCS4_CFCS0      (AT91C_PIO_PB7) //
#define AT91C_PB7_NPCS1           (AT91C_PIO_PB7) //
#define AT91C_PIO_PB8             (1 <<  8) // Pin Controlled by PB8
#define AT91C_PB8_CFE1            (AT91C_PIO_PB8) //
#define AT91C_PB8_PWM0            (AT91C_PIO_PB8) //
#define AT91C_PIO_PB9             (1 <<  9) // Pin Controlled by PB9
#define AT91C_PB9_CFE2            (AT91C_PIO_PB9) //
#define AT91C_PB9_PWM1            (AT91C_PIO_PB9) //
#define AT91C_PIO_PC0             (1 <<  0) // Pin Controlled by PC0
#define AT91C_PC0_TF0             (AT91C_PIO_PC0) //
#define AT91C_PIO_PC1             (1 <<  1) // Pin Controlled by PC1
#define AT91C_PC1_TK0             (AT91C_PIO_PC1) //
#define AT91C_PC1_LCDPWR          (AT91C_PIO_PC1) //
#define AT91C_PIO_PC10            (1 << 10) // Pin Controlled by PC10
#define AT91C_PC10_LCDD2          (AT91C_PIO_PC10) //
#define AT91C_PC10_LCDD4          (AT91C_PIO_PC10) //
#define AT91C_PIO_PC11            (1 << 11) // Pin Controlled by PC11
#define AT91C_PC11_LCDD3          (AT91C_PIO_PC11) //
#define AT91C_PC11_LCDD5          (AT91C_PIO_PC11) //
#define AT91C_PIO_PC12            (1 << 12) // Pin Controlled by PC12
#define AT91C_PC12_LCDD4          (AT91C_PIO_PC12) //
#define AT91C_PC12_LCDD6          (AT91C_PIO_PC12) //
#define AT91C_PIO_PC13            (1 << 13) // Pin Controlled by PC13
#define AT91C_PC13_LCDD5          (AT91C_PIO_PC13) //
#define AT91C_PC13_LCDD7          (AT91C_PIO_PC13) //
#define AT91C_PIO_PC14            (1 << 14) // Pin Controlled by PC14
#define AT91C_PC14_LCDD6          (AT91C_PIO_PC14) //
#define AT91C_PC14_LCDD10         (AT91C_PIO_PC14) //
#define AT91C_PIO_PC15            (1 << 15) // Pin Controlled by PC15
#define AT91C_PC15_LCDD7          (AT91C_PIO_PC15) //
#define AT91C_PC15_LCDD11         (AT91C_PIO_PC15) //
#define AT91C_PIO_PC16            (1 << 16) // Pin Controlled by PC16
#define AT91C_PC16_LCDD8          (AT91C_PIO_PC16) //
#define AT91C_PC16_LCDD12         (AT91C_PIO_PC16) //
#define AT91C_PIO_PC17            (1 << 17) // Pin Controlled by PC17
#define AT91C_PC17_LCDD9          (AT91C_PIO_PC17) //
#define AT91C_PC17_LCDD13         (AT91C_PIO_PC17) //
#define AT91C_PIO_PC18            (1 << 18) // Pin Controlled by PC18
#define AT91C_PC18_LCDD10         (AT91C_PIO_PC18) //
#define AT91C_PC18_LCDD14         (AT91C_PIO_PC18) //
#define AT91C_PIO_PC19            (1 << 19) // Pin Controlled by PC19
#define AT91C_PC19_LCDD11         (AT91C_PIO_PC19) //
#define AT91C_PC19_LCDD15         (AT91C_PIO_PC19) //
#define AT91C_PIO_PC2             (1 <<  2) // Pin Controlled by PC2
#define AT91C_PC2_LCDMOD          (AT91C_PIO_PC2) //
#define AT91C_PC2_PWM0            (AT91C_PIO_PC2) //
#define AT91C_PIO_PC20            (1 << 20) // Pin Controlled by PC20
#define AT91C_PC20_LCDD12         (AT91C_PIO_PC20) //
#define AT91C_PC20_LCDD18         (AT91C_PIO_PC20) //
#define AT91C_PIO_PC21            (1 << 21) // Pin Controlled by PC21
#define AT91C_PC21_LCDD13         (AT91C_PIO_PC21) //
#define AT91C_PC21_LCDD19         (AT91C_PIO_PC21) //
#define AT91C_PIO_PC22            (1 << 22) // Pin Controlled by PC22
#define AT91C_PC22_LCDD14         (AT91C_PIO_PC22) //
#define AT91C_PC22_LCDD20         (AT91C_PIO_PC22) //
#define AT91C_PIO_PC23            (1 << 23) // Pin Controlled by PC23
#define AT91C_PC23_LCDD15         (AT91C_PIO_PC23) //
#define AT91C_PC23_LCDD21         (AT91C_PIO_PC23) //
#define AT91C_PIO_PC24            (1 << 24) // Pin Controlled by PC24
#define AT91C_PC24_LCDD16         (AT91C_PIO_PC24) //
#define AT91C_PC24_LCDD22         (AT91C_PIO_PC24) //
#define AT91C_PIO_PC25            (1 << 25) // Pin Controlled by PC25
#define AT91C_PC25_LCDD17         (AT91C_PIO_PC25) //
#define AT91C_PC25_LCDD23         (AT91C_PIO_PC25) //
#define AT91C_PIO_PC26            (1 << 26) // Pin Controlled by PC26
#define AT91C_PC26_LCDD18         (AT91C_PIO_PC26) //
#define AT91C_PIO_PC27            (1 << 27) // Pin Controlled by PC27
#define AT91C_PC27_LCDD19         (AT91C_PIO_PC27) //
#define AT91C_PIO_PC28            (1 << 28) // Pin Controlled by PC28
#define AT91C_PC28_LCDD20         (AT91C_PIO_PC28) //
#define AT91C_PIO_PC29            (1 << 29) // Pin Controlled by PC29
#define AT91C_PC29_LCDD21         (AT91C_PIO_PC29) //
#define AT91C_PC29_TIOA1          (AT91C_PIO_PC29) //
#define AT91C_PIO_PC3             (1 <<  3) // Pin Controlled by PC3
#define AT91C_PC3_LCDCC           (AT91C_PIO_PC3) //
#define AT91C_PC3_PWM1            (AT91C_PIO_PC3) //
#define AT91C_PIO_PC30            (1 << 30) // Pin Controlled by PC30
#define AT91C_PC30_LCDD22         (AT91C_PIO_PC30) //
#define AT91C_PC30_TIOB1          (AT91C_PIO_PC30) //
#define AT91C_PIO_PC31            (1 << 31) // Pin Controlled by PC31
#define AT91C_PC31_LCDD23         (AT91C_PIO_PC31) //
#define AT91C_PC31_TCLK1          (AT91C_PIO_PC31) //
#define AT91C_PIO_PC4             (1 <<  4) // Pin Controlled by PC4
#define AT91C_PC4_LCDVSYNC        (AT91C_PIO_PC4) //
#define AT91C_PIO_PC5             (1 <<  5) // Pin Controlled by PC5
#define AT91C_PC5_LCDHSYNC        (AT91C_PIO_PC5) //
#define AT91C_PIO_PC6             (1 <<  6) // Pin Controlled by PC6
#define AT91C_PC6_LCDDOTCK        (AT91C_PIO_PC6) //
#define AT91C_PIO_PC7             (1 <<  7) // Pin Controlled by PC7
#define AT91C_PC7_LCDDEN          (AT91C_PIO_PC7) //
#define AT91C_PIO_PC8             (1 <<  8) // Pin Controlled by PC8
#define AT91C_PC8_LCDD0           (AT91C_PIO_PC8) //
#define AT91C_PC8_LCDD2           (AT91C_PIO_PC8) //
#define AT91C_PIO_PC9             (1 <<  9) // Pin Controlled by PC9
#define AT91C_PC9_LCDD1           (AT91C_PIO_PC9) //
#define AT91C_PC9_LCDD3           (AT91C_PIO_PC9) //
#define AT91C_PIO_PD0             (1 <<  0) // Pin Controlled by PD0
#define AT91C_PD0_NCS2            (AT91C_PIO_PD0) //
#define AT91C_PIO_PD1             (1 <<  1) // Pin Controlled by PD1
#define AT91C_PD1_AC97_FS         (AT91C_PIO_PD1) //
#define AT91C_PIO_PD10            (1 << 10) // Pin Controlled by PD10
#define AT91C_PD10_TWD1           (AT91C_PIO_PD10) //
#define AT91C_PD10_TIOA2          (AT91C_PIO_PD10) //
#define AT91C_PIO_PD11            (1 << 11) // Pin Controlled by PD11
#define AT91C_PD11_TWCK1          (AT91C_PIO_PD11) //
#define AT91C_PD11_TIOB2          (AT91C_PIO_PD11) //
#define AT91C_PIO_PD12            (1 << 12) // Pin Controlled by PD12
#define AT91C_PD12_PWM2           (AT91C_PIO_PD12) //
#define AT91C_PD12_PCK1           (AT91C_PIO_PD12) //
#define AT91C_PIO_PD13            (1 << 13) // Pin Controlled by PD13
#define AT91C_PD13_NCS5_CFCS1     (AT91C_PIO_PD13) //
#define AT91C_PD13_NPCS3          (AT91C_PIO_PD13) //
#define AT91C_PIO_PD14            (1 << 14) // Pin Controlled by PD14
#define AT91C_PD14_DSR0           (AT91C_PIO_PD14) //
#define AT91C_PD14_PWM0           (AT91C_PIO_PD14) //
#define AT91C_PIO_PD15            (1 << 15) // Pin Controlled by PD15
#define AT91C_PD15_DTR0           (AT91C_PIO_PD15) //
#define AT91C_PD15_PWM1           (AT91C_PIO_PD15) //
#define AT91C_PIO_PD16            (1 << 16) // Pin Controlled by PD16
#define AT91C_PD16_DCD0           (AT91C_PIO_PD16) //
#define AT91C_PD16_PWM2           (AT91C_PIO_PD16) //
#define AT91C_PIO_PD17            (1 << 17) // Pin Controlled by PD17
#define AT91C_PD17_RI0            (AT91C_PIO_PD17) //
#define AT91C_PIO_PD18            (1 << 18) // Pin Controlled by PD18
#define AT91C_PD18_PWM3           (AT91C_PIO_PD18) //
#define AT91C_PIO_PD19            (1 << 19) // Pin Controlled by PD19
#define AT91C_PD19_PCK0           (AT91C_PIO_PD19) //
#define AT91C_PIO_PD2             (1 <<  2) // Pin Controlled by PD2
#define AT91C_PD2_AC97_CK         (AT91C_PIO_PD2) //
#define AT91C_PD2_SCK1            (AT91C_PIO_PD2) //
#define AT91C_PIO_PD20            (1 << 20) // Pin Controlled by PD20
#define AT91C_PD20_PCK1           (AT91C_PIO_PD20) //
#define AT91C_PIO_PD21            (1 << 21) // Pin Controlled by PD21
#define AT91C_PD21_TCLK2          (AT91C_PIO_PD21) //
#define AT91C_PIO_PD3             (1 <<  3) // Pin Controlled by PD3
#define AT91C_PD3_AC97_TX         (AT91C_PIO_PD3) //
#define AT91C_PD3_CTS3            (AT91C_PIO_PD3) //
#define AT91C_PIO_PD4             (1 <<  4) // Pin Controlled by PD4
#define AT91C_PD4_AC97_RX         (AT91C_PIO_PD4) //
#define AT91C_PD4_RTS3            (AT91C_PIO_PD4) //
#define AT91C_PIO_PD5             (1 <<  5) // Pin Controlled by PD5
#define AT91C_PD5_DTXD            (AT91C_PIO_PD5) //
#define AT91C_PD5_PWM2            (AT91C_PIO_PD5) //
#define AT91C_PIO_PD6             (1 <<  6) // Pin Controlled by PD6
#define AT91C_PD6_AD4             (AT91C_PIO_PD6) //
#define AT91C_PIO_PD7             (1 <<  7) // Pin Controlled by PD7
#define AT91C_PD7_AD5             (AT91C_PIO_PD7) //
#define AT91C_PIO_PD8             (1 <<  8) // Pin Controlled by PD8
#define AT91C_PD8_NPCS2           (AT91C_PIO_PD8) //
#define AT91C_PD8_PWM3            (AT91C_PIO_PD8) //
#define AT91C_PIO_PD9             (1 <<  9) // Pin Controlled by PD9
#define AT91C_PD9_SCK2            (AT91C_PIO_PD9) //
#define AT91C_PD9_NPCS3           (AT91C_PIO_PD9) //

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91SAM9RL64
// *****************************************************************************
#define AT91C_ID_FIQ              ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS              ( 1) // System Controller
#define AT91C_ID_PIOA             ( 2) // Parallel IO Controller A
#define AT91C_ID_PIOB             ( 3) // Parallel IO Controller B
#define AT91C_ID_PIOC             ( 4) // Parallel IO Controller C
#define AT91C_ID_PIOD             ( 5) // Parallel IO Controller D
#define AT91C_ID_US0              ( 6) // USART 0
#define AT91C_ID_US1              ( 7) // USART 1
#define AT91C_ID_US2              ( 8) // USART 2
#define AT91C_ID_US3              ( 9) // USART 2
#define AT91C_ID_MCI              (10) // Multimedia Card Interface
#define AT91C_ID_TWI0             (11) // TWI 0
#define AT91C_ID_TWI1             (12) // TWI 1
#define AT91C_ID_SPI0             (13) // Serial Peripheral Interface
#define AT91C_ID_SSC0             (14) // Serial Synchronous Controller 0
#define AT91C_ID_SSC1             (15) // Serial Synchronous Controller 1
#define AT91C_ID_TC0              (16) // Timer Counter 0
#define AT91C_ID_TC1              (17) // Timer Counter 1
#define AT91C_ID_TC2              (18) // Timer Counter 2
#define AT91C_ID_PWMC             (19) // Pulse Width Modulation Controller
#define AT91C_ID_TSC              (20) // Touch Screen Controller
#define AT91C_ID_HDMA             (21) // HDMA
#define AT91C_ID_UDPHS            (22) // USB Device HS
#define AT91C_ID_LCDC             (23) // LCD Controller
#define AT91C_ID_AC97C            (24) // AC97 Controller
#define AT91C_ID_IRQ0             (31) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ALL_INT             (0x81FFFFFF) // ALL VALID INTERRUPTS

// *****************************************************************************
//               BASE ADDRESS DEFINITIONS FOR AT91SAM9RL64
// *****************************************************************************
#define AT91C_BASE_SYS            (0xFFFFC000) // (SYS) Base Address
#define AT91C_BASE_EBI            (0xFFFFE800) // (EBI) Base Address
#define AT91C_BASE_SDRAMC         (0xFFFFEA00) // (SDRAMC) Base Address
#define AT91C_BASE_SMC            (0xFFFFEC00) // (SMC) Base Address
#define AT91C_BASE_MATRIX         (0xFFFFEE00) // (MATRIX) Base Address
#define AT91C_BASE_CCFG           (0xFFFFEF10) // (CCFG) Base Address
#define AT91C_BASE_AIC            (0xFFFFF000) // (AIC) Base Address
#define AT91C_BASE_PDC_DBGU       (0xFFFFF300) // (PDC_DBGU) Base Address
#define AT91C_BASE_DBGU           (0xFFFFF200) // (DBGU) Base Address
#define AT91C_BASE_PIOA           (0xFFFFF400) // (PIOA) Base Address
#define AT91C_BASE_PIOB           (0xFFFFF600) // (PIOB) Base Address
#define AT91C_BASE_PIOC           (0xFFFFF800) // (PIOC) Base Address
#define AT91C_BASE_PIOD           (0xFFFFFA00) // (PIOD) Base Address
#define AT91C_BASE_PMC            (0xFFFFFC00) // (PMC) Base Address
#define AT91C_BASE_CKGR           (0xFFFFFC1C) // (CKGR) Base Address
#define AT91C_BASE_RSTC           (0xFFFFFD00) // (RSTC) Base Address
#define AT91C_BASE_SHDWC          (0xFFFFFD10) // (SHDWC) Base Address
#define AT91C_BASE_RTTC           (0xFFFFFD20) // (RTTC) Base Address
#define AT91C_BASE_PITC           (0xFFFFFD30) // (PITC) Base Address
#define AT91C_BASE_WDTC           (0xFFFFFD40) // (WDTC) Base Address
#define AT91C_BASE_RTC            (0xFFFFFE00) // (RTC) Base Address
#define AT91C_BASE_TC0            (0xFFFA0000) // (TC0) Base Address
#define AT91C_BASE_TC1            (0xFFFA0040) // (TC1) Base Address
#define AT91C_BASE_TC2            (0xFFFA0080) // (TC2) Base Address
#define AT91C_BASE_TCB0           (0xFFFA0000) // (TCB0) Base Address
#define AT91C_BASE_TCB1           (0xFFFA0040) // (TCB1) Base Address
#define AT91C_BASE_TCB2           (0xFFFA0080) // (TCB2) Base Address
#define AT91C_BASE_PDC_MCI        (0xFFFA4100) // (PDC_MCI) Base Address
#define AT91C_BASE_MCI            (0xFFFA4000) // (MCI) Base Address
#define AT91C_BASE_PDC_TWI0       (0xFFFA8100) // (PDC_TWI0) Base Address
#define AT91C_BASE_TWI0           (0xFFFA8000) // (TWI0) Base Address
#define AT91C_BASE_TWI1           (0xFFFAC000) // (TWI1) Base Address
#define AT91C_BASE_PDC_US0        (0xFFFB0100) // (PDC_US0) Base Address
#define AT91C_BASE_US0            (0xFFFB0000) // (US0) Base Address
#define AT91C_BASE_PDC_US1        (0xFFFB4100) // (PDC_US1) Base Address
#define AT91C_BASE_US1            (0xFFFB4000) // (US1) Base Address
#define AT91C_BASE_PDC_US2        (0xFFFB8100) // (PDC_US2) Base Address
#define AT91C_BASE_US2            (0xFFFB8000) // (US2) Base Address
#define AT91C_BASE_PDC_US3        (0xFFFBC100) // (PDC_US3) Base Address
#define AT91C_BASE_US3            (0xFFFBC000) // (US3) Base Address
#define AT91C_BASE_PDC_SSC0       (0xFFFC0100) // (PDC_SSC0) Base Address
#define AT91C_BASE_SSC0           (0xFFFC0000) // (SSC0) Base Address
#define AT91C_BASE_PDC_SSC1       (0xFFFC4100) // (PDC_SSC1) Base Address
#define AT91C_BASE_SSC1           (0xFFFC4000) // (SSC1) Base Address
#define AT91C_BASE_PWMC_CH0       (0xFFFC8200) // (PWMC_CH0) Base Address
#define AT91C_BASE_PWMC_CH1       (0xFFFC8220) // (PWMC_CH1) Base Address
#define AT91C_BASE_PWMC_CH2       (0xFFFC8240) // (PWMC_CH2) Base Address
#define AT91C_BASE_PWMC_CH3       (0xFFFC8260) // (PWMC_CH3) Base Address
#define AT91C_BASE_PWMC           (0xFFFC8000) // (PWMC) Base Address
#define AT91C_BASE_PDC_SPI        (0xFFFCC100) // (PDC_SPI) Base Address
#define AT91C_BASE_SPI0            (0xFFFCC000) // (SPI) Base Address
#define AT91C_BASE_PDC_TSC        (0xFFFD0100) // (PDC_TSC) Base Address
#define AT91C_BASE_TSC            (0xFFFD0000) // (TSC) Base Address
#define AT91C_BASE_UDPHS_EPTFIFO  (0x00600000) // (UDPHS_EPTFIFO) Base Address
#define AT91C_BASE_UDPHS_EPT_0    (0xFFFD4100) // (UDPHS_EPT_0) Base Address
#define AT91C_BASE_UDPHS_EPT_1    (0xFFFD4120) // (UDPHS_EPT_1) Base Address
#define AT91C_BASE_UDPHS_EPT_2    (0xFFFD4140) // (UDPHS_EPT_2) Base Address
#define AT91C_BASE_UDPHS_EPT_3    (0xFFFD4160) // (UDPHS_EPT_3) Base Address
#define AT91C_BASE_UDPHS_EPT_4    (0xFFFD4180) // (UDPHS_EPT_4) Base Address
#define AT91C_BASE_UDPHS_EPT_5    (0xFFFD41A0) // (UDPHS_EPT_5) Base Address
#define AT91C_BASE_UDPHS_EPT_6    (0xFFFD41C0) // (UDPHS_EPT_6) Base Address
#define AT91C_BASE_UDPHS_EPT_7    (0xFFFD41E0) // (UDPHS_EPT_7) Base Address
#define AT91C_BASE_UDPHS_EPT_8    (0xFFFD4200) // (UDPHS_EPT_8) Base Address
#define AT91C_BASE_UDPHS_EPT_9    (0xFFFD4220) // (UDPHS_EPT_9) Base Address
#define AT91C_BASE_UDPHS_EPT_10   (0xFFFD4240) // (UDPHS_EPT_10) Base Address
#define AT91C_BASE_UDPHS_EPT_11   (0xFFFD4260) // (UDPHS_EPT_11) Base Address
#define AT91C_BASE_UDPHS_EPT_12   (0xFFFD4280) // (UDPHS_EPT_12) Base Address
#define AT91C_BASE_UDPHS_EPT_13   (0xFFFD42A0) // (UDPHS_EPT_13) Base Address
#define AT91C_BASE_UDPHS_EPT_14   (0xFFFD42C0) // (UDPHS_EPT_14) Base Address
#define AT91C_BASE_UDPHS_EPT_15   (0xFFFD42E0) // (UDPHS_EPT_15) Base Address
#define AT91C_BASE_UDPHS_DMA_1    (0xFFFD4310) // (UDPHS_DMA_1) Base Address
#define AT91C_BASE_UDPHS_DMA_2    (0xFFFD4320) // (UDPHS_DMA_2) Base Address
#define AT91C_BASE_UDPHS_DMA_3    (0xFFFD4330) // (UDPHS_DMA_3) Base Address
#define AT91C_BASE_UDPHS_DMA_4    (0xFFFD4340) // (UDPHS_DMA_4) Base Address
#define AT91C_BASE_UDPHS_DMA_5    (0xFFFD4350) // (UDPHS_DMA_5) Base Address
#define AT91C_BASE_UDPHS_DMA_6    (0xFFFD4360) // (UDPHS_DMA_6) Base Address
#define AT91C_BASE_UDPHS_DMA_7    (0xFFFD4370) // (UDPHS_DMA_7) Base Address
#define AT91C_BASE_UDPHS          (0xFFFD4000) // (UDPHS) Base Address
#define AT91C_BASE_PDC_AC97C      (0xFFFD8100) // (PDC_AC97C) Base Address
#define AT91C_BASE_AC97C          (0xFFFD8000) // (AC97C) Base Address
#define AT91C_BASE_LCDC           (0x00500000) // (LCDC) Base Address
#define AT91C_BASE_LCDC_16B_TFT   (0x00500000) // (LCDC_16B_TFT) Base Address
#define AT91C_BASE_HDMA_CH_0      (0xFFFFE63C) // (HDMA_CH_0) Base Address
#define AT91C_BASE_HDMA_CH_1      (0xFFFFE664) // (HDMA_CH_1) Base Address
#define AT91C_BASE_HDMA           (0xFFFFE600) // (HDMA) Base Address
#define AT91C_BASE_HECC           (0xFFFFE800) // (HECC) Base Address

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91SAM9RL64
// *****************************************************************************
// ITCM
#define AT91C_ITCM 	              (0x00100000) // Maximum ITCM Area base address
#define AT91C_ITCM_SIZE	          (0x00010000) // Maximum ITCM Area size in byte (64 Kbytes)
// DTCM
#define AT91C_DTCM 	              (0x00200000) // Maximum DTCM Area base address
#define AT91C_DTCM_SIZE	          (0x00010000) // Maximum DTCM Area size in byte (64 Kbytes)
// IRAM
#define AT91C_IRAM 	              (0x00300000) // Maximum Internal SRAM base address
#define AT91C_IRAM_SIZE	          (0x00010000) // Maximum Internal SRAM size in byte (64 Kbytes)
// IRAM_MIN
#define AT91C_IRAM_MIN	           (0x00300000) // Minimum Internal RAM base address
#define AT91C_IRAM_MIN_SIZE	      (0x00004000) // Minimum Internal RAM size in byte (16 Kbytes)
// IROM
#define AT91C_IROM 	              (0x00400000) // Internal ROM base address
#define AT91C_IROM_SIZE	          (0x00008000) // Internal ROM size in byte (32 Kbytes)
// EBI_CS0
#define AT91C_EBI_CS0	            (0x10000000) // EBI Chip Select 0 base address
#define AT91C_EBI_CS0_SIZE	       (0x10000000) // EBI Chip Select 0 size in byte (262144 Kbytes)
// EBI_CS1
#define AT91C_EBI_CS1	            (0x20000000) // EBI Chip Select 1 base address
#define AT91C_EBI_CS1_SIZE	       (0x10000000) // EBI Chip Select 1 size in byte (262144 Kbytes)
// EBI_SDRAM
#define AT91C_EBI_SDRAM	          (0x20000000) // SDRAM on EBI Chip Select 1 base address
#define AT91C_EBI_SDRAM_SIZE	     (0x10000000) // SDRAM on EBI Chip Select 1 size in byte (262144 Kbytes)
// EBI_SDRAM_16BIT
#define AT91C_EBI_SDRAM_16BIT	    (0x20000000) // SDRAM on EBI Chip Select 1 base address
#define AT91C_EBI_SDRAM_16BIT_SIZE	 (0x02000000) // SDRAM on EBI Chip Select 1 size in byte (32768 Kbytes)
// EBI_SDRAM_32BIT
#define AT91C_EBI_SDRAM_32BIT	    (0x20000000) // SDRAM on EBI Chip Select 1 base address
#define AT91C_EBI_SDRAM_32BIT_SIZE	 (0x04000000) // SDRAM on EBI Chip Select 1 size in byte (65536 Kbytes)
// EBI_CS2
#define AT91C_EBI_CS2	            (0x30000000) // EBI Chip Select 2 base address
#define AT91C_EBI_CS2_SIZE	       (0x10000000) // EBI Chip Select 2 size in byte (262144 Kbytes)
// EBI_CS3
#define AT91C_EBI_CS3	            (0x40000000) // EBI Chip Select 3 base address
#define AT91C_EBI_CS3_SIZE	       (0x10000000) // EBI Chip Select 3 size in byte (262144 Kbytes)
// EBI_SM
#define AT91C_EBI_SM	             (0x40000000) // NANDFLASH on EBI Chip Select 3 base address
#define AT91C_EBI_SM_SIZE	        (0x10000000) // NANDFLASH on EBI Chip Select 3 size in byte (262144 Kbytes)
// EBI_CS4
#define AT91C_EBI_CS4	            (0x50000000) // EBI Chip Select 4 base address
#define AT91C_EBI_CS4_SIZE	       (0x10000000) // EBI Chip Select 4 size in byte (262144 Kbytes)
// EBI_CF0
#define AT91C_EBI_CF0	            (0x50000000) // CompactFlash 0 on EBI Chip Select 4 base address
#define AT91C_EBI_CF0_SIZE	       (0x10000000) // CompactFlash 0 on EBI Chip Select 4 size in byte (262144 Kbytes)
// EBI_CS5
#define AT91C_EBI_CS5	            (0x60000000) // EBI Chip Select 5 base address
#define AT91C_EBI_CS5_SIZE	       (0x10000000) // EBI Chip Select 5 size in byte (262144 Kbytes)
// EBI_CF1
#define AT91C_EBI_CF1	            (0x60000000) // CompactFlash 1 on EBIChip Select 5 base address
#define AT91C_EBI_CF1_SIZE	       (0x10000000) // CompactFlash 1 on EBIChip Select 5 size in byte (262144 Kbytes)

#define AT91C_NR_PIO               (32 * 4)
