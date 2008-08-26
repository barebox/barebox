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
// File Name           : AT91SAM9XE128.h
// Object              : AT91SAM9XE128 definitions
// Generated           : AT91 SW Application Group  01/30/2007 (14:21:40)
// 
// CVS Reference       : /AT91SAM9XE128.pl/1.1/Tue Jan 30 13:20:58 2007//
// CVS Reference       : /SYS_SAM9260.pl/1.2/Fri Sep 30 12:12:15 2005//
// CVS Reference       : /HMATRIX1_SAM9260.pl/1.5/Mon Jan 23 16:58:39 2006//
// CVS Reference       : /CCR_SAM9260.pl/1.1/Fri Sep 30 12:12:14 2005//
// CVS Reference       : /PMC_SAM9262.pl/1.4/Mon Mar  7 18:03:13 2005//
// CVS Reference       : /ADC_6051C.pl/1.1/Mon Jan 31 13:12:40 2005//
// CVS Reference       : /HSDRAMC1_6100A.pl/1.2/Mon Aug  9 10:52:25 2004//
// CVS Reference       : /HSMC3_6105A.pl/1.4/Tue Nov 16 09:16:23 2004//
// CVS Reference       : /AIC_6075A.pl/1.1/Mon Jul 12 17:04:01 2004//
// CVS Reference       : /PDC_6074C.pl/1.2/Thu Feb  3 09:02:11 2005//
// CVS Reference       : /DBGU_6059D.pl/1.1/Mon Jan 31 13:54:41 2005//
// CVS Reference       : /PIO_6057A.pl/1.2/Thu Feb  3 10:29:42 2005//
// CVS Reference       : /RSTC_6098A.pl/1.3/Thu Nov  4 13:57:00 2004//
// CVS Reference       : /SHDWC_6122A.pl/1.3/Wed Oct  6 14:16:58 2004//
// CVS Reference       : /RTTC_6081A.pl/1.2/Thu Nov  4 13:57:22 2004//
// CVS Reference       : /PITC_6079A.pl/1.2/Thu Nov  4 13:56:22 2004//
// CVS Reference       : /WDTC_6080A.pl/1.3/Thu Nov  4 13:58:52 2004//
// CVS Reference       : /EFC2_IGS036.pl/1.2/Fri Nov 10 10:47:53 2006//
// CVS Reference       : /TC_6082A.pl/1.7/Wed Mar  9 16:31:51 2005//
// CVS Reference       : /MCI_6101E.pl/1.1/Fri Jun  3 13:20:23 2005//
// CVS Reference       : /TWI_6061B.pl/1.2/Fri Aug  4 08:53:02 2006//
// CVS Reference       : /US_6089C.pl/1.1/Mon Jan 31 13:56:02 2005//
// CVS Reference       : /SSC_6078A.pl/1.1/Tue Jul 13 07:10:41 2004//
// CVS Reference       : /SPI_6088D.pl/1.3/Fri May 20 14:23:02 2005//
// CVS Reference       : /EMACB_6119A.pl/1.6/Wed Jul 13 15:25:00 2005//
// CVS Reference       : /UDP_6ept_puon.pl/1.1/Wed Aug 30 14:20:53 2006//
// CVS Reference       : /UHP_6127A.pl/1.1/Wed Feb 23 16:03:17 2005//
// CVS Reference       : /EBI_SAM9260.pl/1.1/Fri Sep 30 12:12:14 2005//
// CVS Reference       : /HECC_6143A.pl/1.1/Wed Feb  9 17:16:57 2005//
// CVS Reference       : /ISI_xxxxx.pl/1.3/Thu Mar  3 11:11:48 2005//
//  ----------------------------------------------------------------------------

// Hardware register definition

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR System Peripherals
// *****************************************************************************
// -------- GPBR : (SYS Offset: 0x1350) GPBR General Purpose Register -------- 
// -------- GPBR : (SYS Offset: 0x1354) GPBR General Purpose Register -------- 
// -------- GPBR : (SYS Offset: 0x1358) GPBR General Purpose Register -------- 
// -------- GPBR : (SYS Offset: 0x135c) GPBR General Purpose Register -------- 

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR External Bus Interface
// *****************************************************************************
// *** Register offset in AT91S_EBI structure ***
#define EBI_DUMMY       ( 0) // Dummy register - Do not use

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
#define MATRIX_MCFG0    ( 0) //  Master Configuration Register 0 (ram96k)     
#define MATRIX_MCFG1    ( 4) //  Master Configuration Register 1 (rom)    
#define MATRIX_MCFG2    ( 8) //  Master Configuration Register 2 (hperiphs) 
#define MATRIX_MCFG3    (12) //  Master Configuration Register 3 (ebi)
#define MATRIX_MCFG4    (16) //  Master Configuration Register 4 (bridge)    
#define MATRIX_MCFG5    (20) //  Master Configuration Register 5 (mailbox)    
#define MATRIX_MCFG6    (24) //  Master Configuration Register 6 (ram16k)  
#define MATRIX_MCFG7    (28) //  Master Configuration Register 7 (teak_prog)     
#define MATRIX_SCFG0    (64) //  Slave Configuration Register 0 (ram96k)     
#define MATRIX_SCFG1    (68) //  Slave Configuration Register 1 (rom)    
#define MATRIX_SCFG2    (72) //  Slave Configuration Register 2 (hperiphs) 
#define MATRIX_SCFG3    (76) //  Slave Configuration Register 3 (ebi)
#define MATRIX_SCFG4    (80) //  Slave Configuration Register 4 (bridge)    
#define MATRIX_PRAS0    (128) //  PRAS0 (ram0) 
#define MATRIX_PRBS0    (132) //  PRBS0 (ram0) 
#define MATRIX_PRAS1    (136) //  PRAS1 (ram1) 
#define MATRIX_PRBS1    (140) //  PRBS1 (ram1) 
#define MATRIX_PRAS2    (144) //  PRAS2 (ram2) 
#define MATRIX_PRBS2    (148) //  PRBS2 (ram2) 
#define MATRIX_MRCR     (256) //  Master Remp Control Register 
#define MATRIX_EBI      (284) //  Slave 3 (ebi) Special Function Register
#define MATRIX_TEAKCFG  (300) //  Slave 7 (teak_prog) Special Function Register
#define MATRIX_VERSION  (508) //  Version Register
// -------- MATRIX_SCFG0 : (MATRIX Offset: 0x40) Slave Configuration Register 0 -------- 
#define AT91C_MATRIX_SLOT_CYCLE   (0xFF <<  0) // (MATRIX) Maximum Number of Allowed Cycles for a Burst
#define AT91C_MATRIX_DEFMSTR_TYPE (0x3 << 16) // (MATRIX) Default Master Type
#define 	AT91C_MATRIX_DEFMSTR_TYPE_NO_DEFMSTR           (0x0 << 16) // (MATRIX) No Default Master. At the end of current slave access, if no other master request is pending, the slave is deconnected from all masters. This results in having a one cycle latency for the first transfer of a burst.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_LAST_DEFMSTR         (0x1 << 16) // (MATRIX) Last Default Master. At the end of current slave access, if no other master request is pending, the slave stay connected with the last master having accessed it. This results in not having the one cycle latency when the last master re-trying access on the slave.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR        (0x2 << 16) // (MATRIX) Fixed Default Master. At the end of current slave access, if no other master request is pending, the slave connects with fixed which number is in FIXED_DEFMSTR field. This results in not having the one cycle latency when the fixed master re-trying access on the slave.
#define AT91C_MATRIX_FIXED_DEFMSTR0 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_HPDC3                (0x2 << 18) // (MATRIX) HPDC3 Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR0_DMA                  (0x4 << 18) // (MATRIX) DMA Master is Default Master
// -------- MATRIX_SCFG1 : (MATRIX Offset: 0x44) Slave Configuration Register 1 -------- 
#define AT91C_MATRIX_FIXED_DEFMSTR1 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_HPDC3                (0x2 << 18) // (MATRIX) HPDC3 Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR1_DMA                  (0x4 << 18) // (MATRIX) DMA Master is Default Master
// -------- MATRIX_SCFG2 : (MATRIX Offset: 0x48) Slave Configuration Register 2 -------- 
#define AT91C_MATRIX_FIXED_DEFMSTR2 (0x1 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR2_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR2_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
// -------- MATRIX_SCFG3 : (MATRIX Offset: 0x4c) Slave Configuration Register 3 -------- 
#define AT91C_MATRIX_FIXED_DEFMSTR3 (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_HPDC3                (0x2 << 18) // (MATRIX) HPDC3 Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR3_DMA                  (0x4 << 18) // (MATRIX) DMA Master is Default Master
// -------- MATRIX_SCFG4 : (MATRIX Offset: 0x50) Slave Configuration Register 4 -------- 
#define AT91C_MATRIX_FIXED_DEFMSTR4 (0x3 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR4_HPDC3                (0x2 << 18) // (MATRIX) HPDC3 Master is Default Master
// -------- MATRIX_MRCR : (MATRIX Offset: 0x100) MRCR Register -------- 
#define AT91C_MATRIX_RCA926I      (0x1 <<  0) // (MATRIX) Remap Command for ARM926EJ-S Instruction Master
#define AT91C_MATRIX_RCA926D      (0x1 <<  1) // (MATRIX) Remap Command for ARM926EJ-S Data Master
// -------- MATRIX_EBI : (MATRIX Offset: 0x11c) EBI (Slave 3) Special Function Register -------- 
#define AT91C_MATRIX_CS1A         (0x1 <<  1) // (MATRIX) Chip Select 1 Assignment
#define 	AT91C_MATRIX_CS1A_SMC                  (0x0 <<  1) // (MATRIX) Chip Select 1 is assigned to the Static Memory Controller.
#define 	AT91C_MATRIX_CS1A_SDRAMC               (0x1 <<  1) // (MATRIX) Chip Select 1 is assigned to the SDRAM Controller.
#define AT91C_MATRIX_CS3A         (0x1 <<  3) // (MATRIX) Chip Select 3 Assignment
#define 	AT91C_MATRIX_CS3A_SMC                  (0x0 <<  3) // (MATRIX) Chip Select 3 is only assigned to the Static Memory Controller and NCS3 behaves as defined by the SMC.
#define 	AT91C_MATRIX_CS3A_SM                   (0x1 <<  3) // (MATRIX) Chip Select 3 is assigned to the Static Memory Controller and the SmartMedia Logic is activated.
#define AT91C_MATRIX_CS4A         (0x1 <<  4) // (MATRIX) Chip Select 4 Assignment
#define 	AT91C_MATRIX_CS4A_SMC                  (0x0 <<  4) // (MATRIX) Chip Select 4 is only assigned to the Static Memory Controller and NCS4 behaves as defined by the SMC.
#define 	AT91C_MATRIX_CS4A_CF                   (0x1 <<  4) // (MATRIX) Chip Select 4 is assigned to the Static Memory Controller and the CompactFlash Logic (first slot) is activated.
#define AT91C_MATRIX_CS5A         (0x1 <<  5) // (MATRIX) Chip Select 5 Assignment
#define 	AT91C_MATRIX_CS5A_SMC                  (0x0 <<  5) // (MATRIX) Chip Select 5 is only assigned to the Static Memory Controller and NCS5 behaves as defined by the SMC
#define 	AT91C_MATRIX_CS5A_CF                   (0x1 <<  5) // (MATRIX) Chip Select 5 is assigned to the Static Memory Controller and the CompactFlash Logic (second slot) is activated.
#define AT91C_MATRIX_DBPUC        (0x1 <<  8) // (MATRIX) Data Bus Pull-up Configuration
// -------- MATRIX_TEAKCFG : (MATRIX Offset: 0x12c) Slave 7 Special Function Register -------- 
#define AT91C_TEAK_PROGRAM_ACCESS (0x1 <<  0) // (MATRIX) TEAK program memory access from AHB
#define 	AT91C_TEAK_PROGRAM_ACCESS_DISABLED             (0x0) // (MATRIX) TEAK program access disabled
#define 	AT91C_TEAK_PROGRAM_ACCESS_ENABLED              (0x1) // (MATRIX) TEAK program access enabled
#define AT91C_TEAK_BOOT           (0x1 <<  1) // (MATRIX) TEAK program start from boot routine
#define 	AT91C_TEAK_BOOT_DISABLED             (0x0 <<  1) // (MATRIX) TEAK program starts from boot routine disabled
#define 	AT91C_TEAK_BOOT_ENABLED              (0x1 <<  1) // (MATRIX) TEAK program starts from boot routine enabled
#define AT91C_TEAK_NRESET         (0x1 <<  2) // (MATRIX) active low TEAK reset
#define 	AT91C_TEAK_NRESET_ENABLED              (0x0 <<  2) // (MATRIX) active low TEAK reset enabled
#define 	AT91C_TEAK_NRESET_DISABLED             (0x1 <<  2) // (MATRIX) active low TEAK reset disabled
#define AT91C_TEAK_LVECTORP       (0x3FFFF << 14) // (MATRIX) boot routine start address

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Chip Configuration Registers
// *****************************************************************************
// *** Register offset in AT91S_CCFG structure ***
#define CCFG_EBICSA     (12) //  EBI Chip Select Assignement Register
#define CCFG_MATRIXVERSION (236) //  Version Register
// -------- CCFG_EBICSA : (CCFG Offset: 0xc) EBI Chip Select Assignement Register -------- 
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

