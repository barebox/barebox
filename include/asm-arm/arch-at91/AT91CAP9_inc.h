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
// File Name           : AT91CAP9.h
// Object              : AT91CAP9 definitions
// Generated           : AT91 SW Application Group  04/18/2006 (16:56:20)
//
// CVS Reference       : /AT91CAP9.pl/1.1/Tue Aug 16 16:50:18 2005//
// CVS Reference       : /SYS_AT91CAP9.pl/0/dummy timestamp//
// CVS Reference       : /HECC_6143A.pl/1.1/Wed Feb  9 17:16:57 2005//
// CVS Reference       : /HBCRAMC1_XXXX.pl/0/dummy timestamp//
// CVS Reference       : /HSDRAMC1_6100A.pl/1.2/Thu Feb 17 11:25:50 2005//
// CVS Reference       : /DDRSDRC_XXXX.pl/1.1/Fri Oct 28 14:10:02 2005//
// CVS Reference       : /HSMC3_6105A.pl/1.4/Mon Aug 22 13:08:54 2005//
// CVS Reference       : /HMATRIX1_CAP9.pl/1.1/Mon Oct 17 10:12:18 2005//
// CVS Reference       : /CCR_CAP9.pl/0/dummy timestamp//
// CVS Reference       : /PDC_6074C.pl/1.2/Thu Feb  3 09:02:11 2005//
// CVS Reference       : /DBGU_6059D.pl/1.1/Mon Jan 31 13:54:41 2005//
// CVS Reference       : /AIC_6075A.pl/1.1/Thu Feb 17 11:25:46 2005//
// CVS Reference       : /PIO_6057A.pl/1.2/Mon Aug 22 13:04:19 2005//
// CVS Reference       : /PMC_CAP9.pl/0/dummy timestamp//
// CVS Reference       : /RSTC_6098A.pl/1.3/Mon Aug 22 13:04:20 2005//
// CVS Reference       : /SHDWC_6122A.pl/1.3/Thu Feb 17 11:25:56 2005//
// CVS Reference       : /RTTC_6081A.pl/1.2/Mon Aug 22 13:04:20 2005//
// CVS Reference       : /PITC_6079A.pl/1.2/Mon Aug 22 13:04:19 2005//
// CVS Reference       : /WDTC_6080A.pl/1.3/Mon Aug 22 13:12:55 2005//
// CVS Reference       : /UDP_6083C.pl/1.2/Tue May 10 12:40:17 2005//
// CVS Reference       : /UDPHS_SAM9265.pl/1.3/Result of merge//
// CVS Reference       : /TC_6082A.pl/1.7/Mon Aug 22 13:12:50 2005//
// CVS Reference       : /MCI_6101E.pl/1.1/Fri Jun  3 13:20:23 2005//
// CVS Reference       : /TWI_6061B.pl/1.1/Tue Sep 13 15:05:42 2005//
// CVS Reference       : /US_6089C.pl/1.1/Mon Jan 31 13:56:02 2005//
// CVS Reference       : /SSC_6078A.pl/1.1/Thu Feb 17 11:25:51 2005//
// CVS Reference       : /AC97C_XXXX.pl/1.3/Tue Feb 22 17:08:27 2005//
// CVS Reference       : /SPI_6088D.pl/1.3/Fri May 20 14:23:02 2005//
// CVS Reference       : /CAN_6019B.pl/1.1/Mon Jan 31 13:54:30 2005//
// CVS Reference       : /AES_6149A.pl/1.10/Mon Feb  7 09:46:08 2005//
// CVS Reference       : /DES3_6150A.pl/1.1/Mon Aug 22 13:01:47 2005//
// CVS Reference       : /PWM_6044D.pl/1.2/Tue May 10 12:39:09 2005//
// CVS Reference       : /EMACB_6119A.pl/1.6/Wed Jul 13 15:25:00 2005//
// CVS Reference       : /ADC_6051C.pl/1.1/Mon Jan 31 13:12:40 2005//
// CVS Reference       : /ISI_xxxxx.pl/1.3/Thu Mar  3 11:11:48 2005//
// CVS Reference       : /LCDC_6063A.pl/1.2/Mon Aug 22 13:04:17 2005//
// CVS Reference       : /HDMA_XXXX.pl/1.1/Tue Oct 11 12:51:53 2005//
// CVS Reference       : /UHP_6127A.pl/1.1/Wed Feb 23 16:03:17 2005//
//  ----------------------------------------------------------------------------

#ifndef _AT91CAP9_INC_H_
#define _AT91CAP9_INC_H_

