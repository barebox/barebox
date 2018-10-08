#ifndef __MACH_ESDCTL_V2_H
#define __MACH_ESDCTL_V2_H

/* SDRAM Controller registers */
#define IMX_ESDCTL0 0x00 /* Enhanced SDRAM Control Register 0       */
#define IMX_ESDCFG0 0x04 /* Enhanced SDRAM Configuration Register 0 */
#define IMX_ESDCTL1 0x08 /* Enhanced SDRAM Control Register 1       */
#define IMX_ESDCFG1 0x0C /* Enhanced SDRAM Configuration Register 1 */
#define IMX_ESDMISC 0x10 /* Enhanced SDRAM Miscellanious Register   */

#define ESDCTL0_SDE				(1 << 31)
#define ESDCTL0_SMODE_NORMAL			(0 << 28)
#define ESDCTL0_SMODE_PRECHARGE			(1 << 28)
#define ESDCTL0_SMODE_AUTO_REFRESH		(2 << 28)
#define ESDCTL0_SMODE_LOAD_MODE			(3 << 28)
#define ESDCTL0_SMODE_MANUAL_SELF_REFRESH	(4 << 28)
#define ESDCTL0_SP				(1 << 27)
#define ESDCTL0_ROW11				(0 << 24)
#define ESDCTL0_ROW12				(1 << 24)
#define ESDCTL0_ROW13				(2 << 24)
#define ESDCTL0_ROW14				(3 << 24)
#define ESDCTL0_ROW15				(4 << 24)
#define ESDCTL0_ROW_MASK			(7 << 24)
#define ESDCTL0_COL8				(0 << 20)
#define ESDCTL0_COL9				(1 << 20)
#define ESDCTL0_COL10				(2 << 20)
#define ESDCTL0_COL_MASK			(3 << 20)
#define ESDCTL0_DSIZ_31_16			(0 << 16)
#define ESDCTL0_DSIZ_15_0			(1 << 16)
#define ESDCTL0_DSIZ_31_0			(2 << 16)
#define ESDCTL0_DSIZ_MASK			(3 << 16)
#define ESDCTL0_REF1				(1 << 13)
#define ESDCTL0_REF2				(2 << 13)
#define ESDCTL0_REF4				(3 << 13)
#define ESDCTL0_REF8				(4 << 13)
#define ESDCTL0_REF16				(5 << 13)
#define ESDCTL0_PWDT_DISABLED			(0 << 10)
#define ESDCTL0_PWDT_PRECHARGE_PWDN		(1 << 10)
#define ESDCTL0_PWDT_PWDN_64			(2 << 10)
#define ESDCTL0_PWDT_PWDN_128			(3 << 10)
#define ESDCTL0_FP				(1 << 8)
#define ESDCTL0_BL				(1 << 7)

#define ESDMISC_RST			0x00000002
#define ESDMISC_MDDR_EN			0x00000004
#define ESDMISC_MDDR_DIS		0x00000000
#define ESDMISC_MDDR_DL_RST		0x00000008
#define ESDMISC_MDDR_MDIS		0x00000010
#define ESDMISC_LHD			0x00000020
#define ESDMISC_SDRAMRDY		0x80000000
#define ESDMISC_DDR2_8_BANK		BIT(6)

#define	ESDCFGx_tXP_MASK 		0x00600000
#define ESDCFGx_tXP_1			0x00000000
#define ESDCFGx_tXP_2			0x00200000
#define ESDCFGx_tXP_3			0x00400000
#define ESDCFGx_tXP_4			0x00600000

#define ESDCFGx_tWTR_MASK		0x00100000
#define ESDCFGx_tWTR_1			0x00000000
#define ESDCFGx_tWTR_2			0x00100000

#define ESDCFGx_tRP_MASK		0x000c0000
#define ESDCFGx_tRP_1			0x00000000
#define ESDCFGx_tRP_2			0x00040000
#define ESDCFGx_tRP_3			0x00080000
#define ESDCFGx_tRP_4			0x000c0000