#define AT91C_NR_PIO               (32 * 3)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Embedded Flash Controller 2.0
// *****************************************************************************
// *** Register offset in AT91S_EFC structure ***
#define EFC_FMR         ( 0) // EFC Flash Mode Register
#define EFC_FCR         ( 4) // EFC Flash Command Register
#define EFC_FSR         ( 8) // EFC Flash Status Register
#define EFC_FRR         (12) // EFC Flash Result Register
#define EFC_FVR         (16) // EFC Flash Version Register
// -------- EFC_FMR : (EFC Offset: 0x0) EFC Flash Mode Register -------- 
#define AT91C_EFC_FRDY            (0x1 <<  0) // (EFC) Ready Interrupt Enable
#define AT91C_EFC_FWS             (0xF <<  8) // (EFC) Flash Wait State.
#define 	AT91C_EFC_FWS_0WS                  (0x0 <<  8) // (EFC) 0 Wait State
#define 	AT91C_EFC_FWS_1WS                  (0x1 <<  8) // (EFC) 1 Wait State
#define 	AT91C_EFC_FWS_2WS                  (0x2 <<  8) // (EFC) 2 Wait States
#define 	AT91C_EFC_FWS_3WS                  (0x3 <<  8) // (EFC) 3 Wait States
#define 	AT91C_EFC_FWS_4WS                  (0x4 <<  8) // (EFC) 4 Wait States
#define 	AT91C_EFC_FWS_5WS                  (0x5 <<  8) // (EFC) 5 Wait States
#define 	AT91C_EFC_FWS_6WS                  (0x6 <<  8) // (EFC) 6 Wait States
// -------- EFC_FCR : (EFC Offset: 0x4) EFC Flash Command Register -------- 
#define AT91C_EFC_FCMD            (0xFF <<  0) // (EFC) Flash Command
#define 	AT91C_EFC_FCMD_GETD                 (0x0) // (EFC) Get Flash Descriptor
#define 	AT91C_EFC_FCMD_WP                   (0x1) // (EFC) Write Page
#define 	AT91C_EFC_FCMD_WPL                  (0x2) // (EFC) Write Page and Lock
#define 	AT91C_EFC_FCMD_EWP                  (0x3) // (EFC) Erase Page and Write Page
#define 	AT91C_EFC_FCMD_EWPL                 (0x4) // (EFC) Erase Page and Write Page then Lock
#define 	AT91C_EFC_FCMD_EA                   (0x5) // (EFC) Erase All
#define 	AT91C_EFC_FCMD_EPL                  (0x6) // (EFC) Erase Plane
#define 	AT91C_EFC_FCMD_EPA                  (0x7) // (EFC) Erase Pages
#define 	AT91C_EFC_FCMD_SLB                  (0x8) // (EFC) Set Lock Bit
#define 	AT91C_EFC_FCMD_CLB                  (0x9) // (EFC) Clear Lock Bit
#define 	AT91C_EFC_FCMD_GLB                  (0xA) // (EFC) Get Lock Bit
#define 	AT91C_EFC_FCMD_SFB                  (0xB) // (EFC) Set Fuse Bit
#define 	AT91C_EFC_FCMD_CFB                  (0xC) // (EFC) Clear Fuse Bit
#define 	AT91C_EFC_FCMD_GFB                  (0xD) // (EFC) Get Fuse Bit
#define AT91C_EFC_FARG            (0xFFFF <<  8) // (EFC) Flash Command Argument
#define AT91C_EFC_FKEY            (0xFF << 24) // (EFC) Flash Writing Protection Key
// -------- EFC_FSR : (EFC Offset: 0x8) EFC Flash Status Register -------- 
#define AT91C_EFC_FRDY_S          (0x1 <<  0) // (EFC) Flash Ready Status
#define AT91C_EFC_FCMDE           (0x1 <<  1) // (EFC) Flash Command Error Status
#define AT91C_EFC_LOCKE           (0x1 <<  2) // (EFC) Flash Lock Error Status
// -------- EFC_FRR : (EFC Offset: 0xc) EFC Flash Result Register -------- 
#define AT91C_EFC_FVALUE          (0x0 <<  0) // (EFC) Flash Result Value

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
#define AT91C_TC_CPCDIS           (0x1 <<  7) // (TC) Counter Clock Disable with RC Compare
#define AT91C_TC_LDBDIS           (0x1 <<  7) // (TC) Counter Clock Disabled with RB Loading
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
#define AT91C_TC_EEVT             (0x3 << 10) // (TC) External Event  Selection
#define 	AT91C_TC_EEVT_TIOB                 (0x0 << 10) // (TC) Signal selected as external event: TIOB TIOB direction: input
#define 	AT91C_TC_EEVT_XC0                  (0x1 << 10) // (TC) Signal selected as external event: XC0 TIOB direction: output
#define 	AT91C_TC_EEVT_XC1                  (0x2 << 10) // (TC) Signal selected as external event: XC1 TIOB direction: output
#define 	AT91C_TC_EEVT_XC2                  (0x3 << 10) // (TC) Signal selected as external event: XC2 TIOB direction: output
#define AT91C_TC_ABETRG           (0x1 << 10) // (TC) TIOA or TIOB External Trigger Selection
#define AT91C_TC_ENETRG           (0x1 << 12) // (TC) External Event Trigger enable
#define AT91C_TC_WAVESEL          (0x3 << 13) // (TC) Waveform  Selection
#define 	AT91C_TC_WAVESEL_UP                   (0x0 << 13) // (TC) UP mode without atomatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN               (0x1 << 13) // (TC) UPDOWN mode without automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UP_AUTO              (0x2 << 13) // (TC) UP mode with automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN_AUTO          (0x3 << 13) // (TC) UPDOWN mode with automatic trigger on RC Compare
#define AT91C_TC_CPCTRG           (0x1 << 14) // (TC) RC Compare Trigger Enable
#define AT91C_TC_WAVE             (0x1 << 15) // (TC) 
#define AT91C_TC_ACPA             (0x3 << 16) // (TC) RA Compare Effect on TIOA
#define 	AT91C_TC_ACPA_NONE                 (0x0 << 16) // (TC) Effect: none
#define 	AT91C_TC_ACPA_SET                  (0x1 << 16) // (TC) Effect: set
#define 	AT91C_TC_ACPA_CLEAR                (0x2 << 16) // (TC) Effect: clear
#define 	AT91C_TC_ACPA_TOGGLE               (0x3 << 16) // (TC) Effect: toggle
#define AT91C_TC_LDRA             (0x3 << 16) // (TC) RA Loading Selection
#define 	AT91C_TC_LDRA_NONE                 (0x0 << 16) // (TC) Edge: None
#define 	AT91C_TC_LDRA_RISING               (0x1 << 16) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRA_FALLING              (0x2 << 16) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRA_BOTH                 (0x3 << 16) // (TC) Edge: each edge of TIOA
#define AT91C_TC_ACPC             (0x3 << 18) // (TC) RC Compare Effect on TIOA
#define 	AT91C_TC_ACPC_NONE                 (0x0 << 18) // (TC) Effect: none
#define 	AT91C_TC_ACPC_SET                  (0x1 << 18) // (TC) Effect: set
#define 	AT91C_TC_ACPC_CLEAR                (0x2 << 18) // (TC) Effect: clear
#define 	AT91C_TC_ACPC_TOGGLE               (0x3 << 18) // (TC) Effect: toggle
#define AT91C_TC_LDRB             (0x3 << 18) // (TC) RB Loading Selection
#define 	AT91C_TC_LDRB_NONE                 (0x0 << 18) // (TC) Edge: None
#define 	AT91C_TC_LDRB_RISING               (0x1 << 18) // (TC) Edge: rising edge of TIOA
#define 	AT91C_TC_LDRB_FALLING              (0x2 << 18) // (TC) Edge: falling edge of TIOA
#define 	AT91C_TC_LDRB_BOTH                 (0x3 << 18) // (TC) Edge: each edge of TIOA
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
#define AT91C_TWI_NACK_SLAVE      (0x1 <<  8) // (TWI) Not Acknowledged
#define AT91C_TWI_NACK_MASTER     (0x1 <<  8) // (TWI) Not Acknowledged
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
#define AT91C_SSC_TXSYN           (0x1 << 10) // (SSC) Transmit Sync
#define AT91C_SSC_RXSYN           (0x1 << 11) // (SSC) Receive Sync
#define AT91C_SSC_TXENA           (0x1 << 16) // (SSC) Transmit Enable
#define AT91C_SSC_RXENA           (0x1 << 17) // (SSC) Receive Enable
// -------- SSC_IER : (SSC Offset: 0x44) SSC Interrupt Enable Register -------- 
// -------- SSC_IDR : (SSC Offset: 0x48) SSC Interrupt Disable Register -------- 
// -------- SSC_IMR : (SSC Offset: 0x4c) SSC Interrupt Mask Register -------- 

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
//              SOFTWARE API DEFINITION  FOR Analog to Digital Convertor
// *****************************************************************************
// *** Register offset in AT91S_ADC structure ***
#define ADC_CR          ( 0) // ADC Control Register
#define ADC_MR          ( 4) // ADC Mode Register
#define ADC_CHER        (16) // ADC Channel Enable Register
#define ADC_CHDR        (20) // ADC Channel Disable Register
#define ADC_CHSR        (24) // ADC Channel Status Register
#define ADC_SR          (28) // ADC Status Register
#define ADC_LCDR        (32) // ADC Last Converted Data Register
#define ADC_IER         (36) // ADC Interrupt Enable Register
#define ADC_IDR         (40) // ADC Interrupt Disable Register
#define ADC_IMR         (44) // ADC Interrupt Mask Register
#define ADC_CDR0        (48) // ADC Channel Data Register 0
#define ADC_CDR1        (52) // ADC Channel Data Register 1
#define ADC_CDR2        (56) // ADC Channel Data Register 2
#define ADC_CDR3        (60) // ADC Channel Data Register 3
#define ADC_CDR4        (64) // ADC Channel Data Register 4
#define ADC_CDR5        (68) // ADC Channel Data Register 5
#define ADC_CDR6        (72) // ADC Channel Data Register 6
#define ADC_CDR7        (76) // ADC Channel Data Register 7
#define ADC_RPR         (256) // Receive Pointer Register
#define ADC_RCR         (260) // Receive Counter Register
#define ADC_TPR         (264) // Transmit Pointer Register
#define ADC_TCR         (268) // Transmit Counter Register
#define ADC_RNPR        (272) // Receive Next Pointer Register
#define ADC_RNCR        (276) // Receive Next Counter Register
#define ADC_TNPR        (280) // Transmit Next Pointer Register
#define ADC_TNCR        (284) // Transmit Next Counter Register
#define ADC_PTCR        (288) // PDC Transfer Control Register
#define ADC_PTSR        (292) // PDC Transfer Status Register
// -------- ADC_CR : (ADC Offset: 0x0) ADC Control Register -------- 
#define AT91C_ADC_SWRST           (0x1 <<  0) // (ADC) Software Reset
#define AT91C_ADC_START           (0x1 <<  1) // (ADC) Start Conversion
// -------- ADC_MR : (ADC Offset: 0x4) ADC Mode Register -------- 
#define AT91C_ADC_TRGEN           (0x1 <<  0) // (ADC) Trigger Enable
#define 	AT91C_ADC_TRGEN_DIS                  (0x0) // (ADC) Hradware triggers are disabled. Starting a conversion is only possible by software
#define 	AT91C_ADC_TRGEN_EN                   (0x1) // (ADC) Hardware trigger selected by TRGSEL field is enabled.
#define AT91C_ADC_TRGSEL          (0x7 <<  1) // (ADC) Trigger Selection
#define 	AT91C_ADC_TRGSEL_TIOA0                (0x0 <<  1) // (ADC) Selected TRGSEL = TIAO0
#define 	AT91C_ADC_TRGSEL_TIOA1                (0x1 <<  1) // (ADC) Selected TRGSEL = TIAO1
#define 	AT91C_ADC_TRGSEL_TIOA2                (0x2 <<  1) // (ADC) Selected TRGSEL = TIAO2
#define 	AT91C_ADC_TRGSEL_TIOA3                (0x3 <<  1) // (ADC) Selected TRGSEL = TIAO3
#define 	AT91C_ADC_TRGSEL_TIOA4                (0x4 <<  1) // (ADC) Selected TRGSEL = TIAO4
#define 	AT91C_ADC_TRGSEL_TIOA5                (0x5 <<  1) // (ADC) Selected TRGSEL = TIAO5
#define 	AT91C_ADC_TRGSEL_EXT                  (0x6 <<  1) // (ADC) Selected TRGSEL = External Trigger
#define AT91C_ADC_LOWRES          (0x1 <<  4) // (ADC) Resolution.
#define 	AT91C_ADC_LOWRES_10_BIT               (0x0 <<  4) // (ADC) 10-bit resolution
#define 	AT91C_ADC_LOWRES_8_BIT                (0x1 <<  4) // (ADC) 8-bit resolution
#define AT91C_ADC_SLEEP           (0x1 <<  5) // (ADC) Sleep Mode
#define 	AT91C_ADC_SLEEP_NORMAL_MODE          (0x0 <<  5) // (ADC) Normal Mode
#define 	AT91C_ADC_SLEEP_MODE                 (0x1 <<  5) // (ADC) Sleep Mode
#define AT91C_ADC_PRESCAL         (0x3F <<  8) // (ADC) Prescaler rate selection
#define AT91C_ADC_STARTUP         (0x1F << 16) // (ADC) Startup Time
#define AT91C_ADC_SHTIM           (0xF << 24) // (ADC) Sample & Hold Time
// -------- 	ADC_CHER : (ADC Offset: 0x10) ADC Channel Enable Register -------- 
#define AT91C_ADC_CH0             (0x1 <<  0) // (ADC) Channel 0
#define AT91C_ADC_CH1             (0x1 <<  1) // (ADC) Channel 1
#define AT91C_ADC_CH2             (0x1 <<  2) // (ADC) Channel 2
#define AT91C_ADC_CH3             (0x1 <<  3) // (ADC) Channel 3
#define AT91C_ADC_CH4             (0x1 <<  4) // (ADC) Channel 4
#define AT91C_ADC_CH5             (0x1 <<  5) // (ADC) Channel 5
#define AT91C_ADC_CH6             (0x1 <<  6) // (ADC) Channel 6
#define AT91C_ADC_CH7             (0x1 <<  7) // (ADC) Channel 7
// -------- 	ADC_CHDR : (ADC Offset: 0x14) ADC Channel Disable Register -------- 
// -------- 	ADC_CHSR : (ADC Offset: 0x18) ADC Channel Status Register -------- 
// -------- ADC_SR : (ADC Offset: 0x1c) ADC Status Register -------- 
#define AT91C_ADC_EOC0            (0x1 <<  0) // (ADC) End of Conversion
#define AT91C_ADC_EOC1            (0x1 <<  1) // (ADC) End of Conversion
#define AT91C_ADC_EOC2            (0x1 <<  2) // (ADC) End of Conversion
#define AT91C_ADC_EOC3            (0x1 <<  3) // (ADC) End of Conversion
#define AT91C_ADC_EOC4            (0x1 <<  4) // (ADC) End of Conversion
#define AT91C_ADC_EOC5            (0x1 <<  5) // (ADC) End of Conversion
#define AT91C_ADC_EOC6            (0x1 <<  6) // (ADC) End of Conversion
#define AT91C_ADC_EOC7            (0x1 <<  7) // (ADC) End of Conversion
#define AT91C_ADC_OVRE0           (0x1 <<  8) // (ADC) Overrun Error
#define AT91C_ADC_OVRE1           (0x1 <<  9) // (ADC) Overrun Error
#define AT91C_ADC_OVRE2           (0x1 << 10) // (ADC) Overrun Error
#define AT91C_ADC_OVRE3           (0x1 << 11) // (ADC) Overrun Error
#define AT91C_ADC_OVRE4           (0x1 << 12) // (ADC) Overrun Error
#define AT91C_ADC_OVRE5           (0x1 << 13) // (ADC) Overrun Error
#define AT91C_ADC_OVRE6           (0x1 << 14) // (ADC) Overrun Error
#define AT91C_ADC_OVRE7           (0x1 << 15) // (ADC) Overrun Error
#define AT91C_ADC_DRDY            (0x1 << 16) // (ADC) Data Ready
#define AT91C_ADC_GOVRE           (0x1 << 17) // (ADC) General Overrun
#define AT91C_ADC_ENDRX           (0x1 << 18) // (ADC) End of Receiver Transfer
#define AT91C_ADC_RXBUFF          (0x1 << 19) // (ADC) RXBUFF Interrupt
// -------- ADC_LCDR : (ADC Offset: 0x20) ADC Last Converted Data Register -------- 
#define AT91C_ADC_LDATA           (0x3FF <<  0) // (ADC) Last Data Converted
// -------- ADC_IER : (ADC Offset: 0x24) ADC Interrupt Enable Register -------- 
// -------- ADC_IDR : (ADC Offset: 0x28) ADC Interrupt Disable Register -------- 
// -------- ADC_IMR : (ADC Offset: 0x2c) ADC Interrupt Mask Register -------- 
// -------- ADC_CDR0 : (ADC Offset: 0x30) ADC Channel Data Register 0 -------- 
#define AT91C_ADC_DATA            (0x3FF <<  0) // (ADC) Converted Data
// -------- ADC_CDR1 : (ADC Offset: 0x34) ADC Channel Data Register 1 -------- 
// -------- ADC_CDR2 : (ADC Offset: 0x38) ADC Channel Data Register 2 -------- 
// -------- ADC_CDR3 : (ADC Offset: 0x3c) ADC Channel Data Register 3 -------- 
// -------- ADC_CDR4 : (ADC Offset: 0x40) ADC Channel Data Register 4 -------- 
// -------- ADC_CDR5 : (ADC Offset: 0x44) ADC Channel Data Register 5 -------- 
// -------- ADC_CDR6 : (ADC Offset: 0x48) ADC Channel Data Register 6 -------- 
// -------- ADC_CDR7 : (ADC Offset: 0x4c) ADC Channel Data Register 7 -------- 

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
#define AT91C_UDP_STALLSENT       (0x1 <<  3) // (UDP) Stall sent (Control, bulk, interrupt endpoints)
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
#define AT91C_UDP_PUON            (0x1 <<  9) // (UDP) Pull-up ON

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
//               REGISTER ADDRESS DEFINITION FOR AT91SAM9XE128
// *****************************************************************************
// ========== Register definition for SYS peripheral ========== 
#define AT91C_SYS_GPBR1           (0xFFFFFFFF) // (SYS) General Purpose Register 1
#define AT91C_SYS_GPBR2           (0xFFFFFFFF) // (SYS) General Purpose Register 2
#define AT91C_SYS_GPBR0           (0xFFFFFFFF) // (SYS) General Purpose Register 0
#define AT91C_SYS_GPBR3           (0xFFFFFFFF) // (SYS) General Purpose Register 3
// ========== Register definition for EBI peripheral ========== 
#define AT91C_EBI_DUMMY           (0xFFFFEA00) // (EBI) Dummy register - Do not use
// ========== Register definition for HECC peripheral ========== 
#define AT91C_HECC_VR             (0xFFFFE8FC) // (HECC)  ECC Version register
#define AT91C_HECC_NPR            (0xFFFFE810) // (HECC)  ECC Parity N register
#define AT91C_HECC_SR             (0xFFFFE808) // (HECC)  ECC Status register
#define AT91C_HECC_PR             (0xFFFFE80C) // (HECC)  ECC Parity register
#define AT91C_HECC_MR             (0xFFFFE804) // (HECC)  ECC Page size register
#define AT91C_HECC_CR             (0xFFFFE800) // (HECC)  ECC reset register
// ========== Register definition for SDRAMC peripheral ========== 
#define AT91C_SDRAMC_MR           (0xFFFFEA00) // (SDRAMC) SDRAM Controller Mode Register
#define AT91C_SDRAMC_IMR          (0xFFFFEA1C) // (SDRAMC) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC_LPR          (0xFFFFEA10) // (SDRAMC) SDRAM Controller Low Power Register
#define AT91C_SDRAMC_ISR          (0xFFFFEA20) // (SDRAMC) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC_IDR          (0xFFFFEA18) // (SDRAMC) SDRAM Controller Interrupt Disable Register
#define AT91C_SDRAMC_CR           (0xFFFFEA08) // (SDRAMC) SDRAM Controller Configuration Register
#define AT91C_SDRAMC_TR           (0xFFFFEA04) // (SDRAMC) SDRAM Controller Refresh Timer Register
#define AT91C_SDRAMC_MDR          (0xFFFFEA24) // (SDRAMC) SDRAM Memory Device Register
#define AT91C_SDRAMC_HSR          (0xFFFFEA0C) // (SDRAMC) SDRAM Controller High Speed Register
#define AT91C_SDRAMC_IER          (0xFFFFEA14) // (SDRAMC) SDRAM Controller Interrupt Enable Register
// ========== Register definition for SMC0 peripheral ========== 
#define AT91C_SMC0_CTRL1          (0xFFFFEC1C) // (SMC0)  Control Register for CS 1
#define AT91C_SMC0_PULSE7         (0xFFFFEC74) // (SMC0)  Pulse Register for CS 7
#define AT91C_SMC0_PULSE6         (0xFFFFEC64) // (SMC0)  Pulse Register for CS 6
#define AT91C_SMC0_SETUP4         (0xFFFFEC40) // (SMC0)  Setup Register for CS 4
#define AT91C_SMC0_PULSE3         (0xFFFFEC34) // (SMC0)  Pulse Register for CS 3
#define AT91C_SMC0_CYCLE5         (0xFFFFEC58) // (SMC0)  Cycle Register for CS 5
#define AT91C_SMC0_CYCLE2         (0xFFFFEC28) // (SMC0)  Cycle Register for CS 2
#define AT91C_SMC0_CTRL2          (0xFFFFEC2C) // (SMC0)  Control Register for CS 2
#define AT91C_SMC0_CTRL0          (0xFFFFEC0C) // (SMC0)  Control Register for CS 0
#define AT91C_SMC0_PULSE5         (0xFFFFEC54) // (SMC0)  Pulse Register for CS 5
#define AT91C_SMC0_PULSE1         (0xFFFFEC14) // (SMC0)  Pulse Register for CS 1
#define AT91C_SMC0_PULSE0         (0xFFFFEC04) // (SMC0)  Pulse Register for CS 0
#define AT91C_SMC0_CYCLE7         (0xFFFFEC78) // (SMC0)  Cycle Register for CS 7
#define AT91C_SMC0_CTRL4          (0xFFFFEC4C) // (SMC0)  Control Register for CS 4
#define AT91C_SMC0_CTRL3          (0xFFFFEC3C) // (SMC0)  Control Register for CS 3
#define AT91C_SMC0_SETUP7         (0xFFFFEC70) // (SMC0)  Setup Register for CS 7
#define AT91C_SMC0_CTRL7          (0xFFFFEC7C) // (SMC0)  Control Register for CS 7
#define AT91C_SMC0_SETUP1         (0xFFFFEC10) // (SMC0)  Setup Register for CS 1
#define AT91C_SMC0_CYCLE0         (0xFFFFEC08) // (SMC0)  Cycle Register for CS 0
#define AT91C_SMC0_CTRL5          (0xFFFFEC5C) // (SMC0)  Control Register for CS 5
#define AT91C_SMC0_CYCLE1         (0xFFFFEC18) // (SMC0)  Cycle Register for CS 1
#define AT91C_SMC0_CTRL6          (0xFFFFEC6C) // (SMC0)  Control Register for CS 6
#define AT91C_SMC0_SETUP0         (0xFFFFEC00) // (SMC0)  Setup Register for CS 0
#define AT91C_SMC0_PULSE4         (0xFFFFEC44) // (SMC0)  Pulse Register for CS 4
#define AT91C_SMC0_SETUP5         (0xFFFFEC50) // (SMC0)  Setup Register for CS 5
#define AT91C_SMC0_SETUP2         (0xFFFFEC20) // (SMC0)  Setup Register for CS 2
#define AT91C_SMC0_CYCLE3         (0xFFFFEC38) // (SMC0)  Cycle Register for CS 3
#define AT91C_SMC0_CYCLE6         (0xFFFFEC68) // (SMC0)  Cycle Register for CS 6
#define AT91C_SMC0_SETUP6         (0xFFFFEC60) // (SMC0)  Setup Register for CS 6
#define AT91C_SMC0_CYCLE4         (0xFFFFEC48) // (SMC0)  Cycle Register for CS 4
#define AT91C_SMC0_PULSE2         (0xFFFFEC24) // (SMC0)  Pulse Register for CS 2
#define AT91C_SMC0_SETUP3         (0xFFFFEC30) // (SMC0)  Setup Register for CS 3
// ========== Register definition for MATRIX peripheral ========== 
#define AT91C_MATRIX_MCFG0        (0xFFFFEE00) // (MATRIX)  Master Configuration Register 0 (ram96k)     
#define AT91C_MATRIX_MCFG7        (0xFFFFEE1C) // (MATRIX)  Master Configuration Register 7 (teak_prog)     
#define AT91C_MATRIX_SCFG1        (0xFFFFEE44) // (MATRIX)  Slave Configuration Register 1 (rom)    
#define AT91C_MATRIX_MCFG4        (0xFFFFEE10) // (MATRIX)  Master Configuration Register 4 (bridge)    
#define AT91C_MATRIX_VERSION      (0xFFFFEFFC) // (MATRIX)  Version Register
#define AT91C_MATRIX_MCFG2        (0xFFFFEE08) // (MATRIX)  Master Configuration Register 2 (hperiphs) 
#define AT91C_MATRIX_PRBS0        (0xFFFFEE84) // (MATRIX)  PRBS0 (ram0) 
#define AT91C_MATRIX_SCFG3        (0xFFFFEE4C) // (MATRIX)  Slave Configuration Register 3 (ebi)
#define AT91C_MATRIX_MCFG6        (0xFFFFEE18) // (MATRIX)  Master Configuration Register 6 (ram16k)  
#define AT91C_MATRIX_EBI          (0xFFFFEF1C) // (MATRIX)  Slave 3 (ebi) Special Function Register
#define AT91C_MATRIX_SCFG0        (0xFFFFEE40) // (MATRIX)  Slave Configuration Register 0 (ram96k)     
#define AT91C_MATRIX_PRAS0        (0xFFFFEE80) // (MATRIX)  PRAS0 (ram0) 
#define AT91C_MATRIX_MCFG3        (0xFFFFEE0C) // (MATRIX)  Master Configuration Register 3 (ebi)
#define AT91C_MATRIX_PRAS1        (0xFFFFEE88) // (MATRIX)  PRAS1 (ram1) 
#define AT91C_MATRIX_PRAS2        (0xFFFFEE90) // (MATRIX)  PRAS2 (ram2) 
#define AT91C_MATRIX_SCFG2        (0xFFFFEE48) // (MATRIX)  Slave Configuration Register 2 (hperiphs) 
#define AT91C_MATRIX_MCFG5        (0xFFFFEE14) // (MATRIX)  Master Configuration Register 5 (mailbox)    
#define AT91C_MATRIX_MCFG1        (0xFFFFEE04) // (MATRIX)  Master Configuration Register 1 (rom)    
#define AT91C_MATRIX_MRCR         (0xFFFFEF00) // (MATRIX)  Master Remp Control Register 
#define AT91C_MATRIX_PRBS2        (0xFFFFEE94) // (MATRIX)  PRBS2 (ram2) 
#define AT91C_MATRIX_SCFG4        (0xFFFFEE50) // (MATRIX)  Slave Configuration Register 4 (bridge)    
#define AT91C_MATRIX_TEAKCFG      (0xFFFFEF2C) // (MATRIX)  Slave 7 (teak_prog) Special Function Register
#define AT91C_MATRIX_PRBS1        (0xFFFFEE8C) // (MATRIX)  PRBS1 (ram1) 
// ========== Register definition for CCFG peripheral ========== 
#define AT91C_CCFG_MATRIXVERSION  (0xFFFFEFFC) // (CCFG)  Version Register
#define AT91C_CCFG_EBICSA         (0xFFFFEF1C) // (CCFG)  EBI Chip Select Assignement Register
// ========== Register definition for PDC_DBGU peripheral ========== 
#define AT91C_DBGU_TCR            (0xFFFFF30C) // (PDC_DBGU) Transmit Counter Register
#define AT91C_DBGU_RNPR           (0xFFFFF310) // (PDC_DBGU) Receive Next Pointer Register
#define AT91C_DBGU_TNPR           (0xFFFFF318) // (PDC_DBGU) Transmit Next Pointer Register
#define AT91C_DBGU_TPR            (0xFFFFF308) // (PDC_DBGU) Transmit Pointer Register
#define AT91C_DBGU_RPR            (0xFFFFF300) // (PDC_DBGU) Receive Pointer Register
#define AT91C_DBGU_RCR            (0xFFFFF304) // (PDC_DBGU) Receive Counter Register
#define AT91C_DBGU_RNCR           (0xFFFFF314) // (PDC_DBGU) Receive Next Counter Register
#define AT91C_DBGU_PTCR           (0xFFFFF320) // (PDC_DBGU) PDC Transfer Control Register
#define AT91C_DBGU_PTSR           (0xFFFFF324) // (PDC_DBGU) PDC Transfer Status Register
#define AT91C_DBGU_TNCR           (0xFFFFF31C) // (PDC_DBGU) Transmit Next Counter Register
// ========== Register definition for DBGU peripheral ========== 
#define AT91C_DBGU_EXID           (0xFFFFF244) // (DBGU) Chip ID Extension Register
#define AT91C_DBGU_BRGR           (0xFFFFF220) // (DBGU) Baud Rate Generator Register
#define AT91C_DBGU_IDR            (0xFFFFF20C) // (DBGU) Interrupt Disable Register
#define AT91C_DBGU_CSR            (0xFFFFF214) // (DBGU) Channel Status Register
#define AT91C_DBGU_CIDR           (0xFFFFF240) // (DBGU) Chip ID Register
#define AT91C_DBGU_MR             (0xFFFFF204) // (DBGU) Mode Register
#define AT91C_DBGU_IMR            (0xFFFFF210) // (DBGU) Interrupt Mask Register
#define AT91C_DBGU_CR             (0xFFFFF200) // (DBGU) Control Register
#define AT91C_DBGU_FNTR           (0xFFFFF248) // (DBGU) Force NTRST Register
#define AT91C_DBGU_THR            (0xFFFFF21C) // (DBGU) Transmitter Holding Register
#define AT91C_DBGU_RHR            (0xFFFFF218) // (DBGU) Receiver Holding Register
#define AT91C_DBGU_IER            (0xFFFFF208) // (DBGU) Interrupt Enable Register
// ========== Register definition for AIC peripheral ========== 
#define AT91C_AIC_IVR             (0xFFFFF100) // (AIC) IRQ Vector Register
#define AT91C_AIC_SMR             (0xFFFFF000) // (AIC) Source Mode Register
#define AT91C_AIC_FVR             (0xFFFFF104) // (AIC) FIQ Vector Register
#define AT91C_AIC_DCR             (0xFFFFF138) // (AIC) Debug Control Register (Protect)
#define AT91C_AIC_EOICR           (0xFFFFF130) // (AIC) End of Interrupt Command Register
#define AT91C_AIC_SVR             (0xFFFFF080) // (AIC) Source Vector Register
#define AT91C_AIC_FFSR            (0xFFFFF148) // (AIC) Fast Forcing Status Register
#define AT91C_AIC_ICCR            (0xFFFFF128) // (AIC) Interrupt Clear Command Register
#define AT91C_AIC_ISR             (0xFFFFF108) // (AIC) Interrupt Status Register
#define AT91C_AIC_IMR             (0xFFFFF110) // (AIC) Interrupt Mask Register
#define AT91C_AIC_IPR             (0xFFFFF10C) // (AIC) Interrupt Pending Register
#define AT91C_AIC_FFER            (0xFFFFF140) // (AIC) Fast Forcing Enable Register
#define AT91C_AIC_IECR            (0xFFFFF120) // (AIC) Interrupt Enable Command Register
#define AT91C_AIC_ISCR            (0xFFFFF12C) // (AIC) Interrupt Set Command Register
#define AT91C_AIC_FFDR            (0xFFFFF144) // (AIC) Fast Forcing Disable Register
#define AT91C_AIC_CISR            (0xFFFFF114) // (AIC) Core Interrupt Status Register
#define AT91C_AIC_IDCR            (0xFFFFF124) // (AIC) Interrupt Disable Command Register
#define AT91C_AIC_SPU             (0xFFFFF134) // (AIC) Spurious Vector Register
// ========== Register definition for PIOA peripheral ========== 
#define AT91C_PIOA_ODR            (0xFFFFF414) // (PIOA) Output Disable Registerr
#define AT91C_PIOA_SODR           (0xFFFFF430) // (PIOA) Set Output Data Register
#define AT91C_PIOA_ISR            (0xFFFFF44C) // (PIOA) Interrupt Status Register
#define AT91C_PIOA_ABSR           (0xFFFFF478) // (PIOA) AB Select Status Register
#define AT91C_PIOA_IER            (0xFFFFF440) // (PIOA) Interrupt Enable Register
#define AT91C_PIOA_PPUDR          (0xFFFFF460) // (PIOA) Pull-up Disable Register
#define AT91C_PIOA_IMR            (0xFFFFF448) // (PIOA) Interrupt Mask Register
#define AT91C_PIOA_PER            (0xFFFFF400) // (PIOA) PIO Enable Register
#define AT91C_PIOA_IFDR           (0xFFFFF424) // (PIOA) Input Filter Disable Register
#define AT91C_PIOA_OWDR           (0xFFFFF4A4) // (PIOA) Output Write Disable Register
#define AT91C_PIOA_MDSR           (0xFFFFF458) // (PIOA) Multi-driver Status Register
#define AT91C_PIOA_IDR            (0xFFFFF444) // (PIOA) Interrupt Disable Register
#define AT91C_PIOA_ODSR           (0xFFFFF438) // (PIOA) Output Data Status Register
#define AT91C_PIOA_PPUSR          (0xFFFFF468) // (PIOA) Pull-up Status Register
#define AT91C_PIOA_OWSR           (0xFFFFF4A8) // (PIOA) Output Write Status Register
#define AT91C_PIOA_BSR            (0xFFFFF474) // (PIOA) Select B Register
#define AT91C_PIOA_OWER           (0xFFFFF4A0) // (PIOA) Output Write Enable Register
#define AT91C_PIOA_IFER           (0xFFFFF420) // (PIOA) Input Filter Enable Register
#define AT91C_PIOA_PDSR           (0xFFFFF43C) // (PIOA) Pin Data Status Register
#define AT91C_PIOA_PPUER          (0xFFFFF464) // (PIOA) Pull-up Enable Register
#define AT91C_PIOA_OSR            (0xFFFFF418) // (PIOA) Output Status Register
#define AT91C_PIOA_ASR            (0xFFFFF470) // (PIOA) Select A Register
#define AT91C_PIOA_MDDR           (0xFFFFF454) // (PIOA) Multi-driver Disable Register
#define AT91C_PIOA_CODR           (0xFFFFF434) // (PIOA) Clear Output Data Register
#define AT91C_PIOA_MDER           (0xFFFFF450) // (PIOA) Multi-driver Enable Register
#define AT91C_PIOA_PDR            (0xFFFFF404) // (PIOA) PIO Disable Register
#define AT91C_PIOA_IFSR           (0xFFFFF428) // (PIOA) Input Filter Status Register
#define AT91C_PIOA_OER            (0xFFFFF410) // (PIOA) Output Enable Register
#define AT91C_PIOA_PSR            (0xFFFFF408) // (PIOA) PIO Status Register
// ========== Register definition for PIOB peripheral ========== 
#define AT91C_PIOB_OWDR           (0xFFFFF6A4) // (PIOB) Output Write Disable Register
#define AT91C_PIOB_MDER           (0xFFFFF650) // (PIOB) Multi-driver Enable Register
#define AT91C_PIOB_PPUSR          (0xFFFFF668) // (PIOB) Pull-up Status Register
#define AT91C_PIOB_IMR            (0xFFFFF648) // (PIOB) Interrupt Mask Register
#define AT91C_PIOB_ASR            (0xFFFFF670) // (PIOB) Select A Register
#define AT91C_PIOB_PPUDR          (0xFFFFF660) // (PIOB) Pull-up Disable Register
#define AT91C_PIOB_PSR            (0xFFFFF608) // (PIOB) PIO Status Register
#define AT91C_PIOB_IER            (0xFFFFF640) // (PIOB) Interrupt Enable Register
#define AT91C_PIOB_CODR           (0xFFFFF634) // (PIOB) Clear Output Data Register
#define AT91C_PIOB_OWER           (0xFFFFF6A0) // (PIOB) Output Write Enable Register
#define AT91C_PIOB_ABSR           (0xFFFFF678) // (PIOB) AB Select Status Register
#define AT91C_PIOB_IFDR           (0xFFFFF624) // (PIOB) Input Filter Disable Register
#define AT91C_PIOB_PDSR           (0xFFFFF63C) // (PIOB) Pin Data Status Register
#define AT91C_PIOB_IDR            (0xFFFFF644) // (PIOB) Interrupt Disable Register
#define AT91C_PIOB_OWSR           (0xFFFFF6A8) // (PIOB) Output Write Status Register
#define AT91C_PIOB_PDR            (0xFFFFF604) // (PIOB) PIO Disable Register
#define AT91C_PIOB_ODR            (0xFFFFF614) // (PIOB) Output Disable Registerr
#define AT91C_PIOB_IFSR           (0xFFFFF628) // (PIOB) Input Filter Status Register
#define AT91C_PIOB_PPUER          (0xFFFFF664) // (PIOB) Pull-up Enable Register
#define AT91C_PIOB_SODR           (0xFFFFF630) // (PIOB) Set Output Data Register
#define AT91C_PIOB_ISR            (0xFFFFF64C) // (PIOB) Interrupt Status Register
#define AT91C_PIOB_ODSR           (0xFFFFF638) // (PIOB) Output Data Status Register
#define AT91C_PIOB_OSR            (0xFFFFF618) // (PIOB) Output Status Register
#define AT91C_PIOB_MDSR           (0xFFFFF658) // (PIOB) Multi-driver Status Register
#define AT91C_PIOB_IFER           (0xFFFFF620) // (PIOB) Input Filter Enable Register
#define AT91C_PIOB_BSR            (0xFFFFF674) // (PIOB) Select B Register
#define AT91C_PIOB_MDDR           (0xFFFFF654) // (PIOB) Multi-driver Disable Register
#define AT91C_PIOB_OER            (0xFFFFF610) // (PIOB) Output Enable Register
#define AT91C_PIOB_PER            (0xFFFFF600) // (PIOB) PIO Enable Register
// ========== Register definition for PIOC peripheral ========== 
#define AT91C_PIOC_OWDR           (0xFFFFF8A4) // (PIOC) Output Write Disable Register
#define AT91C_PIOC_SODR           (0xFFFFF830) // (PIOC) Set Output Data Register
#define AT91C_PIOC_PPUER          (0xFFFFF864) // (PIOC) Pull-up Enable Register
#define AT91C_PIOC_CODR           (0xFFFFF834) // (PIOC) Clear Output Data Register
#define AT91C_PIOC_PSR            (0xFFFFF808) // (PIOC) PIO Status Register
#define AT91C_PIOC_PDR            (0xFFFFF804) // (PIOC) PIO Disable Register
#define AT91C_PIOC_ODR            (0xFFFFF814) // (PIOC) Output Disable Registerr
#define AT91C_PIOC_PPUSR          (0xFFFFF868) // (PIOC) Pull-up Status Register
#define AT91C_PIOC_ABSR           (0xFFFFF878) // (PIOC) AB Select Status Register
#define AT91C_PIOC_IFSR           (0xFFFFF828) // (PIOC) Input Filter Status Register
#define AT91C_PIOC_OER            (0xFFFFF810) // (PIOC) Output Enable Register
#define AT91C_PIOC_IMR            (0xFFFFF848) // (PIOC) Interrupt Mask Register
#define AT91C_PIOC_ASR            (0xFFFFF870) // (PIOC) Select A Register
#define AT91C_PIOC_MDDR           (0xFFFFF854) // (PIOC) Multi-driver Disable Register
#define AT91C_PIOC_OWSR           (0xFFFFF8A8) // (PIOC) Output Write Status Register
#define AT91C_PIOC_PER            (0xFFFFF800) // (PIOC) PIO Enable Register
#define AT91C_PIOC_IDR            (0xFFFFF844) // (PIOC) Interrupt Disable Register
#define AT91C_PIOC_MDER           (0xFFFFF850) // (PIOC) Multi-driver Enable Register
#define AT91C_PIOC_PDSR           (0xFFFFF83C) // (PIOC) Pin Data Status Register
#define AT91C_PIOC_MDSR           (0xFFFFF858) // (PIOC) Multi-driver Status Register
#define AT91C_PIOC_OWER           (0xFFFFF8A0) // (PIOC) Output Write Enable Register
#define AT91C_PIOC_BSR            (0xFFFFF874) // (PIOC) Select B Register
#define AT91C_PIOC_PPUDR          (0xFFFFF860) // (PIOC) Pull-up Disable Register
#define AT91C_PIOC_IFDR           (0xFFFFF824) // (PIOC) Input Filter Disable Register
#define AT91C_PIOC_IER            (0xFFFFF840) // (PIOC) Interrupt Enable Register
#define AT91C_PIOC_OSR            (0xFFFFF818) // (PIOC) Output Status Register
#define AT91C_PIOC_ODSR           (0xFFFFF838) // (PIOC) Output Data Status Register
#define AT91C_PIOC_ISR            (0xFFFFF84C) // (PIOC) Interrupt Status Register
#define AT91C_PIOC_IFER           (0xFFFFF820) // (PIOC) Input Filter Enable Register
// ========== Register definition for EFC peripheral ========== 
#define AT91C_EFC_FVR             (0xFFFFFA10) // (EFC) EFC Flash Version Register
#define AT91C_EFC_FCR             (0xFFFFFA04) // (EFC) EFC Flash Command Register
#define AT91C_EFC_FMR             (0xFFFFFA00) // (EFC) EFC Flash Mode Register
#define AT91C_EFC_FRR             (0xFFFFFA0C) // (EFC) EFC Flash Result Register
#define AT91C_EFC_FSR             (0xFFFFFA08) // (EFC) EFC Flash Status Register
// ========== Register definition for CKGR peripheral ========== 
#define AT91C_CKGR_MOR            (0xFFFFFC20) // (CKGR) Main Oscillator Register
#define AT91C_CKGR_PLLBR          (0xFFFFFC2C) // (CKGR) PLL B Register
#define AT91C_CKGR_MCFR           (0xFFFFFC24) // (CKGR) Main Clock  Frequency Register
#define AT91C_CKGR_PLLAR          (0xFFFFFC28) // (CKGR) PLL A Register
// ========== Register definition for PMC peripheral ========== 
#define AT91C_PMC_PCER            (0xFFFFFC10) // (PMC) Peripheral Clock Enable Register
#define AT91C_PMC_PCKR            (0xFFFFFC40) // (PMC) Programmable Clock Register
#define AT91C_PMC_MCKR            (0xFFFFFC30) // (PMC) Master Clock Register
#define AT91C_PMC_PLLAR           (0xFFFFFC28) // (PMC) PLL A Register
#define AT91C_PMC_PCDR            (0xFFFFFC14) // (PMC) Peripheral Clock Disable Register
#define AT91C_PMC_SCSR            (0xFFFFFC08) // (PMC) System Clock Status Register
#define AT91C_PMC_MCFR            (0xFFFFFC24) // (PMC) Main Clock  Frequency Register
#define AT91C_PMC_IMR             (0xFFFFFC6C) // (PMC) Interrupt Mask Register
#define AT91C_PMC_IER             (0xFFFFFC60) // (PMC) Interrupt Enable Register
#define AT91C_PMC_MOR             (0xFFFFFC20) // (PMC) Main Oscillator Register
#define AT91C_PMC_IDR             (0xFFFFFC64) // (PMC) Interrupt Disable Register
#define AT91C_PMC_PLLBR           (0xFFFFFC2C) // (PMC) PLL B Register
#define AT91C_PMC_SCDR            (0xFFFFFC04) // (PMC) System Clock Disable Register
#define AT91C_PMC_PCSR            (0xFFFFFC18) // (PMC) Peripheral Clock Status Register
#define AT91C_PMC_SCER            (0xFFFFFC00) // (PMC) System Clock Enable Register
#define AT91C_PMC_SR              (0xFFFFFC68) // (PMC) Status Register
// ========== Register definition for RSTC peripheral ========== 
#define AT91C_RSTC_RCR            (0xFFFFFD00) // (RSTC) Reset Control Register
#define AT91C_RSTC_RMR            (0xFFFFFD08) // (RSTC) Reset Mode Register
#define AT91C_RSTC_RSR            (0xFFFFFD04) // (RSTC) Reset Status Register
// ========== Register definition for SHDWC peripheral ========== 
#define AT91C_SHDWC_SHSR          (0xFFFFFD18) // (SHDWC) Shut Down Status Register
#define AT91C_SHDWC_SHMR          (0xFFFFFD14) // (SHDWC) Shut Down Mode Register
#define AT91C_SHDWC_SHCR          (0xFFFFFD10) // (SHDWC) Shut Down Control Register
// ========== Register definition for RTTC peripheral ========== 
#define AT91C_RTTC_RTSR           (0xFFFFFD2C) // (RTTC) Real-time Status Register
#define AT91C_RTTC_RTMR           (0xFFFFFD20) // (RTTC) Real-time Mode Register
#define AT91C_RTTC_RTVR           (0xFFFFFD28) // (RTTC) Real-time Value Register
#define AT91C_RTTC_RTAR           (0xFFFFFD24) // (RTTC) Real-time Alarm Register
// ========== Register definition for PITC peripheral ========== 
#define AT91C_PITC_PIVR           (0xFFFFFD38) // (PITC) Period Interval Value Register
#define AT91C_PITC_PISR           (0xFFFFFD34) // (PITC) Period Interval Status Register
#define AT91C_PITC_PIIR           (0xFFFFFD3C) // (PITC) Period Interval Image Register
#define AT91C_PITC_PIMR           (0xFFFFFD30) // (PITC) Period Interval Mode Register
// ========== Register definition for WDTC peripheral ========== 
#define AT91C_WDTC_WDCR           (0xFFFFFD40) // (WDTC) Watchdog Control Register
#define AT91C_WDTC_WDSR           (0xFFFFFD48) // (WDTC) Watchdog Status Register
#define AT91C_WDTC_WDMR           (0xFFFFFD44) // (WDTC) Watchdog Mode Register
// ========== Register definition for TC0 peripheral ========== 
#define AT91C_TC0_SR              (0xFFFA0020) // (TC0) Status Register
#define AT91C_TC0_RC              (0xFFFA001C) // (TC0) Register C
#define AT91C_TC0_RB              (0xFFFA0018) // (TC0) Register B
#define AT91C_TC0_CCR             (0xFFFA0000) // (TC0) Channel Control Register
#define AT91C_TC0_CMR             (0xFFFA0004) // (TC0) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC0_IER             (0xFFFA0024) // (TC0) Interrupt Enable Register
#define AT91C_TC0_RA              (0xFFFA0014) // (TC0) Register A
#define AT91C_TC0_IDR             (0xFFFA0028) // (TC0) Interrupt Disable Register
#define AT91C_TC0_CV              (0xFFFA0010) // (TC0) Counter Value
#define AT91C_TC0_IMR             (0xFFFA002C) // (TC0) Interrupt Mask Register
// ========== Register definition for TC1 peripheral ========== 
#define AT91C_TC1_RB              (0xFFFA0058) // (TC1) Register B
#define AT91C_TC1_CCR             (0xFFFA0040) // (TC1) Channel Control Register
#define AT91C_TC1_IER             (0xFFFA0064) // (TC1) Interrupt Enable Register
#define AT91C_TC1_IDR             (0xFFFA0068) // (TC1) Interrupt Disable Register
#define AT91C_TC1_SR              (0xFFFA0060) // (TC1) Status Register
#define AT91C_TC1_CMR             (0xFFFA0044) // (TC1) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC1_RA              (0xFFFA0054) // (TC1) Register A
#define AT91C_TC1_RC              (0xFFFA005C) // (TC1) Register C
#define AT91C_TC1_IMR             (0xFFFA006C) // (TC1) Interrupt Mask Register
#define AT91C_TC1_CV              (0xFFFA0050) // (TC1) Counter Value
// ========== Register definition for TC2 peripheral ========== 
#define AT91C_TC2_CMR             (0xFFFA0084) // (TC2) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC2_CCR             (0xFFFA0080) // (TC2) Channel Control Register
#define AT91C_TC2_CV              (0xFFFA0090) // (TC2) Counter Value
#define AT91C_TC2_RA              (0xFFFA0094) // (TC2) Register A
#define AT91C_TC2_RB              (0xFFFA0098) // (TC2) Register B
#define AT91C_TC2_IDR             (0xFFFA00A8) // (TC2) Interrupt Disable Register
#define AT91C_TC2_IMR             (0xFFFA00AC) // (TC2) Interrupt Mask Register
#define AT91C_TC2_RC              (0xFFFA009C) // (TC2) Register C
#define AT91C_TC2_IER             (0xFFFA00A4) // (TC2) Interrupt Enable Register
#define AT91C_TC2_SR              (0xFFFA00A0) // (TC2) Status Register
// ========== Register definition for TC3 peripheral ========== 
#define AT91C_TC3_IER             (0xFFFDC024) // (TC3) Interrupt Enable Register
#define AT91C_TC3_RB              (0xFFFDC018) // (TC3) Register B
#define AT91C_TC3_CMR             (0xFFFDC004) // (TC3) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC3_RC              (0xFFFDC01C) // (TC3) Register C
#define AT91C_TC3_CCR             (0xFFFDC000) // (TC3) Channel Control Register
#define AT91C_TC3_SR              (0xFFFDC020) // (TC3) Status Register
#define AT91C_TC3_CV              (0xFFFDC010) // (TC3) Counter Value
#define AT91C_TC3_RA              (0xFFFDC014) // (TC3) Register A
#define AT91C_TC3_IDR             (0xFFFDC028) // (TC3) Interrupt Disable Register
#define AT91C_TC3_IMR             (0xFFFDC02C) // (TC3) Interrupt Mask Register
// ========== Register definition for TC4 peripheral ========== 
#define AT91C_TC4_CMR             (0xFFFDC044) // (TC4) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC4_RC              (0xFFFDC05C) // (TC4) Register C
#define AT91C_TC4_SR              (0xFFFDC060) // (TC4) Status Register
#define AT91C_TC4_RB              (0xFFFDC058) // (TC4) Register B
#define AT91C_TC4_IER             (0xFFFDC064) // (TC4) Interrupt Enable Register
#define AT91C_TC4_CV              (0xFFFDC050) // (TC4) Counter Value
#define AT91C_TC4_RA              (0xFFFDC054) // (TC4) Register A
#define AT91C_TC4_IDR             (0xFFFDC068) // (TC4) Interrupt Disable Register
#define AT91C_TC4_IMR             (0xFFFDC06C) // (TC4) Interrupt Mask Register
#define AT91C_TC4_CCR             (0xFFFDC040) // (TC4) Channel Control Register
// ========== Register definition for TC5 peripheral ========== 
#define AT91C_TC5_RB              (0xFFFDC098) // (TC5) Register B
#define AT91C_TC5_RA              (0xFFFDC094) // (TC5) Register A
#define AT91C_TC5_CV              (0xFFFDC090) // (TC5) Counter Value
#define AT91C_TC5_CCR             (0xFFFDC080) // (TC5) Channel Control Register
#define AT91C_TC5_SR              (0xFFFDC0A0) // (TC5) Status Register
#define AT91C_TC5_IER             (0xFFFDC0A4) // (TC5) Interrupt Enable Register
#define AT91C_TC5_IDR             (0xFFFDC0A8) // (TC5) Interrupt Disable Register
#define AT91C_TC5_RC              (0xFFFDC09C) // (TC5) Register C
#define AT91C_TC5_IMR             (0xFFFDC0AC) // (TC5) Interrupt Mask Register
#define AT91C_TC5_CMR             (0xFFFDC084) // (TC5) Channel Mode Register (Capture Mode / Waveform Mode)
// ========== Register definition for TCB0 peripheral ========== 
#define AT91C_TCB0_BMR            (0xFFFA00C4) // (TCB0) TC Block Mode Register
#define AT91C_TCB0_BCR            (0xFFFA00C0) // (TCB0) TC Block Control Register
// ========== Register definition for TCB1 peripheral ========== 
#define AT91C_TCB1_BCR            (0xFFFDC0C0) // (TCB1) TC Block Control Register
#define AT91C_TCB1_BMR            (0xFFFDC0C4) // (TCB1) TC Block Mode Register
// ========== Register definition for PDC_MCI peripheral ========== 
#define AT91C_MCI_RNCR            (0xFFFA8114) // (PDC_MCI) Receive Next Counter Register
#define AT91C_MCI_TCR             (0xFFFA810C) // (PDC_MCI) Transmit Counter Register
#define AT91C_MCI_RCR             (0xFFFA8104) // (PDC_MCI) Receive Counter Register
#define AT91C_MCI_TNPR            (0xFFFA8118) // (PDC_MCI) Transmit Next Pointer Register
#define AT91C_MCI_RNPR            (0xFFFA8110) // (PDC_MCI) Receive Next Pointer Register
#define AT91C_MCI_RPR             (0xFFFA8100) // (PDC_MCI) Receive Pointer Register
#define AT91C_MCI_TNCR            (0xFFFA811C) // (PDC_MCI) Transmit Next Counter Register
#define AT91C_MCI_TPR             (0xFFFA8108) // (PDC_MCI) Transmit Pointer Register
#define AT91C_MCI_PTSR            (0xFFFA8124) // (PDC_MCI) PDC Transfer Status Register
#define AT91C_MCI_PTCR            (0xFFFA8120) // (PDC_MCI) PDC Transfer Control Register
// ========== Register definition for MCI peripheral ========== 
#define AT91C_MCI_RDR             (0xFFFA8030) // (MCI) MCI Receive Data Register
#define AT91C_MCI_CMDR            (0xFFFA8014) // (MCI) MCI Command Register
#define AT91C_MCI_VR              (0xFFFA80FC) // (MCI) MCI Version Register
#define AT91C_MCI_IDR             (0xFFFA8048) // (MCI) MCI Interrupt Disable Register
#define AT91C_MCI_DTOR            (0xFFFA8008) // (MCI) MCI Data Timeout Register
#define AT91C_MCI_TDR             (0xFFFA8034) // (MCI) MCI Transmit Data Register
#define AT91C_MCI_IER             (0xFFFA8044) // (MCI) MCI Interrupt Enable Register
#define AT91C_MCI_BLKR            (0xFFFA8018) // (MCI) MCI Block Register
#define AT91C_MCI_MR              (0xFFFA8004) // (MCI) MCI Mode Register
#define AT91C_MCI_IMR             (0xFFFA804C) // (MCI) MCI Interrupt Mask Register
#define AT91C_MCI_CR              (0xFFFA8000) // (MCI) MCI Control Register
#define AT91C_MCI_ARGR            (0xFFFA8010) // (MCI) MCI Argument Register
#define AT91C_MCI_SDCR            (0xFFFA800C) // (MCI) MCI SD Card Register
#define AT91C_MCI_SR              (0xFFFA8040) // (MCI) MCI Status Register
#define AT91C_MCI_RSPR            (0xFFFA8020) // (MCI) MCI Response Register
// ========== Register definition for PDC_TWI0 peripheral ========== 
#define AT91C_TWI0_PTSR           (0xFFFAC124) // (PDC_TWI0) PDC Transfer Status Register
#define AT91C_TWI0_RPR            (0xFFFAC100) // (PDC_TWI0) Receive Pointer Register
#define AT91C_TWI0_RNCR           (0xFFFAC114) // (PDC_TWI0) Receive Next Counter Register
#define AT91C_TWI0_RCR            (0xFFFAC104) // (PDC_TWI0) Receive Counter Register
#define AT91C_TWI0_PTCR           (0xFFFAC120) // (PDC_TWI0) PDC Transfer Control Register
#define AT91C_TWI0_TPR            (0xFFFAC108) // (PDC_TWI0) Transmit Pointer Register
#define AT91C_TWI0_RNPR           (0xFFFAC110) // (PDC_TWI0) Receive Next Pointer Register
#define AT91C_TWI0_TNPR           (0xFFFAC118) // (PDC_TWI0) Transmit Next Pointer Register
#define AT91C_TWI0_TCR            (0xFFFAC10C) // (PDC_TWI0) Transmit Counter Register
#define AT91C_TWI0_TNCR           (0xFFFAC11C) // (PDC_TWI0) Transmit Next Counter Register
// ========== Register definition for TWI0 peripheral ========== 
#define AT91C_TWI0_THR            (0xFFFAC034) // (TWI0) Transmit Holding Register
#define AT91C_TWI0_IDR            (0xFFFAC028) // (TWI0) Interrupt Disable Register
#define AT91C_TWI0_SMR            (0xFFFAC008) // (TWI0) Slave Mode Register
#define AT91C_TWI0_CWGR           (0xFFFAC010) // (TWI0) Clock Waveform Generator Register
#define AT91C_TWI0_IADR           (0xFFFAC00C) // (TWI0) Internal Address Register
#define AT91C_TWI0_RHR            (0xFFFAC030) // (TWI0) Receive Holding Register
#define AT91C_TWI0_IER            (0xFFFAC024) // (TWI0) Interrupt Enable Register
#define AT91C_TWI0_MMR            (0xFFFAC004) // (TWI0) Master Mode Register
#define AT91C_TWI0_SR             (0xFFFAC020) // (TWI0) Status Register
#define AT91C_TWI0_IMR            (0xFFFAC02C) // (TWI0) Interrupt Mask Register
#define AT91C_TWI0_CR             (0xFFFAC000) // (TWI0) Control Register
// ========== Register definition for PDC_TWI1 peripheral ========== 
#define AT91C_TWI1_PTSR           (0xFFFD8124) // (PDC_TWI1) PDC Transfer Status Register
#define AT91C_TWI1_PTCR           (0xFFFD8120) // (PDC_TWI1) PDC Transfer Control Register
#define AT91C_TWI1_TNPR           (0xFFFD8118) // (PDC_TWI1) Transmit Next Pointer Register
#define AT91C_TWI1_TNCR           (0xFFFD811C) // (PDC_TWI1) Transmit Next Counter Register
#define AT91C_TWI1_RNPR           (0xFFFD8110) // (PDC_TWI1) Receive Next Pointer Register
#define AT91C_TWI1_RNCR           (0xFFFD8114) // (PDC_TWI1) Receive Next Counter Register
#define AT91C_TWI1_RPR            (0xFFFD8100) // (PDC_TWI1) Receive Pointer Register
#define AT91C_TWI1_TCR            (0xFFFD810C) // (PDC_TWI1) Transmit Counter Register
#define AT91C_TWI1_TPR            (0xFFFD8108) // (PDC_TWI1) Transmit Pointer Register
#define AT91C_TWI1_RCR            (0xFFFD8104) // (PDC_TWI1) Receive Counter Register
// ========== Register definition for TWI1 peripheral ========== 
#define AT91C_TWI1_RHR            (0xFFFD8030) // (TWI1) Receive Holding Register
#define AT91C_TWI1_IER            (0xFFFD8024) // (TWI1) Interrupt Enable Register
#define AT91C_TWI1_CWGR           (0xFFFD8010) // (TWI1) Clock Waveform Generator Register
#define AT91C_TWI1_MMR            (0xFFFD8004) // (TWI1) Master Mode Register
#define AT91C_TWI1_IADR           (0xFFFD800C) // (TWI1) Internal Address Register
#define AT91C_TWI1_THR            (0xFFFD8034) // (TWI1) Transmit Holding Register
#define AT91C_TWI1_IMR            (0xFFFD802C) // (TWI1) Interrupt Mask Register
#define AT91C_TWI1_SR             (0xFFFD8020) // (TWI1) Status Register
#define AT91C_TWI1_IDR            (0xFFFD8028) // (TWI1) Interrupt Disable Register
#define AT91C_TWI1_CR             (0xFFFD8000) // (TWI1) Control Register
#define AT91C_TWI1_SMR            (0xFFFD8008) // (TWI1) Slave Mode Register
// ========== Register definition for PDC_US0 peripheral ========== 
#define AT91C_US0_TCR             (0xFFFB010C) // (PDC_US0) Transmit Counter Register
#define AT91C_US0_PTCR            (0xFFFB0120) // (PDC_US0) PDC Transfer Control Register
#define AT91C_US0_RNCR            (0xFFFB0114) // (PDC_US0) Receive Next Counter Register
#define AT91C_US0_PTSR            (0xFFFB0124) // (PDC_US0) PDC Transfer Status Register
#define AT91C_US0_TNCR            (0xFFFB011C) // (PDC_US0) Transmit Next Counter Register
#define AT91C_US0_RNPR            (0xFFFB0110) // (PDC_US0) Receive Next Pointer Register
#define AT91C_US0_RCR             (0xFFFB0104) // (PDC_US0) Receive Counter Register
#define AT91C_US0_TPR             (0xFFFB0108) // (PDC_US0) Transmit Pointer Register
#define AT91C_US0_TNPR            (0xFFFB0118) // (PDC_US0) Transmit Next Pointer Register
#define AT91C_US0_RPR             (0xFFFB0100) // (PDC_US0) Receive Pointer Register
// ========== Register definition for US0 peripheral ========== 
#define AT91C_US0_RHR             (0xFFFB0018) // (US0) Receiver Holding Register
#define AT91C_US0_NER             (0xFFFB0044) // (US0) Nb Errors Register
#define AT91C_US0_IER             (0xFFFB0008) // (US0) Interrupt Enable Register
#define AT91C_US0_CR              (0xFFFB0000) // (US0) Control Register
#define AT91C_US0_THR             (0xFFFB001C) // (US0) Transmitter Holding Register
#define AT91C_US0_CSR             (0xFFFB0014) // (US0) Channel Status Register
#define AT91C_US0_BRGR            (0xFFFB0020) // (US0) Baud Rate Generator Register
#define AT91C_US0_RTOR            (0xFFFB0024) // (US0) Receiver Time-out Register
#define AT91C_US0_TTGR            (0xFFFB0028) // (US0) Transmitter Time-guard Register
#define AT91C_US0_IDR             (0xFFFB000C) // (US0) Interrupt Disable Register
#define AT91C_US0_MR              (0xFFFB0004) // (US0) Mode Register
#define AT91C_US0_IF              (0xFFFB004C) // (US0) IRDA_FILTER Register
#define AT91C_US0_FIDI            (0xFFFB0040) // (US0) FI_DI_Ratio Register
#define AT91C_US0_IMR             (0xFFFB0010) // (US0) Interrupt Mask Register
// ========== Register definition for PDC_US1 peripheral ========== 
#define AT91C_US1_PTCR            (0xFFFB4120) // (PDC_US1) PDC Transfer Control Register
#define AT91C_US1_RCR             (0xFFFB4104) // (PDC_US1) Receive Counter Register
#define AT91C_US1_RPR             (0xFFFB4100) // (PDC_US1) Receive Pointer Register
#define AT91C_US1_PTSR            (0xFFFB4124) // (PDC_US1) PDC Transfer Status Register
#define AT91C_US1_TPR             (0xFFFB4108) // (PDC_US1) Transmit Pointer Register
#define AT91C_US1_TCR             (0xFFFB410C) // (PDC_US1) Transmit Counter Register
#define AT91C_US1_RNPR            (0xFFFB4110) // (PDC_US1) Receive Next Pointer Register
#define AT91C_US1_TNCR            (0xFFFB411C) // (PDC_US1) Transmit Next Counter Register
#define AT91C_US1_RNCR            (0xFFFB4114) // (PDC_US1) Receive Next Counter Register
#define AT91C_US1_TNPR            (0xFFFB4118) // (PDC_US1) Transmit Next Pointer Register
// ========== Register definition for US1 peripheral ========== 
#define AT91C_US1_THR             (0xFFFB401C) // (US1) Transmitter Holding Register
#define AT91C_US1_TTGR            (0xFFFB4028) // (US1) Transmitter Time-guard Register
#define AT91C_US1_BRGR            (0xFFFB4020) // (US1) Baud Rate Generator Register
#define AT91C_US1_IDR             (0xFFFB400C) // (US1) Interrupt Disable Register
#define AT91C_US1_MR              (0xFFFB4004) // (US1) Mode Register
#define AT91C_US1_RTOR            (0xFFFB4024) // (US1) Receiver Time-out Register
#define AT91C_US1_CR              (0xFFFB4000) // (US1) Control Register
#define AT91C_US1_IMR             (0xFFFB4010) // (US1) Interrupt Mask Register
#define AT91C_US1_FIDI            (0xFFFB4040) // (US1) FI_DI_Ratio Register
#define AT91C_US1_RHR             (0xFFFB4018) // (US1) Receiver Holding Register
#define AT91C_US1_IER             (0xFFFB4008) // (US1) Interrupt Enable Register
#define AT91C_US1_CSR             (0xFFFB4014) // (US1) Channel Status Register
#define AT91C_US1_IF              (0xFFFB404C) // (US1) IRDA_FILTER Register
#define AT91C_US1_NER             (0xFFFB4044) // (US1) Nb Errors Register
// ========== Register definition for PDC_US2 peripheral ========== 
#define AT91C_US2_TNCR            (0xFFFB811C) // (PDC_US2) Transmit Next Counter Register
#define AT91C_US2_RNCR            (0xFFFB8114) // (PDC_US2) Receive Next Counter Register
#define AT91C_US2_TNPR            (0xFFFB8118) // (PDC_US2) Transmit Next Pointer Register
#define AT91C_US2_PTCR            (0xFFFB8120) // (PDC_US2) PDC Transfer Control Register
#define AT91C_US2_TCR             (0xFFFB810C) // (PDC_US2) Transmit Counter Register
#define AT91C_US2_RPR             (0xFFFB8100) // (PDC_US2) Receive Pointer Register
#define AT91C_US2_TPR             (0xFFFB8108) // (PDC_US2) Transmit Pointer Register
#define AT91C_US2_RCR             (0xFFFB8104) // (PDC_US2) Receive Counter Register
#define AT91C_US2_PTSR            (0xFFFB8124) // (PDC_US2) PDC Transfer Status Register
#define AT91C_US2_RNPR            (0xFFFB8110) // (PDC_US2) Receive Next Pointer Register
// ========== Register definition for US2 peripheral ========== 
#define AT91C_US2_RTOR            (0xFFFB8024) // (US2) Receiver Time-out Register
#define AT91C_US2_CSR             (0xFFFB8014) // (US2) Channel Status Register
#define AT91C_US2_CR              (0xFFFB8000) // (US2) Control Register
#define AT91C_US2_BRGR            (0xFFFB8020) // (US2) Baud Rate Generator Register
#define AT91C_US2_NER             (0xFFFB8044) // (US2) Nb Errors Register
#define AT91C_US2_FIDI            (0xFFFB8040) // (US2) FI_DI_Ratio Register
#define AT91C_US2_TTGR            (0xFFFB8028) // (US2) Transmitter Time-guard Register
#define AT91C_US2_RHR             (0xFFFB8018) // (US2) Receiver Holding Register
#define AT91C_US2_IDR             (0xFFFB800C) // (US2) Interrupt Disable Register
#define AT91C_US2_THR             (0xFFFB801C) // (US2) Transmitter Holding Register
#define AT91C_US2_MR              (0xFFFB8004) // (US2) Mode Register
#define AT91C_US2_IMR             (0xFFFB8010) // (US2) Interrupt Mask Register
#define AT91C_US2_IF              (0xFFFB804C) // (US2) IRDA_FILTER Register
#define AT91C_US2_IER             (0xFFFB8008) // (US2) Interrupt Enable Register
// ========== Register definition for PDC_US3 peripheral ========== 
#define AT91C_US3_RNPR            (0xFFFD0110) // (PDC_US3) Receive Next Pointer Register
#define AT91C_US3_RNCR            (0xFFFD0114) // (PDC_US3) Receive Next Counter Register
#define AT91C_US3_PTSR            (0xFFFD0124) // (PDC_US3) PDC Transfer Status Register
#define AT91C_US3_PTCR            (0xFFFD0120) // (PDC_US3) PDC Transfer Control Register
#define AT91C_US3_TCR             (0xFFFD010C) // (PDC_US3) Transmit Counter Register
#define AT91C_US3_TNPR            (0xFFFD0118) // (PDC_US3) Transmit Next Pointer Register
#define AT91C_US3_RCR             (0xFFFD0104) // (PDC_US3) Receive Counter Register
#define AT91C_US3_TPR             (0xFFFD0108) // (PDC_US3) Transmit Pointer Register
#define AT91C_US3_TNCR            (0xFFFD011C) // (PDC_US3) Transmit Next Counter Register
#define AT91C_US3_RPR             (0xFFFD0100) // (PDC_US3) Receive Pointer Register
// ========== Register definition for US3 peripheral ========== 
#define AT91C_US3_NER             (0xFFFD0044) // (US3) Nb Errors Register
#define AT91C_US3_RTOR            (0xFFFD0024) // (US3) Receiver Time-out Register
#define AT91C_US3_IDR             (0xFFFD000C) // (US3) Interrupt Disable Register
#define AT91C_US3_MR              (0xFFFD0004) // (US3) Mode Register
#define AT91C_US3_FIDI            (0xFFFD0040) // (US3) FI_DI_Ratio Register
#define AT91C_US3_BRGR            (0xFFFD0020) // (US3) Baud Rate Generator Register
#define AT91C_US3_THR             (0xFFFD001C) // (US3) Transmitter Holding Register
#define AT91C_US3_CR              (0xFFFD0000) // (US3) Control Register
#define AT91C_US3_IF              (0xFFFD004C) // (US3) IRDA_FILTER Register
#define AT91C_US3_IER             (0xFFFD0008) // (US3) Interrupt Enable Register
#define AT91C_US3_TTGR            (0xFFFD0028) // (US3) Transmitter Time-guard Register
#define AT91C_US3_RHR             (0xFFFD0018) // (US3) Receiver Holding Register
#define AT91C_US3_IMR             (0xFFFD0010) // (US3) Interrupt Mask Register
#define AT91C_US3_CSR             (0xFFFD0014) // (US3) Channel Status Register
// ========== Register definition for PDC_US4 peripheral ========== 
#define AT91C_US4_TNCR            (0xFFFD411C) // (PDC_US4) Transmit Next Counter Register
#define AT91C_US4_RPR             (0xFFFD4100) // (PDC_US4) Receive Pointer Register
#define AT91C_US4_RNCR            (0xFFFD4114) // (PDC_US4) Receive Next Counter Register
#define AT91C_US4_TPR             (0xFFFD4108) // (PDC_US4) Transmit Pointer Register
#define AT91C_US4_PTCR            (0xFFFD4120) // (PDC_US4) PDC Transfer Control Register
#define AT91C_US4_TCR             (0xFFFD410C) // (PDC_US4) Transmit Counter Register
#define AT91C_US4_RCR             (0xFFFD4104) // (PDC_US4) Receive Counter Register
#define AT91C_US4_RNPR            (0xFFFD4110) // (PDC_US4) Receive Next Pointer Register
#define AT91C_US4_TNPR            (0xFFFD4118) // (PDC_US4) Transmit Next Pointer Register
#define AT91C_US4_PTSR            (0xFFFD4124) // (PDC_US4) PDC Transfer Status Register
// ========== Register definition for US4 peripheral ========== 
#define AT91C_US4_BRGR            (0xFFFD4020) // (US4) Baud Rate Generator Register
#define AT91C_US4_THR             (0xFFFD401C) // (US4) Transmitter Holding Register
#define AT91C_US4_RTOR            (0xFFFD4024) // (US4) Receiver Time-out Register
#define AT91C_US4_IMR             (0xFFFD4010) // (US4) Interrupt Mask Register
#define AT91C_US4_NER             (0xFFFD4044) // (US4) Nb Errors Register
#define AT91C_US4_TTGR            (0xFFFD4028) // (US4) Transmitter Time-guard Register
#define AT91C_US4_FIDI            (0xFFFD4040) // (US4) FI_DI_Ratio Register
#define AT91C_US4_MR              (0xFFFD4004) // (US4) Mode Register
#define AT91C_US4_IER             (0xFFFD4008) // (US4) Interrupt Enable Register
#define AT91C_US4_RHR             (0xFFFD4018) // (US4) Receiver Holding Register
#define AT91C_US4_CR              (0xFFFD4000) // (US4) Control Register
#define AT91C_US4_IF              (0xFFFD404C) // (US4) IRDA_FILTER Register
#define AT91C_US4_IDR             (0xFFFD400C) // (US4) Interrupt Disable Register
#define AT91C_US4_CSR             (0xFFFD4014) // (US4) Channel Status Register
// ========== Register definition for PDC_SSC0 peripheral ========== 
#define AT91C_SSC0_TNPR           (0xFFFBC118) // (PDC_SSC0) Transmit Next Pointer Register
#define AT91C_SSC0_TCR            (0xFFFBC10C) // (PDC_SSC0) Transmit Counter Register
#define AT91C_SSC0_RNCR           (0xFFFBC114) // (PDC_SSC0) Receive Next Counter Register
#define AT91C_SSC0_RPR            (0xFFFBC100) // (PDC_SSC0) Receive Pointer Register
#define AT91C_SSC0_TPR            (0xFFFBC108) // (PDC_SSC0) Transmit Pointer Register
#define AT91C_SSC0_RCR            (0xFFFBC104) // (PDC_SSC0) Receive Counter Register
#define AT91C_SSC0_RNPR           (0xFFFBC110) // (PDC_SSC0) Receive Next Pointer Register
#define AT91C_SSC0_PTCR           (0xFFFBC120) // (PDC_SSC0) PDC Transfer Control Register
#define AT91C_SSC0_TNCR           (0xFFFBC11C) // (PDC_SSC0) Transmit Next Counter Register
#define AT91C_SSC0_PTSR           (0xFFFBC124) // (PDC_SSC0) PDC Transfer Status Register
// ========== Register definition for SSC0 peripheral ========== 
#define AT91C_SSC0_IMR            (0xFFFBC04C) // (SSC0) Interrupt Mask Register
#define AT91C_SSC0_RFMR           (0xFFFBC014) // (SSC0) Receive Frame Mode Register
#define AT91C_SSC0_CR             (0xFFFBC000) // (SSC0) Control Register
#define AT91C_SSC0_TFMR           (0xFFFBC01C) // (SSC0) Transmit Frame Mode Register
#define AT91C_SSC0_CMR            (0xFFFBC004) // (SSC0) Clock Mode Register
#define AT91C_SSC0_IER            (0xFFFBC044) // (SSC0) Interrupt Enable Register
#define AT91C_SSC0_RHR            (0xFFFBC020) // (SSC0) Receive Holding Register
#define AT91C_SSC0_RCMR           (0xFFFBC010) // (SSC0) Receive Clock ModeRegister
#define AT91C_SSC0_SR             (0xFFFBC040) // (SSC0) Status Register
#define AT91C_SSC0_RSHR           (0xFFFBC030) // (SSC0) Receive Sync Holding Register
#define AT91C_SSC0_THR            (0xFFFBC024) // (SSC0) Transmit Holding Register
#define AT91C_SSC0_TCMR           (0xFFFBC018) // (SSC0) Transmit Clock Mode Register
#define AT91C_SSC0_IDR            (0xFFFBC048) // (SSC0) Interrupt Disable Register
#define AT91C_SSC0_TSHR           (0xFFFBC034) // (SSC0) Transmit Sync Holding Register
// ========== Register definition for PDC_SPI0 peripheral ========== 
#define AT91C_SPI0_PTCR           (0xFFFC8120) // (PDC_SPI0) PDC Transfer Control Register
#define AT91C_SPI0_TCR            (0xFFFC810C) // (PDC_SPI0) Transmit Counter Register
#define AT91C_SPI0_RPR            (0xFFFC8100) // (PDC_SPI0) Receive Pointer Register
#define AT91C_SPI0_TPR            (0xFFFC8108) // (PDC_SPI0) Transmit Pointer Register
#define AT91C_SPI0_PTSR           (0xFFFC8124) // (PDC_SPI0) PDC Transfer Status Register
#define AT91C_SPI0_RNCR           (0xFFFC8114) // (PDC_SPI0) Receive Next Counter Register
#define AT91C_SPI0_TNPR           (0xFFFC8118) // (PDC_SPI0) Transmit Next Pointer Register
#define AT91C_SPI0_RCR            (0xFFFC8104) // (PDC_SPI0) Receive Counter Register
#define AT91C_SPI0_RNPR           (0xFFFC8110) // (PDC_SPI0) Receive Next Pointer Register
#define AT91C_SPI0_TNCR           (0xFFFC811C) // (PDC_SPI0) Transmit Next Counter Register
// ========== Register definition for SPI0 peripheral ========== 
#define AT91C_SPI0_IDR            (0xFFFC8018) // (SPI0) Interrupt Disable Register
#define AT91C_SPI0_TDR            (0xFFFC800C) // (SPI0) Transmit Data Register
#define AT91C_SPI0_SR             (0xFFFC8010) // (SPI0) Status Register
#define AT91C_SPI0_CR             (0xFFFC8000) // (SPI0) Control Register
#define AT91C_SPI0_CSR            (0xFFFC8030) // (SPI0) Chip Select Register
#define AT91C_SPI0_RDR            (0xFFFC8008) // (SPI0) Receive Data Register
#define AT91C_SPI0_MR             (0xFFFC8004) // (SPI0) Mode Register
#define AT91C_SPI0_IER            (0xFFFC8014) // (SPI0) Interrupt Enable Register
#define AT91C_SPI0_IMR            (0xFFFC801C) // (SPI0) Interrupt Mask Register
// ========== Register definition for PDC_SPI1 peripheral ========== 
#define AT91C_SPI1_PTCR           (0xFFFCC120) // (PDC_SPI1) PDC Transfer Control Register
#define AT91C_SPI1_RNPR           (0xFFFCC110) // (PDC_SPI1) Receive Next Pointer Register
#define AT91C_SPI1_RCR            (0xFFFCC104) // (PDC_SPI1) Receive Counter Register
#define AT91C_SPI1_TPR            (0xFFFCC108) // (PDC_SPI1) Transmit Pointer Register
#define AT91C_SPI1_PTSR           (0xFFFCC124) // (PDC_SPI1) PDC Transfer Status Register
#define AT91C_SPI1_TNCR           (0xFFFCC11C) // (PDC_SPI1) Transmit Next Counter Register
#define AT91C_SPI1_RPR            (0xFFFCC100) // (PDC_SPI1) Receive Pointer Register
#define AT91C_SPI1_TCR            (0xFFFCC10C) // (PDC_SPI1) Transmit Counter Register
#define AT91C_SPI1_RNCR           (0xFFFCC114) // (PDC_SPI1) Receive Next Counter Register
#define AT91C_SPI1_TNPR           (0xFFFCC118) // (PDC_SPI1) Transmit Next Pointer Register
// ========== Register definition for SPI1 peripheral ========== 
#define AT91C_SPI1_IER            (0xFFFCC014) // (SPI1) Interrupt Enable Register
#define AT91C_SPI1_RDR            (0xFFFCC008) // (SPI1) Receive Data Register
#define AT91C_SPI1_SR             (0xFFFCC010) // (SPI1) Status Register
#define AT91C_SPI1_IMR            (0xFFFCC01C) // (SPI1) Interrupt Mask Register
#define AT91C_SPI1_TDR            (0xFFFCC00C) // (SPI1) Transmit Data Register
#define AT91C_SPI1_IDR            (0xFFFCC018) // (SPI1) Interrupt Disable Register
#define AT91C_SPI1_CSR            (0xFFFCC030) // (SPI1) Chip Select Register
#define AT91C_SPI1_CR             (0xFFFCC000) // (SPI1) Control Register
#define AT91C_SPI1_MR             (0xFFFCC004) // (SPI1) Mode Register
// ========== Register definition for PDC_ADC peripheral ========== 
#define AT91C_ADC_PTCR            (0xFFFE0120) // (PDC_ADC) PDC Transfer Control Register
#define AT91C_ADC_TPR             (0xFFFE0108) // (PDC_ADC) Transmit Pointer Register
#define AT91C_ADC_TCR             (0xFFFE010C) // (PDC_ADC) Transmit Counter Register
#define AT91C_ADC_RCR             (0xFFFE0104) // (PDC_ADC) Receive Counter Register
#define AT91C_ADC_PTSR            (0xFFFE0124) // (PDC_ADC) PDC Transfer Status Register
#define AT91C_ADC_RNPR            (0xFFFE0110) // (PDC_ADC) Receive Next Pointer Register
#define AT91C_ADC_RPR             (0xFFFE0100) // (PDC_ADC) Receive Pointer Register
#define AT91C_ADC_TNCR            (0xFFFE011C) // (PDC_ADC) Transmit Next Counter Register
#define AT91C_ADC_RNCR            (0xFFFE0114) // (PDC_ADC) Receive Next Counter Register
#define AT91C_ADC_TNPR            (0xFFFE0118) // (PDC_ADC) Transmit Next Pointer Register
// ========== Register definition for ADC peripheral ========== 
#define AT91C_ADC_CHDR            (0xFFFE0014) // (ADC) ADC Channel Disable Register
#define AT91C_ADC_CDR3            (0xFFFE003C) // (ADC) ADC Channel Data Register 3
#define AT91C_ADC_CR              (0xFFFE0000) // (ADC) ADC Control Register
#define AT91C_ADC_IMR             (0xFFFE002C) // (ADC) ADC Interrupt Mask Register
#define AT91C_ADC_CDR2            (0xFFFE0038) // (ADC) ADC Channel Data Register 2
#define AT91C_ADC_SR              (0xFFFE001C) // (ADC) ADC Status Register
#define AT91C_ADC_IER             (0xFFFE0024) // (ADC) ADC Interrupt Enable Register
#define AT91C_ADC_CDR7            (0xFFFE004C) // (ADC) ADC Channel Data Register 7
#define AT91C_ADC_CDR0            (0xFFFE0030) // (ADC) ADC Channel Data Register 0
#define AT91C_ADC_CDR5            (0xFFFE0044) // (ADC) ADC Channel Data Register 5
#define AT91C_ADC_CDR4            (0xFFFE0040) // (ADC) ADC Channel Data Register 4
#define AT91C_ADC_CHER            (0xFFFE0010) // (ADC) ADC Channel Enable Register
#define AT91C_ADC_CHSR            (0xFFFE0018) // (ADC) ADC Channel Status Register
#define AT91C_ADC_MR              (0xFFFE0004) // (ADC) ADC Mode Register
#define AT91C_ADC_CDR6            (0xFFFE0048) // (ADC) ADC Channel Data Register 6
#define AT91C_ADC_LCDR            (0xFFFE0020) // (ADC) ADC Last Converted Data Register
#define AT91C_ADC_CDR1            (0xFFFE0034) // (ADC) ADC Channel Data Register 1
#define AT91C_ADC_IDR             (0xFFFE0028) // (ADC) ADC Interrupt Disable Register
// ========== Register definition for EMACB peripheral ========== 
#define AT91C_EMACB_USRIO         (0xFFFC40C0) // (EMACB) USER Input/Output Register
#define AT91C_EMACB_RSE           (0xFFFC4074) // (EMACB) Receive Symbol Errors Register
#define AT91C_EMACB_SCF           (0xFFFC4044) // (EMACB) Single Collision Frame Register
#define AT91C_EMACB_STE           (0xFFFC4084) // (EMACB) SQE Test Error Register
#define AT91C_EMACB_SA1H          (0xFFFC409C) // (EMACB) Specific Address 1 Top, Last 2 bytes
#define AT91C_EMACB_ROV           (0xFFFC4070) // (EMACB) Receive Overrun Errors Register
#define AT91C_EMACB_TBQP          (0xFFFC401C) // (EMACB) Transmit Buffer Queue Pointer
#define AT91C_EMACB_IMR           (0xFFFC4030) // (EMACB) Interrupt Mask Register
#define AT91C_EMACB_IER           (0xFFFC4028) // (EMACB) Interrupt Enable Register
#define AT91C_EMACB_REV           (0xFFFC40FC) // (EMACB) Revision Register
#define AT91C_EMACB_SA3L          (0xFFFC40A8) // (EMACB) Specific Address 3 Bottom, First 4 bytes
#define AT91C_EMACB_ELE           (0xFFFC4078) // (EMACB) Excessive Length Errors Register
#define AT91C_EMACB_HRT           (0xFFFC4094) // (EMACB) Hash Address Top[63:32]
#define AT91C_EMACB_SA2L          (0xFFFC40A0) // (EMACB) Specific Address 2 Bottom, First 4 bytes
#define AT91C_EMACB_RRE           (0xFFFC406C) // (EMACB) Receive Ressource Error Register
#define AT91C_EMACB_FRO           (0xFFFC404C) // (EMACB) Frames Received OK Register
#define AT91C_EMACB_TPQ           (0xFFFC40BC) // (EMACB) Transmit Pause Quantum Register
#define AT91C_EMACB_ISR           (0xFFFC4024) // (EMACB) Interrupt Status Register
#define AT91C_EMACB_TSR           (0xFFFC4014) // (EMACB) Transmit Status Register
#define AT91C_EMACB_RLE           (0xFFFC4088) // (EMACB) Receive Length Field Mismatch Register
#define AT91C_EMACB_USF           (0xFFFC4080) // (EMACB) Undersize Frames Register
#define AT91C_EMACB_WOL           (0xFFFC40C4) // (EMACB) Wake On LAN Register
#define AT91C_EMACB_TPF           (0xFFFC408C) // (EMACB) Transmitted Pause Frames Register
#define AT91C_EMACB_PTR           (0xFFFC4038) // (EMACB) Pause Time Register
#define AT91C_EMACB_TUND          (0xFFFC4064) // (EMACB) Transmit Underrun Error Register
#define AT91C_EMACB_MAN           (0xFFFC4034) // (EMACB) PHY Maintenance Register
#define AT91C_EMACB_RJA           (0xFFFC407C) // (EMACB) Receive Jabbers Register
#define AT91C_EMACB_SA4L          (0xFFFC40B0) // (EMACB) Specific Address 4 Bottom, First 4 bytes
#define AT91C_EMACB_CSE           (0xFFFC4068) // (EMACB) Carrier Sense Error Register
#define AT91C_EMACB_HRB           (0xFFFC4090) // (EMACB) Hash Address Bottom[31:0]
#define AT91C_EMACB_ALE           (0xFFFC4054) // (EMACB) Alignment Error Register
#define AT91C_EMACB_SA1L          (0xFFFC4098) // (EMACB) Specific Address 1 Bottom, First 4 bytes
#define AT91C_EMACB_NCR           (0xFFFC4000) // (EMACB) Network Control Register
#define AT91C_EMACB_FTO           (0xFFFC4040) // (EMACB) Frames Transmitted OK Register
#define AT91C_EMACB_ECOL          (0xFFFC4060) // (EMACB) Excessive Collision Register
#define AT91C_EMACB_DTF           (0xFFFC4058) // (EMACB) Deferred Transmission Frame Register
#define AT91C_EMACB_SA4H          (0xFFFC40B4) // (EMACB) Specific Address 4 Top, Last 2 bytes
#define AT91C_EMACB_FCSE          (0xFFFC4050) // (EMACB) Frame Check Sequence Error Register
#define AT91C_EMACB_TID           (0xFFFC40B8) // (EMACB) Type ID Checking Register
#define AT91C_EMACB_PFR           (0xFFFC403C) // (EMACB) Pause Frames received Register
#define AT91C_EMACB_IDR           (0xFFFC402C) // (EMACB) Interrupt Disable Register
#define AT91C_EMACB_SA3H          (0xFFFC40AC) // (EMACB) Specific Address 3 Top, Last 2 bytes
#define AT91C_EMACB_NSR           (0xFFFC4008) // (EMACB) Network Status Register
#define AT91C_EMACB_MCF           (0xFFFC4048) // (EMACB) Multiple Collision Frame Register
#define AT91C_EMACB_RBQP          (0xFFFC4018) // (EMACB) Receive Buffer Queue Pointer
#define AT91C_EMACB_RSR           (0xFFFC4020) // (EMACB) Receive Status Register
#define AT91C_EMACB_SA2H          (0xFFFC40A4) // (EMACB) Specific Address 2 Top, Last 2 bytes
#define AT91C_EMACB_NCFGR         (0xFFFC4004) // (EMACB) Network Configuration Register
#define AT91C_EMACB_LCOL          (0xFFFC405C) // (EMACB) Late Collision Register
// ========== Register definition for UDP peripheral ========== 
#define AT91C_UDP_GLBSTATE        (0xFFFA4004) // (UDP) Global State Register
#define AT91C_UDP_FDR             (0xFFFA4050) // (UDP) Endpoint FIFO Data Register
#define AT91C_UDP_RSTEP           (0xFFFA4028) // (UDP) Reset Endpoint Register
#define AT91C_UDP_FADDR           (0xFFFA4008) // (UDP) Function Address Register
#define AT91C_UDP_NUM             (0xFFFA4000) // (UDP) Frame Number Register
#define AT91C_UDP_IDR             (0xFFFA4014) // (UDP) Interrupt Disable Register
#define AT91C_UDP_IMR             (0xFFFA4018) // (UDP) Interrupt Mask Register
#define AT91C_UDP_CSR             (0xFFFA4030) // (UDP) Endpoint Control and Status Register
#define AT91C_UDP_IER             (0xFFFA4010) // (UDP) Interrupt Enable Register
#define AT91C_UDP_ICR             (0xFFFA4020) // (UDP) Interrupt Clear Register
#define AT91C_UDP_TXVC            (0xFFFA4074) // (UDP) Transceiver Control Register
#define AT91C_UDP_ISR             (0xFFFA401C) // (UDP) Interrupt Status Register
// ========== Register definition for UHP peripheral ========== 
#define AT91C_UHP_HcInterruptStatus (0x0050000C) // (UHP) Interrupt Status Register
#define AT91C_UHP_HcCommandStatus (0x00500008) // (UHP) Command & status Register
#define AT91C_UHP_HcRhStatus      (0x00500050) // (UHP) Root Hub Status register
#define AT91C_UHP_HcInterruptDisable (0x00500014) // (UHP) Interrupt Disable Register
#define AT91C_UHP_HcPeriodicStart (0x00500040) // (UHP) Periodic Start
#define AT91C_UHP_HcControlCurrentED (0x00500024) // (UHP) Endpoint Control and Status Register
#define AT91C_UHP_HcPeriodCurrentED (0x0050001C) // (UHP) Current Isochronous or Interrupt Endpoint Descriptor
#define AT91C_UHP_HcBulkHeadED    (0x00500028) // (UHP) First endpoint register of the Bulk list
#define AT91C_UHP_HcRevision      (0x00500000) // (UHP) Revision
#define AT91C_UHP_HcBulkCurrentED (0x0050002C) // (UHP) Current endpoint of the Bulk list
#define AT91C_UHP_HcRhDescriptorB (0x0050004C) // (UHP) Root Hub characteristics B
#define AT91C_UHP_HcControlHeadED (0x00500020) // (UHP) First Endpoint Descriptor of the Control list
#define AT91C_UHP_HcFmRemaining   (0x00500038) // (UHP) Bit time remaining in the current Frame
#define AT91C_UHP_HcHCCA          (0x00500018) // (UHP) Pointer to the Host Controller Communication Area
#define AT91C_UHP_HcLSThreshold   (0x00500044) // (UHP) LS Threshold
#define AT91C_UHP_HcRhPortStatus  (0x00500054) // (UHP) Root Hub Port Status Register
#define AT91C_UHP_HcInterruptEnable (0x00500010) // (UHP) Interrupt Enable Register
#define AT91C_UHP_HcFmNumber      (0x0050003C) // (UHP) Frame number
#define AT91C_UHP_HcFmInterval    (0x00500034) // (UHP) Bit time between 2 consecutive SOFs
#define AT91C_UHP_HcControl       (0x00500004) // (UHP) Operating modes for the Host Controller
#define AT91C_UHP_HcBulkDoneHead  (0x00500030) // (UHP) Last completed transfer descriptor
#define AT91C_UHP_HcRhDescriptorA (0x00500048) // (UHP) Root Hub characteristics A
// ========== Register definition for HECC peripheral ========== 
// ========== Register definition for HISI peripheral ========== 
#define AT91C_HISI_PSIZE          (0xFFFC0020) // (HISI) Preview Size Register
#define AT91C_HISI_CR1            (0xFFFC0000) // (HISI) Control Register 1
#define AT91C_HISI_R2YSET1        (0xFFFC003C) // (HISI) Color Space Conversion Register
#define AT91C_HISI_CDBA           (0xFFFC002C) // (HISI) Codec Dma Address Register
#define AT91C_HISI_IDR            (0xFFFC0010) // (HISI) Interrupt Disable Register
#define AT91C_HISI_R2YSET2        (0xFFFC0040) // (HISI) Color Space Conversion Register
#define AT91C_HISI_Y2RSET1        (0xFFFC0034) // (HISI) Color Space Conversion Register
#define AT91C_HISI_PFBD           (0xFFFC0028) // (HISI) Preview Frame Buffer Address Register
#define AT91C_HISI_CR2            (0xFFFC0004) // (HISI) Control Register 2
#define AT91C_HISI_Y2RSET0        (0xFFFC0030) // (HISI) Color Space Conversion Register
#define AT91C_HISI_PDECF          (0xFFFC0024) // (HISI) Preview Decimation Factor Register
#define AT91C_HISI_IMR            (0xFFFC0014) // (HISI) Interrupt Mask Register
#define AT91C_HISI_IER            (0xFFFC000C) // (HISI) Interrupt Enable Register
#define AT91C_HISI_R2YSET0        (0xFFFC0038) // (HISI) Color Space Conversion Register
#define AT91C_HISI_SR             (0xFFFC0008) // (HISI) Status Register

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91SAM9XE128
// *****************************************************************************
#define AT91C_PIO_PA0             (1 <<  0) // Pin Controlled by PA0
#define AT91C_PA0_SPI0_MISO       (AT91C_PIO_PA0) //  SPI 0 Master In Slave
#define AT91C_PA0_MCDB0           (AT91C_PIO_PA0) //  Multimedia Card B Data 0
#define AT91C_PIO_PA1             (1 <<  1) // Pin Controlled by PA1
#define AT91C_PA1_SPI0_MOSI       (AT91C_PIO_PA1) //  SPI 0 Master Out Slave
#define AT91C_PA1_MCCDB           (AT91C_PIO_PA1) //  Multimedia Card B Command
#define AT91C_PIO_PA10            (1 << 10) // Pin Controlled by PA10
#define AT91C_PA10_MCDA2          (AT91C_PIO_PA10) //  Multimedia Card A Data 2
#define AT91C_PA10_ETX2_0         (AT91C_PIO_PA10) //  Ethernet MAC Transmit Data 2
#define AT91C_PIO_PA11            (1 << 11) // Pin Controlled by PA11
#define AT91C_PA11_MCDA3          (AT91C_PIO_PA11) //  Multimedia Card A Data 3
#define AT91C_PA11_ETX3_0         (AT91C_PIO_PA11) //  Ethernet MAC Transmit Data 3
#define AT91C_PIO_PA12            (1 << 12) // Pin Controlled by PA12
#define AT91C_PA12_ETX0           (AT91C_PIO_PA12) //  Ethernet MAC Transmit Data 0
#define AT91C_PIO_PA13            (1 << 13) // Pin Controlled by PA13
#define AT91C_PA13_ETX1           (AT91C_PIO_PA13) //  Ethernet MAC Transmit Data 1
#define AT91C_PIO_PA14            (1 << 14) // Pin Controlled by PA14
#define AT91C_PA14_ERX0           (AT91C_PIO_PA14) //  Ethernet MAC Receive Data 0
#define AT91C_PIO_PA15            (1 << 15) // Pin Controlled by PA15
#define AT91C_PA15_ERX1           (AT91C_PIO_PA15) //  Ethernet MAC Receive Data 1
#define AT91C_PIO_PA16            (1 << 16) // Pin Controlled by PA16
#define AT91C_PA16_ETXEN          (AT91C_PIO_PA16) //  Ethernet MAC Transmit Enable
#define AT91C_PIO_PA17            (1 << 17) // Pin Controlled by PA17
#define AT91C_PA17_ERXDV          (AT91C_PIO_PA17) //  Ethernet MAC Receive Data Valid
#define AT91C_PIO_PA18            (1 << 18) // Pin Controlled by PA18
#define AT91C_PA18_ERXER          (AT91C_PIO_PA18) //  Ethernet MAC Receive Error
#define AT91C_PIO_PA19            (1 << 19) // Pin Controlled by PA19
#define AT91C_PA19_ETXCK          (AT91C_PIO_PA19) //  Ethernet MAC Transmit Clock/Reference Clock
#define AT91C_PIO_PA2             (1 <<  2) // Pin Controlled by PA2
#define AT91C_PA2_SPI0_SPCK       (AT91C_PIO_PA2) //  SPI 0 Serial Clock
#define AT91C_PIO_PA20            (1 << 20) // Pin Controlled by PA20
#define AT91C_PA20_EMDC           (AT91C_PIO_PA20) //  Ethernet MAC Management Data Clock
#define AT91C_PIO_PA21            (1 << 21) // Pin Controlled by PA21
#define AT91C_PA21_EMDIO          (AT91C_PIO_PA21) //  Ethernet MAC Management Data Input/Output
#define AT91C_PIO_PA22            (1 << 22) // Pin Controlled by PA22
#define AT91C_PA22_ADTRG          (AT91C_PIO_PA22) //  ADC Trigger
#define AT91C_PA22_ETXER          (AT91C_PIO_PA22) //  Ethernet MAC Transmikt Coding Error
#define AT91C_PIO_PA23            (1 << 23) // Pin Controlled by PA23
#define AT91C_PA23_TWD0           (AT91C_PIO_PA23) //  TWI Two-wire Serial Data 0
#define AT91C_PA23_ETX2_1         (AT91C_PIO_PA23) //  Ethernet MAC Transmit Data 2
#define AT91C_PIO_PA24            (1 << 24) // Pin Controlled by PA24
#define AT91C_PA24_TWCK0          (AT91C_PIO_PA24) //  TWI Two-wire Serial Clock 0
#define AT91C_PA24_ETX3_1         (AT91C_PIO_PA24) //  Ethernet MAC Transmit Data 3
#define AT91C_PIO_PA25            (1 << 25) // Pin Controlled by PA25
#define AT91C_PA25_TCLK0          (AT91C_PIO_PA25) //  Timer Counter 0 external clock input
#define AT91C_PA25_ERX2           (AT91C_PIO_PA25) //  Ethernet MAC Receive Data 2
#define AT91C_PIO_PA26            (1 << 26) // Pin Controlled by PA26
#define AT91C_PA26_TIOA0          (AT91C_PIO_PA26) //  Timer Counter 0 Multipurpose Timer I/O Pin A
#define AT91C_PA26_ERX3           (AT91C_PIO_PA26) //  Ethernet MAC Receive Data 3
#define AT91C_PIO_PA27            (1 << 27) // Pin Controlled by PA27
#define AT91C_PA27_TIOA1          (AT91C_PIO_PA27) //  Timer Counter 1 Multipurpose Timer I/O Pin A
#define AT91C_PA27_ERXCK          (AT91C_PIO_PA27) //  Ethernet MAC Receive Clock
#define AT91C_PIO_PA28            (1 << 28) // Pin Controlled by PA28
#define AT91C_PA28_TIOA2          (AT91C_PIO_PA28) //  Timer Counter 2 Multipurpose Timer I/O Pin A
#define AT91C_PA28_ECRS           (AT91C_PIO_PA28) //  Ethernet MAC Carrier Sense/Carrier Sense and Data Valid
#define AT91C_PIO_PA29            (1 << 29) // Pin Controlled by PA29
#define AT91C_PA29_SCK1           (AT91C_PIO_PA29) //  USART 1 Serial Clock
#define AT91C_PA29_ECOL           (AT91C_PIO_PA29) //  Ethernet MAC Collision Detected
#define AT91C_PIO_PA3             (1 <<  3) // Pin Controlled by PA3
#define AT91C_PA3_SPI0_NPCS0      (AT91C_PIO_PA3) //  SPI 0 Peripheral Chip Select 0
#define AT91C_PA3_MCDB3           (AT91C_PIO_PA3) //  Multimedia Card B Data 3
#define AT91C_PIO_PA30            (1 << 30) // Pin Controlled by PA30
#define AT91C_PA30_SCK2           (AT91C_PIO_PA30) //  USART 2 Serial Clock
#define AT91C_PA30_RXD4           (AT91C_PIO_PA30) //  USART 4 Receive Data
#define AT91C_PIO_PA31            (1 << 31) // Pin Controlled by PA31
#define AT91C_PA31_SCK0           (AT91C_PIO_PA31) //  USART 0 Serial Clock
#define AT91C_PA31_TXD4           (AT91C_PIO_PA31) //  USART 4 Transmit Data
#define AT91C_PIO_PA4             (1 <<  4) // Pin Controlled by PA4
#define AT91C_PA4_RTS2            (AT91C_PIO_PA4) //  USART 2 Ready To Send
#define AT91C_PA4_MCDB2           (AT91C_PIO_PA4) //  Multimedia Card B Data 2
#define AT91C_PIO_PA5             (1 <<  5) // Pin Controlled by PA5
#define AT91C_PA5_CTS2            (AT91C_PIO_PA5) //  USART 2 Clear To Send
#define AT91C_PA5_MCDB1           (AT91C_PIO_PA5) //  Multimedia Card B Data 1
#define AT91C_PIO_PA6             (1 <<  6) // Pin Controlled by PA6
#define AT91C_PA6_MCDA0           (AT91C_PIO_PA6) //  Multimedia Card A Data 0
#define AT91C_PIO_PA7             (1 <<  7) // Pin Controlled by PA7
#define AT91C_PA7_MCCDA           (AT91C_PIO_PA7) //  Multimedia Card A Command
#define AT91C_PIO_PA8             (1 <<  8) // Pin Controlled by PA8
#define AT91C_PA8_MCCK            (AT91C_PIO_PA8) //  Multimedia Card Clock
#define AT91C_PIO_PA9             (1 <<  9) // Pin Controlled by PA9
#define AT91C_PA9_MCDA1           (AT91C_PIO_PA9) //  Multimedia Card A Data 1
#define AT91C_PIO_PB0             (1 <<  0) // Pin Controlled by PB0
#define AT91C_PB0_SPI1_MISO       (AT91C_PIO_PB0) //  SPI 1 Master In Slave
#define AT91C_PB0_TIOA3           (AT91C_PIO_PB0) //  Timer Counter 3 Multipurpose Timer I/O Pin A
#define AT91C_PIO_PB1             (1 <<  1) // Pin Controlled by PB1
#define AT91C_PB1_SPI1_MOSI       (AT91C_PIO_PB1) //  SPI 1 Master Out Slave
#define AT91C_PB1_TIOB3           (AT91C_PIO_PB1) //  Timer Counter 3 Multipurpose Timer I/O Pin B
#define AT91C_PIO_PB10            (1 << 10) // Pin Controlled by PB10
#define AT91C_PB10_TXD3           (AT91C_PIO_PB10) //  USART 3 Transmit Data
#define AT91C_PB10_ISI_D8         (AT91C_PIO_PB10) //  Image Sensor Data 8
#define AT91C_PIO_PB11            (1 << 11) // Pin Controlled by PB11
#define AT91C_PB11_RXD3           (AT91C_PIO_PB11) //  USART 3 Receive Data
#define AT91C_PB11_ISI_D9         (AT91C_PIO_PB11) //  Image Sensor Data 9
#define AT91C_PIO_PB12            (1 << 12) // Pin Controlled by PB12
#define AT91C_PB12_TWD1           (AT91C_PIO_PB12) //  TWI Two-wire Serial Data 1
#define AT91C_PB12_ISI_D10        (AT91C_PIO_PB12) //  Image Sensor Data 10
#define AT91C_PIO_PB13            (1 << 13) // Pin Controlled by PB13
#define AT91C_PB13_TWCK1          (AT91C_PIO_PB13) //  TWI Two-wire Serial Clock 1
#define AT91C_PB13_ISI_D11        (AT91C_PIO_PB13) //  Image Sensor Data 11
#define AT91C_PIO_PB14            (1 << 14) // Pin Controlled by PB14
#define AT91C_PB14_DRXD           (AT91C_PIO_PB14) //  DBGU Debug Receive Data
#define AT91C_PIO_PB15            (1 << 15) // Pin Controlled by PB15
#define AT91C_PB15_DTXD           (AT91C_PIO_PB15) //  DBGU Debug Transmit Data
#define AT91C_PIO_PB16            (1 << 16) // Pin Controlled by PB16
#define AT91C_PB16_TK0            (AT91C_PIO_PB16) //  SSC0 Transmit Clock
#define AT91C_PB16_TCLK3          (AT91C_PIO_PB16) //  Timer Counter 3 external clock input
#define AT91C_PIO_PB17            (1 << 17) // Pin Controlled by PB17
#define AT91C_PB17_TF0            (AT91C_PIO_PB17) //  SSC0 Transmit Frame Sync
#define AT91C_PB17_TCLK4          (AT91C_PIO_PB17) //  Timer Counter 4 external clock input
#define AT91C_PIO_PB18            (1 << 18) // Pin Controlled by PB18
#define AT91C_PB18_TD0            (AT91C_PIO_PB18) //  SSC0 Transmit data
#define AT91C_PB18_TIOB4          (AT91C_PIO_PB18) //  Timer Counter 4 Multipurpose Timer I/O Pin B
#define AT91C_PIO_PB19            (1 << 19) // Pin Controlled by PB19
#define AT91C_PB19_RD0            (AT91C_PIO_PB19) //  SSC0 Receive Data
#define AT91C_PB19_TIOB5          (AT91C_PIO_PB19) //  Timer Counter 5 Multipurpose Timer I/O Pin B
#define AT91C_PIO_PB2             (1 <<  2) // Pin Controlled by PB2
#define AT91C_PB2_SPI1_SPCK       (AT91C_PIO_PB2) //  SPI 1 Serial Clock
#define AT91C_PB2_TIOA4           (AT91C_PIO_PB2) //  Timer Counter 4 Multipurpose Timer I/O Pin A
#define AT91C_PIO_PB20            (1 << 20) // Pin Controlled by PB20
#define AT91C_PB20_RK0            (AT91C_PIO_PB20) //  SSC0 Receive Clock
#define AT91C_PB20_ISI_D0         (AT91C_PIO_PB20) //  Image Sensor Data 0
#define AT91C_PIO_PB21            (1 << 21) // Pin Controlled by PB21
#define AT91C_PB21_RF0            (AT91C_PIO_PB21) //  SSC0 Receive Frame Sync
#define AT91C_PB21_ISI_D1         (AT91C_PIO_PB21) //  Image Sensor Data 1
#define AT91C_PIO_PB22            (1 << 22) // Pin Controlled by PB22
#define AT91C_PB22_DSR0           (AT91C_PIO_PB22) //  USART 0 Data Set ready
#define AT91C_PB22_ISI_D2         (AT91C_PIO_PB22) //  Image Sensor Data 2
#define AT91C_PIO_PB23            (1 << 23) // Pin Controlled by PB23
#define AT91C_PB23_DCD0           (AT91C_PIO_PB23) //  USART 0 Data Carrier Detect
#define AT91C_PB23_ISI_D3         (AT91C_PIO_PB23) //  Image Sensor Data 3
#define AT91C_PIO_PB24            (1 << 24) // Pin Controlled by PB24
#define AT91C_PB24_DTR0           (AT91C_PIO_PB24) //  USART 0 Data Terminal ready
#define AT91C_PB24_ISI_D4         (AT91C_PIO_PB24) //  Image Sensor Data 4
#define AT91C_PIO_PB25            (1 << 25) // Pin Controlled by PB25
#define AT91C_PB25_RI0            (AT91C_PIO_PB25) //  USART 0 Ring Indicator
#define AT91C_PB25_ISI_D5         (AT91C_PIO_PB25) //  Image Sensor Data 5
#define AT91C_PIO_PB26            (1 << 26) // Pin Controlled by PB26
#define AT91C_PB26_RTS0           (AT91C_PIO_PB26) //  USART 0 Ready To Send
#define AT91C_PB26_ISI_D6         (AT91C_PIO_PB26) //  Image Sensor Data 6
#define AT91C_PIO_PB27            (1 << 27) // Pin Controlled by PB27
#define AT91C_PB27_CTS0           (AT91C_PIO_PB27) //  USART 0 Clear To Send
#define AT91C_PB27_ISI_D7         (AT91C_PIO_PB27) //  Image Sensor Data 7
#define AT91C_PIO_PB28            (1 << 28) // Pin Controlled by PB28
#define AT91C_PB28_RTS1           (AT91C_PIO_PB28) //  USART 1 Ready To Send
#define AT91C_PB28_ISI_PCK        (AT91C_PIO_PB28) //  Image Sensor Data Clock
#define AT91C_PIO_PB29            (1 << 29) // Pin Controlled by PB29
#define AT91C_PB29_CTS1           (AT91C_PIO_PB29) //  USART 1 Clear To Send
#define AT91C_PB29_ISI_VSYNC      (AT91C_PIO_PB29) //  Image Sensor Vertical Synchro
#define AT91C_PIO_PB3             (1 <<  3) // Pin Controlled by PB3
#define AT91C_PB3_SPI1_NPCS0      (AT91C_PIO_PB3) //  SPI 1 Peripheral Chip Select 0
#define AT91C_PB3_TIOA5           (AT91C_PIO_PB3) //  Timer Counter 5 Multipurpose Timer I/O Pin A
#define AT91C_PIO_PB30            (1 << 30) // Pin Controlled by PB30
#define AT91C_PB30_PCK0_0         (AT91C_PIO_PB30) //  PMC Programmable Clock Output 0
#define AT91C_PB30_ISI_HSYNC      (AT91C_PIO_PB30) //  Image Sensor Horizontal Synchro
#define AT91C_PIO_PB31            (1 << 31) // Pin Controlled by PB31
#define AT91C_PB31_PCK1_0         (AT91C_PIO_PB31) //  PMC Programmable Clock Output 1
#define AT91C_PB31_ISI_MCK        (AT91C_PIO_PB31) //  Image Sensor Reference Clock
#define AT91C_PIO_PB4             (1 <<  4) // Pin Controlled by PB4
#define AT91C_PB4_TXD0            (AT91C_PIO_PB4) //  USART 0 Transmit Data
#define AT91C_PIO_PB5             (1 <<  5) // Pin Controlled by PB5
#define AT91C_PB5_RXD0            (AT91C_PIO_PB5) //  USART 0 Receive Data
#define AT91C_PIO_PB6             (1 <<  6) // Pin Controlled by PB6
#define AT91C_PB6_TXD1            (AT91C_PIO_PB6) //  USART 1 Transmit Data
#define AT91C_PB6_TCLK1           (AT91C_PIO_PB6) //  Timer Counter 1 external clock input
#define AT91C_PIO_PB7             (1 <<  7) // Pin Controlled by PB7
#define AT91C_PB7_RXD1            (AT91C_PIO_PB7) //  USART 1 Receive Data
#define AT91C_PB7_TCLK2           (AT91C_PIO_PB7) //  Timer Counter 2 external clock input
#define AT91C_PIO_PB8             (1 <<  8) // Pin Controlled by PB8
#define AT91C_PB8_TXD2            (AT91C_PIO_PB8) //  USART 2 Transmit Data
#define AT91C_PIO_PB9             (1 <<  9) // Pin Controlled by PB9
#define AT91C_PB9_RXD2            (AT91C_PIO_PB9) //  USART 2 Receive Data
#define AT91C_PIO_PC0             (1 <<  0) // Pin Controlled by PC0
#define AT91C_PC0_AD0             (AT91C_PIO_PC0) //  ADC Analog Input 0
#define AT91C_PC0_SCK3            (AT91C_PIO_PC0) //  USART 3 Serial Clock
#define AT91C_PIO_PC1             (1 <<  1) // Pin Controlled by PC1
#define AT91C_PC1_AD1             (AT91C_PIO_PC1) //  ADC Analog Input 1
#define AT91C_PC1_PCK0            (AT91C_PIO_PC1) //  PMC Programmable Clock Output 0
#define AT91C_PIO_PC10            (1 << 10) // Pin Controlled by PC10
#define AT91C_PC10_A25_CFRNW      (AT91C_PIO_PC10) //  Address Bus[25]
#define AT91C_PC10_CTS3           (AT91C_PIO_PC10) //  USART 3 Clear To Send
#define AT91C_PIO_PC11            (1 << 11) // Pin Controlled by PC11
#define AT91C_PC11_NCS2           (AT91C_PIO_PC11) //  Chip Select Line 2
#define AT91C_PC11_SPI0_NPCS1     (AT91C_PIO_PC11) //  SPI 0 Peripheral Chip Select 1
#define AT91C_PIO_PC12            (1 << 12) // Pin Controlled by PC12
#define AT91C_PC12_IRQ0           (AT91C_PIO_PC12) //  External Interrupt 0
#define AT91C_PC12_NCS7           (AT91C_PIO_PC12) //  Chip Select Line 7
#define AT91C_PIO_PC13            (1 << 13) // Pin Controlled by PC13
#define AT91C_PC13_FIQ            (AT91C_PIO_PC13) //  AIC Fast Interrupt Input
#define AT91C_PC13_NCS6           (AT91C_PIO_PC13) //  Chip Select Line 6
#define AT91C_PIO_PC14            (1 << 14) // Pin Controlled by PC14
#define AT91C_PC14_NCS3_NANDCS    (AT91C_PIO_PC14) //  Chip Select Line 3
#define AT91C_PC14_IRQ2           (AT91C_PIO_PC14) //  External Interrupt 2
#define AT91C_PIO_PC15            (1 << 15) // Pin Controlled by PC15
#define AT91C_PC15_NWAIT          (AT91C_PIO_PC15) //  External Wait Signal
#define AT91C_PC15_IRQ1           (AT91C_PIO_PC15) //  External Interrupt 1
#define AT91C_PIO_PC16            (1 << 16) // Pin Controlled by PC16
#define AT91C_PC16_D16            (AT91C_PIO_PC16) //  Data Bus[16]
#define AT91C_PC16_SPI0_NPCS2     (AT91C_PIO_PC16) //  SPI 0 Peripheral Chip Select 2
#define AT91C_PIO_PC17            (1 << 17) // Pin Controlled by PC17
#define AT91C_PC17_D17            (AT91C_PIO_PC17) //  Data Bus[17]
#define AT91C_PC17_SPI0_NPCS3     (AT91C_PIO_PC17) //  SPI 0 Peripheral Chip Select 3
#define AT91C_PIO_PC18            (1 << 18) // Pin Controlled by PC18
#define AT91C_PC18_D18            (AT91C_PIO_PC18) //  Data Bus[18]
#define AT91C_PC18_SPI1_NPCS1_1   (AT91C_PIO_PC18) //  SPI 1 Peripheral Chip Select 1
#define AT91C_PIO_PC19            (1 << 19) // Pin Controlled by PC19
#define AT91C_PC19_D19            (AT91C_PIO_PC19) //  Data Bus[19]
#define AT91C_PC19_SPI1_NPCS2_1   (AT91C_PIO_PC19) //  SPI 1 Peripheral Chip Select 2
#define AT91C_PIO_PC2             (1 <<  2) // Pin Controlled by PC2
#define AT91C_PC2_AD2             (AT91C_PIO_PC2) //  ADC Analog Input 2
#define AT91C_PC2_PCK1            (AT91C_PIO_PC2) //  PMC Programmable Clock Output 1
#define AT91C_PIO_PC20            (1 << 20) // Pin Controlled by PC20
#define AT91C_PC20_D20            (AT91C_PIO_PC20) //  Data Bus[20]
#define AT91C_PC20_SPI1_NPCS3_1   (AT91C_PIO_PC20) //  SPI 1 Peripheral Chip Select 3
#define AT91C_PIO_PC21            (1 << 21) // Pin Controlled by PC21
#define AT91C_PC21_D21            (AT91C_PIO_PC21) //  Data Bus[21]
#define AT91C_PC21_EF100          (AT91C_PIO_PC21) //  Ethernet MAC Force 100 Mbits/sec
#define AT91C_PIO_PC22            (1 << 22) // Pin Controlled by PC22
#define AT91C_PC22_D22            (AT91C_PIO_PC22) //  Data Bus[22]
#define AT91C_PC22_TCLK5          (AT91C_PIO_PC22) //  Timer Counter 5 external clock input
#define AT91C_PIO_PC23            (1 << 23) // Pin Controlled by PC23
#define AT91C_PC23_D23            (AT91C_PIO_PC23) //  Data Bus[23]
#define AT91C_PIO_PC24            (1 << 24) // Pin Controlled by PC24
#define AT91C_PC24_D24            (AT91C_PIO_PC24) //  Data Bus[24]
#define AT91C_PIO_PC25            (1 << 25) // Pin Controlled by PC25
#define AT91C_PC25_D25            (AT91C_PIO_PC25) //  Data Bus[25]
#define AT91C_PIO_PC26            (1 << 26) // Pin Controlled by PC26
#define AT91C_PC26_D26            (AT91C_PIO_PC26) //  Data Bus[26]
#define AT91C_PIO_PC27            (1 << 27) // Pin Controlled by PC27
#define AT91C_PC27_D27            (AT91C_PIO_PC27) //  Data Bus[27]
#define AT91C_PIO_PC28            (1 << 28) // Pin Controlled by PC28
#define AT91C_PC28_D28            (AT91C_PIO_PC28) //  Data Bus[28]
#define AT91C_PIO_PC29            (1 << 29) // Pin Controlled by PC29
#define AT91C_PC29_D29            (AT91C_PIO_PC29) //  Data Bus[29]
#define AT91C_PIO_PC3             (1 <<  3) // Pin Controlled by PC3
#define AT91C_PC3_AD3             (AT91C_PIO_PC3) //  ADC Analog Input 3
#define AT91C_PC3_SPI1_NPCS3_0    (AT91C_PIO_PC3) //  SPI 1 Peripheral Chip Select 3
#define AT91C_PIO_PC30            (1 << 30) // Pin Controlled by PC30
#define AT91C_PC30_D30            (AT91C_PIO_PC30) //  Data Bus[30]
#define AT91C_PIO_PC31            (1 << 31) // Pin Controlled by PC31
#define AT91C_PC31_D31            (AT91C_PIO_PC31) //  Data Bus[31]
#define AT91C_PIO_PC4             (1 <<  4) // Pin Controlled by PC4
#define AT91C_PC4_A23             (AT91C_PIO_PC4) //  Address Bus[23]
#define AT91C_PC4_SPI1_NPCS2_0    (AT91C_PIO_PC4) //  SPI 1 Peripheral Chip Select 2
#define AT91C_PIO_PC5             (1 <<  5) // Pin Controlled by PC5
#define AT91C_PC5_A24             (AT91C_PIO_PC5) //  Address Bus[24]
#define AT91C_PC5_SPI1_NPCS1_0    (AT91C_PIO_PC5) //  SPI 1 Peripheral Chip Select 1
#define AT91C_PIO_PC6             (1 <<  6) // Pin Controlled by PC6
#define AT91C_PC6_TIOB2           (AT91C_PIO_PC6) //  Timer Counter 2 Multipurpose Timer I/O Pin B
#define AT91C_PC6_CFCE1           (AT91C_PIO_PC6) //  Compact Flash Enable 1
#define AT91C_PIO_PC7             (1 <<  7) // Pin Controlled by PC7
#define AT91C_PC7_TIOB1           (AT91C_PIO_PC7) //  Timer Counter 1 Multipurpose Timer I/O Pin B
#define AT91C_PC7_CFCE2           (AT91C_PIO_PC7) //  Compact Flash Enable 2
#define AT91C_PIO_PC8             (1 <<  8) // Pin Controlled by PC8
#define AT91C_PC8_NCS4_CFCS0      (AT91C_PIO_PC8) //  Chip Select Line 4
#define AT91C_PC8_RTS3            (AT91C_PIO_PC8) //  USART 3 Ready To Send
#define AT91C_PIO_PC9             (1 <<  9) // Pin Controlled by PC9
#define AT91C_PC9_NCS5_CFCS1      (AT91C_PIO_PC9) //  Chip Select Line 5
#define AT91C_PC9_TIOB0           (AT91C_PIO_PC9) //  Timer Counter 0 Multipurpose Timer I/O Pin B

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91SAM9XE128
// *****************************************************************************
#define AT91C_ID_FIQ              ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS              ( 1) // System Controller
#define AT91C_ID_PIOA             ( 2) // Parallel IO Controller A
#define AT91C_ID_PIOB             ( 3) // Parallel IO Controller B
#define AT91C_ID_PIOC             ( 4) // Parallel IO Controller C
#define AT91C_ID_ADC              ( 5) // ADC
#define AT91C_ID_US0              ( 6) // USART 0
#define AT91C_ID_US1              ( 7) // USART 1
#define AT91C_ID_US2              ( 8) // USART 2
#define AT91C_ID_MCI              ( 9) // Multimedia Card Interface 0
#define AT91C_ID_UDP              (10) // USB Device Port
#define AT91C_ID_TWI0             (11) // Two-Wire Interface 0
#define AT91C_ID_SPI0             (12) // Serial Peripheral Interface 0
#define AT91C_ID_SPI1             (13) // Serial Peripheral Interface 1
#define AT91C_ID_SSC0             (14) // Serial Synchronous Controller 0
#define AT91C_ID_TC0              (17) // Timer Counter 0
#define AT91C_ID_TC1              (18) // Timer Counter 1
#define AT91C_ID_TC2              (19) // Timer Counter 2
#define AT91C_ID_UHP              (20) // USB Host Port
#define AT91C_ID_EMAC             (21) // Ethernet Mac
#define AT91C_ID_HISI             (22) // Image Sensor Interface
#define AT91C_ID_US3              (23) // USART 3
#define AT91C_ID_US4              (24) // USART 4
#define AT91C_ID_TWI1             (25) // Two-Wire Interface 1
#define AT91C_ID_TC3              (26) // Timer Counter 3
#define AT91C_ID_TC4              (27) // Timer Counter 4
#define AT91C_ID_TC5              (28) // Timer Counter 5
#define AT91C_ID_IRQ0             (29) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ID_IRQ1             (30) // Advanced Interrupt Controller (IRQ1)
#define AT91C_ID_IRQ2             (31) // Advanced Interrupt Controller (IRQ2)
#define AT91C_ALL_INT             (0xFFFE7FFF) // ALL VALID INTERRUPTS