// Hardware register definition

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
//              SOFTWARE API DEFINITION  FOR Busr Cellular RAM Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_BCRAMC structure ***
#define BCRAMC_CR       ( 0) // BCRAM Controller Configuration Register
#define BCRAMC_TPR      ( 4) // BCRAM Controller Timing Parameter Register
#define BCRAMC_HSR      ( 8) // BCRAM Controller High Speed Register
#define BCRAMC_LPR      (12) // BCRAM Controller Low Power Register
#define BCRAMC_MDR      (16) // BCRAM Memory Device Register
#define BCRAMC_PADDSR   (236) // BCRAM PADDR Size Register
#define BCRAMC_IPNR1    (240) // BCRAM IP Name Register 1
#define BCRAMC_IPNR2    (244) // BCRAM IP Name Register 2
#define BCRAMC_IPFR     (248) // BCRAM IP Features Register
#define BCRAMC_VR       (252) // BCRAM Version Register
// -------- BCRAMC_CR : (BCRAMC Offset: 0x0) BCRAM Controller Configuration Register --------
#define AT91C_BCRAMC_EN           (0x1 <<  0) // (BCRAMC) Enable
#define AT91C_BCRAMC_CAS          (0x7 <<  4) // (BCRAMC) CAS Latency
#define 	AT91C_BCRAMC_CAS_2                    (0x2 <<  4) // (BCRAMC) 2 cycles Latency for Cellular RAM v1.0/1.5/2.0
#define 	AT91C_BCRAMC_CAS_3                    (0x3 <<  4) // (BCRAMC) 3 cycles Latency for Cellular RAM v1.0/1.5/2.0
#define 	AT91C_BCRAMC_CAS_4                    (0x4 <<  4) // (BCRAMC) 4 cycles Latency for Cellular RAM v1.5/2.0
#define 	AT91C_BCRAMC_CAS_5                    (0x5 <<  4) // (BCRAMC) 5 cycles Latency for Cellular RAM v1.5/2.0
#define 	AT91C_BCRAMC_CAS_6                    (0x6 <<  4) // (BCRAMC) 6 cycles Latency for Cellular RAM v1.5/2.0
#define AT91C_BCRAMC_DBW          (0x1 <<  8) // (BCRAMC) Data Bus Width
#define 	AT91C_BCRAMC_DBW_32_BITS              (0x0 <<  8) // (BCRAMC) 32 Bits datas bus
#define 	AT91C_BCRAMC_DBW_16_BITS              (0x1 <<  8) // (BCRAMC) 16 Bits datas bus
#define AT91C_BCRAM_NWIR          (0x3 << 12) // (BCRAMC) Number Of Words in Row
#define 	AT91C_BCRAM_NWIR_64                   (0x0 << 12) // (BCRAMC) 64 Words in Row
#define 	AT91C_BCRAM_NWIR_128                  (0x1 << 12) // (BCRAMC) 128 Words in Row
#define 	AT91C_BCRAM_NWIR_256                  (0x2 << 12) // (BCRAMC) 256 Words in Row
#define 	AT91C_BCRAM_NWIR_512                  (0x3 << 12) // (BCRAMC) 512 Words in Row
#define AT91C_BCRAM_ADMX          (0x1 << 16) // (BCRAMC) ADDR / DATA Mux
#define 	AT91C_BCRAM_ADMX_NO_MUX               (0x0 << 16) // (BCRAMC) No ADD/DATA Mux for Cellular RAM v1.0/1.5/2.0
#define 	AT91C_BCRAM_ADMX_MUX                  (0x1 << 16) // (BCRAMC) ADD/DATA Mux Only for Cellular RAM v2.0
#define AT91C_BCRAM_DS            (0x3 << 20) // (BCRAMC) Drive Strength
#define 	AT91C_BCRAM_DS_FULL_DRIVE           (0x0 << 20) // (BCRAMC) Full Cellular RAM Drive
#define 	AT91C_BCRAM_DS_HALF_DRIVE           (0x1 << 20) // (BCRAMC) Half Cellular RAM Drive
#define 	AT91C_BCRAM_DS_QUARTER_DRIVE        (0x2 << 20) // (BCRAMC) Quarter Cellular RAM Drive
#define AT91C_BCRAM_VFLAT         (0x1 << 24) // (BCRAMC) Variable or Fixed Latency
#define 	AT91C_BCRAM_VFLAT_VARIABLE             (0x0 << 24) // (BCRAMC) Variable Latency
#define 	AT91C_BCRAM_VFLAT_FIXED                (0x1 << 24) // (BCRAMC) Fixed Latency
// -------- BCRAMC_TPR : (BCRAMC Offset: 0x4) BCRAMC Timing Parameter Register --------
#define AT91C_BCRAMC_TCW          (0xF <<  0) // (BCRAMC) Chip Enable to End of Write
#define 	AT91C_BCRAMC_TCW_0                    (0x0) // (BCRAMC) Value :  0
#define 	AT91C_BCRAMC_TCW_1                    (0x1) // (BCRAMC) Value :  1
#define 	AT91C_BCRAMC_TCW_2                    (0x2) // (BCRAMC) Value :  2
#define 	AT91C_BCRAMC_TCW_3                    (0x3) // (BCRAMC) Value :  3
#define 	AT91C_BCRAMC_TCW_4                    (0x4) // (BCRAMC) Value :  4
#define 	AT91C_BCRAMC_TCW_5                    (0x5) // (BCRAMC) Value :  5
#define 	AT91C_BCRAMC_TCW_6                    (0x6) // (BCRAMC) Value :  6
#define 	AT91C_BCRAMC_TCW_7                    (0x7) // (BCRAMC) Value :  7
#define 	AT91C_BCRAMC_TCW_8                    (0x8) // (BCRAMC) Value :  8
#define 	AT91C_BCRAMC_TCW_9                    (0x9) // (BCRAMC) Value :  9
#define 	AT91C_BCRAMC_TCW_10                   (0xA) // (BCRAMC) Value : 10
#define 	AT91C_BCRAMC_TCW_11                   (0xB) // (BCRAMC) Value : 11
#define 	AT91C_BCRAMC_TCW_12                   (0xC) // (BCRAMC) Value : 12
#define 	AT91C_BCRAMC_TCW_13                   (0xD) // (BCRAMC) Value : 13
#define 	AT91C_BCRAMC_TCW_14                   (0xE) // (BCRAMC) Value : 14
#define 	AT91C_BCRAMC_TCW_15                   (0xF) // (BCRAMC) Value : 15
#define AT91C_BCRAMC_TCRES        (0x3 <<  4) // (BCRAMC) CRE Setup
#define 	AT91C_BCRAMC_TCRES_0                    (0x0 <<  4) // (BCRAMC) Value :  0
#define 	AT91C_BCRAMC_TCRES_1                    (0x1 <<  4) // (BCRAMC) Value :  1
#define 	AT91C_BCRAMC_TCRES_2                    (0x2 <<  4) // (BCRAMC) Value :  2
#define 	AT91C_BCRAMC_TCRES_3                    (0x3 <<  4) // (BCRAMC) Value :  3
#define AT91C_BCRAMC_TCKA         (0xF <<  8) // (BCRAMC) WE High to CLK Valid
#define 	AT91C_BCRAMC_TCKA_0                    (0x0 <<  8) // (BCRAMC) Value :  0
#define 	AT91C_BCRAMC_TCKA_1                    (0x1 <<  8) // (BCRAMC) Value :  1
#define 	AT91C_BCRAMC_TCKA_2                    (0x2 <<  8) // (BCRAMC) Value :  2
#define 	AT91C_BCRAMC_TCKA_3                    (0x3 <<  8) // (BCRAMC) Value :  3
#define 	AT91C_BCRAMC_TCKA_4                    (0x4 <<  8) // (BCRAMC) Value :  4
#define 	AT91C_BCRAMC_TCKA_5                    (0x5 <<  8) // (BCRAMC) Value :  5
#define 	AT91C_BCRAMC_TCKA_6                    (0x6 <<  8) // (BCRAMC) Value :  6
#define 	AT91C_BCRAMC_TCKA_7                    (0x7 <<  8) // (BCRAMC) Value :  7
#define 	AT91C_BCRAMC_TCKA_8                    (0x8 <<  8) // (BCRAMC) Value :  8
#define 	AT91C_BCRAMC_TCKA_9                    (0x9 <<  8) // (BCRAMC) Value :  9
#define 	AT91C_BCRAMC_TCKA_10                   (0xA <<  8) // (BCRAMC) Value : 10
#define 	AT91C_BCRAMC_TCKA_11                   (0xB <<  8) // (BCRAMC) Value : 11
#define 	AT91C_BCRAMC_TCKA_12                   (0xC <<  8) // (BCRAMC) Value : 12
#define 	AT91C_BCRAMC_TCKA_13                   (0xD <<  8) // (BCRAMC) Value : 13
#define 	AT91C_BCRAMC_TCKA_14                   (0xE <<  8) // (BCRAMC) Value : 14
#define 	AT91C_BCRAMC_TCKA_15                   (0xF <<  8) // (BCRAMC) Value : 15
// -------- BCRAMC_HSR : (BCRAMC Offset: 0x8) BCRAM Controller High Speed Register --------
#define AT91C_BCRAMC_DA           (0x1 <<  0) // (BCRAMC) Decode Cycle Enable Bit
#define 	AT91C_BCRAMC_DA_DISABLE              (0x0) // (BCRAMC) Disable Decode Cycle
#define 	AT91C_BCRAMC_DA_ENABLE               (0x1) // (BCRAMC) Enable Decode Cycle
// -------- BCRAMC_LPR : (BCRAMC Offset: 0xc) BCRAM Controller Low-power Register --------
#define AT91C_BCRAMC_PAR          (0x7 <<  0) // (BCRAMC) Partial Array Refresh
#define 	AT91C_BCRAMC_PAR_FULL                 (0x0) // (BCRAMC) Full Refresh
#define 	AT91C_BCRAMC_PAR_PARTIAL_BOTTOM_HALF  (0x1) // (BCRAMC) Partial Bottom Half Refresh
#define 	AT91C_BCRAMC_PAR_PARTIAL_BOTTOM_QUARTER (0x2) // (BCRAMC) Partial Bottom Quarter Refresh
#define 	AT91C_BCRAMC_PAR_PARTIAL_BOTTOM_EIGTH (0x3) // (BCRAMC) Partial Bottom eigth Refresh
#define 	AT91C_BCRAMC_PAR_NONE                 (0x4) // (BCRAMC) Not Refreshed
#define 	AT91C_BCRAMC_PAR_PARTIAL_TOP_HALF     (0x5) // (BCRAMC) Partial Top Half Refresh
#define 	AT91C_BCRAMC_PAR_PARTIAL_TOP_QUARTER  (0x6) // (BCRAMC) Partial Top Quarter Refresh
#define 	AT91C_BCRAMC_PAR_PARTIAL_TOP_EIGTH    (0x7) // (BCRAMC) Partial Top eigth Refresh
#define AT91C_BCRAMC_TCR          (0x3 <<  4) // (BCRAMC) Temperature Compensated Self Refresh
#define 	AT91C_BCRAMC_TCR_85C                  (0x0 <<  4) // (BCRAMC) +85C Temperature
#define 	AT91C_BCRAMC_TCR_INTERNAL_OR_70C      (0x1 <<  4) // (BCRAMC) Internal Sensor or +70C Temperature
#define 	AT91C_BCRAMC_TCR_45C                  (0x2 <<  4) // (BCRAMC) +45C Temperature
#define 	AT91C_BCRAMC_TCR_15C                  (0x3 <<  4) // (BCRAMC) +15C Temperature
#define AT91C_BCRAMC_LPCB         (0x3 <<  8) // (BCRAMC) Low-power Command Bit
#define 	AT91C_BCRAMC_LPCB_DISABLE              (0x0 <<  8) // (BCRAMC) Disable Low Power Features
#define 	AT91C_BCRAMC_LPCB_STANDBY              (0x1 <<  8) // (BCRAMC) Enable Cellular RAM Standby Mode
#define 	AT91C_BCRAMC_LPCB_DEEP_POWER_DOWN      (0x2 <<  8) // (BCRAMC) Enable Cellular RAM Deep Power Down Mode
// -------- BCRAMC_MDR : (BCRAMC Offset: 0x10) BCRAM Controller Memory Device Register --------
#define AT91C_BCRAMC_MD           (0x3 <<  0) // (BCRAMC) Memory Device Type
#define 	AT91C_BCRAMC_MD_BCRAM_V10            (0x0) // (BCRAMC) Busrt Cellular RAM v1.0
#define 	AT91C_BCRAMC_MD_BCRAM_V15            (0x1) // (BCRAMC) Busrt Cellular RAM v1.5
#define 	AT91C_BCRAMC_MD_BCRAM_V20            (0x2) // (BCRAMC) Busrt Cellular RAM v2.0

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
//              SOFTWARE API DEFINITION  FOR DDR/SDRAM Controller
// *****************************************************************************
// *** Register offset in AT91S_SDDRC structure ***
#define SDDRC_MR        ( 0) //
#define SDDRC_RTR       ( 4) //
#define SDDRC_CR        ( 8) //
#define SDDRC_T0PR      (12) //
#define SDDRC_T1PR      (16) //
#define SDDRC_HS        (20) //
#define SDDRC_LPR       (24) //
#define SDDRC_MDR       (28) //
#define SDDRC_VERSION   (252) //
// -------- SDDRC_MR : (SDDRC Offset: 0x0)  --------
#define AT91C_MODE                (0xF <<  0) // (SDDRC)
#define 	AT91C_MODE_NORMAL_CMD           (0x0) // (SDDRC) Normal Mode
#define 	AT91C_MODE_NOP_CMD              (0x1) // (SDDRC) Issue a NOP Command at every access
#define 	AT91C_MODE_PRCGALL_CMD          (0x2) // (SDDRC) Issue a All Banks Precharge Command at every access
#define 	AT91C_MODE_LMR_CMD              (0x3) // (SDDRC) Issue a Load Mode Register at every access
#define 	AT91C_MODE_RFSH_CMD             (0x4) // (SDDRC) Issue a Refresh
#define 	AT91C_MODE_EXT_LMR_CMD          (0x5) // (SDDRC) Issue an Extended Load Mode Register
#define 	AT91C_MODE_DEEP_CMD             (0x6) // (SDDRC) Enter Deep Power Mode
// -------- SDDRC_RTR : (SDDRC Offset: 0x4)  --------
#define AT91C_COUNT               (0xFFF <<  0) // (SDDRC)
// -------- SDDRC_CR : (SDDRC Offset: 0x8)  --------
#define AT91C_NC                  (0x3 <<  0) // (SDDRC)
#define 	AT91C_NC_DDR9_SDR8            (0x0) // (SDDRC) DDR 9 Bits | SDR 8 Bits
#define 	AT91C_NC_DDR10_SDR9           (0x1) // (SDDRC) DDR 10 Bits | SDR 9 Bits
#define 	AT91C_NC_DDR11_SDR10          (0x2) // (SDDRC) DDR 11 Bits | SDR 10 Bits
#define 	AT91C_NC_DDR12_SDR11          (0x3) // (SDDRC) DDR 12 Bits | SDR 11 Bits
#define AT91C_NR                  (0x3 <<  2) // (SDDRC)
#define 	AT91C_NR_11                   (0x0 <<  2) // (SDDRC) 11 Bits
#define 	AT91C_NR_12                   (0x1 <<  2) // (SDDRC) 12 Bits
#define 	AT91C_NR_13                   (0x2 <<  2) // (SDDRC) 13 Bits
#define 	AT91C_NR_14                   (0x3 <<  2) // (SDDRC) 14 Bits
#define AT91C_CAS                 (0x7 <<  4) // (SDDRC)
#define 	AT91C_CAS_2                    (0x2 <<  4) // (SDDRC) 2 cycles
#define 	AT91C_CAS_3                    (0x3 <<  4) // (SDDRC) 3 cycles
#define AT91C_DLL                 (0x1 <<  7) // (SDDRC)
#define 	AT91C_DLL_RESET_DISABLED       (0x0 <<  7) // (SDDRC) Disable DLL reset
#define 	AT91C_DLL_RESET_ENABLED        (0x1 <<  7) // (SDDRC) Enable DLL reset
#define AT91C_DIC_DS              (0x1 <<  8) // (SDDRC)
// -------- SDDRC_T0PR : (SDDRC Offset: 0xc)  --------
#define AT91C_TRAS                (0xF <<  0) // (SDDRC)
#define 	AT91C_TRAS_0                    (0x0) // (SDDRC) Value :  0
#define 	AT91C_TRAS_1                    (0x1) // (SDDRC) Value :  1
#define 	AT91C_TRAS_2                    (0x2) // (SDDRC) Value :  2
#define 	AT91C_TRAS_3                    (0x3) // (SDDRC) Value :  3
#define 	AT91C_TRAS_4                    (0x4) // (SDDRC) Value :  4
#define 	AT91C_TRAS_5                    (0x5) // (SDDRC) Value :  5
#define 	AT91C_TRAS_6                    (0x6) // (SDDRC) Value :  6
#define 	AT91C_TRAS_7                    (0x7) // (SDDRC) Value :  7
#define 	AT91C_TRAS_8                    (0x8) // (SDDRC) Value :  8
#define 	AT91C_TRAS_9                    (0x9) // (SDDRC) Value :  9
#define 	AT91C_TRAS_10                   (0xA) // (SDDRC) Value : 10
#define 	AT91C_TRAS_11                   (0xB) // (SDDRC) Value : 11
#define 	AT91C_TRAS_12                   (0xC) // (SDDRC) Value : 12
#define 	AT91C_TRAS_13                   (0xD) // (SDDRC) Value : 13
#define 	AT91C_TRAS_14                   (0xE) // (SDDRC) Value : 14
#define 	AT91C_TRAS_15                   (0xF) // (SDDRC) Value : 15
#define AT91C_TRCD                (0xF <<  4) // (SDDRC)
#define 	AT91C_TRCD_0                    (0x0 <<  4) // (SDDRC) Value :  0
#define 	AT91C_TRCD_1                    (0x1 <<  4) // (SDDRC) Value :  1
#define 	AT91C_TRCD_2                    (0x2 <<  4) // (SDDRC) Value :  2
#define 	AT91C_TRCD_3                    (0x3 <<  4) // (SDDRC) Value :  3
#define 	AT91C_TRCD_4                    (0x4 <<  4) // (SDDRC) Value :  4
#define 	AT91C_TRCD_5                    (0x5 <<  4) // (SDDRC) Value :  5
#define 	AT91C_TRCD_6                    (0x6 <<  4) // (SDDRC) Value :  6
#define 	AT91C_TRCD_7                    (0x7 <<  4) // (SDDRC) Value :  7
#define 	AT91C_TRCD_8                    (0x8 <<  4) // (SDDRC) Value :  8
#define 	AT91C_TRCD_9                    (0x9 <<  4) // (SDDRC) Value :  9
#define 	AT91C_TRCD_10                   (0xA <<  4) // (SDDRC) Value : 10
#define 	AT91C_TRCD_11                   (0xB <<  4) // (SDDRC) Value : 11
#define 	AT91C_TRCD_12                   (0xC <<  4) // (SDDRC) Value : 12
#define 	AT91C_TRCD_13                   (0xD <<  4) // (SDDRC) Value : 13
#define 	AT91C_TRCD_14                   (0xE <<  4) // (SDDRC) Value : 14
#define 	AT91C_TRCD_15                   (0xF <<  4) // (SDDRC) Value : 15
#define AT91C_TWR                 (0xF <<  8) // (SDDRC)
#define 	AT91C_TWR_0                    (0x0 <<  8) // (SDDRC) Value :  0
#define 	AT91C_TWR_1                    (0x1 <<  8) // (SDDRC) Value :  1
#define 	AT91C_TWR_2                    (0x2 <<  8) // (SDDRC) Value :  2
#define 	AT91C_TWR_3                    (0x3 <<  8) // (SDDRC) Value :  3
#define 	AT91C_TWR_4                    (0x4 <<  8) // (SDDRC) Value :  4
#define 	AT91C_TWR_5                    (0x5 <<  8) // (SDDRC) Value :  5
#define 	AT91C_TWR_6                    (0x6 <<  8) // (SDDRC) Value :  6
#define 	AT91C_TWR_7                    (0x7 <<  8) // (SDDRC) Value :  7
#define 	AT91C_TWR_8                    (0x8 <<  8) // (SDDRC) Value :  8
#define 	AT91C_TWR_9                    (0x9 <<  8) // (SDDRC) Value :  9
#define 	AT91C_TWR_10                   (0xA <<  8) // (SDDRC) Value : 10
#define 	AT91C_TWR_11                   (0xB <<  8) // (SDDRC) Value : 11
#define 	AT91C_TWR_12                   (0xC <<  8) // (SDDRC) Value : 12
#define 	AT91C_TWR_13                   (0xD <<  8) // (SDDRC) Value : 13
#define 	AT91C_TWR_14                   (0xE <<  8) // (SDDRC) Value : 14
#define 	AT91C_TWR_15                   (0xF <<  8) // (SDDRC) Value : 15
#define AT91C_TRC                 (0xF << 12) // (SDDRC)
#define 	AT91C_TRC_0                    (0x0 << 12) // (SDDRC) Value :  0
#define 	AT91C_TRC_1                    (0x1 << 12) // (SDDRC) Value :  1
#define 	AT91C_TRC_2                    (0x2 << 12) // (SDDRC) Value :  2
#define 	AT91C_TRC_3                    (0x3 << 12) // (SDDRC) Value :  3
#define 	AT91C_TRC_4                    (0x4 << 12) // (SDDRC) Value :  4
#define 	AT91C_TRC_5                    (0x5 << 12) // (SDDRC) Value :  5
#define 	AT91C_TRC_6                    (0x6 << 12) // (SDDRC) Value :  6
#define 	AT91C_TRC_7                    (0x7 << 12) // (SDDRC) Value :  7
#define 	AT91C_TRC_8                    (0x8 << 12) // (SDDRC) Value :  8
#define 	AT91C_TRC_9                    (0x9 << 12) // (SDDRC) Value :  9
#define 	AT91C_TRC_10                   (0xA << 12) // (SDDRC) Value : 10
#define 	AT91C_TRC_11                   (0xB << 12) // (SDDRC) Value : 11
#define 	AT91C_TRC_12                   (0xC << 12) // (SDDRC) Value : 12
#define 	AT91C_TRC_13                   (0xD << 12) // (SDDRC) Value : 13
#define 	AT91C_TRC_14                   (0xE << 12) // (SDDRC) Value : 14
#define 	AT91C_TRC_15                   (0xF << 12) // (SDDRC) Value : 15
#define AT91C_TRP                 (0xF << 16) // (SDDRC)
#define 	AT91C_TRP_0                    (0x0 << 16) // (SDDRC) Value :  0
#define 	AT91C_TRP_1                    (0x1 << 16) // (SDDRC) Value :  1
#define 	AT91C_TRP_2                    (0x2 << 16) // (SDDRC) Value :  2
#define 	AT91C_TRP_3                    (0x3 << 16) // (SDDRC) Value :  3
#define 	AT91C_TRP_4                    (0x4 << 16) // (SDDRC) Value :  4
#define 	AT91C_TRP_5                    (0x5 << 16) // (SDDRC) Value :  5
#define 	AT91C_TRP_6                    (0x6 << 16) // (SDDRC) Value :  6
#define 	AT91C_TRP_7                    (0x7 << 16) // (SDDRC) Value :  7
#define 	AT91C_TRP_8                    (0x8 << 16) // (SDDRC) Value :  8
#define 	AT91C_TRP_9                    (0x9 << 16) // (SDDRC) Value :  9
#define 	AT91C_TRP_10                   (0xA << 16) // (SDDRC) Value : 10
#define 	AT91C_TRP_11                   (0xB << 16) // (SDDRC) Value : 11
#define 	AT91C_TRP_12                   (0xC << 16) // (SDDRC) Value : 12
#define 	AT91C_TRP_13                   (0xD << 16) // (SDDRC) Value : 13
#define 	AT91C_TRP_14                   (0xE << 16) // (SDDRC) Value : 14
#define 	AT91C_TRP_15                   (0xF << 16) // (SDDRC) Value : 15
#define AT91C_TRRD                (0xF << 20) // (SDDRC)
#define 	AT91C_TRRD_0                    (0x0 << 20) // (SDDRC) Value :  0
#define 	AT91C_TRRD_1                    (0x1 << 20) // (SDDRC) Value :  1
#define 	AT91C_TRRD_2                    (0x2 << 20) // (SDDRC) Value :  2
#define 	AT91C_TRRD_3                    (0x3 << 20) // (SDDRC) Value :  3
#define 	AT91C_TRRD_4                    (0x4 << 20) // (SDDRC) Value :  4
#define 	AT91C_TRRD_5                    (0x5 << 20) // (SDDRC) Value :  5
#define 	AT91C_TRRD_6                    (0x6 << 20) // (SDDRC) Value :  6
#define 	AT91C_TRRD_7                    (0x7 << 20) // (SDDRC) Value :  7
#define 	AT91C_TRRD_8                    (0x8 << 20) // (SDDRC) Value :  8
#define 	AT91C_TRRD_9                    (0x9 << 20) // (SDDRC) Value :  9
#define 	AT91C_TRRD_10                   (0xA << 20) // (SDDRC) Value : 10
#define 	AT91C_TRRD_11                   (0xB << 20) // (SDDRC) Value : 11
#define 	AT91C_TRRD_12                   (0xC << 20) // (SDDRC) Value : 12
#define 	AT91C_TRRD_13                   (0xD << 20) // (SDDRC) Value : 13
#define 	AT91C_TRRD_14                   (0xE << 20) // (SDDRC) Value : 14
#define 	AT91C_TRRD_15                   (0xF << 20) // (SDDRC) Value : 15
#define AT91C_TWTR                (0x1 << 24) // (SDDRC)
#define 	AT91C_TWTR_0                    (0x0 << 24) // (SDDRC) Value :  0
#define 	AT91C_TWTR_1                    (0x1 << 24) // (SDDRC) Value :  1
#define AT91C_TMRD                (0xF << 28) // (SDDRC)
#define 	AT91C_TMRD_0                    (0x0 << 28) // (SDDRC) Value :  0
#define 	AT91C_TMRD_1                    (0x1 << 28) // (SDDRC) Value :  1
#define 	AT91C_TMRD_2                    (0x2 << 28) // (SDDRC) Value :  2
#define 	AT91C_TMRD_3                    (0x3 << 28) // (SDDRC) Value :  3
#define 	AT91C_TMRD_4                    (0x4 << 28) // (SDDRC) Value :  4
#define 	AT91C_TMRD_5                    (0x5 << 28) // (SDDRC) Value :  5
#define 	AT91C_TMRD_6                    (0x6 << 28) // (SDDRC) Value :  6
#define 	AT91C_TMRD_7                    (0x7 << 28) // (SDDRC) Value :  7
#define 	AT91C_TMRD_8                    (0x8 << 28) // (SDDRC) Value :  8
#define 	AT91C_TMRD_9                    (0x9 << 28) // (SDDRC) Value :  9
#define 	AT91C_TMRD_10                   (0xA << 28) // (SDDRC) Value : 10
#define 	AT91C_TMRD_11                   (0xB << 28) // (SDDRC) Value : 11
#define 	AT91C_TMRD_12                   (0xC << 28) // (SDDRC) Value : 12
#define 	AT91C_TMRD_13                   (0xD << 28) // (SDDRC) Value : 13
#define 	AT91C_TMRD_14                   (0xE << 28) // (SDDRC) Value : 14
#define 	AT91C_TMRD_15                   (0xF << 28) // (SDDRC) Value : 15
// -------- SDDRC_T1PR : (SDDRC Offset: 0x10)  --------
#define AT91C_TRFC                (0x1F <<  0) // (SDDRC)
#define 	AT91C_TRFC_0                    (0x0) // (SDDRC) Value :  0
#define 	AT91C_TRFC_1                    (0x1) // (SDDRC) Value :  1
#define 	AT91C_TRFC_2                    (0x2) // (SDDRC) Value :  2
#define 	AT91C_TRFC_3                    (0x3) // (SDDRC) Value :  3
#define 	AT91C_TRFC_4                    (0x4) // (SDDRC) Value :  4
#define 	AT91C_TRFC_5                    (0x5) // (SDDRC) Value :  5
#define 	AT91C_TRFC_6                    (0x6) // (SDDRC) Value :  6
#define 	AT91C_TRFC_7                    (0x7) // (SDDRC) Value :  7
#define 	AT91C_TRFC_8                    (0x8) // (SDDRC) Value :  8
#define 	AT91C_TRFC_9                    (0x9) // (SDDRC) Value :  9
#define 	AT91C_TRFC_10                   (0xA) // (SDDRC) Value : 10
#define 	AT91C_TRFC_11                   (0xB) // (SDDRC) Value : 11
#define 	AT91C_TRFC_12                   (0xC) // (SDDRC) Value : 12
#define 	AT91C_TRFC_13                   (0xD) // (SDDRC) Value : 13
#define 	AT91C_TRFC_14                   (0xE) // (SDDRC) Value : 14
#define 	AT91C_TRFC_15                   (0xF) // (SDDRC) Value : 15
#define AT91C_TXSNR               (0xFF <<  8) // (SDDRC)
#define 	AT91C_TXSNR_0                    (0x0 <<  8) // (SDDRC) Value :  0
#define 	AT91C_TXSNR_1                    (0x1 <<  8) // (SDDRC) Value :  1
#define 	AT91C_TXSNR_2                    (0x2 <<  8) // (SDDRC) Value :  2
#define 	AT91C_TXSNR_3                    (0x3 <<  8) // (SDDRC) Value :  3
#define 	AT91C_TXSNR_4                    (0x4 <<  8) // (SDDRC) Value :  4
#define 	AT91C_TXSNR_5                    (0x5 <<  8) // (SDDRC) Value :  5
#define 	AT91C_TXSNR_6                    (0x6 <<  8) // (SDDRC) Value :  6
#define 	AT91C_TXSNR_7                    (0x7 <<  8) // (SDDRC) Value :  7
#define 	AT91C_TXSNR_8                    (0x8 <<  8) // (SDDRC) Value :  8
#define 	AT91C_TXSNR_9                    (0x9 <<  8) // (SDDRC) Value :  9
#define 	AT91C_TXSNR_10                   (0xA <<  8) // (SDDRC) Value : 10
#define 	AT91C_TXSNR_11                   (0xB <<  8) // (SDDRC) Value : 11
#define 	AT91C_TXSNR_12                   (0xC <<  8) // (SDDRC) Value : 12
#define 	AT91C_TXSNR_13                   (0xD <<  8) // (SDDRC) Value : 13
#define 	AT91C_TXSNR_14                   (0xE <<  8) // (SDDRC) Value : 14
#define 	AT91C_TXSNR_15                   (0xF <<  8) // (SDDRC) Value : 15
#define AT91C_TXSRD               (0xFF << 16) // (SDDRC)
#define 	AT91C_TXSRD_0                    (0x0 << 16) // (SDDRC) Value :  0
#define 	AT91C_TXSRD_1                    (0x1 << 16) // (SDDRC) Value :  1
#define 	AT91C_TXSRD_2                    (0x2 << 16) // (SDDRC) Value :  2
#define 	AT91C_TXSRD_3                    (0x3 << 16) // (SDDRC) Value :  3
#define 	AT91C_TXSRD_4                    (0x4 << 16) // (SDDRC) Value :  4
#define 	AT91C_TXSRD_5                    (0x5 << 16) // (SDDRC) Value :  5
#define 	AT91C_TXSRD_6                    (0x6 << 16) // (SDDRC) Value :  6
#define 	AT91C_TXSRD_7                    (0x7 << 16) // (SDDRC) Value :  7
#define 	AT91C_TXSRD_8                    (0x8 << 16) // (SDDRC) Value :  8
#define 	AT91C_TXSRD_9                    (0x9 << 16) // (SDDRC) Value :  9
#define 	AT91C_TXSRD_10                   (0xA << 16) // (SDDRC) Value : 10
#define 	AT91C_TXSRD_11                   (0xB << 16) // (SDDRC) Value : 11
#define 	AT91C_TXSRD_12                   (0xC << 16) // (SDDRC) Value : 12
#define 	AT91C_TXSRD_13                   (0xD << 16) // (SDDRC) Value : 13
#define 	AT91C_TXSRD_14                   (0xE << 16) // (SDDRC) Value : 14
#define 	AT91C_TXSRD_15                   (0xF << 16) // (SDDRC) Value : 15
// -------- SDDRC_HS : (SDDRC Offset: 0x14)  --------
#define AT91C_DA                  (0x1 <<  0) // (SDDRC)
#define AT91C_OVL                 (0x1 <<  1) // (SDDRC)
// -------- SDDRC_LPR : (SDDRC Offset: 0x18)  --------
#define AT91C_LPCB                (0x3 <<  0) // (SDDRC)
#define AT91C_PASR                (0x7 <<  4) // (SDDRC)
#define AT91C_LP_TRC              (0x3 <<  8) // (SDDRC)
#define AT91C_DS                  (0x3 << 10) // (SDDRC)
#define AT91C_TIMEOUT             (0x3 << 12) // (SDDRC)
// -------- SDDRC_MDR : (SDDRC Offset: 0x1c)  --------
#define AT91C_MD                  (0x3 <<  0) // (SDDRC)
#define 	AT91C_MD_SDR_SDRAM            (0x0) // (SDDRC) SDR_SDRAM
#define 	AT91C_MD_LP_SDR_SDRAM         (0x1) // (SDDRC) Low Power SDR_SDRAM
#define 	AT91C_MD_DDR_SDRAM            (0x2) // (SDDRC) DDR_SDRAM
#define 	AT91C_MD_LP_DDR_SDRAM         (0x3) // (SDDRC) Low Power DDR_SDRAM
#define AT91C_B16MODE             (0x1 <<  4) // (SDDRC)
#define 	AT91C_B16MODE_32_BITS              (0x0 <<  4) // (SDDRC) 32 Bits datas bus
#define 	AT91C_B16MODE_16_BITS              (0x1 <<  4) // (SDDRC) 16 Bits datas bus

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
//              SOFTWARE API DEFINITION  FOR Slave Priority Registers
// *****************************************************************************
// *** Register offset in AT91S_MATRIX_PRS structure ***
#define MATRIX_PRAS     ( 0) //  Slave Priority Registers A for Slave
#define MATRIX_PRBS     ( 4) //  Slave Priority Registers B for Slave

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR AHB Matrix Interface
// *****************************************************************************
// *** Register offset in AT91S_MATRIX structure ***
#define MATRIX_MCFG     ( 0) //  Master Configuration Register
#define MATRIX_SCFG     (64) //  Slave Configuration Register
#define MATRIX_PRS      (128) //  Slave Priority Registers
#define MATRIX_MRCR     (256) //  Master Remp Control Register
// -------- MATRIX_MCFG : (MATRIX Offset: 0x0) Master Configuration Register rom --------
#define AT91C_MATRIX_ULBT         (0x7 <<  0) // (MATRIX) Undefined Length Burst Type
// -------- MATRIX_SCFG : (MATRIX Offset: 0x40) Slave Configuration Register --------
#define AT91C_MATRIX_SLOT_CYCLE   (0xFF <<  0) // (MATRIX) Maximum Number of Allowed Cycles for a Burst
#define AT91C_MATRIX_DEFMSTR_TYPE (0x3 << 16) // (MATRIX) Default Master Type
#define 	AT91C_MATRIX_DEFMSTR_TYPE_NO_DEFMSTR           (0x0 << 16) // (MATRIX) No Default Master. At the end of current slave access, if no other master request is pending, the slave is deconnected from all masters. This results in having a one cycle latency for the first transfer of a burst.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_LAST_DEFMSTR         (0x1 << 16) // (MATRIX) Last Default Master. At the end of current slave access, if no other master request is pending, the slave stay connected with the last master having accessed it. This results in not having the one cycle latency when the last master re-trying access on the slave.
#define 	AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR        (0x2 << 16) // (MATRIX) Fixed Default Master. At the end of current slave access, if no other master request is pending, the slave connects with fixed which number is in FIXED_DEFMSTR field. This results in not having the one cycle latency when the fixed master re-trying access on the slave.
#define AT91C_MATRIX_FIXED_DEFMSTR (0x7 << 18) // (MATRIX) Fixed Index of Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_ARM926I              (0x0 << 18) // (MATRIX) ARM926EJ-S Instruction Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_ARM926D              (0x1 << 18) // (MATRIX) ARM926EJ-S Data Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_PDC                  (0x2 << 18) // (MATRIX) PDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_LCDC                 (0x3 << 18) // (MATRIX) LCDC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_2DGC                 (0x4 << 18) // (MATRIX) 2DGC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_ISI                  (0x5 << 18) // (MATRIX) ISI Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_DMA                  (0x6 << 18) // (MATRIX) DMA Controller Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_EMAC                 (0x7 << 18) // (MATRIX) EMAC Master is Default Master
#define 	AT91C_MATRIX_FIXED_DEFMSTR_USB                  (0x8 << 18) // (MATRIX) USB Master is Default Master
#define AT91C_MATRIX_ARBT         (0x3 << 24) // (MATRIX) Arbitration Type
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
#define AT91C_MATRIX_RCB9         (0x1 <<  9) // (MATRIX) Remap Command Bit for USB
#define AT91C_MATRIX_RCB10        (0x1 << 10) // (MATRIX) Remap Command Bit for USB
#define AT91C_MATRIX_RCB11        (0x1 << 11) // (MATRIX) Remap Command Bit for USB

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR AHB CCFG Interface
// *****************************************************************************
// *** Register offset in AT91S_CCFG structure ***
#define CCFG_MPBS0      ( 4) //  Slave 1 (MP Block Slave 0) Special Function Register
#define CCFG_UDPHS      ( 8) //  Slave 2 (AHB Periphs) Special Function Register
#define CCFG_MPBS1      (12) //  Slave 3 (MP Block Slave 1) Special Function Register
#define CCFG_EBICSA     (16) //  EBI Chip Select Assignement Register
#define CCFG_MPBS2      (28) //  Slave 7 (MP Block Slave 2) Special Function Register
#define CCFG_MPBS3      (32) //  Slave 7 (MP Block Slave 3) Special Function Register
#define CCFG_BRIDGE     (36) //  Slave 8 (APB Bridge) Special Function Register
#define CCFG_MATRIXVERSION (236) //  Version Register
// -------- CCFG_UDPHS : (CCFG Offset: 0x8) UDPHS Configuration --------
#define AT91C_CCFG_UDPHS_UDP_SELECT (0x1 << 31) // (CCFG) UDPHS or UDP Selection
#define 	AT91C_CCFG_UDPHS_UDP_SELECT_UDPHS                (0x0 << 31) // (CCFG) UDPHS Selected.
#define 	AT91C_CCFG_UDPHS_UDP_SELECT_UDP                  (0x1 << 31) // (CCFG) UDP Selected.
// -------- CCFG_EBICSA : (CCFG Offset: 0x10) EBI Chip Select Assignement Register --------
#define AT91C_EBI_CS1A            (0x1 <<  1) // (CCFG) Chip Select 1 Assignment
#define 	AT91C_EBI_CS1A_SMC                  (0x0 <<  1) // (CCFG) Chip Select 1 is assigned to the Static Memory Controller.
#define 	AT91C_EBI_CS1A_BCRAMC               (0x1 <<  1) // (CCFG) Chip Select 1 is assigned to the BCRAM Controller.
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
#define AT91C_EBI_DDRPUC          (0x1 <<  9) // (CCFG) DDDR DQS Pull-up Configuration
#define AT91C_EBI_SUP             (0x1 << 16) // (CCFG) EBI Supply
#define 	AT91C_EBI_SUP_1V8                  (0x0 << 16) // (CCFG) EBI Supply is 1.8V
#define 	AT91C_EBI_SUP_3V3                  (0x1 << 16) // (CCFG) EBI Supply is 3.3V
#define AT91C_EBI_LP              (0x1 << 17) // (CCFG) EBI Low Power Reduction
#define 	AT91C_EBI_LP_LOW_DRIVE            (0x0 << 17) // (CCFG) EBI Pads are in Standard drive
#define 	AT91C_EBI_LP_STD_DRIVE            (0x1 << 17) // (CCFG) EBI Pads are in Low Drive (Low Power)
#define AT91C_CCFG_DDR_SDR_SELECT (0x1 << 31) // (CCFG) DDR or SDR Selection
#define 	AT91C_CCFG_DDR_SDR_SELECT_DDR                  (0x0 << 31) // (CCFG) DDR Selected.
#define 	AT91C_CCFG_DDR_SDR_SELECT_SDR                  (0x1 << 31) // (CCFG) SDR Selected.
// -------- CCFG_BRIDGE : (CCFG Offset: 0x24) BRIDGE Configuration --------
#define AT91C_CCFG_AES_TDES_SELECT (0x1 << 31) // (CCFG) AES or TDES Selection
#define 	AT91C_CCFG_AES_TDES_SELECT_AES                  (0x0 << 31) // (CCFG) AES Selected.
#define 	AT91C_CCFG_AES_TDES_SELECT_TDES                 (0x1 << 31) // (CCFG) TDES Selected.

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
#define AT91C_CKGR_UPLLEN         (0x1 << 16) // (CKGR) UTMI PLL Enable
#define 	AT91C_CKGR_UPLLEN_DISABLED             (0x0 << 16) // (CKGR) The UTMI PLL is disabled
#define 	AT91C_CKGR_UPLLEN_ENABLED              (0x1 << 16) // (CKGR) The UTMI PLL is enabled
#define AT91C_CKGR_PLLCOUNT       (0xF << 20) // (CKGR) UTMI Oscillator Start-up Time
#define AT91C_CKGR_BIASEN         (0x1 << 24) // (CKGR) UTMI BIAS Enable
#define 	AT91C_CKGR_BIASEN_DISABLED             (0x0 << 24) // (CKGR) The UTMI BIAS is disabled
#define 	AT91C_CKGR_BIASEN_ENABLED              (0x1 << 24) // (CKGR) The UTMI BIAS is enabled
#define AT91C_CKGR_BIASCOUNT      (0xF << 28) // (CKGR) UTMI BIAS Start-up Time
// -------- CKGR_MOR : (CKGR Offset: 0x4) Main Oscillator Register --------
#define AT91C_CKGR_MOSCEN         (0x1 <<  0) // (CKGR) Main Oscillator Enable
#define AT91C_CKGR_OSCBYPASS      (0x1 <<  1) // (CKGR) Main Oscillator Bypass
#define AT91C_CKGR_OSCOUNT        (0xFF <<  8) // (CKGR) Main Oscillator Start-up Time
// -------- CKGR_MCFR : (CKGR Offset: 0x8) Main Clock Frequency Register --------
#define AT91C_CKGR_MAINF          (0xFFFF <<  0) // (CKGR) Main Clock Frequency
#define AT91C_CKGR_MAINRDY        (0x1 << 16) // (CKGR) Main Clock Ready
// -------- CKGR_PLLAR : (CKGR Offset: 0xc) PLL A Register --------
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
// -------- CKGR_PLLBR : (CKGR Offset: 0x10) PLL B Register --------
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
#define AT91C_PMC_LOCKU           (0x1 <<  6) // (PMC) PLL UTMI Status/Enable/Disable/Mask
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
#define AT91C_UDP_PUON            (0x1 <<  9) // (UDP) Pull-up ON

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB Enpoint FIFO data register
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
//              SOFTWARE API DEFINITION  FOR USB Endpoint struct
// *****************************************************************************
// *** Register offset in AT91S_UDPHS_EPT structure ***
#define UDPHS_EPTCFG    ( 0) // Endpoint Config Register
#define UDPHS_EPTCTLENB ( 4) // Endpoint Control Enable Register
#define UDPHS_EPTCTLDIS ( 8) // Endpoint Control Disable Register
#define UDPHS_EPTCTL    (12) // Endpoint Control Register
#define UDPHS_EPTSETSTA (20) // Endpoint Set Status Register
#define UDPHS_EPTCLRSTA (24) // Endpoint Clear Status Register
#define UDPHS_EPTSTA    (28) // Endpoint Status Register
// -------- UDPHS_EPTCFG : (UDPHS_EPT Offset: 0x0) Endpoint Config Register --------
#define AT91C_EPT_SIZE            (0x1 <<  0) // (UDPHS_EPT) Endpoint Size
#define 	AT91C_EPT_SIZE_8                    (0x0) // (UDPHS_EPT)    8 bytes
#define 	AT91C_EPT_SIZE_16                   (0x1) // (UDPHS_EPT)   16 bytes
#define 	AT91C_EPT_SIZE_32                   (0x2) // (UDPHS_EPT)   32 bytes
#define 	AT91C_EPT_SIZE_64                   (0x3) // (UDPHS_EPT)   64 bytes
#define 	AT91C_EPT_SIZE_128                  (0x4) // (UDPHS_EPT)  128 bytes
#define 	AT91C_EPT_SIZE_256                  (0x5) // (UDPHS_EPT)  256 bytes
#define 	AT91C_EPT_SIZE_512                  (0x6) // (UDPHS_EPT)  512 bytes
#define 	AT91C_EPT_SIZE_1024                 (0x7) // (UDPHS_EPT) 1024 bytes
#define AT91C_EPT_DIR             (0x1 <<  3) // (UDPHS_EPT) Endpoint Direction 0:OUT, 1:IN
#define 	AT91C_EPT_DIR_OUT                  (0x0 <<  3) // (UDPHS_EPT) Direction OUT
#define 	AT91C_EPT_DIR_IN                   (0x1 <<  3) // (UDPHS_EPT) Direction IN
#define AT91C_EPT_TYPE            (0x1 <<  4) // (UDPHS_EPT) Endpoint Type
#define 	AT91C_EPT_TYPE_CTL_EPT              (0x0 <<  4) // (UDPHS_EPT) Control endpoint
#define 	AT91C_EPT_TYPE_ISO_EPT              (0x1 <<  4) // (UDPHS_EPT) Isochronous endpoint
#define 	AT91C_EPT_TYPE_BUL_EPT              (0x2 <<  4) // (UDPHS_EPT) Bulk endpoint
#define 	AT91C_EPT_TYPE_INT_EPT              (0x3 <<  4) // (UDPHS_EPT) Interrupt endpoint
#define AT91C_BK_NUMBER           (0x1 <<  6) // (UDPHS_EPT) Number of Banks
#define 	AT91C_BK_NUMBER_0                    (0x0 <<  6) // (UDPHS_EPT) Zero Bank, the EndPoint is not mapped in memory
#define 	AT91C_BK_NUMBER_1                    (0x1 <<  6) // (UDPHS_EPT) One Bank (Bank0)
#define 	AT91C_BK_NUMBER_2                    (0x2 <<  6) // (UDPHS_EPT) Double bank (Ping-Pong : Bank0 / Bank1)
#define 	AT91C_BK_NUMBER_3                    (0x3 <<  6) // (UDPHS_EPT) Triple Bank (Bank0 / Bank1 / Bank2)
#define AT91C_NB_TRANS            (0x1 <<  8) // (UDPHS_EPT) Number Of Transaction per Micro-Frame (High-Bandwidth iso only)
#define AT91C_EPT_MAPPED          (0x1 << 31) // (UDPHS_EPT) Endpoint Mapped (read only
// -------- UDPHS_EPTCTLENB : (UDPHS_EPT Offset: 0x4) Endpoint Control Enable Register --------
#define AT91C_EPT_ENABLE          (0x1 <<  0) // (UDPHS_EPT) Endpoint Enable
#define AT91C_AUTO_VALID          (0x1 <<  1) // (UDPHS_EPT) Packet Auto-Valid Enable/Disable
#define AT91C_INT_DIS_DMA         (0x1 <<  3) // (UDPHS_EPT) Endpoint Interrupts DMA Request Enable/Disable
#define AT91C_NYET_DIS            (0x1 <<  4) // (UDPHS_EPT) NO NYET Enable/Disable
#define AT91C_DATAX_RX            (0x1 <<  6) // (UDPHS_EPT) DATAx Interrupt Enable/Disable
#define AT91C_MDATA_RX            (0x1 <<  7) // (UDPHS_EPT) MDATA Interrupt Enabled/Disable
#define AT91C_OVERFLOW_ERROR      (0x1 <<  8) // (UDPHS_EPT) OverFlow Error Interrupt Enable/Disable/Status
#define AT91C_RX_BK_RDY           (0x1 <<  9) // (UDPHS_EPT) Received OUT Data Interrupt Enable/Clear/Disable
#define AT91C_TX_COMPLETE         (0x1 << 10) // (UDPHS_EPT) Transmitted IN Data Complete Interrupt Enable/Disable or Transmitted IN Data Complete (clear)
#define AT91C_TX_BK_RDY_ERROR_TRANS (0x1 << 11) // (UDPHS_EPT) TX Packet Ready / Transaction Error / Interrupt Enable/Disable
#define AT91C_RX_SETUP_ERROR_FLOW_ISO (0x1 << 12) // (UDPHS_EPT) Received SETUP / Error Flow Clear/Interrupt Enable/Disable
#define AT91C_STALL_SENT_ERROR_CRC_ISO (0x1 << 13) // (UDPHS_EPT) Stall Sent / CRC error / Interrupt Enable/Disable
#define AT91C_NAK_IN              (0x1 << 14) // (UDPHS_EPT) NAKIN / Clear / Interrupt Enable/Disable
#define AT91C_NAK_OUT             (0x1 << 15) // (UDPHS_EPT) NAKOUT / Clear / Interrupt Enable/Disable
#define AT91C_BUSY_BANK           (0x1 << 18) // (UDPHS_EPT) Busy Bank Interrupt Enable/Disable
#define AT91C_SHORT_PACKET        (0x1 << 31) // (UDPHS_EPT) Short Packet / Interrupt Enable/Disable
// -------- UDPHS_EPTCTLDIS : (UDPHS_EPT Offset: 0x8) Endpoint Control Disable Register --------
#define AT91C_EPT_DISABLE         (0x1 <<  0) // (UDPHS_EPT) Endpoint Disable
// -------- UDPHS_EPTCTL : (UDPHS_EPT Offset: 0xc) Endpoint Control Register --------
// -------- UDPHS_EPTSETSTA : (UDPHS_EPT Offset: 0x14) Endpoint Set Status Register --------
#define AT91C_FORCE_STALL         (0x1 <<  5) // (UDPHS_EPT) Stall Handshake Request Set/Clear/Status
#define AT91C_KILL_BANK           (0x1 <<  9) // (UDPHS_EPT) KILL Bank Set (For IN EndPoint)
#define AT91C_TX_BK_RDY           (0x1 << 11) // (UDPHS_EPT) TX Packet Ready Set
// -------- UDPHS_EPTCLRSTA : (UDPHS_EPT Offset: 0x18) Endpoint Clear Status Register --------
#define AT91C_TOGGLE_SEQ          (0x1 <<  6) // (UDPHS_EPT) Data Toggle Clear
#define AT91C_STALL_SENT          (0x1 << 13) // (UDPHS_EPT) Stall Sent Clear
// -------- UDPHS_EPTSTA : (UDPHS_EPT Offset: 0x1c) Endpoint Status Register --------
#define AT91C_TOGGLE_SEQ_STA      (0x3 <<  6) // (UDPHS_EPT) Toggle Sequencing
#define 	AT91C_TOGGLE_SEQ_STA_00                   (0x0 <<  6) // (UDPHS_EPT) Data0
#define 	AT91C_TOGGLE_SEQ_STA_01                   (0x1 <<  6) // (UDPHS_EPT) Data1
#define 	AT91C_TOGGLE_SEQ_STA_10                   (0x2 <<  6) // (UDPHS_EPT) Data2 (only for High-Bandwidth Isochronous EndPoint)
#define 	AT91C_TOGGLE_SEQ_STA_11                   (0x3 <<  6) // (UDPHS_EPT) MData (only for High-Bandwidth Isochronous EndPoint)
#define AT91C_RX_BK_RDY_KILL_BANK (0x1 <<  9) // (UDPHS_EPT) Received OUT Data / KILL Bank
#define AT91C_CURRENT_BANK_CONTROL_DIR (0x3 << 16) // (UDPHS_EPT)
#define 	AT91C_CURRENT_BANK_CONTROL_DIR_00                   (0x0 << 16) // (UDPHS_EPT) Bank 0
#define 	AT91C_CURRENT_BANK_CONTROL_DIR_01                   (0x1 << 16) // (UDPHS_EPT) Bank 1
#define 	AT91C_CURRENT_BANK_CONTROL_DIR_10                   (0x2 << 16) // (UDPHS_EPT) Bank 2
#define 	AT91C_CURRENT_BANK_CONTROL_DIR_11                   (0x3 << 16) // (UDPHS_EPT) Invalid
#define AT91C_BUSY_BANK_STA       (0x3 << 18) // (UDPHS_EPT) Busy Bank Number
#define 	AT91C_BUSY_BANK_STA_00                   (0x0 << 18) // (UDPHS_EPT) All banks are free
#define 	AT91C_BUSY_BANK_STA_01                   (0x1 << 18) // (UDPHS_EPT) 1 busy bank
#define 	AT91C_BUSY_BANK_STA_10                   (0x2 << 18) // (UDPHS_EPT) 2 busy banks
#define 	AT91C_BUSY_BANK_STA_11                   (0x3 << 18) // (UDPHS_EPT) 3 busy banks
#define AT91C_BYTE_COUNT          (0x7FF << 20) // (UDPHS_EPT) USB Byte Count

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB DMA struct
// *****************************************************************************
// *** Register offset in AT91S_UDPHS_DMA structure ***
#define UDPHS_DMANXTDSC ( 0) // DMA Channel Next Descriptor Address
#define UDPHS_DMAADDRESS ( 4) // DMA Channel AHB1 Address Register
#define UDPHS_DMACONTROL ( 8) // DMA Channel Control Register
#define UDPHS_DMASTATUS (12) // DMA Channel Status Register
// -------- UDPHS_DMANXTDSC : (UDPHS_DMA Offset: 0x0) Next Descriptor Address Register --------
#define AT91C_NEXT_DESCRIPTOR_ADDRESS (0xFFFFFFF <<  4) // (UDPHS_DMA) next Channel Descriptor
// -------- UDPHS_DMAADDRESS : (UDPHS_DMA Offset: 0x4) DMA Channel AHB1 Address Register --------
#define AT91C_BUFFER_AHB1_ADDRESS (0x0 <<  0) // (UDPHS_DMA) starting address of a DMA Channel transfer
// -------- UDPHS_DMACONTROL : (UDPHS_DMA Offset: 0x8) DMA Channel Control Register --------
#define AT91C_CHANNEL_ENABLE      (0x1 <<  0) // (UDPHS_DMA) Channel Enable
#define AT91C_LOAD_NEXT_CHANNEL_TRANSFER_DESCRIPTOR_ENABLE (0x1 <<  1) // (UDPHS_DMA) Load Next Channel Transfer Descriptor Enable
#define AT91C_BUFFER_CLOSE_INPUT_ENABLE (0x1 <<  2) // (UDPHS_DMA) Buffer Close Input Enable
#define AT91C_END_OF_DMA_BUFFER_OUTPUT_ENABLE (0x1 <<  3) // (UDPHS_DMA) End of DMA Buffer Output Enable
#define AT91C_UDPHS_END_OF_TRANSFER_INTERRUPT_ENABLE (0x1 <<  4) // (UDPHS_DMA) USB End Of Transfer Interrupt Enable
#define AT91C_END_OF_CHANNEL_BUFFER_INTERRUPT_ENABLE (0x1 <<  5) // (UDPHS_DMA) End Of Channel Buffer Interrupt Enable
#define AT91C_DESCRIPTOR_LOADED_INTERRUPT_ENABLE (0x1 <<  6) // (UDPHS_DMA) Descriptor Loaded Interrupt Enable
#define AT91C_BURST_LOCK_ENABLE   (0x1 <<  7) // (UDPHS_DMA) Burst Lock Enable
#define AT91C_BUFFER_BYTE_LENGTH  (0xFFFF << 16) // (UDPHS_DMA) Buffer Byte Length (write only)
// -------- UDPHS_DMASTATUS : (UDPHS_DMA Offset: 0xc) DMA Channelx Status Register --------
#define AT91C_CHANNEL_ENABLED     (0x1 <<  0) // (UDPHS_DMA) Channel Enabled
#define AT91C_CHANNEL_ACTIVE      (0x1 <<  1) // (UDPHS_DMA)
#define AT91C_UDPHS_END_OF_CHANNEL_TRANSFER_STATUS (0x1 <<  4) // (UDPHS_DMA)
#define AT91C_END_OF_CHANNEL_BUFFER_STATUS (0x1 <<  5) // (UDPHS_DMA)
#define AT91C_DESCRIPTOR_LOADED_STATUS (0x1 <<  6) // (UDPHS_DMA)
#define AT91C_BUFFER_BYTE_COUNT   (0xFFFF << 16) // (UDPHS_DMA)

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR USB High Speed Device Interface
// *****************************************************************************
// *** Register offset in AT91S_UDPHS structure ***
#define UDPHS_CTRL      ( 0) // USB Control Register
#define UDPHS_FNUM      ( 4) // USB Frame Number Register
#define UDPHS_IEN       (16) // USB Interrupt Enable Register
#define UDPHS_INTSTA    (20) // USB Interrupt Status Register
#define UDPHS_CLRINT    (24) // USB Clear Interrupt Register
#define UDPHS_EPTRST    (28) // USB Endpoints Reset Register
#define UDPHS_TSTCNTA   (212) // USB Test CounterA Register
#define UDPHS_TSTCNTB   (216) // USB Test CounterB Register
#define UDPHS_TST       (224) // USB Test Register
#define UDPHS_IPPADDRSIZE (236) // HUSB2DEV PADDRSIZE Register
#define UDPHS_IPNAME1   (240) // HUSB2DEV Name1 Register
#define UDPHS_IPNAME2   (244) // HUSB2DEV Name2 Register
#define UDPHS_IPFEATURES (248) // HUSB2DEV Features Register
#define UDPHS_IPVERSION (252) // HUSB2DEV Version Register
#define UDPHS_EPT       (256) // USB Endpoint struct
#define UDPHS_DMA       (768) // USB DMA channel struct (not use [0])
// -------- UDPHS_CTRL : (UDPHS Offset: 0x0) USB Control Register --------
#define AT91C_DEV_ADDR            (0x7F <<  0) // (UDPHS) USB Address
#define AT91C_FADDR_EN            (0x1 <<  7) // (UDPHS) Function Address Enable
#define AT91C_EN_USB              (0x1 <<  8) // (UDPHS) USB Enable
#define AT91C_DETACH              (0x1 <<  9) // (UDPHS) Detach Command
#define AT91C_REMOTE_WAKE_UP      (0x1 << 10) // (UDPHS) Send Remote Wake Up
#define AT91C_PULLDOWN_DIS        (0x1 << 11) // (UDPHS) PullDown Disable
// -------- UDPHS_FNUM : (UDPHS Offset: 0x4) USB Frame Number Register --------
#define AT91C_MICRO_FRAME_NUM     (0x7 <<  0) // (UDPHS) Micro Frame Number
#define AT91C_FRAME_NUMBER        (0x7FF <<  3) // (UDPHS) Frame Number as defined in the Packet Field Formats
#define AT91C_FRAME_NUM_ERROR     (0x1 << 31) // (UDPHS) Frame Number CRC Error
// -------- UDPHS_IEN : (UDPHS Offset: 0x10) USB Interrupt Enable Register --------
#define AT91C_DET_SUSPEND         (0x1 <<  1) // (UDPHS) Suspend Interrupt Enable/Clear/Status
#define AT91C_MICRO_SOF           (0x1 <<  2) // (UDPHS) Micro-SOF Interrupt Enable/Clear/Status
#define AT91C_IEN_SOF             (0x1 <<  3) // (UDPHS) SOF Interrupt Enable/Clear/Status
#define AT91C_END_OF_RESET        (0x1 <<  4) // (UDPHS) End Of Reset Interrupt Enable/Clear/Status
#define AT91C_WAKE_UP             (0x1 <<  5) // (UDPHS) Wake Up CPU Interrupt Enable/Clear/Status
#define AT91C_END_OF_RESUME       (0x1 <<  6) // (UDPHS) End Of Resume Interrupt Enable/Clear/Status
#define AT91C_UPSTREAM_RESUME     (0x1 <<  7) // (UDPHS) Upstream Resume Interrupt Enable/Clear/Status
#define AT91C_EPT_INT_0           (0x1 <<  8) // (UDPHS) Endpoint 0 Interrupt Enable/Status
#define AT91C_EPT_INT_1           (0x1 <<  9) // (UDPHS) Endpoint 1 Interrupt Enable/Status
#define AT91C_EPT_INT_2           (0x1 << 10) // (UDPHS) Endpoint 2 Interrupt Enable/Status
#define AT91C_EPT_INT_3           (0x1 << 11) // (UDPHS) Endpoint 3 Interrupt Enable/Status
#define AT91C_EPT_INT_4           (0x1 << 12) // (UDPHS) Endpoint 4 Interrupt Enable/Status
#define AT91C_EPT_INT_5           (0x1 << 13) // (UDPHS) Endpoint 5 Interrupt Enable/Status
#define AT91C_EPT_INT_6           (0x1 << 14) // (UDPHS) Endpoint 6 Interrupt Enable/Status
#define AT91C_EPT_INT_7           (0x1 << 15) // (UDPHS) Endpoint 7 Interrupt Enable/Status
#define AT91C_EPT_INT_8           (0x1 << 16) // (UDPHS) Endpoint 8 Interrupt Enable/Status
#define AT91C_EPT_INT_9           (0x1 << 17) // (UDPHS) Endpoint 9 Interrupt Enable/Status
#define AT91C_EPT_INT_10          (0x1 << 18) // (UDPHS) Endpoint 10 Interrupt Enable/Status
#define AT91C_EPT_INT_11          (0x1 << 19) // (UDPHS) Endpoint 11 Interrupt Enable/Status
#define AT91C_EPT_INT_12          (0x1 << 20) // (UDPHS) Endpoint 12 Interrupt Enable/Status
#define AT91C_EPT_INT_13          (0x1 << 21) // (UDPHS) Endpoint 13 Interrupt Enable/Status
#define AT91C_EPT_INT_14          (0x1 << 22) // (UDPHS) Endpoint 14 Interrupt Enable/Status
#define AT91C_EPT_INT_15          (0x1 << 23) // (UDPHS) Endpoint 15 Interrupt Enable/Status
#define AT91C_DMA_INT_1           (0x1 << 25) // (UDPHS) DMA Channel 1 Interrupt Enable/Status
#define AT91C_DMA_INT_2           (0x1 << 26) // (UDPHS) DMA Channel 2 Interrupt Enable/Status
#define AT91C_DMA_INT_3           (0x1 << 27) // (UDPHS) DMA Channel 3 Interrupt Enable/Status
#define AT91C_DMA_INT_4           (0x1 << 28) // (UDPHS) DMA Channel 4 Interrupt Enable/Status
#define AT91C_DMA_INT_5           (0x1 << 29) // (UDPHS) DMA Channel 5 Interrupt Enable/Status
#define AT91C_DMA_INT_6           (0x1 << 30) // (UDPHS) DMA Channel 6 Interrupt Enable/Status
#define AT91C_DMA_INT_7           (0x1 << 31) // (UDPHS) DMA Channel 7 Interrupt Enable/Status
// -------- UDPHS_INTSTA : (UDPHS Offset: 0x14) USB Interrupt Status Register --------
#define AT91C_SPEED               (0x1 <<  0) // (UDPHS) Speed Status
// -------- UDPHS_CLRINT : (UDPHS Offset: 0x18)  --------
// -------- UDPHS_EPTRST : (UDPHS Offset: 0x1c) USB Endpoints Reset Register --------
#define AT91C_RESET_EPT_0         (0x1 <<  0) // (UDPHS) Endpoint Reset 0
#define AT91C_RESET_EPT_1         (0x1 <<  1) // (UDPHS) Endpoint Reset 1
#define AT91C_RESET_EPT_2         (0x1 <<  2) // (UDPHS) Endpoint Reset 2
#define AT91C_RESET_EPT_3         (0x1 <<  3) // (UDPHS) Endpoint Reset 3
#define AT91C_RESET_EPT_4         (0x1 <<  4) // (UDPHS) Endpoint Reset 4
#define AT91C_RESET_EPT_5         (0x1 <<  5) // (UDPHS) Endpoint Reset 5
#define AT91C_RESET_EPT_6         (0x1 <<  6) // (UDPHS) Endpoint Reset 6
#define AT91C_RESET_EPT_7         (0x1 <<  7) // (UDPHS) Endpoint Reset 7
#define AT91C_RESET_EPT_8         (0x1 <<  8) // (UDPHS) Endpoint Reset 8
#define AT91C_RESET_EPT_9         (0x1 <<  9) // (UDPHS) Endpoint Reset 9
#define AT91C_RESET_EPT_10        (0x1 << 10) // (UDPHS) Endpoint Reset 10
#define AT91C_RESET_EPT_11        (0x1 << 11) // (UDPHS) Endpoint Reset 11
#define AT91C_RESET_EPT_12        (0x1 << 12) // (UDPHS) Endpoint Reset 12
#define AT91C_RESET_EPT_13        (0x1 << 13) // (UDPHS) Endpoint Reset 13
#define AT91C_RESET_EPT_14        (0x1 << 14) // (UDPHS) Endpoint Reset 14
#define AT91C_RESET_EPT_15        (0x1 << 15) // (UDPHS) Endpoint Reset 15
// -------- UDPHS_TST : (UDPHS Offset: 0xe0) USB Test Register --------
#define AT91C_SPEED_CFG           (0x3 <<  0) // (UDPHS) Speed Configuration
#define 	AT91C_SPEED_CFG_NM                   (0x0) // (UDPHS) Normal Mode
#define 	AT91C_SPEED_CFG_RS                   (0x1) // (UDPHS) Reserved
#define 	AT91C_SPEED_CFG_HS                   (0x2) // (UDPHS) Force High Speed
#define 	AT91C_SPEED_CFG_FS                   (0x3) // (UDPHS) Force Full-Speed
#define AT91C_TST_J_MODE          (0x1 <<  2) // (UDPHS) TestJMode
#define AT91C_TST_K_MODE          (0x1 <<  3) // (UDPHS) TestKMode
#define AT91C_TST_PKT_MODE        (0x1 <<  4) // (UDPHS) TestPacketMode
#define AT91C_OPMODE2             (0x1 <<  5) // (UDPHS) OpMode2
// -------- UDPHS_IPPADDRSIZE : (UDPHS Offset: 0xec) HUSB2DEV PADDRSIZE Register --------
#define AT91C_IPPADDRSIZE         (0x0 <<  0) // (UDPHS) 2^HUSB2DEV_PADDR_SIZE
// -------- UDPHS_IPNAME1 : (UDPHS Offset: 0xf0) HUSB2DEV Name Register --------
#define AT91C_IPNAME1             (0x0 <<  0) // (UDPHS) ASCII string HUSB
// -------- UDPHS_IPNAME2 : (UDPHS Offset: 0xf4) HUSB2DEV Name Register --------
#define AT91C_IPNAME2             (0x0 <<  0) // (UDPHS) ASCII string 2DEV
// -------- UDPHS_IPFEATURES : (UDPHS Offset: 0xf8) HUSB2DEV Features Register --------
#define AT91C_EPT_NBR_MAX         (0xF <<  0) // (UDPHS) Max Number of Endpoints
#define AT91C_DMA_CHANNEL_NBR     (0x7 <<  4) // (UDPHS) Number of DMA Channels
#define AT91C_DMA_BUFFER_SIZE     (0x1 <<  7) // (UDPHS) DMA Buffer Size
#define AT91C_DMA_FIFO_WORD_DEPTH (0xF <<  8) // (UDPHS) DMA FIFO Depth in words
#define AT91C_FIFO_MAX_SIZE       (0x7 << 12) // (UDPHS) DPRAM size
#define AT91C_BYTE_WRITE_DPRAM    (0x1 << 15) // (UDPHS) DPRAM byte write capability
#define AT91C_DATA_BUS_16_8       (0x1 << 16) // (UDPHS) UTMI DataBus16_8
#define AT91C_EN_HIGH_BD_ISO_EPT_1 (0x1 << 17) // (UDPHS) Endpoint 1 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_2 (0x1 << 18) // (UDPHS) Endpoint 2 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_3 (0x1 << 19) // (UDPHS) Endpoint 3 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_4 (0x1 << 20) // (UDPHS) Endpoint 4 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_5 (0x1 << 21) // (UDPHS) Endpoint 5 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_6 (0x1 << 22) // (UDPHS) Endpoint 6 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_7 (0x1 << 23) // (UDPHS) Endpoint 7 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_8 (0x1 << 24) // (UDPHS) Endpoint 8 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_9 (0x1 << 25) // (UDPHS) Endpoint 9 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_10 (0x1 << 26) // (UDPHS) Endpoint 10 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_11 (0x1 << 27) // (UDPHS) Endpoint 11 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_12 (0x1 << 28) // (UDPHS) Endpoint 12 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_13 (0x1 << 29) // (UDPHS) Endpoint 13 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_14 (0x1 << 30) // (UDPHS) Endpoint 14 High Bandwidth Isochronous Capability
#define AT91C_EN_HIGH_BD_ISO_EPT_15 (0x1 << 31) // (UDPHS) Endpoint 15 High Bandwidth Isochronous Capability
// -------- UDPHS_IPVERSION : (UDPHS Offset: 0xfc) HUSB2DEV Version Register --------
#define AT91C_VERSION_NUM         (0xFFFF <<  0) // (UDPHS) Give the IP version
#define AT91C_METAL_FIX_NUM       (0x7 << 16) // (UDPHS) Give the number of metal fixes

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
//              SOFTWARE API DEFINITION  FOR Triple Data Encryption Standard
// *****************************************************************************
// *** Register offset in AT91S_TDES structure ***
#define TDES_CR         ( 0) // Control Register
#define TDES_MR         ( 4) // Mode Register
#define TDES_IER        (16) // Interrupt Enable Register
#define TDES_IDR        (20) // Interrupt Disable Register
#define TDES_IMR        (24) // Interrupt Mask Register
#define TDES_ISR        (28) // Interrupt Status Register
#define TDES_KEY1WxR    (32) // Key 1 Word x Register
#define TDES_KEY2WxR    (40) // Key 2 Word x Register
#define TDES_KEY3WxR    (48) // Key 3 Word x Register
#define TDES_IDATAxR    (64) // Input Data x Register
#define TDES_ODATAxR    (80) // Output Data x Register
#define TDES_IVxR       (96) // Initialization Vector x Register
#define TDES_VR         (252) // TDES Version Register
#define TDES_RPR        (256) // Receive Pointer Register
#define TDES_RCR        (260) // Receive Counter Register
#define TDES_TPR        (264) // Transmit Pointer Register
#define TDES_TCR        (268) // Transmit Counter Register
#define TDES_RNPR       (272) // Receive Next Pointer Register
#define TDES_RNCR       (276) // Receive Next Counter Register
#define TDES_TNPR       (280) // Transmit Next Pointer Register
#define TDES_TNCR       (284) // Transmit Next Counter Register
#define TDES_PTCR       (288) // PDC Transfer Control Register
#define TDES_PTSR       (292) // PDC Transfer Status Register
// -------- TDES_CR : (TDES Offset: 0x0) Control Register --------
#define AT91C_TDES_START          (0x1 <<  0) // (TDES) Starts Processing
#define AT91C_TDES_SWRST          (0x1 <<  8) // (TDES) Software Reset
// -------- TDES_MR : (TDES Offset: 0x4) Mode Register --------
#define AT91C_TDES_CIPHER         (0x1 <<  0) // (TDES) Processing Mode
#define AT91C_TDES_TDESMOD        (0x1 <<  1) // (TDES) Single or Triple DES Mode
#define AT91C_TDES_KEYMOD         (0x1 <<  4) // (TDES) Key Mode
#define AT91C_TDES_SMOD           (0x3 <<  8) // (TDES) Start Mode
#define 	AT91C_TDES_SMOD_MANUAL               (0x0 <<  8) // (TDES) Manual Mode: The START bit in register TDES_CR must be set to begin encryption or decryption.
#define 	AT91C_TDES_SMOD_AUTO                 (0x1 <<  8) // (TDES) Auto Mode: no action in TDES_CR is necessary (cf datasheet).
#define 	AT91C_TDES_SMOD_PDC                  (0x2 <<  8) // (TDES) PDC Mode (cf datasheet).
#define AT91C_TDES_OPMOD          (0x3 << 12) // (TDES) Operation Mode
#define 	AT91C_TDES_OPMOD_ECB                  (0x0 << 12) // (TDES) ECB Electronic CodeBook mode.
#define 	AT91C_TDES_OPMOD_CBC                  (0x1 << 12) // (TDES) CBC Cipher Block Chaining mode.
#define 	AT91C_TDES_OPMOD_OFB                  (0x2 << 12) // (TDES) OFB Output Feedback mode.
#define 	AT91C_TDES_OPMOD_CFB                  (0x3 << 12) // (TDES) CFB Cipher Feedback mode.
#define AT91C_TDES_LOD            (0x1 << 15) // (TDES) Last Output Data Mode
#define AT91C_TDES_CFBS           (0x3 << 16) // (TDES) Cipher Feedback Data Size
#define 	AT91C_TDES_CFBS_64_BIT               (0x0 << 16) // (TDES) 64-bit.
#define 	AT91C_TDES_CFBS_32_BIT               (0x1 << 16) // (TDES) 32-bit.
#define 	AT91C_TDES_CFBS_16_BIT               (0x2 << 16) // (TDES) 16-bit.
#define 	AT91C_TDES_CFBS_8_BIT                (0x3 << 16) // (TDES) 8-bit.
// -------- TDES_IER : (TDES Offset: 0x10) Interrupt Enable Register --------
#define AT91C_TDES_DATRDY         (0x1 <<  0) // (TDES) DATRDY
#define AT91C_TDES_ENDRX          (0x1 <<  1) // (TDES) PDC Read Buffer End
#define AT91C_TDES_ENDTX          (0x1 <<  2) // (TDES) PDC Write Buffer End
#define AT91C_TDES_RXBUFF         (0x1 <<  3) // (TDES) PDC Read Buffer Full
#define AT91C_TDES_TXBUFE         (0x1 <<  4) // (TDES) PDC Write Buffer Empty
#define AT91C_TDES_URAD           (0x1 <<  8) // (TDES) Unspecified Register Access Detection
// -------- TDES_IDR : (TDES Offset: 0x14) Interrupt Disable Register --------
// -------- TDES_IMR : (TDES Offset: 0x18) Interrupt Mask Register --------
// -------- TDES_ISR : (TDES Offset: 0x1c) Interrupt Status Register --------
#define AT91C_TDES_URAT           (0x3 << 12) // (TDES) Unspecified Register Access Type Status
#define 	AT91C_TDES_URAT_IN_DAT_WRITE_DATPROC (0x0 << 12) // (TDES) Input data register written during the data processing in PDC mode.
#define 	AT91C_TDES_URAT_OUT_DAT_READ_DATPROC (0x1 << 12) // (TDES) Output data register read during the data processing.
#define 	AT91C_TDES_URAT_MODEREG_WRITE_DATPROC (0x2 << 12) // (TDES) Mode register written during the data processing.
#define 	AT91C_TDES_URAT_WO_REG_READ          (0x3 << 12) // (TDES) Write-only register read access.

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
// -------- HDMA_SADDR : (HDMA_CH Offset: 0x0)  --------
#define AT91C_SADDR               (0x0 <<  0) // (HDMA_CH)
// -------- HDMA_DADDR : (HDMA_CH Offset: 0x4)  --------
#define AT91C_DADDR               (0x0 <<  0) // (HDMA_CH)
// -------- HDMA_DSCR : (HDMA_CH Offset: 0x8)  --------
#define AT91C_DSCR_IF             (0x3 <<  0) // (HDMA_CH)
#define AT91C_DSCR                (0x3FFFFFFF <<  2) // (HDMA_CH)
// -------- HDMA_CTRLA : (HDMA_CH Offset: 0xc)  --------
#define AT91C_BTSIZE              (0xFFFF <<  0) // (HDMA_CH)
#define AT91C_SCSIZE              (0x7 << 16) // (HDMA_CH)
#define AT91C_DCSIZE              (0x7 << 20) // (HDMA_CH)
#define AT91C_SRC_WIDTH           (0x3 << 24) // (HDMA_CH)
#define AT91C_DST_WIDTH           (0x3 << 28) // (HDMA_CH)
#define AT91C_DONE                (0x1 << 31) // (HDMA_CH)
// -------- HDMA_CTRLB : (HDMA_CH Offset: 0x10)  --------
#define AT91C_SIF                 (0x3 <<  0) // (HDMA_CH)
#define AT91C_DIF                 (0x3 <<  4) // (HDMA_CH)
#define AT91C_SRC_PIP             (0x1 <<  8) // (HDMA_CH)
#define AT91C_DST_PIP             (0x1 << 12) // (HDMA_CH)
#define AT91C_SRC_DSCR            (0x1 << 16) // (HDMA_CH)
#define AT91C_DST_DSCR            (0x1 << 20) // (HDMA_CH)
#define AT91C_FC                  (0x7 << 21) // (HDMA_CH)
#define AT91C_SRC_INCR            (0x3 << 24) // (HDMA_CH)
#define AT91C_DST_INCR            (0x3 << 28) // (HDMA_CH)
#define AT91C_AUTO                (0x1 << 31) // (HDMA_CH)
// -------- HDMA_CFG : (HDMA_CH Offset: 0x14)  --------
#define AT91C_SRC_PER             (0xF <<  0) // (HDMA_CH)
#define AT91C_DST_PER             (0xF <<  4) // (HDMA_CH)
#define AT91C_SRC_REP             (0x1 <<  8) // (HDMA_CH)
#define AT91C_SRC_H2SEL           (0x1 <<  9) // (HDMA_CH)
#define AT91C_DST_REP             (0x1 << 12) // (HDMA_CH)
#define AT91C_DST_H2SEL           (0x1 << 13) // (HDMA_CH)
#define AT91C_SOD                 (0x1 << 16) // (HDMA_CH)
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

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR System Peripherals
// *****************************************************************************
// -------- GPBR : (SYS Offset: 0x1b50) GPBR General Purpose Register --------
#define AT91C_GPBR_GPRV           (0x0 <<  0) // (SYS) General Purpose Register Value

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
//               REGISTER ADDRESS DEFINITION FOR AT91CAP9
// *****************************************************************************
// ========== Register definition for HECC peripheral ==========
#define AT91C_HECC_VR             (0xFFFFE2FC) // (HECC)  ECC Version register
#define AT91C_HECC_SR             (0xFFFFE208) // (HECC)  ECC Status register
#define AT91C_HECC_CR             (0xFFFFE200) // (HECC)  ECC reset register
#define AT91C_HECC_NPR            (0xFFFFE210) // (HECC)  ECC Parity N register
#define AT91C_HECC_PR             (0xFFFFE20C) // (HECC)  ECC Parity register
#define AT91C_HECC_MR             (0xFFFFE204) // (HECC)  ECC Page size register
// ========== Register definition for BCRAMC peripheral ==========
#define AT91C_BCRAMC_IPNR1        (0xFFFFE4F0) // (BCRAMC) BCRAM IP Name Register 1
#define AT91C_BCRAMC_HSR          (0xFFFFE408) // (BCRAMC) BCRAM Controller High Speed Register
#define AT91C_BCRAMC_CR           (0xFFFFE400) // (BCRAMC) BCRAM Controller Configuration Register
#define AT91C_BCRAMC_TPR          (0xFFFFE404) // (BCRAMC) BCRAM Controller Timing Parameter Register
#define AT91C_BCRAMC_LPR          (0xFFFFE40C) // (BCRAMC) BCRAM Controller Low Power Register
#define AT91C_BCRAMC_IPNR2        (0xFFFFE4F4) // (BCRAMC) BCRAM IP Name Register 2
#define AT91C_BCRAMC_IPFR         (0xFFFFE4F8) // (BCRAMC) BCRAM IP Features Register
#define AT91C_BCRAMC_VR           (0xFFFFE4FC) // (BCRAMC) BCRAM Version Register
#define AT91C_BCRAMC_MDR          (0xFFFFE410) // (BCRAMC) BCRAM Memory Device Register
#define AT91C_BCRAMC_PADDSR       (0xFFFFE4EC) // (BCRAMC) BCRAM PADDR Size Register
// ========== Register definition for SDRAMC peripheral ==========
#define AT91C_SDRAMC_TR           (0xFFFFE604) // (SDRAMC) SDRAM Controller Refresh Timer Register
#define AT91C_SDRAMC_ISR          (0xFFFFE620) // (SDRAMC) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC_HSR          (0xFFFFE60C) // (SDRAMC) SDRAM Controller High Speed Register
#define AT91C_SDRAMC_IMR          (0xFFFFE61C) // (SDRAMC) SDRAM Controller Interrupt Mask Register
#define AT91C_SDRAMC_IER          (0xFFFFE614) // (SDRAMC) SDRAM Controller Interrupt Enable Register
#define AT91C_SDRAMC_MR           (0xFFFFE600) // (SDRAMC) SDRAM Controller Mode Register
#define AT91C_SDRAMC_MDR          (0xFFFFE624) // (SDRAMC) SDRAM Memory Device Register
#define AT91C_SDRAMC_LPR          (0xFFFFE610) // (SDRAMC) SDRAM Controller Low Power Register
#define AT91C_SDRAMC_CR           (0xFFFFE608) // (SDRAMC) SDRAM Controller Configuration Register
#define AT91C_SDRAMC_IDR          (0xFFFFE618) // (SDRAMC) SDRAM Controller Interrupt Disable Register
// ========== Register definition for SDDRC peripheral ==========
#define AT91C_SDDRC_RTR           (0xFFFFE604) // (SDDRC)
#define AT91C_SDDRC_T0PR          (0xFFFFE60C) // (SDDRC)
#define AT91C_SDDRC_MDR           (0xFFFFE61C) // (SDDRC)
#define AT91C_SDDRC_HS            (0xFFFFE614) // (SDDRC)
#define AT91C_SDDRC_VERSION       (0xFFFFE6FC) // (SDDRC)
#define AT91C_SDDRC_MR            (0xFFFFE600) // (SDDRC)
#define AT91C_SDDRC_T1PR          (0xFFFFE610) // (SDDRC)
#define AT91C_SDDRC_CR            (0xFFFFE608) // (SDDRC)
#define AT91C_SDDRC_LPR           (0xFFFFE618) // (SDDRC)
// ========== Register definition for SMC peripheral ==========
#define AT91C_SMC_PULSE7          (0xFFFFE874) // (SMC)  Pulse Register for CS 7
#define AT91C_SMC_SETUP3          (0xFFFFE830) // (SMC)  Setup Register for CS 3
#define AT91C_SMC_CYCLE2          (0xFFFFE828) // (SMC)  Cycle Register for CS 2
#define AT91C_SMC_CTRL1           (0xFFFFE81C) // (SMC)  Control Register for CS 1
#define AT91C_SMC_CTRL0           (0xFFFFE80C) // (SMC)  Control Register for CS 0
#define AT91C_SMC_CYCLE7          (0xFFFFE878) // (SMC)  Cycle Register for CS 7
#define AT91C_SMC_PULSE2          (0xFFFFE824) // (SMC)  Pulse Register for CS 2
#define AT91C_SMC_SETUP6          (0xFFFFE860) // (SMC)  Setup Register for CS 6
#define AT91C_SMC_SETUP2          (0xFFFFE820) // (SMC)  Setup Register for CS 2
#define AT91C_SMC_CYCLE1          (0xFFFFE818) // (SMC)  Cycle Register for CS 1
#define AT91C_SMC_SETUP5          (0xFFFFE850) // (SMC)  Setup Register for CS 5
#define AT91C_SMC_PULSE1          (0xFFFFE814) // (SMC)  Pulse Register for CS 1
#define AT91C_SMC_CYCLE6          (0xFFFFE868) // (SMC)  Cycle Register for CS 6
#define AT91C_SMC_PULSE6          (0xFFFFE864) // (SMC)  Pulse Register for CS 6
#define AT91C_SMC_CTRL2           (0xFFFFE82C) // (SMC)  Control Register for CS 2
#define AT91C_SMC_CTRL5           (0xFFFFE85C) // (SMC)  Control Register for CS 5
#define AT91C_SMC_PULSE4          (0xFFFFE844) // (SMC)  Pulse Register for CS 4
#define AT91C_SMC_CTRL3           (0xFFFFE83C) // (SMC)  Control Register for CS 3
#define AT91C_SMC_CYCLE0          (0xFFFFE808) // (SMC)  Cycle Register for CS 0
#define AT91C_SMC_SETUP1          (0xFFFFE810) // (SMC)  Setup Register for CS 1
#define AT91C_SMC_SETUP4          (0xFFFFE840) // (SMC)  Setup Register for CS 4
#define AT91C_SMC_PULSE5          (0xFFFFE854) // (SMC)  Pulse Register for CS 5
#define AT91C_SMC_CYCLE3          (0xFFFFE838) // (SMC)  Cycle Register for CS 3
#define AT91C_SMC_SETUP7          (0xFFFFE870) // (SMC)  Setup Register for CS 7
#define AT91C_SMC_SETUP0          (0xFFFFE800) // (SMC)  Setup Register for CS 0
#define AT91C_SMC_CTRL4           (0xFFFFE84C) // (SMC)  Control Register for CS 4
#define AT91C_SMC_CYCLE5          (0xFFFFE858) // (SMC)  Cycle Register for CS 5
#define AT91C_SMC_PULSE0          (0xFFFFE804) // (SMC)  Pulse Register for CS 0
#define AT91C_SMC_PULSE3          (0xFFFFE834) // (SMC)  Pulse Register for CS 3
#define AT91C_SMC_CTRL6           (0xFFFFE86C) // (SMC)  Control Register for CS 6
#define AT91C_SMC_CYCLE4          (0xFFFFE848) // (SMC)  Cycle Register for CS 4
#define AT91C_SMC_CTRL7           (0xFFFFE87C) // (SMC)  Control Register for CS 7
// ========== Register definition for MATRIX_PRS peripheral ==========
#define AT91C_MATRIX_PRS_PRAS     (0xFFFFEA80) // (MATRIX_PRS)  Slave Priority Registers A for Slave
#define AT91C_MATRIX_PRS_PRBS     (0xFFFFEA84) // (MATRIX_PRS)  Slave Priority Registers B for Slave
// ========== Register definition for MATRIX peripheral ==========
#define AT91C_MATRIX_MCFG         (0xFFFFEA00) // (MATRIX)  Master Configuration Register
#define AT91C_MATRIX_MRCR         (0xFFFFEB00) // (MATRIX)  Master Remp Control Register
#define AT91C_MATRIX_SCFG         (0xFFFFEA40) // (MATRIX)  Slave Configuration Register
// ========== Register definition for CCFG peripheral ==========
#define AT91C_CCFG_MPBS1          (0xFFFFEB1C) // (CCFG)  Slave 3 (MP Block Slave 1) Special Function Register
#define AT91C_CCFG_BRIDGE         (0xFFFFEB34) // (CCFG)  Slave 8 (APB Bridge) Special Function Register
#define AT91C_CCFG_MPBS3          (0xFFFFEB30) // (CCFG)  Slave 7 (MP Block Slave 3) Special Function Register
#define AT91C_CCFG_MPBS2          (0xFFFFEB2C) // (CCFG)  Slave 7 (MP Block Slave 2) Special Function Register
#define AT91C_CCFG_UDPHS          (0xFFFFEB18) // (CCFG)  Slave 2 (AHB Periphs) Special Function Register
#define AT91C_CCFG_EBICSA         (0xFFFFEB20) // (CCFG)  EBI Chip Select Assignement Register
#define AT91C_CCFG_MPBS0          (0xFFFFEB14) // (CCFG)  Slave 1 (MP Block Slave 0) Special Function Register
#define AT91C_CCFG_MATRIXVERSION  (0xFFFFEBFC) // (CCFG)  Version Register
// ========== Register definition for PDC_DBGU peripheral ==========
#define AT91C_DBGU_PTCR           (0xFFFFEF20) // (PDC_DBGU) PDC Transfer Control Register
#define AT91C_DBGU_RCR            (0xFFFFEF04) // (PDC_DBGU) Receive Counter Register
#define AT91C_DBGU_TCR            (0xFFFFEF0C) // (PDC_DBGU) Transmit Counter Register
#define AT91C_DBGU_RNCR           (0xFFFFEF14) // (PDC_DBGU) Receive Next Counter Register
#define AT91C_DBGU_TNPR           (0xFFFFEF18) // (PDC_DBGU) Transmit Next Pointer Register
#define AT91C_DBGU_RNPR           (0xFFFFEF10) // (PDC_DBGU) Receive Next Pointer Register
#define AT91C_DBGU_PTSR           (0xFFFFEF24) // (PDC_DBGU) PDC Transfer Status Register
#define AT91C_DBGU_RPR            (0xFFFFEF00) // (PDC_DBGU) Receive Pointer Register
#define AT91C_DBGU_TPR            (0xFFFFEF08) // (PDC_DBGU) Transmit Pointer Register
#define AT91C_DBGU_TNCR           (0xFFFFEF1C) // (PDC_DBGU) Transmit Next Counter Register
// ========== Register definition for DBGU peripheral ==========
#define AT91C_DBGU_BRGR           (0xFFFFEE20) // (DBGU) Baud Rate Generator Register
#define AT91C_DBGU_CR             (0xFFFFEE00) // (DBGU) Control Register
#define AT91C_DBGU_THR            (0xFFFFEE1C) // (DBGU) Transmitter Holding Register
#define AT91C_DBGU_IDR            (0xFFFFEE0C) // (DBGU) Interrupt Disable Register
#define AT91C_DBGU_EXID           (0xFFFFEE44) // (DBGU) Chip ID Extension Register
#define AT91C_DBGU_IMR            (0xFFFFEE10) // (DBGU) Interrupt Mask Register
#define AT91C_DBGU_FNTR           (0xFFFFEE48) // (DBGU) Force NTRST Register
#define AT91C_DBGU_IER            (0xFFFFEE08) // (DBGU) Interrupt Enable Register
#define AT91C_DBGU_CSR            (0xFFFFEE14) // (DBGU) Channel Status Register
#define AT91C_DBGU_MR             (0xFFFFEE04) // (DBGU) Mode Register
#define AT91C_DBGU_RHR            (0xFFFFEE18) // (DBGU) Receiver Holding Register
#define AT91C_DBGU_CIDR           (0xFFFFEE40) // (DBGU) Chip ID Register
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
#define AT91C_PIOA_OWDR           (0xFFFFF2A4) // (PIOA) Output Write Disable Register
#define AT91C_PIOA_MDDR           (0xFFFFF254) // (PIOA) Multi-driver Disable Register
#define AT91C_PIOA_IFSR           (0xFFFFF228) // (PIOA) Input Filter Status Register
#define AT91C_PIOA_ISR            (0xFFFFF24C) // (PIOA) Interrupt Status Register
#define AT91C_PIOA_CODR           (0xFFFFF234) // (PIOA) Clear Output Data Register
#define AT91C_PIOA_PDR            (0xFFFFF204) // (PIOA) PIO Disable Register
#define AT91C_PIOA_OWSR           (0xFFFFF2A8) // (PIOA) Output Write Status Register
#define AT91C_PIOA_ASR            (0xFFFFF270) // (PIOA) Select A Register
#define AT91C_PIOA_PPUSR          (0xFFFFF268) // (PIOA) Pull-up Status Register
#define AT91C_PIOA_IMR            (0xFFFFF248) // (PIOA) Interrupt Mask Register
#define AT91C_PIOA_OSR            (0xFFFFF218) // (PIOA) Output Status Register
#define AT91C_PIOA_ABSR           (0xFFFFF278) // (PIOA) AB Select Status Register
#define AT91C_PIOA_MDER           (0xFFFFF250) // (PIOA) Multi-driver Enable Register
#define AT91C_PIOA_IFDR           (0xFFFFF224) // (PIOA) Input Filter Disable Register
#define AT91C_PIOA_PDSR           (0xFFFFF23C) // (PIOA) Pin Data Status Register
#define AT91C_PIOA_SODR           (0xFFFFF230) // (PIOA) Set Output Data Register
#define AT91C_PIOA_BSR            (0xFFFFF274) // (PIOA) Select B Register
#define AT91C_PIOA_OWER           (0xFFFFF2A0) // (PIOA) Output Write Enable Register
#define AT91C_PIOA_IFER           (0xFFFFF220) // (PIOA) Input Filter Enable Register
#define AT91C_PIOA_IDR            (0xFFFFF244) // (PIOA) Interrupt Disable Register
#define AT91C_PIOA_ODR            (0xFFFFF214) // (PIOA) Output Disable Registerr
#define AT91C_PIOA_IER            (0xFFFFF240) // (PIOA) Interrupt Enable Register
#define AT91C_PIOA_PPUER          (0xFFFFF264) // (PIOA) Pull-up Enable Register
#define AT91C_PIOA_MDSR           (0xFFFFF258) // (PIOA) Multi-driver Status Register
#define AT91C_PIOA_OER            (0xFFFFF210) // (PIOA) Output Enable Register
#define AT91C_PIOA_PER            (0xFFFFF200) // (PIOA) PIO Enable Register
#define AT91C_PIOA_PPUDR          (0xFFFFF260) // (PIOA) Pull-up Disable Register
#define AT91C_PIOA_ODSR           (0xFFFFF238) // (PIOA) Output Data Status Register
#define AT91C_PIOA_PSR            (0xFFFFF208) // (PIOA) PIO Status Register
// ========== Register definition for PIOB peripheral ==========
#define AT91C_PIOB_ODR            (0xFFFFF414) // (PIOB) Output Disable Registerr
#define AT91C_PIOB_SODR           (0xFFFFF430) // (PIOB) Set Output Data Register
#define AT91C_PIOB_ISR            (0xFFFFF44C) // (PIOB) Interrupt Status Register
#define AT91C_PIOB_ABSR           (0xFFFFF478) // (PIOB) AB Select Status Register
#define AT91C_PIOB_IER            (0xFFFFF440) // (PIOB) Interrupt Enable Register
#define AT91C_PIOB_PPUDR          (0xFFFFF460) // (PIOB) Pull-up Disable Register
#define AT91C_PIOB_IMR            (0xFFFFF448) // (PIOB) Interrupt Mask Register
#define AT91C_PIOB_PER            (0xFFFFF400) // (PIOB) PIO Enable Register
#define AT91C_PIOB_IFDR           (0xFFFFF424) // (PIOB) Input Filter Disable Register
#define AT91C_PIOB_OWDR           (0xFFFFF4A4) // (PIOB) Output Write Disable Register
#define AT91C_PIOB_MDSR           (0xFFFFF458) // (PIOB) Multi-driver Status Register
#define AT91C_PIOB_IDR            (0xFFFFF444) // (PIOB) Interrupt Disable Register
#define AT91C_PIOB_ODSR           (0xFFFFF438) // (PIOB) Output Data Status Register
#define AT91C_PIOB_PPUSR          (0xFFFFF468) // (PIOB) Pull-up Status Register
#define AT91C_PIOB_OWSR           (0xFFFFF4A8) // (PIOB) Output Write Status Register
#define AT91C_PIOB_BSR            (0xFFFFF474) // (PIOB) Select B Register
#define AT91C_PIOB_OWER           (0xFFFFF4A0) // (PIOB) Output Write Enable Register
#define AT91C_PIOB_IFER           (0xFFFFF420) // (PIOB) Input Filter Enable Register
#define AT91C_PIOB_PDSR           (0xFFFFF43C) // (PIOB) Pin Data Status Register
#define AT91C_PIOB_PPUER          (0xFFFFF464) // (PIOB) Pull-up Enable Register
#define AT91C_PIOB_OSR            (0xFFFFF418) // (PIOB) Output Status Register
#define AT91C_PIOB_ASR            (0xFFFFF470) // (PIOB) Select A Register
#define AT91C_PIOB_MDDR           (0xFFFFF454) // (PIOB) Multi-driver Disable Register
#define AT91C_PIOB_CODR           (0xFFFFF434) // (PIOB) Clear Output Data Register
#define AT91C_PIOB_MDER           (0xFFFFF450) // (PIOB) Multi-driver Enable Register
#define AT91C_PIOB_PDR            (0xFFFFF404) // (PIOB) PIO Disable Register
#define AT91C_PIOB_IFSR           (0xFFFFF428) // (PIOB) Input Filter Status Register
#define AT91C_PIOB_OER            (0xFFFFF410) // (PIOB) Output Enable Register
#define AT91C_PIOB_PSR            (0xFFFFF408) // (PIOB) PIO Status Register
// ========== Register definition for PIOC peripheral ==========
#define AT91C_PIOC_OWDR           (0xFFFFF6A4) // (PIOC) Output Write Disable Register
#define AT91C_PIOC_MDER           (0xFFFFF650) // (PIOC) Multi-driver Enable Register
#define AT91C_PIOC_PPUSR          (0xFFFFF668) // (PIOC) Pull-up Status Register
#define AT91C_PIOC_IMR            (0xFFFFF648) // (PIOC) Interrupt Mask Register
#define AT91C_PIOC_ASR            (0xFFFFF670) // (PIOC) Select A Register
#define AT91C_PIOC_PPUDR          (0xFFFFF660) // (PIOC) Pull-up Disable Register
#define AT91C_PIOC_PSR            (0xFFFFF608) // (PIOC) PIO Status Register
#define AT91C_PIOC_IER            (0xFFFFF640) // (PIOC) Interrupt Enable Register
#define AT91C_PIOC_CODR           (0xFFFFF634) // (PIOC) Clear Output Data Register
#define AT91C_PIOC_OWER           (0xFFFFF6A0) // (PIOC) Output Write Enable Register
#define AT91C_PIOC_ABSR           (0xFFFFF678) // (PIOC) AB Select Status Register
#define AT91C_PIOC_IFDR           (0xFFFFF624) // (PIOC) Input Filter Disable Register
#define AT91C_PIOC_PDSR           (0xFFFFF63C) // (PIOC) Pin Data Status Register
#define AT91C_PIOC_IDR            (0xFFFFF644) // (PIOC) Interrupt Disable Register
#define AT91C_PIOC_OWSR           (0xFFFFF6A8) // (PIOC) Output Write Status Register
#define AT91C_PIOC_PDR            (0xFFFFF604) // (PIOC) PIO Disable Register
#define AT91C_PIOC_ODR            (0xFFFFF614) // (PIOC) Output Disable Registerr
#define AT91C_PIOC_IFSR           (0xFFFFF628) // (PIOC) Input Filter Status Register
#define AT91C_PIOC_PPUER          (0xFFFFF664) // (PIOC) Pull-up Enable Register
#define AT91C_PIOC_SODR           (0xFFFFF630) // (PIOC) Set Output Data Register
#define AT91C_PIOC_ISR            (0xFFFFF64C) // (PIOC) Interrupt Status Register
#define AT91C_PIOC_ODSR           (0xFFFFF638) // (PIOC) Output Data Status Register
#define AT91C_PIOC_OSR            (0xFFFFF618) // (PIOC) Output Status Register
#define AT91C_PIOC_MDSR           (0xFFFFF658) // (PIOC) Multi-driver Status Register
#define AT91C_PIOC_IFER           (0xFFFFF620) // (PIOC) Input Filter Enable Register
#define AT91C_PIOC_BSR            (0xFFFFF674) // (PIOC) Select B Register
#define AT91C_PIOC_MDDR           (0xFFFFF654) // (PIOC) Multi-driver Disable Register
#define AT91C_PIOC_OER            (0xFFFFF610) // (PIOC) Output Enable Register
#define AT91C_PIOC_PER            (0xFFFFF600) // (PIOC) PIO Enable Register
// ========== Register definition for PIOD peripheral ==========
#define AT91C_PIOD_OWDR           (0xFFFFF8A4) // (PIOD) Output Write Disable Register
#define AT91C_PIOD_SODR           (0xFFFFF830) // (PIOD) Set Output Data Register
#define AT91C_PIOD_PPUER          (0xFFFFF864) // (PIOD) Pull-up Enable Register
#define AT91C_PIOD_CODR           (0xFFFFF834) // (PIOD) Clear Output Data Register
#define AT91C_PIOD_PSR            (0xFFFFF808) // (PIOD) PIO Status Register
#define AT91C_PIOD_PDR            (0xFFFFF804) // (PIOD) PIO Disable Register
#define AT91C_PIOD_ODR            (0xFFFFF814) // (PIOD) Output Disable Registerr
#define AT91C_PIOD_PPUSR          (0xFFFFF868) // (PIOD) Pull-up Status Register
#define AT91C_PIOD_ABSR           (0xFFFFF878) // (PIOD) AB Select Status Register
#define AT91C_PIOD_IFSR           (0xFFFFF828) // (PIOD) Input Filter Status Register
#define AT91C_PIOD_OER            (0xFFFFF810) // (PIOD) Output Enable Register
#define AT91C_PIOD_IMR            (0xFFFFF848) // (PIOD) Interrupt Mask Register
#define AT91C_PIOD_ASR            (0xFFFFF870) // (PIOD) Select A Register
#define AT91C_PIOD_MDDR           (0xFFFFF854) // (PIOD) Multi-driver Disable Register
#define AT91C_PIOD_OWSR           (0xFFFFF8A8) // (PIOD) Output Write Status Register
#define AT91C_PIOD_PER            (0xFFFFF800) // (PIOD) PIO Enable Register
#define AT91C_PIOD_IDR            (0xFFFFF844) // (PIOD) Interrupt Disable Register
#define AT91C_PIOD_MDER           (0xFFFFF850) // (PIOD) Multi-driver Enable Register
#define AT91C_PIOD_PDSR           (0xFFFFF83C) // (PIOD) Pin Data Status Register
#define AT91C_PIOD_MDSR           (0xFFFFF858) // (PIOD) Multi-driver Status Register
#define AT91C_PIOD_OWER           (0xFFFFF8A0) // (PIOD) Output Write Enable Register
#define AT91C_PIOD_BSR            (0xFFFFF874) // (PIOD) Select B Register
#define AT91C_PIOD_PPUDR          (0xFFFFF860) // (PIOD) Pull-up Disable Register
#define AT91C_PIOD_IFDR           (0xFFFFF824) // (PIOD) Input Filter Disable Register
#define AT91C_PIOD_IER            (0xFFFFF840) // (PIOD) Interrupt Enable Register
#define AT91C_PIOD_OSR            (0xFFFFF818) // (PIOD) Output Status Register
#define AT91C_PIOD_ODSR           (0xFFFFF838) // (PIOD) Output Data Status Register
#define AT91C_PIOD_ISR            (0xFFFFF84C) // (PIOD) Interrupt Status Register
#define AT91C_PIOD_IFER           (0xFFFFF820) // (PIOD) Input Filter Enable Register
// ========== Register definition for CKGR peripheral ==========
#define AT91C_CKGR_MOR            (0xFFFFFC20) // (CKGR) Main Oscillator Register
#define AT91C_CKGR_PLLBR          (0xFFFFFC2C) // (CKGR) PLL B Register
#define AT91C_CKGR_MCFR           (0xFFFFFC24) // (CKGR) Main Clock  Frequency Register
#define AT91C_CKGR_PLLAR          (0xFFFFFC28) // (CKGR) PLL A Register
#define AT91C_CKGR_UCKR           (0xFFFFFC1C) // (CKGR) UTMI Clock Configuration Register
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
#define AT91C_PMC_UCKR            (0xFFFFFC1C) // (PMC) UTMI Clock Configuration Register
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
// ========== Register definition for UDP peripheral ==========
#define AT91C_UDP_FDR             (0xFFF78050) // (UDP) Endpoint FIFO Data Register
#define AT91C_UDP_IER             (0xFFF78010) // (UDP) Interrupt Enable Register
#define AT91C_UDP_CSR             (0xFFF78030) // (UDP) Endpoint Control and Status Register
#define AT91C_UDP_RSTEP           (0xFFF78028) // (UDP) Reset Endpoint Register
#define AT91C_UDP_GLBSTATE        (0xFFF78004) // (UDP) Global State Register
#define AT91C_UDP_TXVC            (0xFFF78074) // (UDP) Transceiver Control Register
#define AT91C_UDP_IDR             (0xFFF78014) // (UDP) Interrupt Disable Register
#define AT91C_UDP_ISR             (0xFFF7801C) // (UDP) Interrupt Status Register
#define AT91C_UDP_IMR             (0xFFF78018) // (UDP) Interrupt Mask Register
#define AT91C_UDP_FADDR           (0xFFF78008) // (UDP) Function Address Register
#define AT91C_UDP_NUM             (0xFFF78000) // (UDP) Frame Number Register
#define AT91C_UDP_ICR             (0xFFF78020) // (UDP) Interrupt Clear Register
// ========== Register definition for UDPHS_EPTFIFO peripheral ==========
#define AT91C_UDPHS_EPTFIFO_READEPTF (0x006F0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 15
#define AT91C_UDPHS_EPTFIFO_READEPT5 (0x00650000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 5
#define AT91C_UDPHS_EPTFIFO_READEPT1 (0x00610000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 1
#define AT91C_UDPHS_EPTFIFO_READEPTE (0x006E0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 14
#define AT91C_UDPHS_EPTFIFO_READEPT4 (0x00640000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 4
#define AT91C_UDPHS_EPTFIFO_READEPTD (0x006D0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 13
#define AT91C_UDPHS_EPTFIFO_READEPT2 (0x00620000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 2
#define AT91C_UDPHS_EPTFIFO_READEPT6 (0x00660000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 6
#define AT91C_UDPHS_EPTFIFO_READEPT9 (0x00690000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 9
#define AT91C_UDPHS_EPTFIFO_READEPT0 (0x00600000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 0
#define AT91C_UDPHS_EPTFIFO_READEPTA (0x006A0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 10
#define AT91C_UDPHS_EPTFIFO_READEPT3 (0x00630000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 3
#define AT91C_UDPHS_EPTFIFO_READEPTC (0x006C0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 12
#define AT91C_UDPHS_EPTFIFO_READEPTB (0x006B0000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 11
#define AT91C_UDPHS_EPTFIFO_READEPT8 (0x00680000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 8
#define AT91C_UDPHS_EPTFIFO_READEPT7 (0x00670000) // (UDPHS_EPTFIFO) FIFO Endpoint Data Register 7
// ========== Register definition for UDPHS_EPT_0 peripheral ==========
#define AT91C_UDPHS_EPT_0_EPTSTA  (0xFFF7811C) // (UDPHS_EPT_0) Endpoint Status Register
#define AT91C_UDPHS_EPT_0_EPTCTL  (0xFFF7810C) // (UDPHS_EPT_0) Endpoint Control Register
#define AT91C_UDPHS_EPT_0_EPTCTLDIS (0xFFF78108) // (UDPHS_EPT_0) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_0_EPTCFG  (0xFFF78100) // (UDPHS_EPT_0) Endpoint Config Register
#define AT91C_UDPHS_EPT_0_EPTCLRSTA (0xFFF78118) // (UDPHS_EPT_0) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_0_EPTSETSTA (0xFFF78114) // (UDPHS_EPT_0) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_0_EPTCTLENB (0xFFF78104) // (UDPHS_EPT_0) Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_1 peripheral ==========
#define AT91C_UDPHS_EPT_1_EPTCTLENB (0xFFF78124) // (UDPHS_EPT_1) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_1_EPTCFG  (0xFFF78120) // (UDPHS_EPT_1) Endpoint Config Register
#define AT91C_UDPHS_EPT_1_EPTCTL  (0xFFF7812C) // (UDPHS_EPT_1) Endpoint Control Register
#define AT91C_UDPHS_EPT_1_EPTSTA  (0xFFF7813C) // (UDPHS_EPT_1) Endpoint Status Register
#define AT91C_UDPHS_EPT_1_EPTCLRSTA (0xFFF78138) // (UDPHS_EPT_1) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_1_EPTSETSTA (0xFFF78134) // (UDPHS_EPT_1) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_1_EPTCTLDIS (0xFFF78128) // (UDPHS_EPT_1) Endpoint Control Disable Register
// ========== Register definition for UDPHS_EPT_2 peripheral ==========
#define AT91C_UDPHS_EPT_2_EPTCLRSTA (0xFFF78158) // (UDPHS_EPT_2) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_2_EPTCTLDIS (0xFFF78148) // (UDPHS_EPT_2) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_2_EPTSTA  (0xFFF7815C) // (UDPHS_EPT_2) Endpoint Status Register
#define AT91C_UDPHS_EPT_2_EPTSETSTA (0xFFF78154) // (UDPHS_EPT_2) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_2_EPTCTL  (0xFFF7814C) // (UDPHS_EPT_2) Endpoint Control Register
#define AT91C_UDPHS_EPT_2_EPTCFG  (0xFFF78140) // (UDPHS_EPT_2) Endpoint Config Register
#define AT91C_UDPHS_EPT_2_EPTCTLENB (0xFFF78144) // (UDPHS_EPT_2) Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_3 peripheral ==========
#define AT91C_UDPHS_EPT_3_EPTCTL  (0xFFF7816C) // (UDPHS_EPT_3) Endpoint Control Register
#define AT91C_UDPHS_EPT_3_EPTCLRSTA (0xFFF78178) // (UDPHS_EPT_3) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_3_EPTCTLDIS (0xFFF78168) // (UDPHS_EPT_3) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_3_EPTSTA  (0xFFF7817C) // (UDPHS_EPT_3) Endpoint Status Register
#define AT91C_UDPHS_EPT_3_EPTSETSTA (0xFFF78174) // (UDPHS_EPT_3) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_3_EPTCTLENB (0xFFF78164) // (UDPHS_EPT_3) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_3_EPTCFG  (0xFFF78160) // (UDPHS_EPT_3) Endpoint Config Register
// ========== Register definition for UDPHS_EPT_4 peripheral ==========
#define AT91C_UDPHS_EPT_4_EPTCLRSTA (0xFFF78198) // (UDPHS_EPT_4) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_4_EPTCTL  (0xFFF7818C) // (UDPHS_EPT_4) Endpoint Control Register
#define AT91C_UDPHS_EPT_4_EPTCTLENB (0xFFF78184) // (UDPHS_EPT_4) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_4_EPTSTA  (0xFFF7819C) // (UDPHS_EPT_4) Endpoint Status Register
#define AT91C_UDPHS_EPT_4_EPTSETSTA (0xFFF78194) // (UDPHS_EPT_4) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_4_EPTCFG  (0xFFF78180) // (UDPHS_EPT_4) Endpoint Config Register
#define AT91C_UDPHS_EPT_4_EPTCTLDIS (0xFFF78188) // (UDPHS_EPT_4) Endpoint Control Disable Register
// ========== Register definition for UDPHS_EPT_5 peripheral ==========
#define AT91C_UDPHS_EPT_5_EPTSTA  (0xFFF781BC) // (UDPHS_EPT_5) Endpoint Status Register
#define AT91C_UDPHS_EPT_5_EPTCLRSTA (0xFFF781B8) // (UDPHS_EPT_5) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_5_EPTCTLENB (0xFFF781A4) // (UDPHS_EPT_5) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_5_EPTSETSTA (0xFFF781B4) // (UDPHS_EPT_5) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_5_EPTCTLDIS (0xFFF781A8) // (UDPHS_EPT_5) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_5_EPTCFG  (0xFFF781A0) // (UDPHS_EPT_5) Endpoint Config Register
#define AT91C_UDPHS_EPT_5_EPTCTL  (0xFFF781AC) // (UDPHS_EPT_5) Endpoint Control Register
// ========== Register definition for UDPHS_EPT_6 peripheral ==========
#define AT91C_UDPHS_EPT_6_EPTCLRSTA (0xFFF781D8) // (UDPHS_EPT_6) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_6_EPTCTLENB (0xFFF781C4) // (UDPHS_EPT_6) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_6_EPTCTL  (0xFFF781CC) // (UDPHS_EPT_6) Endpoint Control Register
#define AT91C_UDPHS_EPT_6_EPTSETSTA (0xFFF781D4) // (UDPHS_EPT_6) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_6_EPTCTLDIS (0xFFF781C8) // (UDPHS_EPT_6) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_6_EPTSTA  (0xFFF781DC) // (UDPHS_EPT_6) Endpoint Status Register
#define AT91C_UDPHS_EPT_6_EPTCFG  (0xFFF781C0) // (UDPHS_EPT_6) Endpoint Config Register
// ========== Register definition for UDPHS_EPT_7 peripheral ==========
#define AT91C_UDPHS_EPT_7_EPTSETSTA (0xFFF781F4) // (UDPHS_EPT_7) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_7_EPTCFG  (0xFFF781E0) // (UDPHS_EPT_7) Endpoint Config Register
#define AT91C_UDPHS_EPT_7_EPTSTA  (0xFFF781FC) // (UDPHS_EPT_7) Endpoint Status Register
#define AT91C_UDPHS_EPT_7_EPTCLRSTA (0xFFF781F8) // (UDPHS_EPT_7) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_7_EPTCTL  (0xFFF781EC) // (UDPHS_EPT_7) Endpoint Control Register
#define AT91C_UDPHS_EPT_7_EPTCTLDIS (0xFFF781E8) // (UDPHS_EPT_7) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_7_EPTCTLENB (0xFFF781E4) // (UDPHS_EPT_7) Endpoint Control Enable Register
// ========== Register definition for UDPHS_EPT_8 peripheral ==========
#define AT91C_UDPHS_EPT_8_EPTCTL  (0xFFF7820C) // (UDPHS_EPT_8) Endpoint Control Register
#define AT91C_UDPHS_EPT_8_EPTSTA  (0xFFF7821C) // (UDPHS_EPT_8) Endpoint Status Register
#define AT91C_UDPHS_EPT_8_EPTCLRSTA (0xFFF78218) // (UDPHS_EPT_8) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_8_EPTCFG  (0xFFF78200) // (UDPHS_EPT_8) Endpoint Config Register
#define AT91C_UDPHS_EPT_8_EPTSETSTA (0xFFF78214) // (UDPHS_EPT_8) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_8_EPTCTLENB (0xFFF78204) // (UDPHS_EPT_8) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_8_EPTCTLDIS (0xFFF78208) // (UDPHS_EPT_8) Endpoint Control Disable Register
// ========== Register definition for UDPHS_EPT_9 peripheral ==========
#define AT91C_UDPHS_EPT_9_EPTCLRSTA (0xFFF78238) // (UDPHS_EPT_9) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_9_EPTSETSTA (0xFFF78234) // (UDPHS_EPT_9) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_9_EPTCFG  (0xFFF78220) // (UDPHS_EPT_9) Endpoint Config Register
#define AT91C_UDPHS_EPT_9_EPTSTA  (0xFFF7823C) // (UDPHS_EPT_9) Endpoint Status Register
#define AT91C_UDPHS_EPT_9_EPTCTLDIS (0xFFF78228) // (UDPHS_EPT_9) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_9_EPTCTLENB (0xFFF78224) // (UDPHS_EPT_9) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_9_EPTCTL  (0xFFF7822C) // (UDPHS_EPT_9) Endpoint Control Register
// ========== Register definition for UDPHS_EPT_10 peripheral ==========
#define AT91C_UDPHS_EPT_10_EPTCTLDIS (0xFFF78248) // (UDPHS_EPT_10) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_10_EPTCFG (0xFFF78240) // (UDPHS_EPT_10) Endpoint Config Register
#define AT91C_UDPHS_EPT_10_EPTSTA (0xFFF7825C) // (UDPHS_EPT_10) Endpoint Status Register
#define AT91C_UDPHS_EPT_10_EPTCLRSTA (0xFFF78258) // (UDPHS_EPT_10) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_10_EPTCTLENB (0xFFF78244) // (UDPHS_EPT_10) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_10_EPTSETSTA (0xFFF78254) // (UDPHS_EPT_10) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_10_EPTCTL (0xFFF7824C) // (UDPHS_EPT_10) Endpoint Control Register
// ========== Register definition for UDPHS_EPT_11 peripheral ==========
#define AT91C_UDPHS_EPT_11_EPTCTLDIS (0xFFF78268) // (UDPHS_EPT_11) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_11_EPTCLRSTA (0xFFF78278) // (UDPHS_EPT_11) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_11_EPTCTLENB (0xFFF78264) // (UDPHS_EPT_11) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_11_EPTCTL (0xFFF7826C) // (UDPHS_EPT_11) Endpoint Control Register
#define AT91C_UDPHS_EPT_11_EPTSTA (0xFFF7827C) // (UDPHS_EPT_11) Endpoint Status Register
#define AT91C_UDPHS_EPT_11_EPTSETSTA (0xFFF78274) // (UDPHS_EPT_11) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_11_EPTCFG (0xFFF78260) // (UDPHS_EPT_11) Endpoint Config Register
// ========== Register definition for UDPHS_EPT_12 peripheral ==========
#define AT91C_UDPHS_EPT_12_EPTCTLENB (0xFFF78284) // (UDPHS_EPT_12) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_12_EPTCLRSTA (0xFFF78298) // (UDPHS_EPT_12) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_12_EPTCFG (0xFFF78280) // (UDPHS_EPT_12) Endpoint Config Register
#define AT91C_UDPHS_EPT_12_EPTSTA (0xFFF7829C) // (UDPHS_EPT_12) Endpoint Status Register
#define AT91C_UDPHS_EPT_12_EPTCTL (0xFFF7828C) // (UDPHS_EPT_12) Endpoint Control Register
#define AT91C_UDPHS_EPT_12_EPTSETSTA (0xFFF78294) // (UDPHS_EPT_12) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_12_EPTCTLDIS (0xFFF78288) // (UDPHS_EPT_12) Endpoint Control Disable Register
// ========== Register definition for UDPHS_EPT_13 peripheral ==========
#define AT91C_UDPHS_EPT_13_EPTCTLENB (0xFFF782A4) // (UDPHS_EPT_13) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_13_EPTCTL (0xFFF782AC) // (UDPHS_EPT_13) Endpoint Control Register
#define AT91C_UDPHS_EPT_13_EPTCLRSTA (0xFFF782B8) // (UDPHS_EPT_13) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_13_EPTCTLDIS (0xFFF782A8) // (UDPHS_EPT_13) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_13_EPTSETSTA (0xFFF782B4) // (UDPHS_EPT_13) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_13_EPTCFG (0xFFF782A0) // (UDPHS_EPT_13) Endpoint Config Register
#define AT91C_UDPHS_EPT_13_EPTSTA (0xFFF782BC) // (UDPHS_EPT_13) Endpoint Status Register
// ========== Register definition for UDPHS_EPT_14 peripheral ==========
#define AT91C_UDPHS_EPT_14_EPTSETSTA (0xFFF782D4) // (UDPHS_EPT_14) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_14_EPTSTA (0xFFF782DC) // (UDPHS_EPT_14) Endpoint Status Register
#define AT91C_UDPHS_EPT_14_EPTCFG (0xFFF782C0) // (UDPHS_EPT_14) Endpoint Config Register
#define AT91C_UDPHS_EPT_14_EPTCTL (0xFFF782CC) // (UDPHS_EPT_14) Endpoint Control Register
#define AT91C_UDPHS_EPT_14_EPTCTLENB (0xFFF782C4) // (UDPHS_EPT_14) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_14_EPTCLRSTA (0xFFF782D8) // (UDPHS_EPT_14) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_14_EPTCTLDIS (0xFFF782C8) // (UDPHS_EPT_14) Endpoint Control Disable Register
// ========== Register definition for UDPHS_EPT_15 peripheral ==========
#define AT91C_UDPHS_EPT_15_EPTCTLDIS (0xFFF782E8) // (UDPHS_EPT_15) Endpoint Control Disable Register
#define AT91C_UDPHS_EPT_15_EPTSETSTA (0xFFF782F4) // (UDPHS_EPT_15) Endpoint Set Status Register
#define AT91C_UDPHS_EPT_15_EPTCTLENB (0xFFF782E4) // (UDPHS_EPT_15) Endpoint Control Enable Register
#define AT91C_UDPHS_EPT_15_EPTCLRSTA (0xFFF782F8) // (UDPHS_EPT_15) Endpoint Clear Status Register
#define AT91C_UDPHS_EPT_15_EPTSTA (0xFFF782FC) // (UDPHS_EPT_15) Endpoint Status Register
#define AT91C_UDPHS_EPT_15_EPTCTL (0xFFF782EC) // (UDPHS_EPT_15) Endpoint Control Register
#define AT91C_UDPHS_EPT_15_EPTCFG (0xFFF782E0) // (UDPHS_EPT_15) Endpoint Config Register
// ========== Register definition for UDPHS_DMA_1 peripheral ==========
#define AT91C_UDPHS_DMA_1_DMASTATUS (0xFFF7831C) // (UDPHS_DMA_1) DMA Channel Status Register
#define AT91C_UDPHS_DMA_1_DMANXTDSC (0xFFF78310) // (UDPHS_DMA_1) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_1_DMACONTROL (0xFFF78318) // (UDPHS_DMA_1) DMA Channel Control Register
#define AT91C_UDPHS_DMA_1_DMAADDRESS (0xFFF78314) // (UDPHS_DMA_1) DMA Channel AHB1 Address Register
// ========== Register definition for UDPHS_DMA_2 peripheral ==========
#define AT91C_UDPHS_DMA_2_DMACONTROL (0xFFF78328) // (UDPHS_DMA_2) DMA Channel Control Register
#define AT91C_UDPHS_DMA_2_DMASTATUS (0xFFF7832C) // (UDPHS_DMA_2) DMA Channel Status Register
#define AT91C_UDPHS_DMA_2_DMAADDRESS (0xFFF78324) // (UDPHS_DMA_2) DMA Channel AHB1 Address Register
#define AT91C_UDPHS_DMA_2_DMANXTDSC (0xFFF78320) // (UDPHS_DMA_2) DMA Channel Next Descriptor Address
// ========== Register definition for UDPHS_DMA_3 peripheral ==========
#define AT91C_UDPHS_DMA_3_DMAADDRESS (0xFFF78334) // (UDPHS_DMA_3) DMA Channel AHB1 Address Register
#define AT91C_UDPHS_DMA_3_DMANXTDSC (0xFFF78330) // (UDPHS_DMA_3) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_3_DMACONTROL (0xFFF78338) // (UDPHS_DMA_3) DMA Channel Control Register
#define AT91C_UDPHS_DMA_3_DMASTATUS (0xFFF7833C) // (UDPHS_DMA_3) DMA Channel Status Register
// ========== Register definition for UDPHS_DMA_4 peripheral ==========
#define AT91C_UDPHS_DMA_4_DMANXTDSC (0xFFF78340) // (UDPHS_DMA_4) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_4_DMAADDRESS (0xFFF78344) // (UDPHS_DMA_4) DMA Channel AHB1 Address Register
#define AT91C_UDPHS_DMA_4_DMACONTROL (0xFFF78348) // (UDPHS_DMA_4) DMA Channel Control Register
#define AT91C_UDPHS_DMA_4_DMASTATUS (0xFFF7834C) // (UDPHS_DMA_4) DMA Channel Status Register
// ========== Register definition for UDPHS_DMA_5 peripheral ==========
#define AT91C_UDPHS_DMA_5_DMASTATUS (0xFFF7835C) // (UDPHS_DMA_5) DMA Channel Status Register
#define AT91C_UDPHS_DMA_5_DMACONTROL (0xFFF78358) // (UDPHS_DMA_5) DMA Channel Control Register
#define AT91C_UDPHS_DMA_5_DMANXTDSC (0xFFF78350) // (UDPHS_DMA_5) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_5_DMAADDRESS (0xFFF78354) // (UDPHS_DMA_5) DMA Channel AHB1 Address Register
// ========== Register definition for UDPHS_DMA_6 peripheral ==========
#define AT91C_UDPHS_DMA_6_DMANXTDSC (0xFFF78360) // (UDPHS_DMA_6) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_6_DMACONTROL (0xFFF78368) // (UDPHS_DMA_6) DMA Channel Control Register
#define AT91C_UDPHS_DMA_6_DMASTATUS (0xFFF7836C) // (UDPHS_DMA_6) DMA Channel Status Register
#define AT91C_UDPHS_DMA_6_DMAADDRESS (0xFFF78364) // (UDPHS_DMA_6) DMA Channel AHB1 Address Register
// ========== Register definition for UDPHS_DMA_7 peripheral ==========
#define AT91C_UDPHS_DMA_7_DMANXTDSC (0xFFF78370) // (UDPHS_DMA_7) DMA Channel Next Descriptor Address
#define AT91C_UDPHS_DMA_7_DMAADDRESS (0xFFF78374) // (UDPHS_DMA_7) DMA Channel AHB1 Address Register
#define AT91C_UDPHS_DMA_7_DMACONTROL (0xFFF78378) // (UDPHS_DMA_7) DMA Channel Control Register
#define AT91C_UDPHS_DMA_7_DMASTATUS (0xFFF7837C) // (UDPHS_DMA_7) DMA Channel Status Register
// ========== Register definition for UDPHS peripheral ==========
#define AT91C_UDPHS_IEN           (0xFFF78010) // (UDPHS) USB Interrupt Enable Register
#define AT91C_UDPHS_IPFEATURES    (0xFFF780F8) // (UDPHS) HUSB2DEV Features Register
#define AT91C_UDPHS_TST           (0xFFF780E0) // (UDPHS) USB Test Register
#define AT91C_UDPHS_FNUM          (0xFFF78004) // (UDPHS) USB Frame Number Register
#define AT91C_UDPHS_TSTCNTB       (0xFFF780D8) // (UDPHS) USB Test CounterB Register
#define AT91C_UDPHS_IPPADDRSIZE   (0xFFF780EC) // (UDPHS) HUSB2DEV PADDRSIZE Register
#define AT91C_UDPHS_INTSTA        (0xFFF78014) // (UDPHS) USB Interrupt Status Register
#define AT91C_UDPHS_EPTRST        (0xFFF7801C) // (UDPHS) USB Endpoints Reset Register
#define AT91C_UDPHS_TSTCNTA       (0xFFF780D4) // (UDPHS) USB Test CounterA Register
#define AT91C_UDPHS_IPNAME2       (0xFFF780F4) // (UDPHS) HUSB2DEV Name2 Register
#define AT91C_UDPHS_IPNAME1       (0xFFF780F0) // (UDPHS) HUSB2DEV Name1 Register
#define AT91C_UDPHS_CLRINT        (0xFFF78018) // (UDPHS) USB Clear Interrupt Register
#define AT91C_UDPHS_IPVERSION     (0xFFF780FC) // (UDPHS) HUSB2DEV Version Register
#define AT91C_UDPHS_CTRL          (0xFFF78000) // (UDPHS) USB Control Register
// ========== Register definition for TC0 peripheral ==========
#define AT91C_TC0_IER             (0xFFF7C024) // (TC0) Interrupt Enable Register
#define AT91C_TC0_IMR             (0xFFF7C02C) // (TC0) Interrupt Mask Register
#define AT91C_TC0_CCR             (0xFFF7C000) // (TC0) Channel Control Register
#define AT91C_TC0_RB              (0xFFF7C018) // (TC0) Register B
#define AT91C_TC0_CV              (0xFFF7C010) // (TC0) Counter Value
#define AT91C_TC0_SR              (0xFFF7C020) // (TC0) Status Register
#define AT91C_TC0_CMR             (0xFFF7C004) // (TC0) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC0_RA              (0xFFF7C014) // (TC0) Register A
#define AT91C_TC0_RC              (0xFFF7C01C) // (TC0) Register C
#define AT91C_TC0_IDR             (0xFFF7C028) // (TC0) Interrupt Disable Register
// ========== Register definition for TC1 peripheral ==========
#define AT91C_TC1_IER             (0xFFF7C064) // (TC1) Interrupt Enable Register
#define AT91C_TC1_SR              (0xFFF7C060) // (TC1) Status Register
#define AT91C_TC1_RC              (0xFFF7C05C) // (TC1) Register C
#define AT91C_TC1_CV              (0xFFF7C050) // (TC1) Counter Value
#define AT91C_TC1_RA              (0xFFF7C054) // (TC1) Register A
#define AT91C_TC1_CMR             (0xFFF7C044) // (TC1) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC1_IDR             (0xFFF7C068) // (TC1) Interrupt Disable Register
#define AT91C_TC1_RB              (0xFFF7C058) // (TC1) Register B
#define AT91C_TC1_IMR             (0xFFF7C06C) // (TC1) Interrupt Mask Register
#define AT91C_TC1_CCR             (0xFFF7C040) // (TC1) Channel Control Register
// ========== Register definition for TC2 peripheral ==========
#define AT91C_TC2_SR              (0xFFF7C0A0) // (TC2) Status Register
#define AT91C_TC2_IMR             (0xFFF7C0AC) // (TC2) Interrupt Mask Register
#define AT91C_TC2_IER             (0xFFF7C0A4) // (TC2) Interrupt Enable Register
#define AT91C_TC2_CV              (0xFFF7C090) // (TC2) Counter Value
#define AT91C_TC2_RB              (0xFFF7C098) // (TC2) Register B
#define AT91C_TC2_CCR             (0xFFF7C080) // (TC2) Channel Control Register
#define AT91C_TC2_CMR             (0xFFF7C084) // (TC2) Channel Mode Register (Capture Mode / Waveform Mode)
#define AT91C_TC2_RA              (0xFFF7C094) // (TC2) Register A
#define AT91C_TC2_IDR             (0xFFF7C0A8) // (TC2) Interrupt Disable Register
#define AT91C_TC2_RC              (0xFFF7C09C) // (TC2) Register C
// ========== Register definition for TCB0 peripheral ==========
#define AT91C_TCB0_BCR            (0xFFF7C0C0) // (TCB0) TC Block Control Register
#define AT91C_TCB0_BMR            (0xFFF7C0C4) // (TCB0) TC Block Mode Register
// ========== Register definition for TCB1 peripheral ==========
#define AT91C_TCB1_BMR            (0xFFF7C104) // (TCB1) TC Block Mode Register
#define AT91C_TCB1_BCR            (0xFFF7C100) // (TCB1) TC Block Control Register
// ========== Register definition for TCB2 peripheral ==========
#define AT91C_TCB2_BCR            (0xFFF7C140) // (TCB2) TC Block Control Register
#define AT91C_TCB2_BMR            (0xFFF7C144) // (TCB2) TC Block Mode Register
// ========== Register definition for PDC_MCI0 peripheral ==========
#define AT91C_MCI0_TCR            (0xFFF8010C) // (PDC_MCI0) Transmit Counter Register
#define AT91C_MCI0_TNCR           (0xFFF8011C) // (PDC_MCI0) Transmit Next Counter Register
#define AT91C_MCI0_RNPR           (0xFFF80110) // (PDC_MCI0) Receive Next Pointer Register
#define AT91C_MCI0_TPR            (0xFFF80108) // (PDC_MCI0) Transmit Pointer Register
#define AT91C_MCI0_TNPR           (0xFFF80118) // (PDC_MCI0) Transmit Next Pointer Register
#define AT91C_MCI0_PTSR           (0xFFF80124) // (PDC_MCI0) PDC Transfer Status Register
#define AT91C_MCI0_RCR            (0xFFF80104) // (PDC_MCI0) Receive Counter Register
#define AT91C_MCI0_PTCR           (0xFFF80120) // (PDC_MCI0) PDC Transfer Control Register
#define AT91C_MCI0_RPR            (0xFFF80100) // (PDC_MCI0) Receive Pointer Register
#define AT91C_MCI0_RNCR           (0xFFF80114) // (PDC_MCI0) Receive Next Counter Register
// ========== Register definition for MCI0 peripheral ==========
#define AT91C_MCI0_CMDR           (0xFFF80014) // (MCI0) MCI Command Register
#define AT91C_MCI0_IMR            (0xFFF8004C) // (MCI0) MCI Interrupt Mask Register
#define AT91C_MCI0_MR             (0xFFF80004) // (MCI0) MCI Mode Register
#define AT91C_MCI0_CR             (0xFFF80000) // (MCI0) MCI Control Register
#define AT91C_MCI0_IER            (0xFFF80044) // (MCI0) MCI Interrupt Enable Register
#define AT91C_MCI0_RDR            (0xFFF80030) // (MCI0) MCI Receive Data Register
#define AT91C_MCI0_SR             (0xFFF80040) // (MCI0) MCI Status Register
#define AT91C_MCI0_DTOR           (0xFFF80008) // (MCI0) MCI Data Timeout Register
#define AT91C_MCI0_SDCR           (0xFFF8000C) // (MCI0) MCI SD Card Register
#define AT91C_MCI0_BLKR           (0xFFF80018) // (MCI0) MCI Block Register
#define AT91C_MCI0_VR             (0xFFF800FC) // (MCI0) MCI Version Register
#define AT91C_MCI0_TDR            (0xFFF80034) // (MCI0) MCI Transmit Data Register
#define AT91C_MCI0_ARGR           (0xFFF80010) // (MCI0) MCI Argument Register
#define AT91C_MCI0_RSPR           (0xFFF80020) // (MCI0) MCI Response Register
#define AT91C_MCI0_IDR            (0xFFF80048) // (MCI0) MCI Interrupt Disable Register
// ========== Register definition for PDC_MCI1 peripheral ==========
#define AT91C_MCI1_PTCR           (0xFFF84120) // (PDC_MCI1) PDC Transfer Control Register
#define AT91C_MCI1_PTSR           (0xFFF84124) // (PDC_MCI1) PDC Transfer Status Register
#define AT91C_MCI1_TPR            (0xFFF84108) // (PDC_MCI1) Transmit Pointer Register
#define AT91C_MCI1_RPR            (0xFFF84100) // (PDC_MCI1) Receive Pointer Register
#define AT91C_MCI1_TNCR           (0xFFF8411C) // (PDC_MCI1) Transmit Next Counter Register
#define AT91C_MCI1_RCR            (0xFFF84104) // (PDC_MCI1) Receive Counter Register
#define AT91C_MCI1_TNPR           (0xFFF84118) // (PDC_MCI1) Transmit Next Pointer Register
#define AT91C_MCI1_TCR            (0xFFF8410C) // (PDC_MCI1) Transmit Counter Register
#define AT91C_MCI1_RNPR           (0xFFF84110) // (PDC_MCI1) Receive Next Pointer Register
#define AT91C_MCI1_RNCR           (0xFFF84114) // (PDC_MCI1) Receive Next Counter Register
// ========== Register definition for MCI1 peripheral ==========
#define AT91C_MCI1_SR             (0xFFF84040) // (MCI1) MCI Status Register
#define AT91C_MCI1_RDR            (0xFFF84030) // (MCI1) MCI Receive Data Register
#define AT91C_MCI1_RSPR           (0xFFF84020) // (MCI1) MCI Response Register
#define AT91C_MCI1_CMDR           (0xFFF84014) // (MCI1) MCI Command Register
#define AT91C_MCI1_IMR            (0xFFF8404C) // (MCI1) MCI Interrupt Mask Register
#define AT91C_MCI1_DTOR           (0xFFF84008) // (MCI1) MCI Data Timeout Register
#define AT91C_MCI1_SDCR           (0xFFF8400C) // (MCI1) MCI SD Card Register
#define AT91C_MCI1_IDR            (0xFFF84048) // (MCI1) MCI Interrupt Disable Register
#define AT91C_MCI1_ARGR           (0xFFF84010) // (MCI1) MCI Argument Register
#define AT91C_MCI1_TDR            (0xFFF84034) // (MCI1) MCI Transmit Data Register
#define AT91C_MCI1_BLKR           (0xFFF84018) // (MCI1) MCI Block Register
#define AT91C_MCI1_VR             (0xFFF840FC) // (MCI1) MCI Version Register
#define AT91C_MCI1_CR             (0xFFF84000) // (MCI1) MCI Control Register
#define AT91C_MCI1_MR             (0xFFF84004) // (MCI1) MCI Mode Register
#define AT91C_MCI1_IER            (0xFFF84044) // (MCI1) MCI Interrupt Enable Register
// ========== Register definition for PDC_TWI peripheral ==========
#define AT91C_TWI_PTSR            (0xFFF88124) // (PDC_TWI) PDC Transfer Status Register
#define AT91C_TWI_RNCR            (0xFFF88114) // (PDC_TWI) Receive Next Counter Register
#define AT91C_TWI_RCR             (0xFFF88104) // (PDC_TWI) Receive Counter Register
#define AT91C_TWI_RNPR            (0xFFF88110) // (PDC_TWI) Receive Next Pointer Register
#define AT91C_TWI_TCR             (0xFFF8810C) // (PDC_TWI) Transmit Counter Register
#define AT91C_TWI_RPR             (0xFFF88100) // (PDC_TWI) Receive Pointer Register
#define AT91C_TWI_PTCR            (0xFFF88120) // (PDC_TWI) PDC Transfer Control Register
#define AT91C_TWI_TPR             (0xFFF88108) // (PDC_TWI) Transmit Pointer Register
#define AT91C_TWI_TNPR            (0xFFF88118) // (PDC_TWI) Transmit Next Pointer Register
#define AT91C_TWI_TNCR            (0xFFF8811C) // (PDC_TWI) Transmit Next Counter Register
// ========== Register definition for TWI peripheral ==========
#define AT91C_TWI_IDR             (0xFFF88028) // (TWI) Interrupt Disable Register
#define AT91C_TWI_RHR             (0xFFF88030) // (TWI) Receive Holding Register
#define AT91C_TWI_IMR             (0xFFF8802C) // (TWI) Interrupt Mask Register
#define AT91C_TWI_THR             (0xFFF88034) // (TWI) Transmit Holding Register
#define AT91C_TWI_IER             (0xFFF88024) // (TWI) Interrupt Enable Register
#define AT91C_TWI_IADR            (0xFFF8800C) // (TWI) Internal Address Register
#define AT91C_TWI_SMR             (0xFFF88008) // (TWI) Slave Mode Register
#define AT91C_TWI_MMR             (0xFFF88004) // (TWI) Master Mode Register
#define AT91C_TWI_CR              (0xFFF88000) // (TWI) Control Register
#define AT91C_TWI_SR              (0xFFF88020) // (TWI) Status Register
#define AT91C_TWI_CWGR            (0xFFF88010) // (TWI) Clock Waveform Generator Register
// ========== Register definition for PDC_US0 peripheral ==========
#define AT91C_US0_TNPR            (0xFFF8C118) // (PDC_US0) Transmit Next Pointer Register
#define AT91C_US0_PTSR            (0xFFF8C124) // (PDC_US0) PDC Transfer Status Register
#define AT91C_US0_PTCR            (0xFFF8C120) // (PDC_US0) PDC Transfer Control Register
#define AT91C_US0_RNCR            (0xFFF8C114) // (PDC_US0) Receive Next Counter Register
#define AT91C_US0_RCR             (0xFFF8C104) // (PDC_US0) Receive Counter Register
#define AT91C_US0_TNCR            (0xFFF8C11C) // (PDC_US0) Transmit Next Counter Register
#define AT91C_US0_TCR             (0xFFF8C10C) // (PDC_US0) Transmit Counter Register
#define AT91C_US0_RNPR            (0xFFF8C110) // (PDC_US0) Receive Next Pointer Register
#define AT91C_US0_RPR             (0xFFF8C100) // (PDC_US0) Receive Pointer Register
#define AT91C_US0_TPR             (0xFFF8C108) // (PDC_US0) Transmit Pointer Register
// ========== Register definition for US0 peripheral ==========
#define AT91C_US0_RTOR            (0xFFF8C024) // (US0) Receiver Time-out Register
#define AT91C_US0_NER             (0xFFF8C044) // (US0) Nb Errors Register
#define AT91C_US0_THR             (0xFFF8C01C) // (US0) Transmitter Holding Register
#define AT91C_US0_MR              (0xFFF8C004) // (US0) Mode Register
#define AT91C_US0_RHR             (0xFFF8C018) // (US0) Receiver Holding Register
#define AT91C_US0_CSR             (0xFFF8C014) // (US0) Channel Status Register
#define AT91C_US0_IMR             (0xFFF8C010) // (US0) Interrupt Mask Register
#define AT91C_US0_IDR             (0xFFF8C00C) // (US0) Interrupt Disable Register
#define AT91C_US0_FIDI            (0xFFF8C040) // (US0) FI_DI_Ratio Register
#define AT91C_US0_CR              (0xFFF8C000) // (US0) Control Register
#define AT91C_US0_IER             (0xFFF8C008) // (US0) Interrupt Enable Register
#define AT91C_US0_TTGR            (0xFFF8C028) // (US0) Transmitter Time-guard Register
#define AT91C_US0_BRGR            (0xFFF8C020) // (US0) Baud Rate Generator Register
#define AT91C_US0_IF              (0xFFF8C04C) // (US0) IRDA_FILTER Register
// ========== Register definition for PDC_US1 peripheral ==========
#define AT91C_US1_PTCR            (0xFFF90120) // (PDC_US1) PDC Transfer Control Register
#define AT91C_US1_TNCR            (0xFFF9011C) // (PDC_US1) Transmit Next Counter Register
#define AT91C_US1_RCR             (0xFFF90104) // (PDC_US1) Receive Counter Register
#define AT91C_US1_RPR             (0xFFF90100) // (PDC_US1) Receive Pointer Register
#define AT91C_US1_TPR             (0xFFF90108) // (PDC_US1) Transmit Pointer Register
#define AT91C_US1_TCR             (0xFFF9010C) // (PDC_US1) Transmit Counter Register
#define AT91C_US1_RNPR            (0xFFF90110) // (PDC_US1) Receive Next Pointer Register
#define AT91C_US1_TNPR            (0xFFF90118) // (PDC_US1) Transmit Next Pointer Register
#define AT91C_US1_RNCR            (0xFFF90114) // (PDC_US1) Receive Next Counter Register
#define AT91C_US1_PTSR            (0xFFF90124) // (PDC_US1) PDC Transfer Status Register
// ========== Register definition for US1 peripheral ==========
#define AT91C_US1_NER             (0xFFF90044) // (US1) Nb Errors Register
#define AT91C_US1_RHR             (0xFFF90018) // (US1) Receiver Holding Register
#define AT91C_US1_RTOR            (0xFFF90024) // (US1) Receiver Time-out Register
#define AT91C_US1_IER             (0xFFF90008) // (US1) Interrupt Enable Register
#define AT91C_US1_IF              (0xFFF9004C) // (US1) IRDA_FILTER Register
#define AT91C_US1_CR              (0xFFF90000) // (US1) Control Register
#define AT91C_US1_IMR             (0xFFF90010) // (US1) Interrupt Mask Register
#define AT91C_US1_TTGR            (0xFFF90028) // (US1) Transmitter Time-guard Register
#define AT91C_US1_MR              (0xFFF90004) // (US1) Mode Register
#define AT91C_US1_IDR             (0xFFF9000C) // (US1) Interrupt Disable Register
#define AT91C_US1_FIDI            (0xFFF90040) // (US1) FI_DI_Ratio Register
#define AT91C_US1_CSR             (0xFFF90014) // (US1) Channel Status Register
#define AT91C_US1_THR             (0xFFF9001C) // (US1) Transmitter Holding Register
#define AT91C_US1_BRGR            (0xFFF90020) // (US1) Baud Rate Generator Register
// ========== Register definition for PDC_US2 peripheral ==========
#define AT91C_US2_RNCR            (0xFFF94114) // (PDC_US2) Receive Next Counter Register
#define AT91C_US2_PTCR            (0xFFF94120) // (PDC_US2) PDC Transfer Control Register
#define AT91C_US2_TNPR            (0xFFF94118) // (PDC_US2) Transmit Next Pointer Register
#define AT91C_US2_TNCR            (0xFFF9411C) // (PDC_US2) Transmit Next Counter Register
#define AT91C_US2_TPR             (0xFFF94108) // (PDC_US2) Transmit Pointer Register
#define AT91C_US2_RCR             (0xFFF94104) // (PDC_US2) Receive Counter Register
#define AT91C_US2_PTSR            (0xFFF94124) // (PDC_US2) PDC Transfer Status Register
#define AT91C_US2_TCR             (0xFFF9410C) // (PDC_US2) Transmit Counter Register
#define AT91C_US2_RPR             (0xFFF94100) // (PDC_US2) Receive Pointer Register
#define AT91C_US2_RNPR            (0xFFF94110) // (PDC_US2) Receive Next Pointer Register
// ========== Register definition for US2 peripheral ==========
#define AT91C_US2_TTGR            (0xFFF94028) // (US2) Transmitter Time-guard Register
#define AT91C_US2_RHR             (0xFFF94018) // (US2) Receiver Holding Register
#define AT91C_US2_IMR             (0xFFF94010) // (US2) Interrupt Mask Register
#define AT91C_US2_IER             (0xFFF94008) // (US2) Interrupt Enable Register
#define AT91C_US2_NER             (0xFFF94044) // (US2) Nb Errors Register
#define AT91C_US2_CR              (0xFFF94000) // (US2) Control Register
#define AT91C_US2_FIDI            (0xFFF94040) // (US2) FI_DI_Ratio Register
#define AT91C_US2_MR              (0xFFF94004) // (US2) Mode Register
#define AT91C_US2_IDR             (0xFFF9400C) // (US2) Interrupt Disable Register
#define AT91C_US2_THR             (0xFFF9401C) // (US2) Transmitter Holding Register
#define AT91C_US2_IF              (0xFFF9404C) // (US2) IRDA_FILTER Register
#define AT91C_US2_BRGR            (0xFFF94020) // (US2) Baud Rate Generator Register
#define AT91C_US2_CSR             (0xFFF94014) // (US2) Channel Status Register
#define AT91C_US2_RTOR            (0xFFF94024) // (US2) Receiver Time-out Register
// ========== Register definition for PDC_SSC0 peripheral ==========
#define AT91C_SSC0_PTSR           (0xFFF98124) // (PDC_SSC0) PDC Transfer Status Register
#define AT91C_SSC0_TCR            (0xFFF9810C) // (PDC_SSC0) Transmit Counter Register
#define AT91C_SSC0_RNPR           (0xFFF98110) // (PDC_SSC0) Receive Next Pointer Register
#define AT91C_SSC0_RNCR           (0xFFF98114) // (PDC_SSC0) Receive Next Counter Register
#define AT91C_SSC0_TNPR           (0xFFF98118) // (PDC_SSC0) Transmit Next Pointer Register
#define AT91C_SSC0_RPR            (0xFFF98100) // (PDC_SSC0) Receive Pointer Register
#define AT91C_SSC0_TPR            (0xFFF98108) // (PDC_SSC0) Transmit Pointer Register
#define AT91C_SSC0_RCR            (0xFFF98104) // (PDC_SSC0) Receive Counter Register
#define AT91C_SSC0_TNCR           (0xFFF9811C) // (PDC_SSC0) Transmit Next Counter Register
#define AT91C_SSC0_PTCR           (0xFFF98120) // (PDC_SSC0) PDC Transfer Control Register
// ========== Register definition for SSC0 peripheral ==========
#define AT91C_SSC0_RFMR           (0xFFF98014) // (SSC0) Receive Frame Mode Register
#define AT91C_SSC0_RHR            (0xFFF98020) // (SSC0) Receive Holding Register
#define AT91C_SSC0_THR            (0xFFF98024) // (SSC0) Transmit Holding Register
#define AT91C_SSC0_CMR            (0xFFF98004) // (SSC0) Clock Mode Register
#define AT91C_SSC0_IMR            (0xFFF9804C) // (SSC0) Interrupt Mask Register
#define AT91C_SSC0_IDR            (0xFFF98048) // (SSC0) Interrupt Disable Register
#define AT91C_SSC0_IER            (0xFFF98044) // (SSC0) Interrupt Enable Register
#define AT91C_SSC0_TSHR           (0xFFF98034) // (SSC0) Transmit Sync Holding Register
#define AT91C_SSC0_SR             (0xFFF98040) // (SSC0) Status Register
#define AT91C_SSC0_CR             (0xFFF98000) // (SSC0) Control Register
#define AT91C_SSC0_RCMR           (0xFFF98010) // (SSC0) Receive Clock ModeRegister
#define AT91C_SSC0_TFMR           (0xFFF9801C) // (SSC0) Transmit Frame Mode Register
#define AT91C_SSC0_RSHR           (0xFFF98030) // (SSC0) Receive Sync Holding Register
#define AT91C_SSC0_TCMR           (0xFFF98018) // (SSC0) Transmit Clock Mode Register
// ========== Register definition for PDC_SSC1 peripheral ==========
#define AT91C_SSC1_TNPR           (0xFFF9C118) // (PDC_SSC1) Transmit Next Pointer Register
#define AT91C_SSC1_PTSR           (0xFFF9C124) // (PDC_SSC1) PDC Transfer Status Register
#define AT91C_SSC1_TNCR           (0xFFF9C11C) // (PDC_SSC1) Transmit Next Counter Register
#define AT91C_SSC1_RNCR           (0xFFF9C114) // (PDC_SSC1) Receive Next Counter Register
#define AT91C_SSC1_TPR            (0xFFF9C108) // (PDC_SSC1) Transmit Pointer Register
#define AT91C_SSC1_RCR            (0xFFF9C104) // (PDC_SSC1) Receive Counter Register
#define AT91C_SSC1_PTCR           (0xFFF9C120) // (PDC_SSC1) PDC Transfer Control Register
#define AT91C_SSC1_RNPR           (0xFFF9C110) // (PDC_SSC1) Receive Next Pointer Register
#define AT91C_SSC1_TCR            (0xFFF9C10C) // (PDC_SSC1) Transmit Counter Register
#define AT91C_SSC1_RPR            (0xFFF9C100) // (PDC_SSC1) Receive Pointer Register
// ========== Register definition for SSC1 peripheral ==========
#define AT91C_SSC1_CMR            (0xFFF9C004) // (SSC1) Clock Mode Register
#define AT91C_SSC1_SR             (0xFFF9C040) // (SSC1) Status Register
#define AT91C_SSC1_TSHR           (0xFFF9C034) // (SSC1) Transmit Sync Holding Register
#define AT91C_SSC1_TCMR           (0xFFF9C018) // (SSC1) Transmit Clock Mode Register
#define AT91C_SSC1_IMR            (0xFFF9C04C) // (SSC1) Interrupt Mask Register
#define AT91C_SSC1_IDR            (0xFFF9C048) // (SSC1) Interrupt Disable Register
#define AT91C_SSC1_RCMR           (0xFFF9C010) // (SSC1) Receive Clock ModeRegister
#define AT91C_SSC1_IER            (0xFFF9C044) // (SSC1) Interrupt Enable Register
#define AT91C_SSC1_RSHR           (0xFFF9C030) // (SSC1) Receive Sync Holding Register
#define AT91C_SSC1_CR             (0xFFF9C000) // (SSC1) Control Register
#define AT91C_SSC1_RHR            (0xFFF9C020) // (SSC1) Receive Holding Register
#define AT91C_SSC1_THR            (0xFFF9C024) // (SSC1) Transmit Holding Register
#define AT91C_SSC1_RFMR           (0xFFF9C014) // (SSC1) Receive Frame Mode Register
#define AT91C_SSC1_TFMR           (0xFFF9C01C) // (SSC1) Transmit Frame Mode Register
// ========== Register definition for PDC_AC97C peripheral ==========
#define AT91C_AC97C_RNPR          (0xFFFA0110) // (PDC_AC97C) Receive Next Pointer Register
#define AT91C_AC97C_TCR           (0xFFFA010C) // (PDC_AC97C) Transmit Counter Register
#define AT91C_AC97C_TNCR          (0xFFFA011C) // (PDC_AC97C) Transmit Next Counter Register
#define AT91C_AC97C_RCR           (0xFFFA0104) // (PDC_AC97C) Receive Counter Register
#define AT91C_AC97C_RNCR          (0xFFFA0114) // (PDC_AC97C) Receive Next Counter Register
#define AT91C_AC97C_PTCR          (0xFFFA0120) // (PDC_AC97C) PDC Transfer Control Register
#define AT91C_AC97C_TPR           (0xFFFA0108) // (PDC_AC97C) Transmit Pointer Register
#define AT91C_AC97C_RPR           (0xFFFA0100) // (PDC_AC97C) Receive Pointer Register
#define AT91C_AC97C_PTSR          (0xFFFA0124) // (PDC_AC97C) PDC Transfer Status Register
#define AT91C_AC97C_TNPR          (0xFFFA0118) // (PDC_AC97C) Transmit Next Pointer Register
// ========== Register definition for AC97C peripheral ==========
#define AT91C_AC97C_CORHR         (0xFFFA0040) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_MR            (0xFFFA0008) // (AC97C) Mode Register
#define AT91C_AC97C_CATHR         (0xFFFA0024) // (AC97C) Channel A Transmit Holding Register
#define AT91C_AC97C_IER           (0xFFFA0054) // (AC97C) Interrupt Enable Register
#define AT91C_AC97C_CASR          (0xFFFA0028) // (AC97C) Channel A Status Register
#define AT91C_AC97C_CBTHR         (0xFFFA0034) // (AC97C) Channel B Transmit Holding Register (optional)
#define AT91C_AC97C_ICA           (0xFFFA0010) // (AC97C) Input Channel AssignementRegister
#define AT91C_AC97C_IMR           (0xFFFA005C) // (AC97C) Interrupt Mask Register
#define AT91C_AC97C_IDR           (0xFFFA0058) // (AC97C) Interrupt Disable Register
#define AT91C_AC97C_CARHR         (0xFFFA0020) // (AC97C) Channel A Receive Holding Register
#define AT91C_AC97C_VERSION       (0xFFFA00FC) // (AC97C) Version Register
#define AT91C_AC97C_CBRHR         (0xFFFA0030) // (AC97C) Channel B Receive Holding Register (optional)
#define AT91C_AC97C_COTHR         (0xFFFA0044) // (AC97C) COdec Transmit Holding Register
#define AT91C_AC97C_OCA           (0xFFFA0014) // (AC97C) Output Channel Assignement Register
#define AT91C_AC97C_CBMR          (0xFFFA003C) // (AC97C) Channel B Mode Register
#define AT91C_AC97C_COMR          (0xFFFA004C) // (AC97C) CODEC Mask Status Register
#define AT91C_AC97C_CBSR          (0xFFFA0038) // (AC97C) Channel B Status Register
#define AT91C_AC97C_COSR          (0xFFFA0048) // (AC97C) CODEC Status Register
#define AT91C_AC97C_CAMR          (0xFFFA002C) // (AC97C) Channel A Mode Register
#define AT91C_AC97C_SR            (0xFFFA0050) // (AC97C) Status Register
// ========== Register definition for PDC_SPI0 peripheral ==========
#define AT91C_SPI0_TPR            (0xFFFA4108) // (PDC_SPI0) Transmit Pointer Register
#define AT91C_SPI0_PTCR           (0xFFFA4120) // (PDC_SPI0) PDC Transfer Control Register
#define AT91C_SPI0_RNPR           (0xFFFA4110) // (PDC_SPI0) Receive Next Pointer Register
#define AT91C_SPI0_TNCR           (0xFFFA411C) // (PDC_SPI0) Transmit Next Counter Register
#define AT91C_SPI0_TCR            (0xFFFA410C) // (PDC_SPI0) Transmit Counter Register
#define AT91C_SPI0_RCR            (0xFFFA4104) // (PDC_SPI0) Receive Counter Register
#define AT91C_SPI0_RNCR           (0xFFFA4114) // (PDC_SPI0) Receive Next Counter Register
#define AT91C_SPI0_TNPR           (0xFFFA4118) // (PDC_SPI0) Transmit Next Pointer Register
#define AT91C_SPI0_RPR            (0xFFFA4100) // (PDC_SPI0) Receive Pointer Register
#define AT91C_SPI0_PTSR           (0xFFFA4124) // (PDC_SPI0) PDC Transfer Status Register
// ========== Register definition for SPI0 peripheral ==========
#define AT91C_SPI0_MR             (0xFFFA4004) // (SPI0) Mode Register
#define AT91C_SPI0_RDR            (0xFFFA4008) // (SPI0) Receive Data Register
#define AT91C_SPI0_CR             (0xFFFA4000) // (SPI0) Control Register
#define AT91C_SPI0_IER            (0xFFFA4014) // (SPI0) Interrupt Enable Register
#define AT91C_SPI0_TDR            (0xFFFA400C) // (SPI0) Transmit Data Register
#define AT91C_SPI0_IDR            (0xFFFA4018) // (SPI0) Interrupt Disable Register
#define AT91C_SPI0_CSR            (0xFFFA4030) // (SPI0) Chip Select Register
#define AT91C_SPI0_SR             (0xFFFA4010) // (SPI0) Status Register
#define AT91C_SPI0_IMR            (0xFFFA401C) // (SPI0) Interrupt Mask Register
// ========== Register definition for PDC_SPI1 peripheral ==========
#define AT91C_SPI1_RNCR           (0xFFFA8114) // (PDC_SPI1) Receive Next Counter Register
#define AT91C_SPI1_TCR            (0xFFFA810C) // (PDC_SPI1) Transmit Counter Register
#define AT91C_SPI1_RCR            (0xFFFA8104) // (PDC_SPI1) Receive Counter Register
#define AT91C_SPI1_TNPR           (0xFFFA8118) // (PDC_SPI1) Transmit Next Pointer Register
#define AT91C_SPI1_RNPR           (0xFFFA8110) // (PDC_SPI1) Receive Next Pointer Register
#define AT91C_SPI1_RPR            (0xFFFA8100) // (PDC_SPI1) Receive Pointer Register
#define AT91C_SPI1_TNCR           (0xFFFA811C) // (PDC_SPI1) Transmit Next Counter Register
#define AT91C_SPI1_TPR            (0xFFFA8108) // (PDC_SPI1) Transmit Pointer Register
#define AT91C_SPI1_PTSR           (0xFFFA8124) // (PDC_SPI1) PDC Transfer Status Register
#define AT91C_SPI1_PTCR           (0xFFFA8120) // (PDC_SPI1) PDC Transfer Control Register
// ========== Register definition for SPI1 peripheral ==========
#define AT91C_SPI1_CSR            (0xFFFA8030) // (SPI1) Chip Select Register
#define AT91C_SPI1_IER            (0xFFFA8014) // (SPI1) Interrupt Enable Register
#define AT91C_SPI1_RDR            (0xFFFA8008) // (SPI1) Receive Data Register
#define AT91C_SPI1_IDR            (0xFFFA8018) // (SPI1) Interrupt Disable Register
#define AT91C_SPI1_MR             (0xFFFA8004) // (SPI1) Mode Register
#define AT91C_SPI1_CR             (0xFFFA8000) // (SPI1) Control Register
#define AT91C_SPI1_SR             (0xFFFA8010) // (SPI1) Status Register
#define AT91C_SPI1_TDR            (0xFFFA800C) // (SPI1) Transmit Data Register
#define AT91C_SPI1_IMR            (0xFFFA801C) // (SPI1) Interrupt Mask Register
// ========== Register definition for CAN_MB0 peripheral ==========
#define AT91C_CAN_MB0_MID         (0xFFFAC208) // (CAN_MB0) MailBox ID Register
#define AT91C_CAN_MB0_MFID        (0xFFFAC20C) // (CAN_MB0) MailBox Family ID Register
#define AT91C_CAN_MB0_MAM         (0xFFFAC204) // (CAN_MB0) MailBox Acceptance Mask Register
#define AT91C_CAN_MB0_MCR         (0xFFFAC21C) // (CAN_MB0) MailBox Control Register
#define AT91C_CAN_MB0_MMR         (0xFFFAC200) // (CAN_MB0) MailBox Mode Register
#define AT91C_CAN_MB0_MDL         (0xFFFAC214) // (CAN_MB0) MailBox Data Low Register
#define AT91C_CAN_MB0_MDH         (0xFFFAC218) // (CAN_MB0) MailBox Data High Register
#define AT91C_CAN_MB0_MSR         (0xFFFAC210) // (CAN_MB0) MailBox Status Register
// ========== Register definition for CAN_MB1 peripheral ==========
#define AT91C_CAN_MB1_MDL         (0xFFFAC234) // (CAN_MB1) MailBox Data Low Register
#define AT91C_CAN_MB1_MAM         (0xFFFAC224) // (CAN_MB1) MailBox Acceptance Mask Register
#define AT91C_CAN_MB1_MID         (0xFFFAC228) // (CAN_MB1) MailBox ID Register
#define AT91C_CAN_MB1_MMR         (0xFFFAC220) // (CAN_MB1) MailBox Mode Register
#define AT91C_CAN_MB1_MCR         (0xFFFAC23C) // (CAN_MB1) MailBox Control Register
#define AT91C_CAN_MB1_MFID        (0xFFFAC22C) // (CAN_MB1) MailBox Family ID Register
#define AT91C_CAN_MB1_MSR         (0xFFFAC230) // (CAN_MB1) MailBox Status Register
#define AT91C_CAN_MB1_MDH         (0xFFFAC238) // (CAN_MB1) MailBox Data High Register
// ========== Register definition for CAN_MB2 peripheral ==========
#define AT91C_CAN_MB2_MID         (0xFFFAC248) // (CAN_MB2) MailBox ID Register
#define AT91C_CAN_MB2_MSR         (0xFFFAC250) // (CAN_MB2) MailBox Status Register
#define AT91C_CAN_MB2_MDL         (0xFFFAC254) // (CAN_MB2) MailBox Data Low Register
#define AT91C_CAN_MB2_MCR         (0xFFFAC25C) // (CAN_MB2) MailBox Control Register
#define AT91C_CAN_MB2_MDH         (0xFFFAC258) // (CAN_MB2) MailBox Data High Register
#define AT91C_CAN_MB2_MAM         (0xFFFAC244) // (CAN_MB2) MailBox Acceptance Mask Register
#define AT91C_CAN_MB2_MMR         (0xFFFAC240) // (CAN_MB2) MailBox Mode Register
#define AT91C_CAN_MB2_MFID        (0xFFFAC24C) // (CAN_MB2) MailBox Family ID Register
// ========== Register definition for CAN_MB3 peripheral ==========
#define AT91C_CAN_MB3_MDL         (0xFFFAC274) // (CAN_MB3) MailBox Data Low Register
#define AT91C_CAN_MB3_MFID        (0xFFFAC26C) // (CAN_MB3) MailBox Family ID Register
#define AT91C_CAN_MB3_MID         (0xFFFAC268) // (CAN_MB3) MailBox ID Register
#define AT91C_CAN_MB3_MDH         (0xFFFAC278) // (CAN_MB3) MailBox Data High Register
#define AT91C_CAN_MB3_MAM         (0xFFFAC264) // (CAN_MB3) MailBox Acceptance Mask Register
#define AT91C_CAN_MB3_MMR         (0xFFFAC260) // (CAN_MB3) MailBox Mode Register
#define AT91C_CAN_MB3_MCR         (0xFFFAC27C) // (CAN_MB3) MailBox Control Register
#define AT91C_CAN_MB3_MSR         (0xFFFAC270) // (CAN_MB3) MailBox Status Register
// ========== Register definition for CAN_MB4 peripheral ==========
#define AT91C_CAN_MB4_MCR         (0xFFFAC29C) // (CAN_MB4) MailBox Control Register
#define AT91C_CAN_MB4_MDH         (0xFFFAC298) // (CAN_MB4) MailBox Data High Register
#define AT91C_CAN_MB4_MID         (0xFFFAC288) // (CAN_MB4) MailBox ID Register
#define AT91C_CAN_MB4_MMR         (0xFFFAC280) // (CAN_MB4) MailBox Mode Register
#define AT91C_CAN_MB4_MSR         (0xFFFAC290) // (CAN_MB4) MailBox Status Register
#define AT91C_CAN_MB4_MFID        (0xFFFAC28C) // (CAN_MB4) MailBox Family ID Register
#define AT91C_CAN_MB4_MAM         (0xFFFAC284) // (CAN_MB4) MailBox Acceptance Mask Register
#define AT91C_CAN_MB4_MDL         (0xFFFAC294) // (CAN_MB4) MailBox Data Low Register
// ========== Register definition for CAN_MB5 peripheral ==========
#define AT91C_CAN_MB5_MDH         (0xFFFAC2B8) // (CAN_MB5) MailBox Data High Register
#define AT91C_CAN_MB5_MID         (0xFFFAC2A8) // (CAN_MB5) MailBox ID Register
#define AT91C_CAN_MB5_MCR         (0xFFFAC2BC) // (CAN_MB5) MailBox Control Register
#define AT91C_CAN_MB5_MSR         (0xFFFAC2B0) // (CAN_MB5) MailBox Status Register
#define AT91C_CAN_MB5_MDL         (0xFFFAC2B4) // (CAN_MB5) MailBox Data Low Register
#define AT91C_CAN_MB5_MMR         (0xFFFAC2A0) // (CAN_MB5) MailBox Mode Register
#define AT91C_CAN_MB5_MAM         (0xFFFAC2A4) // (CAN_MB5) MailBox Acceptance Mask Register
#define AT91C_CAN_MB5_MFID        (0xFFFAC2AC) // (CAN_MB5) MailBox Family ID Register
// ========== Register definition for CAN_MB6 peripheral ==========
#define AT91C_CAN_MB6_MSR         (0xFFFAC2D0) // (CAN_MB6) MailBox Status Register
#define AT91C_CAN_MB6_MMR         (0xFFFAC2C0) // (CAN_MB6) MailBox Mode Register
#define AT91C_CAN_MB6_MFID        (0xFFFAC2CC) // (CAN_MB6) MailBox Family ID Register
#define AT91C_CAN_MB6_MDL         (0xFFFAC2D4) // (CAN_MB6) MailBox Data Low Register
#define AT91C_CAN_MB6_MID         (0xFFFAC2C8) // (CAN_MB6) MailBox ID Register
#define AT91C_CAN_MB6_MCR         (0xFFFAC2DC) // (CAN_MB6) MailBox Control Register
#define AT91C_CAN_MB6_MAM         (0xFFFAC2C4) // (CAN_MB6) MailBox Acceptance Mask Register
#define AT91C_CAN_MB6_MDH         (0xFFFAC2D8) // (CAN_MB6) MailBox Data High Register
// ========== Register definition for CAN_MB7 peripheral ==========
#define AT91C_CAN_MB7_MAM         (0xFFFAC2E4) // (CAN_MB7) MailBox Acceptance Mask Register
#define AT91C_CAN_MB7_MDH         (0xFFFAC2F8) // (CAN_MB7) MailBox Data High Register
#define AT91C_CAN_MB7_MID         (0xFFFAC2E8) // (CAN_MB7) MailBox ID Register
#define AT91C_CAN_MB7_MSR         (0xFFFAC2F0) // (CAN_MB7) MailBox Status Register
#define AT91C_CAN_MB7_MMR         (0xFFFAC2E0) // (CAN_MB7) MailBox Mode Register
#define AT91C_CAN_MB7_MCR         (0xFFFAC2FC) // (CAN_MB7) MailBox Control Register
#define AT91C_CAN_MB7_MFID        (0xFFFAC2EC) // (CAN_MB7) MailBox Family ID Register
#define AT91C_CAN_MB7_MDL         (0xFFFAC2F4) // (CAN_MB7) MailBox Data Low Register
// ========== Register definition for CAN_MB8 peripheral ==========
#define AT91C_CAN_MB8_MDH         (0xFFFAC318) // (CAN_MB8) MailBox Data High Register
#define AT91C_CAN_MB8_MMR         (0xFFFAC300) // (CAN_MB8) MailBox Mode Register
#define AT91C_CAN_MB8_MCR         (0xFFFAC31C) // (CAN_MB8) MailBox Control Register
#define AT91C_CAN_MB8_MSR         (0xFFFAC310) // (CAN_MB8) MailBox Status Register
#define AT91C_CAN_MB8_MAM         (0xFFFAC304) // (CAN_MB8) MailBox Acceptance Mask Register
#define AT91C_CAN_MB8_MFID        (0xFFFAC30C) // (CAN_MB8) MailBox Family ID Register
#define AT91C_CAN_MB8_MID         (0xFFFAC308) // (CAN_MB8) MailBox ID Register
#define AT91C_CAN_MB8_MDL         (0xFFFAC314) // (CAN_MB8) MailBox Data Low Register
// ========== Register definition for CAN_MB9 peripheral ==========
#define AT91C_CAN_MB9_MID         (0xFFFAC328) // (CAN_MB9) MailBox ID Register
#define AT91C_CAN_MB9_MMR         (0xFFFAC320) // (CAN_MB9) MailBox Mode Register
#define AT91C_CAN_MB9_MDH         (0xFFFAC338) // (CAN_MB9) MailBox Data High Register
#define AT91C_CAN_MB9_MSR         (0xFFFAC330) // (CAN_MB9) MailBox Status Register
#define AT91C_CAN_MB9_MAM         (0xFFFAC324) // (CAN_MB9) MailBox Acceptance Mask Register
#define AT91C_CAN_MB9_MDL         (0xFFFAC334) // (CAN_MB9) MailBox Data Low Register
#define AT91C_CAN_MB9_MFID        (0xFFFAC32C) // (CAN_MB9) MailBox Family ID Register
#define AT91C_CAN_MB9_MCR         (0xFFFAC33C) // (CAN_MB9) MailBox Control Register
// ========== Register definition for CAN_MB10 peripheral ==========
#define AT91C_CAN_MB10_MCR        (0xFFFAC35C) // (CAN_MB10) MailBox Control Register
#define AT91C_CAN_MB10_MDH        (0xFFFAC358) // (CAN_MB10) MailBox Data High Register
#define AT91C_CAN_MB10_MAM        (0xFFFAC344) // (CAN_MB10) MailBox Acceptance Mask Register
#define AT91C_CAN_MB10_MID        (0xFFFAC348) // (CAN_MB10) MailBox ID Register
#define AT91C_CAN_MB10_MDL        (0xFFFAC354) // (CAN_MB10) MailBox Data Low Register
#define AT91C_CAN_MB10_MSR        (0xFFFAC350) // (CAN_MB10) MailBox Status Register
#define AT91C_CAN_MB10_MMR        (0xFFFAC340) // (CAN_MB10) MailBox Mode Register
#define AT91C_CAN_MB10_MFID       (0xFFFAC34C) // (CAN_MB10) MailBox Family ID Register
// ========== Register definition for CAN_MB11 peripheral ==========
#define AT91C_CAN_MB11_MSR        (0xFFFAC370) // (CAN_MB11) MailBox Status Register
#define AT91C_CAN_MB11_MFID       (0xFFFAC36C) // (CAN_MB11) MailBox Family ID Register
#define AT91C_CAN_MB11_MDL        (0xFFFAC374) // (CAN_MB11) MailBox Data Low Register
#define AT91C_CAN_MB11_MDH        (0xFFFAC378) // (CAN_MB11) MailBox Data High Register
#define AT91C_CAN_MB11_MID        (0xFFFAC368) // (CAN_MB11) MailBox ID Register
#define AT91C_CAN_MB11_MCR        (0xFFFAC37C) // (CAN_MB11) MailBox Control Register
#define AT91C_CAN_MB11_MMR        (0xFFFAC360) // (CAN_MB11) MailBox Mode Register
#define AT91C_CAN_MB11_MAM        (0xFFFAC364) // (CAN_MB11) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB12 peripheral ==========
#define AT91C_CAN_MB12_MAM        (0xFFFAC384) // (CAN_MB12) MailBox Acceptance Mask Register
#define AT91C_CAN_MB12_MDH        (0xFFFAC398) // (CAN_MB12) MailBox Data High Register
#define AT91C_CAN_MB12_MMR        (0xFFFAC380) // (CAN_MB12) MailBox Mode Register
#define AT91C_CAN_MB12_MSR        (0xFFFAC390) // (CAN_MB12) MailBox Status Register
#define AT91C_CAN_MB12_MFID       (0xFFFAC38C) // (CAN_MB12) MailBox Family ID Register
#define AT91C_CAN_MB12_MID        (0xFFFAC388) // (CAN_MB12) MailBox ID Register
#define AT91C_CAN_MB12_MCR        (0xFFFAC39C) // (CAN_MB12) MailBox Control Register
#define AT91C_CAN_MB12_MDL        (0xFFFAC394) // (CAN_MB12) MailBox Data Low Register
// ========== Register definition for CAN_MB13 peripheral ==========
#define AT91C_CAN_MB13_MDH        (0xFFFAC3B8) // (CAN_MB13) MailBox Data High Register
#define AT91C_CAN_MB13_MFID       (0xFFFAC3AC) // (CAN_MB13) MailBox Family ID Register
#define AT91C_CAN_MB13_MSR        (0xFFFAC3B0) // (CAN_MB13) MailBox Status Register
#define AT91C_CAN_MB13_MID        (0xFFFAC3A8) // (CAN_MB13) MailBox ID Register
#define AT91C_CAN_MB13_MAM        (0xFFFAC3A4) // (CAN_MB13) MailBox Acceptance Mask Register
#define AT91C_CAN_MB13_MMR        (0xFFFAC3A0) // (CAN_MB13) MailBox Mode Register
#define AT91C_CAN_MB13_MCR        (0xFFFAC3BC) // (CAN_MB13) MailBox Control Register
#define AT91C_CAN_MB13_MDL        (0xFFFAC3B4) // (CAN_MB13) MailBox Data Low Register
// ========== Register definition for CAN_MB14 peripheral ==========
#define AT91C_CAN_MB14_MDL        (0xFFFAC3D4) // (CAN_MB14) MailBox Data Low Register
#define AT91C_CAN_MB14_MMR        (0xFFFAC3C0) // (CAN_MB14) MailBox Mode Register
#define AT91C_CAN_MB14_MFID       (0xFFFAC3CC) // (CAN_MB14) MailBox Family ID Register
#define AT91C_CAN_MB14_MCR        (0xFFFAC3DC) // (CAN_MB14) MailBox Control Register
#define AT91C_CAN_MB14_MID        (0xFFFAC3C8) // (CAN_MB14) MailBox ID Register
#define AT91C_CAN_MB14_MDH        (0xFFFAC3D8) // (CAN_MB14) MailBox Data High Register
#define AT91C_CAN_MB14_MSR        (0xFFFAC3D0) // (CAN_MB14) MailBox Status Register
#define AT91C_CAN_MB14_MAM        (0xFFFAC3C4) // (CAN_MB14) MailBox Acceptance Mask Register
// ========== Register definition for CAN_MB15 peripheral ==========
#define AT91C_CAN_MB15_MDL        (0xFFFAC3F4) // (CAN_MB15) MailBox Data Low Register
#define AT91C_CAN_MB15_MSR        (0xFFFAC3F0) // (CAN_MB15) MailBox Status Register
#define AT91C_CAN_MB15_MID        (0xFFFAC3E8) // (CAN_MB15) MailBox ID Register
#define AT91C_CAN_MB15_MAM        (0xFFFAC3E4) // (CAN_MB15) MailBox Acceptance Mask Register
#define AT91C_CAN_MB15_MCR        (0xFFFAC3FC) // (CAN_MB15) MailBox Control Register
#define AT91C_CAN_MB15_MFID       (0xFFFAC3EC) // (CAN_MB15) MailBox Family ID Register
#define AT91C_CAN_MB15_MMR        (0xFFFAC3E0) // (CAN_MB15) MailBox Mode Register
#define AT91C_CAN_MB15_MDH        (0xFFFAC3F8) // (CAN_MB15) MailBox Data High Register
// ========== Register definition for CAN peripheral ==========
#define AT91C_CAN_ACR             (0xFFFAC028) // (CAN) Abort Command Register
#define AT91C_CAN_BR              (0xFFFAC014) // (CAN) Baudrate Register
#define AT91C_CAN_IDR             (0xFFFAC008) // (CAN) Interrupt Disable Register
#define AT91C_CAN_TIMESTP         (0xFFFAC01C) // (CAN) Time Stamp Register
#define AT91C_CAN_SR              (0xFFFAC010) // (CAN) Status Register
#define AT91C_CAN_IMR             (0xFFFAC00C) // (CAN) Interrupt Mask Register
#define AT91C_CAN_TCR             (0xFFFAC024) // (CAN) Transfer Command Register
#define AT91C_CAN_TIM             (0xFFFAC018) // (CAN) Timer Register
#define AT91C_CAN_IER             (0xFFFAC004) // (CAN) Interrupt Enable Register
#define AT91C_CAN_ECR             (0xFFFAC020) // (CAN) Error Counter Register
#define AT91C_CAN_VR              (0xFFFAC0FC) // (CAN) Version Register
#define AT91C_CAN_MR              (0xFFFAC000) // (CAN) Mode Register
// ========== Register definition for PDC_AES peripheral ==========
#define AT91C_AES_TCR             (0xFFFB010C) // (PDC_AES) Transmit Counter Register
#define AT91C_AES_PTCR            (0xFFFB0120) // (PDC_AES) PDC Transfer Control Register
#define AT91C_AES_RNCR            (0xFFFB0114) // (PDC_AES) Receive Next Counter Register
#define AT91C_AES_PTSR            (0xFFFB0124) // (PDC_AES) PDC Transfer Status Register
#define AT91C_AES_TNCR            (0xFFFB011C) // (PDC_AES) Transmit Next Counter Register
#define AT91C_AES_RNPR            (0xFFFB0110) // (PDC_AES) Receive Next Pointer Register
#define AT91C_AES_RCR             (0xFFFB0104) // (PDC_AES) Receive Counter Register
#define AT91C_AES_TPR             (0xFFFB0108) // (PDC_AES) Transmit Pointer Register
#define AT91C_AES_TNPR            (0xFFFB0118) // (PDC_AES) Transmit Next Pointer Register
#define AT91C_AES_RPR             (0xFFFB0100) // (PDC_AES) Receive Pointer Register
// ========== Register definition for AES peripheral ==========
#define AT91C_AES_VR              (0xFFFB00FC) // (AES) AES Version Register
#define AT91C_AES_IMR             (0xFFFB0018) // (AES) Interrupt Mask Register
#define AT91C_AES_CR              (0xFFFB0000) // (AES) Control Register
#define AT91C_AES_ODATAxR         (0xFFFB0050) // (AES) Output Data x Register
#define AT91C_AES_ISR             (0xFFFB001C) // (AES) Interrupt Status Register
#define AT91C_AES_IDR             (0xFFFB0014) // (AES) Interrupt Disable Register
#define AT91C_AES_KEYWxR          (0xFFFB0020) // (AES) Key Word x Register
#define AT91C_AES_IVxR            (0xFFFB0060) // (AES) Initialization Vector x Register
#define AT91C_AES_MR              (0xFFFB0004) // (AES) Mode Register
#define AT91C_AES_IDATAxR         (0xFFFB0040) // (AES) Input Data x Register
#define AT91C_AES_IER             (0xFFFB0010) // (AES) Interrupt Enable Register
// ========== Register definition for PDC_TDES peripheral ==========
#define AT91C_TDES_TCR            (0xFFFB010C) // (PDC_TDES) Transmit Counter Register
#define AT91C_TDES_PTCR           (0xFFFB0120) // (PDC_TDES) PDC Transfer Control Register
#define AT91C_TDES_RNCR           (0xFFFB0114) // (PDC_TDES) Receive Next Counter Register
#define AT91C_TDES_PTSR           (0xFFFB0124) // (PDC_TDES) PDC Transfer Status Register
#define AT91C_TDES_TNCR           (0xFFFB011C) // (PDC_TDES) Transmit Next Counter Register
#define AT91C_TDES_RNPR           (0xFFFB0110) // (PDC_TDES) Receive Next Pointer Register
#define AT91C_TDES_RCR            (0xFFFB0104) // (PDC_TDES) Receive Counter Register
#define AT91C_TDES_TPR            (0xFFFB0108) // (PDC_TDES) Transmit Pointer Register
#define AT91C_TDES_TNPR           (0xFFFB0118) // (PDC_TDES) Transmit Next Pointer Register
#define AT91C_TDES_RPR            (0xFFFB0100) // (PDC_TDES) Receive Pointer Register
// ========== Register definition for TDES peripheral ==========
#define AT91C_TDES_VR             (0xFFFB00FC) // (TDES) TDES Version Register
#define AT91C_TDES_IMR            (0xFFFB0018) // (TDES) Interrupt Mask Register
#define AT91C_TDES_CR             (0xFFFB0000) // (TDES) Control Register
#define AT91C_TDES_ODATAxR        (0xFFFB0050) // (TDES) Output Data x Register
#define AT91C_TDES_ISR            (0xFFFB001C) // (TDES) Interrupt Status Register
#define AT91C_TDES_KEY3WxR        (0xFFFB0030) // (TDES) Key 3 Word x Register
#define AT91C_TDES_IDR            (0xFFFB0014) // (TDES) Interrupt Disable Register
#define AT91C_TDES_KEY1WxR        (0xFFFB0020) // (TDES) Key 1 Word x Register
#define AT91C_TDES_KEY2WxR        (0xFFFB0028) // (TDES) Key 2 Word x Register
#define AT91C_TDES_IVxR           (0xFFFB0060) // (TDES) Initialization Vector x Register
#define AT91C_TDES_MR             (0xFFFB0004) // (TDES) Mode Register
#define AT91C_TDES_IDATAxR        (0xFFFB0040) // (TDES) Input Data x Register
#define AT91C_TDES_IER            (0xFFFB0010) // (TDES) Interrupt Enable Register
// ========== Register definition for PWMC_CH0 peripheral ==========
#define AT91C_PWMC_CH0_CCNTR      (0xFFFB820C) // (PWMC_CH0) Channel Counter Register
#define AT91C_PWMC_CH0_CPRDR      (0xFFFB8208) // (PWMC_CH0) Channel Period Register
#define AT91C_PWMC_CH0_CUPDR      (0xFFFB8210) // (PWMC_CH0) Channel Update Register
#define AT91C_PWMC_CH0_CDTYR      (0xFFFB8204) // (PWMC_CH0) Channel Duty Cycle Register
#define AT91C_PWMC_CH0_CMR        (0xFFFB8200) // (PWMC_CH0) Channel Mode Register
#define AT91C_PWMC_CH0_Reserved   (0xFFFB8214) // (PWMC_CH0) Reserved
// ========== Register definition for PWMC_CH1 peripheral ==========
#define AT91C_PWMC_CH1_CCNTR      (0xFFFB822C) // (PWMC_CH1) Channel Counter Register
#define AT91C_PWMC_CH1_CDTYR      (0xFFFB8224) // (PWMC_CH1) Channel Duty Cycle Register
#define AT91C_PWMC_CH1_CMR        (0xFFFB8220) // (PWMC_CH1) Channel Mode Register
#define AT91C_PWMC_CH1_CPRDR      (0xFFFB8228) // (PWMC_CH1) Channel Period Register
#define AT91C_PWMC_CH1_Reserved   (0xFFFB8234) // (PWMC_CH1) Reserved
#define AT91C_PWMC_CH1_CUPDR      (0xFFFB8230) // (PWMC_CH1) Channel Update Register
// ========== Register definition for PWMC_CH2 peripheral ==========
#define AT91C_PWMC_CH2_CUPDR      (0xFFFB8250) // (PWMC_CH2) Channel Update Register
#define AT91C_PWMC_CH2_CMR        (0xFFFB8240) // (PWMC_CH2) Channel Mode Register
#define AT91C_PWMC_CH2_Reserved   (0xFFFB8254) // (PWMC_CH2) Reserved
#define AT91C_PWMC_CH2_CPRDR      (0xFFFB8248) // (PWMC_CH2) Channel Period Register
#define AT91C_PWMC_CH2_CDTYR      (0xFFFB8244) // (PWMC_CH2) Channel Duty Cycle Register
#define AT91C_PWMC_CH2_CCNTR      (0xFFFB824C) // (PWMC_CH2) Channel Counter Register
// ========== Register definition for PWMC_CH3 peripheral ==========
#define AT91C_PWMC_CH3_CPRDR      (0xFFFB8268) // (PWMC_CH3) Channel Period Register
#define AT91C_PWMC_CH3_Reserved   (0xFFFB8274) // (PWMC_CH3) Reserved
#define AT91C_PWMC_CH3_CUPDR      (0xFFFB8270) // (PWMC_CH3) Channel Update Register
#define AT91C_PWMC_CH3_CDTYR      (0xFFFB8264) // (PWMC_CH3) Channel Duty Cycle Register
#define AT91C_PWMC_CH3_CCNTR      (0xFFFB826C) // (PWMC_CH3) Channel Counter Register
#define AT91C_PWMC_CH3_CMR        (0xFFFB8260) // (PWMC_CH3) Channel Mode Register
// ========== Register definition for PWMC peripheral ==========
#define AT91C_PWMC_IDR            (0xFFFB8014) // (PWMC) PWMC Interrupt Disable Register
#define AT91C_PWMC_MR             (0xFFFB8000) // (PWMC) PWMC Mode Register
#define AT91C_PWMC_VR             (0xFFFB80FC) // (PWMC) PWMC Version Register
#define AT91C_PWMC_IMR            (0xFFFB8018) // (PWMC) PWMC Interrupt Mask Register
#define AT91C_PWMC_SR             (0xFFFB800C) // (PWMC) PWMC Status Register
#define AT91C_PWMC_ISR            (0xFFFB801C) // (PWMC) PWMC Interrupt Status Register
#define AT91C_PWMC_ENA            (0xFFFB8004) // (PWMC) PWMC Enable Register
#define AT91C_PWMC_IER            (0xFFFB8010) // (PWMC) PWMC Interrupt Enable Register
#define AT91C_PWMC_DIS            (0xFFFB8008) // (PWMC) PWMC Disable Register
// ========== Register definition for MACB peripheral ==========
#define AT91C_MACB_ALE            (0xFFFBC054) // (MACB) Alignment Error Register
#define AT91C_MACB_RRE            (0xFFFBC06C) // (MACB) Receive Ressource Error Register
#define AT91C_MACB_SA4H           (0xFFFBC0B4) // (MACB) Specific Address 4 Top, Last 2 bytes
#define AT91C_MACB_TPQ            (0xFFFBC0BC) // (MACB) Transmit Pause Quantum Register
#define AT91C_MACB_RJA            (0xFFFBC07C) // (MACB) Receive Jabbers Register
#define AT91C_MACB_SA2H           (0xFFFBC0A4) // (MACB) Specific Address 2 Top, Last 2 bytes
#define AT91C_MACB_TPF            (0xFFFBC08C) // (MACB) Transmitted Pause Frames Register
#define AT91C_MACB_ROV            (0xFFFBC070) // (MACB) Receive Overrun Errors Register
#define AT91C_MACB_SA4L           (0xFFFBC0B0) // (MACB) Specific Address 4 Bottom, First 4 bytes
#define AT91C_MACB_MAN            (0xFFFBC034) // (MACB) PHY Maintenance Register
#define AT91C_MACB_TID            (0xFFFBC0B8) // (MACB) Type ID Checking Register
#define AT91C_MACB_TBQP           (0xFFFBC01C) // (MACB) Transmit Buffer Queue Pointer
#define AT91C_MACB_SA3L           (0xFFFBC0A8) // (MACB) Specific Address 3 Bottom, First 4 bytes
#define AT91C_MACB_DTF            (0xFFFBC058) // (MACB) Deferred Transmission Frame Register
#define AT91C_MACB_PTR            (0xFFFBC038) // (MACB) Pause Time Register
#define AT91C_MACB_CSE            (0xFFFBC068) // (MACB) Carrier Sense Error Register
#define AT91C_MACB_ECOL           (0xFFFBC060) // (MACB) Excessive Collision Register
#define AT91C_MACB_STE            (0xFFFBC084) // (MACB) SQE Test Error Register
#define AT91C_MACB_MCF            (0xFFFBC048) // (MACB) Multiple Collision Frame Register
#define AT91C_MACB_IER            (0xFFFBC028) // (MACB) Interrupt Enable Register
#define AT91C_MACB_ELE            (0xFFFBC078) // (MACB) Excessive Length Errors Register
#define AT91C_MACB_USRIO          (0xFFFBC0C0) // (MACB) USER Input/Output Register
#define AT91C_MACB_PFR            (0xFFFBC03C) // (MACB) Pause Frames received Register
#define AT91C_MACB_FCSE           (0xFFFBC050) // (MACB) Frame Check Sequence Error Register
#define AT91C_MACB_SA1L           (0xFFFBC098) // (MACB) Specific Address 1 Bottom, First 4 bytes
#define AT91C_MACB_NCR            (0xFFFBC000) // (MACB) Network Control Register
#define AT91C_MACB_HRT            (0xFFFBC094) // (MACB) Hash Address Top[63:32]
#define AT91C_MACB_NCFGR          (0xFFFBC004) // (MACB) Network Configuration Register
#define AT91C_MACB_SCF            (0xFFFBC044) // (MACB) Single Collision Frame Register
#define AT91C_MACB_LCOL           (0xFFFBC05C) // (MACB) Late Collision Register
#define AT91C_MACB_SA3H           (0xFFFBC0AC) // (MACB) Specific Address 3 Top, Last 2 bytes
#define AT91C_MACB_HRB            (0xFFFBC090) // (MACB) Hash Address Bottom[31:0]
#define AT91C_MACB_ISR            (0xFFFBC024) // (MACB) Interrupt Status Register
#define AT91C_MACB_IMR            (0xFFFBC030) // (MACB) Interrupt Mask Register
#define AT91C_MACB_WOL            (0xFFFBC0C4) // (MACB) Wake On LAN Register
#define AT91C_MACB_USF            (0xFFFBC080) // (MACB) Undersize Frames Register
#define AT91C_MACB_TSR            (0xFFFBC014) // (MACB) Transmit Status Register
#define AT91C_MACB_FRO            (0xFFFBC04C) // (MACB) Frames Received OK Register
#define AT91C_MACB_IDR            (0xFFFBC02C) // (MACB) Interrupt Disable Register
#define AT91C_MACB_SA1H           (0xFFFBC09C) // (MACB) Specific Address 1 Top, Last 2 bytes
#define AT91C_MACB_RLE            (0xFFFBC088) // (MACB) Receive Length Field Mismatch Register
#define AT91C_MACB_TUND           (0xFFFBC064) // (MACB) Transmit Underrun Error Register
#define AT91C_MACB_RSR            (0xFFFBC020) // (MACB) Receive Status Register
#define AT91C_MACB_SA2L           (0xFFFBC0A0) // (MACB) Specific Address 2 Bottom, First 4 bytes
#define AT91C_MACB_FTO            (0xFFFBC040) // (MACB) Frames Transmitted OK Register
#define AT91C_MACB_RSE            (0xFFFBC074) // (MACB) Receive Symbol Errors Register
#define AT91C_MACB_NSR            (0xFFFBC008) // (MACB) Network Status Register
#define AT91C_MACB_RBQP           (0xFFFBC018) // (MACB) Receive Buffer Queue Pointer
#define AT91C_MACB_REV            (0xFFFBC0FC) // (MACB) Revision Register
// ========== Register definition for PDC_ADC peripheral ==========
#define AT91C_ADC_TNPR            (0xFFFC0118) // (PDC_ADC) Transmit Next Pointer Register
#define AT91C_ADC_RNPR            (0xFFFC0110) // (PDC_ADC) Receive Next Pointer Register
#define AT91C_ADC_TCR             (0xFFFC010C) // (PDC_ADC) Transmit Counter Register
#define AT91C_ADC_PTCR            (0xFFFC0120) // (PDC_ADC) PDC Transfer Control Register
#define AT91C_ADC_PTSR            (0xFFFC0124) // (PDC_ADC) PDC Transfer Status Register
#define AT91C_ADC_TNCR            (0xFFFC011C) // (PDC_ADC) Transmit Next Counter Register
#define AT91C_ADC_TPR             (0xFFFC0108) // (PDC_ADC) Transmit Pointer Register
#define AT91C_ADC_RCR             (0xFFFC0104) // (PDC_ADC) Receive Counter Register
#define AT91C_ADC_RPR             (0xFFFC0100) // (PDC_ADC) Receive Pointer Register
#define AT91C_ADC_RNCR            (0xFFFC0114) // (PDC_ADC) Receive Next Counter Register
// ========== Register definition for ADC peripheral ==========
#define AT91C_ADC_CDR6            (0xFFFC0048) // (ADC) ADC Channel Data Register 6
#define AT91C_ADC_IMR             (0xFFFC002C) // (ADC) ADC Interrupt Mask Register
#define AT91C_ADC_CHER            (0xFFFC0010) // (ADC) ADC Channel Enable Register
#define AT91C_ADC_CDR4            (0xFFFC0040) // (ADC) ADC Channel Data Register 4
#define AT91C_ADC_CDR1            (0xFFFC0034) // (ADC) ADC Channel Data Register 1
#define AT91C_ADC_IER             (0xFFFC0024) // (ADC) ADC Interrupt Enable Register
#define AT91C_ADC_CHDR            (0xFFFC0014) // (ADC) ADC Channel Disable Register
#define AT91C_ADC_CDR2            (0xFFFC0038) // (ADC) ADC Channel Data Register 2
#define AT91C_ADC_LCDR            (0xFFFC0020) // (ADC) ADC Last Converted Data Register
#define AT91C_ADC_CR              (0xFFFC0000) // (ADC) ADC Control Register
#define AT91C_ADC_CDR5            (0xFFFC0044) // (ADC) ADC Channel Data Register 5
#define AT91C_ADC_CDR3            (0xFFFC003C) // (ADC) ADC Channel Data Register 3
#define AT91C_ADC_MR              (0xFFFC0004) // (ADC) ADC Mode Register
#define AT91C_ADC_IDR             (0xFFFC0028) // (ADC) ADC Interrupt Disable Register
#define AT91C_ADC_CDR0            (0xFFFC0030) // (ADC) ADC Channel Data Register 0
#define AT91C_ADC_CHSR            (0xFFFC0018) // (ADC) ADC Channel Status Register
#define AT91C_ADC_SR              (0xFFFC001C) // (ADC) ADC Status Register
#define AT91C_ADC_CDR7            (0xFFFC004C) // (ADC) ADC Channel Data Register 7
// ========== Register definition for HISI peripheral ==========
#define AT91C_HISI_CDBA           (0xFFFC402C) // (HISI) Codec Dma Address Register
#define AT91C_HISI_PDECF          (0xFFFC4024) // (HISI) Preview Decimation Factor Register
#define AT91C_HISI_IMR            (0xFFFC4014) // (HISI) Interrupt Mask Register
#define AT91C_HISI_IER            (0xFFFC400C) // (HISI) Interrupt Enable Register
#define AT91C_HISI_SR             (0xFFFC4008) // (HISI) Status Register
#define AT91C_HISI_Y2RSET0        (0xFFFC4030) // (HISI) Color Space Conversion Register
#define AT91C_HISI_PFBD           (0xFFFC4028) // (HISI) Preview Frame Buffer Address Register
#define AT91C_HISI_PSIZE          (0xFFFC4020) // (HISI) Preview Size Register
#define AT91C_HISI_IDR            (0xFFFC4010) // (HISI) Interrupt Disable Register
#define AT91C_HISI_R2YSET2        (0xFFFC4040) // (HISI) Color Space Conversion Register
#define AT91C_HISI_R2YSET0        (0xFFFC4038) // (HISI) Color Space Conversion Register
#define AT91C_HISI_CR1            (0xFFFC4000) // (HISI) Control Register 1
#define AT91C_HISI_CR2            (0xFFFC4004) // (HISI) Control Register 2
#define AT91C_HISI_Y2RSET1        (0xFFFC4034) // (HISI) Color Space Conversion Register
#define AT91C_HISI_R2YSET1        (0xFFFC403C) // (HISI) Color Space Conversion Register
// ========== Register definition for LCDC peripheral ==========
#define AT91C_LCDC_MVAL           (0x00500818) // (LCDC) LCD Mode Toggle Rate Value Register
#define AT91C_LCDC_PWRCON         (0x0050083C) // (LCDC) Power Control Register
#define AT91C_LCDC_ISR            (0x00500854) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_FRMP1          (0x00500008) // (LCDC) DMA Frame Pointer Register 1
#define AT91C_LCDC_CTRSTVAL       (0x00500844) // (LCDC) Contrast Value Register
#define AT91C_LCDC_ICR            (0x00500858) // (LCDC) Interrupt Clear Register
#define AT91C_LCDC_TIM1           (0x00500808) // (LCDC) LCD Timing Config 1 Register
#define AT91C_LCDC_DMACON         (0x0050001C) // (LCDC) DMA Control Register
#define AT91C_LCDC_ITR            (0x00500860) // (LCDC) Interrupts Test Register
#define AT91C_LCDC_IDR            (0x0050084C) // (LCDC) Interrupt Disable Register
#define AT91C_LCDC_DP4_7          (0x00500820) // (LCDC) Dithering Pattern DP4_7 Register
#define AT91C_LCDC_DP5_7          (0x0050082C) // (LCDC) Dithering Pattern DP5_7 Register
#define AT91C_LCDC_IRR            (0x00500864) // (LCDC) Interrupts Raw Status Register
#define AT91C_LCDC_DP3_4          (0x00500830) // (LCDC) Dithering Pattern DP3_4 Register
#define AT91C_LCDC_IMR            (0x00500850) // (LCDC) Interrupt Mask Register
#define AT91C_LCDC_LCDFRCFG       (0x00500810) // (LCDC) LCD Frame Config Register
#define AT91C_LCDC_CTRSTCON       (0x00500840) // (LCDC) Contrast Control Register
#define AT91C_LCDC_DP1_2          (0x0050081C) // (LCDC) Dithering Pattern DP1_2 Register
#define AT91C_LCDC_FRMP2          (0x0050000C) // (LCDC) DMA Frame Pointer Register 2
#define AT91C_LCDC_LCDCON1        (0x00500800) // (LCDC) LCD Control 1 Register
#define AT91C_LCDC_DP4_5          (0x00500834) // (LCDC) Dithering Pattern DP4_5 Register
#define AT91C_LCDC_FRMA2          (0x00500014) // (LCDC) DMA Frame Address Register 2
#define AT91C_LCDC_BA1            (0x00500000) // (LCDC) DMA Base Address Register 1
#define AT91C_LCDC_DMA2DCFG       (0x00500020) // (LCDC) DMA 2D addressing configuration
#define AT91C_LCDC_LUT_ENTRY      (0x00500C00) // (LCDC) LUT Entries Register
#define AT91C_LCDC_DP6_7          (0x00500838) // (LCDC) Dithering Pattern DP6_7 Register
#define AT91C_LCDC_FRMCFG         (0x00500018) // (LCDC) DMA Frame Configuration Register
#define AT91C_LCDC_TIM2           (0x0050080C) // (LCDC) LCD Timing Config 2 Register
#define AT91C_LCDC_DP3_5          (0x00500824) // (LCDC) Dithering Pattern DP3_5 Register
#define AT91C_LCDC_FRMA1          (0x00500010) // (LCDC) DMA Frame Address Register 1
#define AT91C_LCDC_IER            (0x00500848) // (LCDC) Interrupt Enable Register
#define AT91C_LCDC_DP2_3          (0x00500828) // (LCDC) Dithering Pattern DP2_3 Register
#define AT91C_LCDC_FIFO           (0x00500814) // (LCDC) LCD FIFO Register
#define AT91C_LCDC_BA2            (0x00500004) // (LCDC) DMA Base Address Register 2
#define AT91C_LCDC_LCDCON2        (0x00500804) // (LCDC) LCD Control 2 Register
#define AT91C_LCDC_GPR            (0x0050085C) // (LCDC) General Purpose Register
// ========== Register definition for HDMA_CH_0 peripheral ==========
#define AT91C_HDMA_CH_0_DADDR     (0xFFFFEC40) // (HDMA_CH_0) HDMA Channel Destination Address Register
#define AT91C_HDMA_CH_0_DPIP      (0xFFFFEC58) // (HDMA_CH_0) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_0_DSCR      (0xFFFFEC44) // (HDMA_CH_0) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_0_CFG       (0xFFFFEC50) // (HDMA_CH_0) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_0_SPIP      (0xFFFFEC54) // (HDMA_CH_0) HDMA Channel Source Picture in Picture Configuration Register
#define AT91C_HDMA_CH_0_CTRLA     (0xFFFFEC48) // (HDMA_CH_0) HDMA Channel Control A Register
#define AT91C_HDMA_CH_0_CTRLB     (0xFFFFEC4C) // (HDMA_CH_0) HDMA Channel Control B Register
#define AT91C_HDMA_CH_0_SADDR     (0xFFFFEC3C) // (HDMA_CH_0) HDMA Channel Source Address Register
// ========== Register definition for HDMA_CH_1 peripheral ==========
#define AT91C_HDMA_CH_1_DPIP      (0xFFFFEC80) // (HDMA_CH_1) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_1_CTRLB     (0xFFFFEC74) // (HDMA_CH_1) HDMA Channel Control B Register
#define AT91C_HDMA_CH_1_SADDR     (0xFFFFEC64) // (HDMA_CH_1) HDMA Channel Source Address Register
#define AT91C_HDMA_CH_1_CFG       (0xFFFFEC78) // (HDMA_CH_1) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_1_DSCR      (0xFFFFEC6C) // (HDMA_CH_1) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_1_DADDR     (0xFFFFEC68) // (HDMA_CH_1) HDMA Channel Destination Address Register
#define AT91C_HDMA_CH_1_CTRLA     (0xFFFFEC70) // (HDMA_CH_1) HDMA Channel Control A Register
#define AT91C_HDMA_CH_1_SPIP      (0xFFFFEC7C) // (HDMA_CH_1) HDMA Channel Source Picture in Picture Configuration Register
// ========== Register definition for HDMA_CH_2 peripheral ==========
#define AT91C_HDMA_CH_2_DSCR      (0xFFFFEC94) // (HDMA_CH_2) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_2_CTRLA     (0xFFFFEC98) // (HDMA_CH_2) HDMA Channel Control A Register
#define AT91C_HDMA_CH_2_SADDR     (0xFFFFEC8C) // (HDMA_CH_2) HDMA Channel Source Address Register
#define AT91C_HDMA_CH_2_CFG       (0xFFFFECA0) // (HDMA_CH_2) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_2_DPIP      (0xFFFFECA8) // (HDMA_CH_2) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_2_SPIP      (0xFFFFECA4) // (HDMA_CH_2) HDMA Channel Source Picture in Picture Configuration Register
#define AT91C_HDMA_CH_2_CTRLB     (0xFFFFEC9C) // (HDMA_CH_2) HDMA Channel Control B Register
#define AT91C_HDMA_CH_2_DADDR     (0xFFFFEC90) // (HDMA_CH_2) HDMA Channel Destination Address Register
// ========== Register definition for HDMA_CH_3 peripheral ==========
#define AT91C_HDMA_CH_3_SPIP      (0xFFFFECCC) // (HDMA_CH_3) HDMA Channel Source Picture in Picture Configuration Register
#define AT91C_HDMA_CH_3_CTRLA     (0xFFFFECC0) // (HDMA_CH_3) HDMA Channel Control A Register
#define AT91C_HDMA_CH_3_DPIP      (0xFFFFECD0) // (HDMA_CH_3) HDMA Channel Destination Picture in Picture Configuration Register
#define AT91C_HDMA_CH_3_CTRLB     (0xFFFFECC4) // (HDMA_CH_3) HDMA Channel Control B Register
#define AT91C_HDMA_CH_3_DSCR      (0xFFFFECBC) // (HDMA_CH_3) HDMA Channel Descriptor Address Register
#define AT91C_HDMA_CH_3_CFG       (0xFFFFECC8) // (HDMA_CH_3) HDMA Channel Configuration Register
#define AT91C_HDMA_CH_3_DADDR     (0xFFFFECB8) // (HDMA_CH_3) HDMA Channel Destination Address Register
#define AT91C_HDMA_CH_3_SADDR     (0xFFFFECB4) // (HDMA_CH_3) HDMA Channel Source Address Register
// ========== Register definition for HDMA peripheral ==========
#define AT91C_HDMA_EBCIDR         (0xFFFFEC1C) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Disable register
#define AT91C_HDMA_LAST           (0xFFFFEC10) // (HDMA) HDMA Software Last Transfer Flag Register
#define AT91C_HDMA_SREQ           (0xFFFFEC08) // (HDMA) HDMA Software Single Request Register
#define AT91C_HDMA_EBCIER         (0xFFFFEC18) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Interrupt Enable register
#define AT91C_HDMA_GCFG           (0xFFFFEC00) // (HDMA) HDMA Global Configuration Register
#define AT91C_HDMA_CHER           (0xFFFFEC28) // (HDMA) HDMA Channel Handler Enable Register
#define AT91C_HDMA_CHDR           (0xFFFFEC2C) // (HDMA) HDMA Channel Handler Disable Register
#define AT91C_HDMA_EBCIMR         (0xFFFFEC20) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Mask Register
#define AT91C_HDMA_BREQ           (0xFFFFEC0C) // (HDMA) HDMA Software Chunk Transfer Request Register
#define AT91C_HDMA_SYNC           (0xFFFFEC14) // (HDMA) HDMA Request Synchronization Register
#define AT91C_HDMA_EN             (0xFFFFEC04) // (HDMA) HDMA Controller Enable Register
#define AT91C_HDMA_EBCISR         (0xFFFFEC24) // (HDMA) HDMA Error, Chained Buffer transfer completed and Buffer transfer completed Status Register
#define AT91C_HDMA_CHSR           (0xFFFFEC30) // (HDMA) HDMA Channel Handler Status Register
// ========== Register definition for SYS peripheral ==========
#define AT91C_SYS_GPBR            (0xFFFFFD50) // (SYS) General Purpose Register
// ========== Register definition for UHP peripheral ==========
#define AT91C_UHP_HcRhPortStatus  (0x00700054) // (UHP) Root Hub Port Status Register
#define AT91C_UHP_HcFmRemaining   (0x00700038) // (UHP) Bit time remaining in the current Frame
#define AT91C_UHP_HcInterruptEnable (0x00700010) // (UHP) Interrupt Enable Register
#define AT91C_UHP_HcControl       (0x00700004) // (UHP) Operating modes for the Host Controller
#define AT91C_UHP_HcPeriodicStart (0x00700040) // (UHP) Periodic Start
#define AT91C_UHP_HcInterruptStatus (0x0070000C) // (UHP) Interrupt Status Register
#define AT91C_UHP_HcRhDescriptorB (0x0070004C) // (UHP) Root Hub characteristics B
#define AT91C_UHP_HcInterruptDisable (0x00700014) // (UHP) Interrupt Disable Register
#define AT91C_UHP_HcPeriodCurrentED (0x0070001C) // (UHP) Current Isochronous or Interrupt Endpoint Descriptor
#define AT91C_UHP_HcRhDescriptorA (0x00700048) // (UHP) Root Hub characteristics A
#define AT91C_UHP_HcRhStatus      (0x00700050) // (UHP) Root Hub Status register
#define AT91C_UHP_HcBulkCurrentED (0x0070002C) // (UHP) Current endpoint of the Bulk list
#define AT91C_UHP_HcControlHeadED (0x00700020) // (UHP) First Endpoint Descriptor of the Control list
#define AT91C_UHP_HcLSThreshold   (0x00700044) // (UHP) LS Threshold
#define AT91C_UHP_HcRevision      (0x00700000) // (UHP) Revision
#define AT91C_UHP_HcBulkDoneHead  (0x00700030) // (UHP) Last completed transfer descriptor
#define AT91C_UHP_HcFmNumber      (0x0070003C) // (UHP) Frame number
#define AT91C_UHP_HcFmInterval    (0x00700034) // (UHP) Bit time between 2 consecutive SOFs
#define AT91C_UHP_HcBulkHeadED    (0x00700028) // (UHP) First endpoint register of the Bulk list
#define AT91C_UHP_HcHCCA          (0x00700018) // (UHP) Pointer to the Host Controller Communication Area
#define AT91C_UHP_HcCommandStatus (0x00700008) // (UHP) Command & status Register
#define AT91C_UHP_HcControlCurrentED (0x00700024) // (UHP) Endpoint Control and Status Register

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91CAP9
// *****************************************************************************
#define AT91C_PIO_PA0             (1 <<  0) // Pin Controlled by PA0
#define AT91C_PA0_MCI0_DA0        (AT91C_PIO_PA0) //
#define AT91C_PA0_SPI0_MISO       (AT91C_PIO_PA0) //
#define AT91C_PIO_PA1             (1 <<  1) // Pin Controlled by PA1
#define AT91C_PA1_MCI0_CDA        (AT91C_PIO_PA1) //
#define AT91C_PA1_SPI0_MOSI       (AT91C_PIO_PA1) //
#define AT91C_PIO_PA10            (1 << 10) // Pin Controlled by PA10
#define AT91C_PA10_IRQ0           (AT91C_PIO_PA10) //
#define AT91C_PA10_PWM1           (AT91C_PIO_PA10) //
#define AT91C_PIO_PA11            (1 << 11) // Pin Controlled by PA11
#define AT91C_PA11_DMARQ0         (AT91C_PIO_PA11) //
#define AT91C_PA11_PWM3           (AT91C_PIO_PA11) //
#define AT91C_PIO_PA12            (1 << 12) // Pin Controlled by PA12
#define AT91C_PA12_CANTX          (AT91C_PIO_PA12) //
#define AT91C_PA12_PCK0           (AT91C_PIO_PA12) //
#define AT91C_PIO_PA13            (1 << 13) // Pin Controlled by PA13
#define AT91C_PA13_CANRX          (AT91C_PIO_PA13) //
#define AT91C_PIO_PA14            (1 << 14) // Pin Controlled by PA14
#define AT91C_PA14_TCLK2          (AT91C_PIO_PA14) //
#define AT91C_PA14_IRQ1           (AT91C_PIO_PA14) //
#define AT91C_PIO_PA15            (1 << 15) // Pin Controlled by PA15
#define AT91C_PA15_DMARQ3         (AT91C_PIO_PA15) //
#define AT91C_PA15_PCK2           (AT91C_PIO_PA15) //
#define AT91C_PIO_PA16            (1 << 16) // Pin Controlled by PA16
#define AT91C_PA16_MCI1_CK        (AT91C_PIO_PA16) //
#define AT91C_PA16_ISI_D0         (AT91C_PIO_PA16) //
#define AT91C_PIO_PA17            (1 << 17) // Pin Controlled by PA17
#define AT91C_PA17_MCI1_CDA       (AT91C_PIO_PA17) //
#define AT91C_PA17_ISI_D1         (AT91C_PIO_PA17) //
#define AT91C_PIO_PA18            (1 << 18) // Pin Controlled by PA18
#define AT91C_PA18_MCI1_DA0       (AT91C_PIO_PA18) //
#define AT91C_PA18_ISI_D2         (AT91C_PIO_PA18) //
#define AT91C_PIO_PA19            (1 << 19) // Pin Controlled by PA19
#define AT91C_PA19_MCI1_DA1       (AT91C_PIO_PA19) //
#define AT91C_PA19_ISI_D3         (AT91C_PIO_PA19) //
#define AT91C_PIO_PA2             (1 <<  2) // Pin Controlled by PA2
#define AT91C_PA2_MCI0_CK         (AT91C_PIO_PA2) //
#define AT91C_PA2_SPI0_SPCK       (AT91C_PIO_PA2) //
#define AT91C_PIO_PA20            (1 << 20) // Pin Controlled by PA20
#define AT91C_PA20_MCI1_DA2       (AT91C_PIO_PA20) //
#define AT91C_PA20_ISI_D4         (AT91C_PIO_PA20) //
#define AT91C_PIO_PA21            (1 << 21) // Pin Controlled by PA21
#define AT91C_PA21_MCI1_DA3       (AT91C_PIO_PA21) //
#define AT91C_PA21_ISI_D5         (AT91C_PIO_PA21) //
#define AT91C_PIO_PA22            (1 << 22) // Pin Controlled by PA22
#define AT91C_PA22_TXD0           (AT91C_PIO_PA22) //
#define AT91C_PA22_ISI_D6         (AT91C_PIO_PA22) //
#define AT91C_PIO_PA23            (1 << 23) // Pin Controlled by PA23
#define AT91C_PA23_RXD0           (AT91C_PIO_PA23) //
#define AT91C_PA23_ISI_D7         (AT91C_PIO_PA23) //
#define AT91C_PIO_PA24            (1 << 24) // Pin Controlled by PA24
#define AT91C_PA24_RTS0           (AT91C_PIO_PA24) //
#define AT91C_PA24_ISI_PCK        (AT91C_PIO_PA24) //
#define AT91C_PIO_PA25            (1 << 25) // Pin Controlled by PA25
#define AT91C_PA25_CTS0           (AT91C_PIO_PA25) //
#define AT91C_PA25_ISI_HSYNC      (AT91C_PIO_PA25) //
#define AT91C_PIO_PA26            (1 << 26) // Pin Controlled by PA26
#define AT91C_PA26_SCK0           (AT91C_PIO_PA26) //
#define AT91C_PA26_ISI_VSYNC      (AT91C_PIO_PA26) //
#define AT91C_PIO_PA27            (1 << 27) // Pin Controlled by PA27
#define AT91C_PA27_PCK1           (AT91C_PIO_PA27) //
#define AT91C_PA27_ISI_MCK        (AT91C_PIO_PA27) //
#define AT91C_PIO_PA28            (1 << 28) // Pin Controlled by PA28
#define AT91C_PA28_SPI0_NPCS3A    (AT91C_PIO_PA28) //
#define AT91C_PA28_ISI_D8         (AT91C_PIO_PA28) //
#define AT91C_PIO_PA29            (1 << 29) // Pin Controlled by PA29
#define AT91C_PA29_TIOA0          (AT91C_PIO_PA29) //
#define AT91C_PA29_ISI_D9         (AT91C_PIO_PA29) //
#define AT91C_PIO_PA3             (1 <<  3) // Pin Controlled by PA3
#define AT91C_PA3_MCI0_DA1        (AT91C_PIO_PA3) //
#define AT91C_PA3_SPI0_NPCS1      (AT91C_PIO_PA3) //
#define AT91C_PIO_PA30            (1 << 30) // Pin Controlled by PA30
#define AT91C_PA30_TIOB0          (AT91C_PIO_PA30) //
#define AT91C_PA30_ISI_D10        (AT91C_PIO_PA30) //
#define AT91C_PIO_PA31            (1 << 31) // Pin Controlled by PA31
#define AT91C_PA31_DMARQ1         (AT91C_PIO_PA31) //
#define AT91C_PA31_ISI_D11        (AT91C_PIO_PA31) //
#define AT91C_PIO_PA4             (1 <<  4) // Pin Controlled by PA4
#define AT91C_PA4_MCI0_DA2        (AT91C_PIO_PA4) //
#define AT91C_PA4_SPI0_NPCS2A     (AT91C_PIO_PA4) //
#define AT91C_PIO_PA5             (1 <<  5) // Pin Controlled by PA5
#define AT91C_PA5_MCI0_DA3        (AT91C_PIO_PA5) //
#define AT91C_PA5_SPI0_NPCS0      (AT91C_PIO_PA5) //
#define AT91C_PIO_PA6             (1 <<  6) // Pin Controlled by PA6
#define AT91C_PA6_AC97FS          (AT91C_PIO_PA6) //
#define AT91C_PIO_PA7             (1 <<  7) // Pin Controlled by PA7
#define AT91C_PA7_AC97CK          (AT91C_PIO_PA7) //
#define AT91C_PIO_PA8             (1 <<  8) // Pin Controlled by PA8
#define AT91C_PA8_AC97TX          (AT91C_PIO_PA8) //
#define AT91C_PIO_PA9             (1 <<  9) // Pin Controlled by PA9
#define AT91C_PA9_AC97RX          (AT91C_PIO_PA9) //
#define AT91C_PIO_PB0             (1 <<  0) // Pin Controlled by PB0
#define AT91C_PB0_TF0             (AT91C_PIO_PB0) //
#define AT91C_PIO_PB1             (1 <<  1) // Pin Controlled by PB1
#define AT91C_PB1_TK0             (AT91C_PIO_PB1) //
#define AT91C_PIO_PB10            (1 << 10) // Pin Controlled by PB10
#define AT91C_PB10_RK1            (AT91C_PIO_PB10) //
#define AT91C_PB10_PCK1           (AT91C_PIO_PB10) //
#define AT91C_PIO_PB11            (1 << 11) // Pin Controlled by PB11
#define AT91C_PB11_RF1            (AT91C_PIO_PB11) //
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
#define AT91C_PIO_PB17            (1 << 17) // Pin Controlled by PB17
#define AT91C_PB17_SPI1_NPCS2B    (AT91C_PIO_PB17) //
#define AT91C_PB17_AD0            (AT91C_PIO_PB17) //
#define AT91C_PIO_PB18            (1 << 18) // Pin Controlled by PB18
#define AT91C_PB18_SPI1_NPCS3B    (AT91C_PIO_PB18) //
#define AT91C_PB18_AD1            (AT91C_PIO_PB18) //
#define AT91C_PIO_PB19            (1 << 19) // Pin Controlled by PB19
#define AT91C_PB19_PWM0           (AT91C_PIO_PB19) //
#define AT91C_PB19_AD2            (AT91C_PIO_PB19) //
#define AT91C_PIO_PB2             (1 <<  2) // Pin Controlled by PB2
#define AT91C_PB2_TD0             (AT91C_PIO_PB2) //
#define AT91C_PIO_PB20            (1 << 20) // Pin Controlled by PB20
#define AT91C_PB20_PWM1           (AT91C_PIO_PB20) //
#define AT91C_PB20_AD3            (AT91C_PIO_PB20) //
#define AT91C_PIO_PB21            (1 << 21) // Pin Controlled by PB21
#define AT91C_PB21_E_TXCK         (AT91C_PIO_PB21) //
#define AT91C_PB21_TIOA2          (AT91C_PIO_PB21) //
#define AT91C_PIO_PB22            (1 << 22) // Pin Controlled by PB22
#define AT91C_PB22_E_RXDV         (AT91C_PIO_PB22) //
#define AT91C_PB22_TIOB2          (AT91C_PIO_PB22) //
#define AT91C_PIO_PB23            (1 << 23) // Pin Controlled by PB23
#define AT91C_PB23_E_TX0          (AT91C_PIO_PB23) //
#define AT91C_PB23_PCK3           (AT91C_PIO_PB23) //
#define AT91C_PIO_PB24            (1 << 24) // Pin Controlled by PB24
#define AT91C_PB24_E_TX1          (AT91C_PIO_PB24) //
#define AT91C_PIO_PB25            (1 << 25) // Pin Controlled by PB25
#define AT91C_PB25_E_RX0          (AT91C_PIO_PB25) //
#define AT91C_PIO_PB26            (1 << 26) // Pin Controlled by PB26
#define AT91C_PB26_E_RX1          (AT91C_PIO_PB26) //
#define AT91C_PIO_PB27            (1 << 27) // Pin Controlled by PB27
#define AT91C_PB27_E_RXER         (AT91C_PIO_PB27) //
#define AT91C_PIO_PB28            (1 << 28) // Pin Controlled by PB28
#define AT91C_PB28_E_TXEN         (AT91C_PIO_PB28) //
#define AT91C_PB28_TCLK0          (AT91C_PIO_PB28) //
#define AT91C_PIO_PB29            (1 << 29) // Pin Controlled by PB29
#define AT91C_PB29_E_MDC          (AT91C_PIO_PB29) //
#define AT91C_PB29_PWM3           (AT91C_PIO_PB29) //
#define AT91C_PIO_PB3             (1 <<  3) // Pin Controlled by PB3
#define AT91C_PB3_RD0             (AT91C_PIO_PB3) //
#define AT91C_PIO_PB30            (1 << 30) // Pin Controlled by PB30
#define AT91C_PB30_E_MDIO         (AT91C_PIO_PB30) //
#define AT91C_PIO_PB31            (1 << 31) // Pin Controlled by PB31
#define AT91C_PB31_ADTRIG         (AT91C_PIO_PB31) //
#define AT91C_PB31_E_F100         (AT91C_PIO_PB31) //
#define AT91C_PIO_PB4             (1 <<  4) // Pin Controlled by PB4
#define AT91C_PB4_RK0             (AT91C_PIO_PB4) //
#define AT91C_PB4_TWD             (AT91C_PIO_PB4) //
#define AT91C_PIO_PB5             (1 <<  5) // Pin Controlled by PB5
#define AT91C_PB5_RF0             (AT91C_PIO_PB5) //
#define AT91C_PB5_TWCK            (AT91C_PIO_PB5) //
#define AT91C_PIO_PB6             (1 <<  6) // Pin Controlled by PB6
#define AT91C_PB6_TF1             (AT91C_PIO_PB6) //
#define AT91C_PB6_TIOA1           (AT91C_PIO_PB6) //
#define AT91C_PIO_PB7             (1 <<  7) // Pin Controlled by PB7
#define AT91C_PB7_TK1             (AT91C_PIO_PB7) //
#define AT91C_PB7_TIOB1           (AT91C_PIO_PB7) //
#define AT91C_PIO_PB8             (1 <<  8) // Pin Controlled by PB8
#define AT91C_PB8_TD1             (AT91C_PIO_PB8) //
#define AT91C_PB8_PWM2            (AT91C_PIO_PB8) //
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
#define AT91C_PC20_E_TX2          (AT91C_PIO_PC20) //
#define AT91C_PIO_PC21            (1 << 21) // Pin Controlled by PC21
#define AT91C_PC21_LCDD17         (AT91C_PIO_PC21) //
#define AT91C_PC21_E_TX3          (AT91C_PIO_PC21) //
#define AT91C_PIO_PC22            (1 << 22) // Pin Controlled by PC22
#define AT91C_PC22_LCDD18         (AT91C_PIO_PC22) //
#define AT91C_PC22_E_RX2          (AT91C_PIO_PC22) //
#define AT91C_PIO_PC23            (1 << 23) // Pin Controlled by PC23
#define AT91C_PC23_LCDD19         (AT91C_PIO_PC23) //
#define AT91C_PC23_E_RX3          (AT91C_PIO_PC23) //
#define AT91C_PIO_PC24            (1 << 24) // Pin Controlled by PC24
#define AT91C_PC24_LCDD20         (AT91C_PIO_PC24) //
#define AT91C_PC24_E_TXER         (AT91C_PIO_PC24) //
#define AT91C_PIO_PC25            (1 << 25) // Pin Controlled by PC25
#define AT91C_PC25_LCDD21         (AT91C_PIO_PC25) //
#define AT91C_PC25_E_CRS          (AT91C_PIO_PC25) //
#define AT91C_PIO_PC26            (1 << 26) // Pin Controlled by PC26
#define AT91C_PC26_LCDD22         (AT91C_PIO_PC26) //
#define AT91C_PC26_E_COL          (AT91C_PIO_PC26) //
#define AT91C_PIO_PC27            (1 << 27) // Pin Controlled by PC27
#define AT91C_PC27_LCDD23         (AT91C_PIO_PC27) //
#define AT91C_PC27_E_RXCK         (AT91C_PIO_PC27) //
#define AT91C_PIO_PC28            (1 << 28) // Pin Controlled by PC28
#define AT91C_PC28_PWM0           (AT91C_PIO_PC28) //
#define AT91C_PC28_TCLK1          (AT91C_PIO_PC28) //
#define AT91C_PIO_PC29            (1 << 29) // Pin Controlled by PC29
#define AT91C_PC29_PCK0           (AT91C_PIO_PC29) //
#define AT91C_PC29_PWM2           (AT91C_PIO_PC29) //
#define AT91C_PIO_PC3             (1 <<  3) // Pin Controlled by PC3
#define AT91C_PC3_LCDDEN          (AT91C_PIO_PC3) //
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
#define AT91C_PD10_EBI_CFCE2      (AT91C_PIO_PD10) //
#define AT91C_PD10_SCK1           (AT91C_PIO_PD10) //
#define AT91C_PIO_PD11            (1 << 11) // Pin Controlled by PD11
#define AT91C_PD11_EBI_NCS2       (AT91C_PIO_PD11) //
#define AT91C_PIO_PD12            (1 << 12) // Pin Controlled by PD12
#define AT91C_PD12_EBI_A23        (AT91C_PIO_PD12) //
#define AT91C_PIO_PD13            (1 << 13) // Pin Controlled by PD13
#define AT91C_PD13_EBI_A24        (AT91C_PIO_PD13) //
#define AT91C_PIO_PD14            (1 << 14) // Pin Controlled by PD14
#define AT91C_PD14_EBI_A25_CFRNW  (AT91C_PIO_PD14) //
#define AT91C_PIO_PD15            (1 << 15) // Pin Controlled by PD15
#define AT91C_PD15_EBI_NCS3_NANDCS (AT91C_PIO_PD15) //
#define AT91C_PIO_PD16            (1 << 16) // Pin Controlled by PD16
#define AT91C_PD16_EBI_D16        (AT91C_PIO_PD16) //
#define AT91C_PIO_PD17            (1 << 17) // Pin Controlled by PD17
#define AT91C_PD17_EBI_D17        (AT91C_PIO_PD17) //
#define AT91C_PIO_PD18            (1 << 18) // Pin Controlled by PD18
#define AT91C_PD18_EBI_D18        (AT91C_PIO_PD18) //
#define AT91C_PIO_PD19            (1 << 19) // Pin Controlled by PD19
#define AT91C_PD19_EBI_D19        (AT91C_PIO_PD19) //
#define AT91C_PIO_PD2             (1 <<  2) // Pin Controlled by PD2
#define AT91C_PD2_TXD2            (AT91C_PIO_PD2) //
#define AT91C_PD2_SPI1_NPCS2D     (AT91C_PIO_PD2) //
#define AT91C_PIO_PD20            (1 << 20) // Pin Controlled by PD20
#define AT91C_PD20_EBI_D20        (AT91C_PIO_PD20) //
#define AT91C_PIO_PD21            (1 << 21) // Pin Controlled by PD21
#define AT91C_PD21_EBI_D21        (AT91C_PIO_PD21) //
#define AT91C_PIO_PD22            (1 << 22) // Pin Controlled by PD22
#define AT91C_PD22_EBI_D22        (AT91C_PIO_PD22) //
#define AT91C_PIO_PD23            (1 << 23) // Pin Controlled by PD23
#define AT91C_PD23_EBI_D23        (AT91C_PIO_PD23) //
#define AT91C_PIO_PD24            (1 << 24) // Pin Controlled by PD24
#define AT91C_PD24_EBI_D24        (AT91C_PIO_PD24) //
#define AT91C_PIO_PD25            (1 << 25) // Pin Controlled by PD25
#define AT91C_PD25_EBI_D25        (AT91C_PIO_PD25) //
#define AT91C_PIO_PD26            (1 << 26) // Pin Controlled by PD26
#define AT91C_PD26_EBI_D26        (AT91C_PIO_PD26) //
#define AT91C_PIO_PD27            (1 << 27) // Pin Controlled by PD27
#define AT91C_PD27_EBI_D27        (AT91C_PIO_PD27) //
#define AT91C_PIO_PD28            (1 << 28) // Pin Controlled by PD28
#define AT91C_PD28_EBI_D28        (AT91C_PIO_PD28) //
#define AT91C_PIO_PD29            (1 << 29) // Pin Controlled by PD29
#define AT91C_PD29_EBI_D29        (AT91C_PIO_PD29) //
#define AT91C_PIO_PD3             (1 <<  3) // Pin Controlled by PD3
#define AT91C_PD3_RXD2            (AT91C_PIO_PD3) //
#define AT91C_PD3_SPI1_NPCS3D     (AT91C_PIO_PD3) //
#define AT91C_PIO_PD30            (1 << 30) // Pin Controlled by PD30
#define AT91C_PD30_EBI_D30        (AT91C_PIO_PD30) //
#define AT91C_PIO_PD31            (1 << 31) // Pin Controlled by PD31
#define AT91C_PD31_EBI_D31        (AT91C_PIO_PD31) //
#define AT91C_PIO_PD4             (1 <<  4) // Pin Controlled by PD4
#define AT91C_PD4_FIQ             (AT91C_PIO_PD4) //
#define AT91C_PIO_PD5             (1 <<  5) // Pin Controlled by PD5
#define AT91C_PD5_DMARQ2          (AT91C_PIO_PD5) //
#define AT91C_PD5_RTS2            (AT91C_PIO_PD5) //
#define AT91C_PIO_PD6             (1 <<  6) // Pin Controlled by PD6
#define AT91C_PD6_EBI_NWAIT       (AT91C_PIO_PD6) //
#define AT91C_PD6_CTS2            (AT91C_PIO_PD6) //
#define AT91C_PIO_PD7             (1 <<  7) // Pin Controlled by PD7
#define AT91C_PD7_EBI_NCS4_CFCS0  (AT91C_PIO_PD7) //
#define AT91C_PD7_RTS1            (AT91C_PIO_PD7) //
#define AT91C_PIO_PD8             (1 <<  8) // Pin Controlled by PD8
#define AT91C_PD8_EBI_NCS5_CFCS1  (AT91C_PIO_PD8) //
#define AT91C_PD8_CTS1            (AT91C_PIO_PD8) //
#define AT91C_PIO_PD9             (1 <<  9) // Pin Controlled by PD9
#define AT91C_PD9_EBI_CFCE1       (AT91C_PIO_PD9) //
#define AT91C_PD9_SCK2            (AT91C_PIO_PD9) //

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91CAP9
// *****************************************************************************
#define AT91C_ID_FIQ              ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS              ( 1) // System Controller
#define AT91C_ID_PIOABCDE         ( 2) // Parallel IO Controller A, Parallel IO Controller B, Parallel IO Controller C, Parallel IO Controller D, Parallel IO Controller E
#define AT91C_ID_MPB0             ( 3) // MP Block Peripheral 0
#define AT91C_ID_MPB1             ( 4) // MP Block Peripheral 1
#define AT91C_ID_MPB2             ( 5) // MP Block Peripheral 2
#define AT91C_ID_MPB3             ( 6) // MP Block Peripheral 3
#define AT91C_ID_MPB4             ( 7) // MP Block Peripheral 4
#define AT91C_ID_US0              ( 8) // USART 0
#define AT91C_ID_US1              ( 9) // USART 1
#define AT91C_ID_US2              (10) // USART 2
#define AT91C_ID_MCI0             (11) // Multimedia Card Interface 0
#define AT91C_ID_MCI1             (12) // Multimedia Card Interface 1
#define AT91C_ID_CAN              (13) // CAN Controller
#define AT91C_ID_TWI              (14) // Two-Wire Interface
#define AT91C_ID_SPI0             (15) // Serial Peripheral Interface 0
#define AT91C_ID_SPI1             (16) // Serial Peripheral Interface 1
#define AT91C_ID_SSC0             (17) // Serial Synchronous Controller 0
#define AT91C_ID_SSC1             (18) // Serial Synchronous Controller 1
#define AT91C_ID_AC97C            (19) // AC97 Controller
#define AT91C_ID_TC012            (20) // Timer Counter 0, Timer Counter 1, Timer Counter 2
#define AT91C_ID_PWMC             (21) // PWM Controller
#define AT91C_ID_EMAC             (22) // Ethernet Mac
#define AT91C_ID_AESTDES          (23) // Advanced Encryption Standard, Triple DES
#define AT91C_ID_ADC              (24) // ADC Controller
#define AT91C_ID_HISI             (25) // Image Sensor Interface
#define AT91C_ID_LCDC             (26) // LCD Controller
#define AT91C_ID_HDMA             (27) // HDMA Controller
#define AT91C_ID_UDPHS            (28) // USB High Speed Device Port
#define AT91C_ID_UHP              (29) // USB Host Port
#define AT91C_ID_IRQ0             (30) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ID_IRQ1             (31) // Advanced Interrupt Controller (IRQ1)
#define AT91C_ALL_INT             (0xFFFFFFFF) // ALL VALID INTERRUPTS