#define ESDCFGx_tMRD_MASK		0x00030000
#define ESDCFGx_tMRD_1			0x00000000
#define ESDCFGx_tMRD_2			0x00010000
#define ESDCFGx_tMRD_3			0x00020000
#define ESDCFGx_tMRD_4			0x00030000


#define ESDCFGx_tWR_MASK		0x00008000
#define ESDCFGx_tWR_1_2			0x00000000
#define ESDCFGx_tWR_2_3			0x00008000

#define ESDCFGx_tRAS_MASK		0x00007000
#define ESDCFGx_tRAS_1			0x00000000
#define ESDCFGx_tRAS_2			0x00001000
#define ESDCFGx_tRAS_3			0x00002000
#define ESDCFGx_tRAS_4			0x00003000
#define ESDCFGx_tRAS_5			0x00004000
#define ESDCFGx_tRAS_6			0x00005000
#define ESDCFGx_tRAS_7			0x00006000
#define ESDCFGx_tRAS_8			0x00007000


#define ESDCFGx_tRRD_MASK		0x00000c00
#define ESDCFGx_tRRD_1			0x00000000
#define ESDCFGx_tRRD_2			0x00000400
#define ESDCFGx_tRRD_3			0x00000800
#define ESDCFGx_tRRD_4			0x00000c00


#define ESDCFGx_tCAS_MASK		0x00000300
#define ESDCFGx_tCAS_2			0x00000200
#define ESDCFGx_tCAS_3			0x00000300

#define ESDCFGx_tRCD_MASK		0x00000070
#define ESDCFGx_tRCD_1			0x00000000
#define ESDCFGx_tRCD_2			0x00000010
#define ESDCFGx_tRCD_3			0x00000020
#define ESDCFGx_tRCD_4			0x00000030
#define ESDCFGx_tRCD_5			0x00000040
#define ESDCFGx_tRCD_6			0x00000050
#define ESDCFGx_tRCD_7			0x00000060
#define ESDCFGx_tRCD_8			0x00000070

#define ESDCFGx_tRC_MASK		0x0000000f
#define ESDCFGx_tRC_20			0x00000000
#define ESDCFGx_tRC_2			0x00000001
#define ESDCFGx_tRC_3			0x00000002
#define ESDCFGx_tRC_4			0x00000003
#define ESDCFGx_tRC_5			0x00000004
#define ESDCFGx_tRC_6			0x00000005
#define ESDCFGx_tRC_7			0x00000006
#define ESDCFGx_tRC_8			0x00000007
#define ESDCFGx_tRC_9			0x00000008
#define ESDCFGx_tRC_10			0x00000009
#define ESDCFGx_tRC_11			0x0000000a
#define ESDCFGx_tRC_12			0x0000000b
#define ESDCFGx_tRC_13			0x0000000c
#define ESDCFGx_tRC_14			0x0000000d
//#define ESDCFGx_tRC_14		0x0000000e	// 15 seems to not exist
#define ESDCFGx_tRC_16			0x0000000f

#ifndef __ASSEMBLY__
void __noreturn imx1_barebox_entry(void *boarddata);
void __noreturn imx25_barebox_entry(void *boarddata);
void __noreturn imx27_barebox_entry(void *boarddata);
void __noreturn imx31_barebox_entry(void *boarddata);
void __noreturn imx35_barebox_entry(void *boarddata);
void __noreturn imx51_barebox_entry(void *boarddata);
void __noreturn imx53_barebox_entry(void *boarddata);
void __noreturn imx6q_barebox_entry(void *boarddata);
void __noreturn imx6ul_barebox_entry(void *boarddata);
void __noreturn vf610_barebox_entry(void *boarddata);
void __noreturn imx8mq_barebox_entry(void *boarddata);
void __noreturn imx7d_barebox_entry(void *boarddata);
void imx_esdctl_disable(void);
#endif

#endif /* __MACH_ESDCTL_V2_H */