// *****************************************************************************
//               BASE ADDRESS DEFINITIONS FOR AT91SAM9XE128
// *****************************************************************************
#define AT91C_BASE_SYS            (0xFFFFFD00) // (SYS) Base Address
#define AT91C_BASE_EBI            (0xFFFFEA00) // (EBI) Base Address
#define AT91C_BASE_HECC           (0xFFFFE800) // (HECC) Base Address
#define AT91C_BASE_SDRAMC         (0xFFFFEA00) // (SDRAMC) Base Address
#define AT91C_BASE_SMC0           (0xFFFFEC00) // (SMC0) Base Address
#define AT91C_BASE_MATRIX         (0xFFFFEE00) // (MATRIX) Base Address
#define AT91C_BASE_CCFG           (0xFFFFEF10) // (CCFG) Base Address
#define AT91C_BASE_PDC_DBGU       (0xFFFFF300) // (PDC_DBGU) Base Address
#define AT91C_BASE_DBGU           (0xFFFFF200) // (DBGU) Base Address
#define AT91C_BASE_AIC            (0xFFFFF000) // (AIC) Base Address
#define AT91C_BASE_PIOA           (0xFFFFF400) // (PIOA) Base Address
#define AT91C_BASE_PIOB           (0xFFFFF600) // (PIOB) Base Address
#define AT91C_BASE_PIOC           (0xFFFFF800) // (PIOC) Base Address
#define AT91C_BASE_EFC            (0xFFFFFA00) // (EFC) Base Address
#define AT91C_BASE_CKGR           (0xFFFFFC20) // (CKGR) Base Address
#define AT91C_BASE_PMC            (0xFFFFFC00) // (PMC) Base Address
#define AT91C_BASE_RSTC           (0xFFFFFD00) // (RSTC) Base Address
#define AT91C_BASE_SHDWC          (0xFFFFFD10) // (SHDWC) Base Address
#define AT91C_BASE_RTTC           (0xFFFFFD20) // (RTTC) Base Address
#define AT91C_BASE_PITC           (0xFFFFFD30) // (PITC) Base Address
#define AT91C_BASE_WDTC           (0xFFFFFD40) // (WDTC) Base Address
#define AT91C_BASE_TC0            (0xFFFA0000) // (TC0) Base Address
#define AT91C_BASE_TC1            (0xFFFA0040) // (TC1) Base Address
#define AT91C_BASE_TC2            (0xFFFA0080) // (TC2) Base Address
#define AT91C_BASE_TC3            (0xFFFDC000) // (TC3) Base Address
#define AT91C_BASE_TC4            (0xFFFDC040) // (TC4) Base Address
#define AT91C_BASE_TC5            (0xFFFDC080) // (TC5) Base Address
#define AT91C_BASE_TCB0           (0xFFFA0000) // (TCB0) Base Address
#define AT91C_BASE_TCB1           (0xFFFDC000) // (TCB1) Base Address
#define AT91C_BASE_PDC_MCI        (0xFFFA8100) // (PDC_MCI) Base Address
#define AT91C_BASE_MCI            (0xFFFA8000) // (MCI) Base Address
#define AT91C_BASE_PDC_TWI0       (0xFFFAC100) // (PDC_TWI0) Base Address
#define AT91C_BASE_TWI0           (0xFFFAC000) // (TWI0) Base Address
#define AT91C_BASE_PDC_TWI1       (0xFFFD8100) // (PDC_TWI1) Base Address
#define AT91C_BASE_TWI1           (0xFFFD8000) // (TWI1) Base Address
#define AT91C_BASE_PDC_US0        (0xFFFB0100) // (PDC_US0) Base Address
#define AT91C_BASE_US0            (0xFFFB0000) // (US0) Base Address
#define AT91C_BASE_PDC_US1        (0xFFFB4100) // (PDC_US1) Base Address
#define AT91C_BASE_US1            (0xFFFB4000) // (US1) Base Address
#define AT91C_BASE_PDC_US2        (0xFFFB8100) // (PDC_US2) Base Address
#define AT91C_BASE_US2            (0xFFFB8000) // (US2) Base Address
#define AT91C_BASE_PDC_US3        (0xFFFD0100) // (PDC_US3) Base Address
#define AT91C_BASE_US3            (0xFFFD0000) // (US3) Base Address
#define AT91C_BASE_PDC_US4        (0xFFFD4100) // (PDC_US4) Base Address
#define AT91C_BASE_US4            (0xFFFD4000) // (US4) Base Address
#define AT91C_BASE_PDC_SSC0       (0xFFFBC100) // (PDC_SSC0) Base Address
#define AT91C_BASE_SSC0           (0xFFFBC000) // (SSC0) Base Address
#define AT91C_BASE_PDC_SPI0       (0xFFFC8100) // (PDC_SPI0) Base Address
#define AT91C_BASE_SPI0           (0xFFFC8000) // (SPI0) Base Address
#define AT91C_BASE_PDC_SPI1       (0xFFFCC100) // (PDC_SPI1) Base Address
#define AT91C_BASE_SPI1           (0xFFFCC000) // (SPI1) Base Address
#define AT91C_BASE_PDC_ADC        (0xFFFE0100) // (PDC_ADC) Base Address
#define AT91C_BASE_ADC            (0xFFFE0000) // (ADC) Base Address
#define AT91C_BASE_EMACB          (0xFFFC4000) // (EMACB) Base Address
#define AT91C_BASE_UDP            (0xFFFA4000) // (UDP) Base Address
#define AT91C_BASE_UHP            (0x00500000) // (UHP) Base Address
#define AT91C_BASE_HISI           (0xFFFC0000) // (HISI) Base Address

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91SAM9XE128
// *****************************************************************************
// IROM
#define AT91C_IROM 	              (0x00100000) // Internal ROM base address
#define AT91C_IROM_SIZE	          (0x00008000) // Internal ROM size in byte (32 Kbytes)
// IRAM
#define AT91C_IRAM 	              (0x00300000) // Maximum IRAM Area : 32Kbyte base address
#define AT91C_IRAM_SIZE	          (0x00008000) // Maximum IRAM Area : 32Kbyte size in byte (32 Kbytes)
// IRAM_MIN
#define AT91C_IRAM_MIN	           (0x00300000) // Minimun IRAM Area : 32Kbyte base address
#define AT91C_IRAM_MIN_SIZE	      (0x00008000) // Minimun IRAM Area : 32Kbyte size in byte (32 Kbytes)
// IFLASH
#define AT91C_IFLASH	             (0x00200000) // Maximum IFLASH Area : 128Kbyte base address
#define AT91C_IFLASH_SIZE	        (0x00020000) // Maximum IFLASH Area : 128Kbyte size in byte (128 Kbytes)
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
#define AT91C_EBI_SM	             (0x40000000) // SmartMedia on Chip Select 3 base address
#define AT91C_EBI_SM_SIZE	        (0x10000000) // SmartMedia on Chip Select 3 size in byte (262144 Kbytes)
// EBI_CS4
#define AT91C_EBI_CS4	            (0x50000000) // EBI Chip Select 4 base address
#define AT91C_EBI_CS4_SIZE	       (0x10000000) // EBI Chip Select 4 size in byte (262144 Kbytes)
// EBI_CF0
#define AT91C_EBI_CF0	            (0x50000000) // CompactFlash 0 on Chip Select 4 base address
#define AT91C_EBI_CF0_SIZE	       (0x10000000) // CompactFlash 0 on Chip Select 4 size in byte (262144 Kbytes)
// EBI_CS5
#define AT91C_EBI_CS5	            (0x60000000) // EBI Chip Select 5 base address
#define AT91C_EBI_CS5_SIZE	       (0x10000000) // EBI Chip Select 5 size in byte (262144 Kbytes)
// EBI_CF1
#define AT91C_EBI_CF1	            (0x60000000) // CompactFlash 1 on Chip Select 5 base address
#define AT91C_EBI_CF1_SIZE	       (0x10000000) // CompactFlash 1 on Chip Select 5 size in byte (262144 Kbytes)
// EBI_CS6
#define AT91C_EBI_CS6	            (0x70000000) // EBI Chip Select 6 base address
#define AT91C_EBI_CS6_SIZE	       (0x10000000) // EBI Chip Select 6 size in byte (262144 Kbytes)
// EBI_CS7
#define AT91C_EBI_CS7	            (0x80000000) // EBI Chip Select 7 base address
#define AT91C_EBI_CS7_SIZE	       (0x10000000) // EBI Chip Select 7 size in byte (262144 Kbytes)