// *****************************************************************************
//               BASE ADDRESS DEFINITIONS FOR AT91CAP9
// *****************************************************************************
#define AT91C_BASE_HECC           (0xFFFFE200) // (HECC) Base Address
#define AT91C_BASE_BCRAMC         (0xFFFFE400) // (BCRAMC) Base Address
#define AT91C_BASE_SDRAMC         (0xFFFFE600) // (SDRAMC) Base Address
#define AT91C_BASE_SDDRC          (0xFFFFE600) // (SDDRC) Base Address
#define AT91C_BASE_SMC            (0xFFFFE800) // (SMC) Base Address
#define AT91C_BASE_MATRIX_PRS     (0xFFFFEA80) // (MATRIX_PRS) Base Address
#define AT91C_BASE_MATRIX         (0xFFFFEA00) // (MATRIX) Base Address
#define AT91C_BASE_CCFG           (0xFFFFEB10) // (CCFG) Base Address
#define AT91C_BASE_PDC_DBGU       (0xFFFFEF00) // (PDC_DBGU) Base Address
#define AT91C_BASE_DBGU           (0xFFFFEE00) // (DBGU) Base Address
#define AT91C_BASE_AIC            (0xFFFFF000) // (AIC) Base Address
#define AT91C_BASE_PIOA           (0xFFFFF200) // (PIOA) Base Address
#define AT91C_BASE_PIOB           (0xFFFFF400) // (PIOB) Base Address
#define AT91C_BASE_PIOC           (0xFFFFF600) // (PIOC) Base Address
#define AT91C_BASE_PIOD           (0xFFFFF800) // (PIOD) Base Address
#define AT91C_BASE_CKGR           (0xFFFFFC1C) // (CKGR) Base Address
#define AT91C_BASE_PMC            (0xFFFFFC00) // (PMC) Base Address
#define AT91C_BASE_RSTC           (0xFFFFFD00) // (RSTC) Base Address
#define AT91C_BASE_SHDWC          (0xFFFFFD10) // (SHDWC) Base Address
#define AT91C_BASE_RTTC           (0xFFFFFD20) // (RTTC) Base Address
#define AT91C_BASE_PITC           (0xFFFFFD30) // (PITC) Base Address
#define AT91C_BASE_WDTC           (0xFFFFFD40) // (WDTC) Base Address
#define AT91C_BASE_UDP            (0xFFF78000) // (UDP) Base Address
#define AT91C_BASE_UDPHS_EPTFIFO  (0x00600000) // (UDPHS_EPTFIFO) Base Address
#define AT91C_BASE_UDPHS_EPT_0    (0xFFF78100) // (UDPHS_EPT_0) Base Address
#define AT91C_BASE_UDPHS_EPT_1    (0xFFF78120) // (UDPHS_EPT_1) Base Address
#define AT91C_BASE_UDPHS_EPT_2    (0xFFF78140) // (UDPHS_EPT_2) Base Address
#define AT91C_BASE_UDPHS_EPT_3    (0xFFF78160) // (UDPHS_EPT_3) Base Address
#define AT91C_BASE_UDPHS_EPT_4    (0xFFF78180) // (UDPHS_EPT_4) Base Address
#define AT91C_BASE_UDPHS_EPT_5    (0xFFF781A0) // (UDPHS_EPT_5) Base Address
#define AT91C_BASE_UDPHS_EPT_6    (0xFFF781C0) // (UDPHS_EPT_6) Base Address
#define AT91C_BASE_UDPHS_EPT_7    (0xFFF781E0) // (UDPHS_EPT_7) Base Address
#define AT91C_BASE_UDPHS_EPT_8    (0xFFF78200) // (UDPHS_EPT_8) Base Address
#define AT91C_BASE_UDPHS_EPT_9    (0xFFF78220) // (UDPHS_EPT_9) Base Address
#define AT91C_BASE_UDPHS_EPT_10   (0xFFF78240) // (UDPHS_EPT_10) Base Address
#define AT91C_BASE_UDPHS_EPT_11   (0xFFF78260) // (UDPHS_EPT_11) Base Address
#define AT91C_BASE_UDPHS_EPT_12   (0xFFF78280) // (UDPHS_EPT_12) Base Address
#define AT91C_BASE_UDPHS_EPT_13   (0xFFF782A0) // (UDPHS_EPT_13) Base Address
#define AT91C_BASE_UDPHS_EPT_14   (0xFFF782C0) // (UDPHS_EPT_14) Base Address
#define AT91C_BASE_UDPHS_EPT_15   (0xFFF782E0) // (UDPHS_EPT_15) Base Address
#define AT91C_BASE_UDPHS_DMA_1    (0xFFF78310) // (UDPHS_DMA_1) Base Address
#define AT91C_BASE_UDPHS_DMA_2    (0xFFF78320) // (UDPHS_DMA_2) Base Address
#define AT91C_BASE_UDPHS_DMA_3    (0xFFF78330) // (UDPHS_DMA_3) Base Address
#define AT91C_BASE_UDPHS_DMA_4    (0xFFF78340) // (UDPHS_DMA_4) Base Address
#define AT91C_BASE_UDPHS_DMA_5    (0xFFF78350) // (UDPHS_DMA_5) Base Address
#define AT91C_BASE_UDPHS_DMA_6    (0xFFF78360) // (UDPHS_DMA_6) Base Address
#define AT91C_BASE_UDPHS_DMA_7    (0xFFF78370) // (UDPHS_DMA_7) Base Address
#define AT91C_BASE_UDPHS          (0xFFF78000) // (UDPHS) Base Address
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
#define AT91C_BASE_PDC_TWI        (0xFFF88100) // (PDC_TWI) Base Address
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
#define AT91C_BASE_PDC_AES        (0xFFFB0100) // (PDC_AES) Base Address
#define AT91C_BASE_AES            (0xFFFB0000) // (AES) Base Address
#define AT91C_BASE_PDC_TDES       (0xFFFB0100) // (PDC_TDES) Base Address
#define AT91C_BASE_TDES           (0xFFFB0000) // (TDES) Base Address
#define AT91C_BASE_PWMC_CH0       (0xFFFB8200) // (PWMC_CH0) Base Address
#define AT91C_BASE_PWMC_CH1       (0xFFFB8220) // (PWMC_CH1) Base Address
#define AT91C_BASE_PWMC_CH2       (0xFFFB8240) // (PWMC_CH2) Base Address
#define AT91C_BASE_PWMC_CH3       (0xFFFB8260) // (PWMC_CH3) Base Address
#define AT91C_BASE_PWMC           (0xFFFB8000) // (PWMC) Base Address
#define AT91C_BASE_MACB           (0xFFFBC000) // (MACB) Base Address
#define AT91C_BASE_PDC_ADC        (0xFFFC0100) // (PDC_ADC) Base Address
#define AT91C_BASE_ADC            (0xFFFC0000) // (ADC) Base Address
#define AT91C_BASE_HISI           (0xFFFC4000) // (HISI) Base Address
#define AT91C_BASE_LCDC           (0x00500000) // (LCDC) Base Address
#define AT91C_BASE_HDMA_CH_0      (0xFFFFEC3C) // (HDMA_CH_0) Base Address
#define AT91C_BASE_HDMA_CH_1      (0xFFFFEC64) // (HDMA_CH_1) Base Address
#define AT91C_BASE_HDMA_CH_2      (0xFFFFEC8C) // (HDMA_CH_2) Base Address
#define AT91C_BASE_HDMA_CH_3      (0xFFFFECB4) // (HDMA_CH_3) Base Address
#define AT91C_BASE_HDMA           (0xFFFFEC00) // (HDMA) Base Address
#define AT91C_BASE_SYS            (0xFFFFE200) // (SYS) Base Address
#define AT91C_BASE_UHP            (0x00700000) // (UHP) Base Address

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91CAP9
// *****************************************************************************
// IRAM
#define AT91C_IRAM 	              (0x00100000) // 16-KBytes FAST SRAM base address
#define AT91C_IRAM_SIZE	          (0x00004000) // 16-KBytes FAST SRAM size in byte (16 Kbytes)
// IRAM_MIN
#define AT91C_IRAM_MIN	           (0x00100000) // Minimum Internal RAM base address
#define AT91C_IRAM_MIN_SIZE	      (0x00004000) // Minimum Internal RAM size in byte (16 Kbytes)
// DPR
#define AT91C_DPR  	              (0x00200000) //  base address
#define AT91C_DPR_SIZE	           (0x00004000) //  size in byte (16 Kbytes)
// IROM
#define AT91C_IROM 	              (0x00400000) // Internal ROM base address
#define AT91C_IROM_SIZE	          (0x00008000) // Internal ROM size in byte (32 Kbytes)
// EBI_CS0
#define AT91C_EBI_CS0	            (0x10000000) // EBI Chip Select 0 base address
#define AT91C_EBI_CS0_SIZE	       (0x10000000) // EBI Chip Select 0 size in byte (262144 Kbytes)
// EBI_CS1
#define AT91C_EBI_CS1	            (0x20000000) // EBI Chip Select 1 base address
#define AT91C_EBI_CS1_SIZE	       (0x10000000) // EBI Chip Select 1 size in byte (262144 Kbytes)
// EBI_BCRAM
#define AT91C_EBI_BCRAM	          (0x20000000) // BCRAM on EBI Chip Select 1 base address
#define AT91C_EBI_BCRAM_SIZE	     (0x10000000) // BCRAM on EBI Chip Select 1 size in byte (262144 Kbytes)
// EBI_BCRAM_16BIT
#define AT91C_EBI_BCRAM_16BIT	    (0x20000000) // BCRAM on EBI Chip Select 1 base address
#define AT91C_EBI_BCRAM_16BIT_SIZE	 (0x02000000) // BCRAM on EBI Chip Select 1 size in byte (32768 Kbytes)
// EBI_BCRAM_32BIT
#define AT91C_EBI_BCRAM_32BIT	    (0x20000000) // BCRAM on EBI Chip Select 1 base address
#define AT91C_EBI_BCRAM_32BIT_SIZE	 (0x04000000) // BCRAM on EBI Chip Select 1 size in byte (65536 Kbytes)
// EBI_CS2
#define AT91C_EBI_CS2	            (0x30000000) // EBI Chip Select 2 base address
#define AT91C_EBI_CS2_SIZE	       (0x10000000) // EBI Chip Select 2 size in byte (262144 Kbytes)
// EBI_CS3
#define AT91C_EBI_CS3	            (0x40000000) // EBI Chip Select 3 base address
#define AT91C_EBI_CS3_SIZE	       (0x10000000) // EBI Chip Select 3 size in byte (262144 Kbytes)
// EBI_SM
#define AT91C_EBI_SM	             (0x40000000) // SmartMedia on EBI Chip Select 3 base address
#define AT91C_EBI_SM_SIZE	        (0x10000000) // SmartMedia on EBI Chip Select 3 size in byte (262144 Kbytes)
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
#define AT91C_EBI_CF1	            (0x60000000) // CompactFlash 1 on EBI Chip Select 5 base address
#define AT91C_EBI_CF1_SIZE	       (0x10000000) // CompactFlash 1 on EBI Chip Select 5 size in byte (262144 Kbytes)
// EBI_SDRAM
#define AT91C_EBI_SDRAM	          (0x70000000) // SDRAM on EBI Chip Select 6 base address
#define AT91C_EBI_SDRAM_SIZE	     (0x10000000) // SDRAM on EBI Chip Select 6 size in byte (262144 Kbytes)
// EBI_SDRAM_16BIT
#define AT91C_EBI_SDRAM_16BIT	    (0x70000000) // SDRAM on EBI Chip Select 6 base address
#define AT91C_EBI_SDRAM_16BIT_SIZE	 (0x02000000) // SDRAM on EBI Chip Select 6 size in byte (32768 Kbytes)
// EBI_SDRAM_32BIT
#define AT91C_EBI_SDRAM_32BIT	    (0x70000000) // SDRAM on EBI Chip Select 6 base address
#define AT91C_EBI_SDRAM_32BIT_SIZE	 (0x04000000) // SDRAM on EBI Chip Select 6 size in byte (65536 Kbytes)

#define AT91C_NR_PIO               (32 * 4)

#endif
