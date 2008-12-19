
/* SDRAM Controller registers */
#define ESDCTL0 __REG(IMX_ESD_BASE + 0x00) /* Enhanced SDRAM Control Register 0       */
#define ESDCFG0 __REG(IMX_ESD_BASE + 0x04) /* Enhanced SDRAM Configuration Register 0 */
#define ESDCTL1 __REG(IMX_ESD_BASE + 0x08) /* Enhanced SDRAM Control Register 1       */
#define ESDCFG1 __REG(IMX_ESD_BASE + 0x0C) /* Enhanced SDRAM Configuration Register 1 */
#define ESDMISC __REG(IMX_ESD_BASE + 0x10) /* Enhanced SDRAM Miscellanious Register   */

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
#define ESDCTL0_COL8				(0 << 20)
#define ESDCTL0_COL9				(1 << 20)
#define ESDCTL0_COL10				(2 << 20)
#define ESDCTL0_DSIZ_31_16			(0 << 16)
#define ESDCTL0_DSIZ_15_0			(1 << 16)
#define ESDCTL0_DSIZ_31_0			(2 << 16)
#define ESDCTL0_REF1				(1 << 13)
#define ESDCTL0_REF2				(2 << 13)
#define ESDCTL0_REF4				(3 << 13)
#define ESDCTL0_REF8				(4 << 13)
#define ESDCTL0_REF16				(5 << 13)
#define ESDCTL0_FP				(1 << 8)
#define ESDCTL0_BL				(1 << 7)

