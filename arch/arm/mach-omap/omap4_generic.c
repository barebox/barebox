#include <common.h>
#include <init.h>
#include <mach/silicon.h>
#include <asm/io.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-clock.h>
#include <mach/syslib.h>
#include <mach/xload.h>

void __noreturn reset_cpu(unsigned long addr)
{
	writel(PRM_RSTCTRL_RESET, PRM_RSTCTRL);

	while (1);
}

#define WATCHDOG_WSPR	0x48
#define WATCHDOG_WWPS	0x34

static void wait_for_command_complete(void)
{
	int pending = 1;

	do {
		pending = readl(OMAP44XX_WDT2_BASE + WATCHDOG_WWPS);
	} while (pending);
}

/* EMIF */
#define EMIF_MOD_ID_REV			0x0000
#define EMIF_STATUS			0x0004
#define EMIF_SDRAM_CONFIG		0x0008
#define EMIF_LPDDR2_NVM_CONFIG		0x000C
#define EMIF_SDRAM_REF_CTRL		0x0010
#define EMIF_SDRAM_REF_CTRL_SHDW	0x0014
#define EMIF_SDRAM_TIM_1		0x0018
#define EMIF_SDRAM_TIM_1_SHDW		0x001C
#define EMIF_SDRAM_TIM_2		0x0020
#define EMIF_SDRAM_TIM_2_SHDW		0x0024
#define EMIF_SDRAM_TIM_3		0x0028
#define EMIF_SDRAM_TIM_3_SHDW		0x002C
#define EMIF_LPDDR2_NVM_TIM		0x0030
#define EMIF_LPDDR2_NVM_TIM_SHDW	0x0034
#define EMIF_PWR_MGMT_CTRL		0x0038
#define EMIF_PWR_MGMT_CTRL_SHDW		0x003C
#define EMIF_LPDDR2_MODE_REG_DATA	0x0040
#define EMIF_LPDDR2_MODE_REG_CFG	0x0050
#define EMIF_L3_CONFIG			0x0054
#define EMIF_L3_CFG_VAL_1		0x0058
#define EMIF_L3_CFG_VAL_2		0x005C
#define IODFT_TLGC			0x0060
#define EMIF_PERF_CNT_1			0x0080
#define EMIF_PERF_CNT_2			0x0084
#define EMIF_PERF_CNT_CFG		0x0088
#define EMIF_PERF_CNT_SEL		0x008C
#define EMIF_PERF_CNT_TIM		0x0090
#define EMIF_READ_IDLE_CTRL		0x0098
#define EMIF_READ_IDLE_CTRL_SHDW	0x009c
#define EMIF_ZQ_CONFIG			0x00C8
#define EMIF_DDR_PHY_CTRL_1		0x00E4
#define EMIF_DDR_PHY_CTRL_1_SHDW	0x00E8
#define EMIF_DDR_PHY_CTRL_2		0x00EC

#define DMM_LISA_MAP_0 			0x0040
#define DMM_LISA_MAP_1 			0x0044
#define DMM_LISA_MAP_2 			0x0048
#define DMM_LISA_MAP_3 			0x004C

#define MR0_ADDR			0
#define MR1_ADDR			1
#define MR2_ADDR			2
#define MR4_ADDR			4
#define MR10_ADDR			10
#define MR16_ADDR			16
#define REF_EN				0x40000000
/* defines for MR1 */
#define MR1_BL4				2
#define MR1_BL8				3
#define MR1_BL16			4

#define MR1_BT_SEQ			0
#define BT_INT				1

#define MR1_WC				0
#define MR1_NWC				1

#define MR1_NWR3			1
#define MR1_NWR4			2
#define MR1_NWR5			3
#define MR1_NWR6			4
#define MR1_NWR7			5
#define MR1_NWR8			6

#define MR1_VALUE	(MR1_NWR3 << 5) | (MR1_WC << 4) | (MR1_BT_SEQ << 3)  \
							| (MR1_BL8 << 0)

/* defines for MR2 */
#define MR2_RL3_WL1			1
#define MR2_RL4_WL2			2
#define MR2_RL5_WL2			3
#define MR2_RL6_WL3			4

/* defines for MR10 */
#define MR10_ZQINIT			0xFF
#define MR10_ZQRESET			0xC3
#define MR10_ZQCL			0xAB
#define MR10_ZQCS			0x56


/* TODO: FREQ update method is not working so shadow registers programming
 * is just for same of completeness. This would be safer if auto
 * trasnitions are working
 */
#define FREQ_UPDATE_EMIF
/* EMIF Needs to be configured@19.2 MHz and shadow registers
 * should be programmed for new OPP.
 */
/* Elpida 2x2Gbit */
#define SDRAM_CONFIG_INIT		0x80800EB1
#define DDR_PHY_CTRL_1_INIT		0x849FFFF5
#define READ_IDLE_CTRL			0x000501FF
#define PWR_MGMT_CTRL			0x4000000f
#define PWR_MGMT_CTRL_OPP100		0x4000000f
#define ZQ_CONFIG			0x500b3215

#define CS1_MR(mr)	((mr) | 0x80000000)

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b" : "=r" (loops) : "0"(loops));
}

int omap4_emif_config(unsigned int base, const struct ddr_regs *ddr_regs)
{
	/*
	 * set SDRAM CONFIG register
	 * EMIF_SDRAM_CONFIG[31:29] REG_SDRAM_TYPE = 4 for LPDDR2-S4
	 * EMIF_SDRAM_CONFIG[28:27] REG_IBANK_POS = 0
	 * EMIF_SDRAM_CONFIG[13:10] REG_CL = 3
	 * EMIF_SDRAM_CONFIG[6:4] REG_IBANK = 3 - 8 banks
	 * EMIF_SDRAM_CONFIG[3] REG_EBANK = 0 - CS0
	 * EMIF_SDRAM_CONFIG[2:0] REG_PAGESIZE = 2  - 512- 9 column
	 * JDEC specs - S4-2Gb --8 banks -- R0-R13, C0-c8
	 */
	writel(readl(base + EMIF_LPDDR2_NVM_CONFIG) & 0xbfffffff,
						 base + EMIF_LPDDR2_NVM_CONFIG);
	writel(ddr_regs->config_init, base + EMIF_SDRAM_CONFIG);

	/* PHY control values */
	writel(DDR_PHY_CTRL_1_INIT, base + EMIF_DDR_PHY_CTRL_1);
	writel(ddr_regs->phy_ctrl_1, base + EMIF_DDR_PHY_CTRL_1_SHDW);

	/*
	 * EMIF_READ_IDLE_CTRL
	 */
	writel(READ_IDLE_CTRL, base + EMIF_READ_IDLE_CTRL);
	writel(READ_IDLE_CTRL, base + EMIF_READ_IDLE_CTRL);

	/*
	 * EMIF_SDRAM_TIM_1
	 */
	writel(ddr_regs->tim1, base + EMIF_SDRAM_TIM_1);
	writel(ddr_regs->tim1, base + EMIF_SDRAM_TIM_1_SHDW);

	/*
	 * EMIF_SDRAM_TIM_2
	 */
	writel(ddr_regs->tim2, base + EMIF_SDRAM_TIM_2);
	writel(ddr_regs->tim2, base + EMIF_SDRAM_TIM_2_SHDW);

	/*
	 * EMIF_SDRAM_TIM_3
	 */
	writel(ddr_regs->tim3, base + EMIF_SDRAM_TIM_3);
	writel(ddr_regs->tim3, base + EMIF_SDRAM_TIM_3_SHDW);

	writel(ddr_regs->zq_config, base + EMIF_ZQ_CONFIG);

	/*
	 * poll MR0 register (DAI bit)
	 * REG_CS[31] = 0 -- Mode register command to CS0
	 * REG_REFRESH_EN[30] = 1 -- Refresh enable after MRW
	 * REG_ADDRESS[7:0] = 00 -- Refresh enable after MRW
	 */

	writel(MR0_ADDR, base + EMIF_LPDDR2_MODE_REG_CFG);

	while (readl(base + EMIF_LPDDR2_MODE_REG_DATA) & 1)
		;

	writel(CS1_MR(MR0_ADDR), base + EMIF_LPDDR2_MODE_REG_CFG);

	while (readl(base + EMIF_LPDDR2_MODE_REG_DATA) & 1)
		;


	/* set MR10 register */
	writel(MR10_ADDR, base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(MR10_ZQINIT, base + EMIF_LPDDR2_MODE_REG_DATA);
	writel(CS1_MR(MR10_ADDR), base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(MR10_ZQINIT, base + EMIF_LPDDR2_MODE_REG_DATA);

	/* wait for tZQINIT=1us  */
	delay(10);

	/* set MR1 register */
	writel(MR1_ADDR, base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(ddr_regs->mr1, base + EMIF_LPDDR2_MODE_REG_DATA);
	writel(CS1_MR(MR1_ADDR), base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(ddr_regs->mr1, base + EMIF_LPDDR2_MODE_REG_DATA);

	/* set MR2 register RL=6 for OPP100 */
	writel(MR2_ADDR, base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(ddr_regs->mr2, base + EMIF_LPDDR2_MODE_REG_DATA);
	writel(CS1_MR(MR2_ADDR), base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(ddr_regs->mr2, base + EMIF_LPDDR2_MODE_REG_DATA);

	/* Set SDRAM CONFIG register again here with final RL-WL value */
	writel(ddr_regs->config_final, base + EMIF_SDRAM_CONFIG);
	writel(ddr_regs->phy_ctrl_1, base + EMIF_DDR_PHY_CTRL_1);

	/*
	 * EMIF_SDRAM_REF_CTRL
	 * refresh rate = DDR_CLK / reg_refresh_rate
	 * 3.9 uS = (400MHz)	/ reg_refresh_rate
	 */
	writel(ddr_regs->ref_ctrl, base + EMIF_SDRAM_REF_CTRL);
	writel(ddr_regs->ref_ctrl, base + EMIF_SDRAM_REF_CTRL_SHDW);

	/* set MR16 register */
	writel(MR16_ADDR | REF_EN, base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(0, base + EMIF_LPDDR2_MODE_REG_DATA);
	writel(CS1_MR(MR16_ADDR | REF_EN),
					       base + EMIF_LPDDR2_MODE_REG_CFG);
	writel(0, base + EMIF_LPDDR2_MODE_REG_DATA);

	/* LPDDR2 init complete */

	return 0;
}

static void reset_phy(unsigned int base)
{
	*(volatile int*)(base + IODFT_TLGC) |= (1 << 10);
}

void omap4_ddr_init(const struct ddr_regs *ddr_regs,
		const struct dpll_param *core)
{
	unsigned int rev;
	rev = omap4_revision();

	if (rev == OMAP4430_ES2_0) {
		writel(0x9e9e9e9e, 0x4A100638);
		writel(0x9e9e9e9e, 0x4A10063c);
		writel(0x9e9e9e9e, 0x4A100640);
		writel(0x9e9e9e9e, 0x4A100648);
		writel(0x9e9e9e9e, 0x4A10064c);
		writel(0x9e9e9e9e, 0x4A100650);
		/* LPDDR2IO set to NMOS PTV */
		writel(0x00ffc000, 0x4A100704);
	}

	/*
	 * DMM Configuration
	 */

	/* Both EMIFs 128 byte interleaved */
	writel(0x80640300, OMAP44XX_DMM_BASE + DMM_LISA_MAP_0);

	*(volatile int*)(OMAP44XX_DMM_BASE + DMM_LISA_MAP_2) = 0x00000000;
	*(volatile int*)(OMAP44XX_DMM_BASE + DMM_LISA_MAP_3) = 0xFF020100;

	/* DDR needs to be initialised @ 19.2 MHz
	 * So put core DPLL in bypass mode
	 * Configure the Core DPLL but don't lock it
	 */
	omap4_configure_core_dpll_no_lock(core);

	/* No IDLE: BUG in SDC */
	sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	while(((*(volatile int*)CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);

	*(volatile int*)(OMAP44XX_EMIF1_BASE + EMIF_PWR_MGMT_CTRL) = 0x0;
	*(volatile int*)(OMAP44XX_EMIF2_BASE + EMIF_PWR_MGMT_CTRL) = 0x0;

	omap4_emif_config(OMAP44XX_EMIF1_BASE, ddr_regs);
	omap4_emif_config(OMAP44XX_EMIF2_BASE, ddr_regs);

	/* Lock Core using shadow CM_SHADOW_FREQ_CONFIG1 */
	omap4_lock_core_dpll_shadow(core);

	/* Set DLL_OVERRIDE = 0 */
	*(volatile int*)CM_DLL_CTRL = 0x0;

	delay(200);

	/* Check for DDR PHY ready for EMIF1 & EMIF2 */
	while((((*(volatile int*)(OMAP44XX_EMIF1_BASE + EMIF_STATUS))&(0x04)) != 0x04) \
	|| (((*(volatile int*)(OMAP44XX_EMIF2_BASE + EMIF_STATUS))&(0x04)) != 0x04));

	/* Reprogram the DDR PYHY Control register */
	/* PHY control values */

	sr32(CM_MEMIF_EMIF_1_CLKCTRL, 0, 32, 0x1);
        sr32(CM_MEMIF_EMIF_2_CLKCTRL, 0, 32, 0x1);

	/* Put the Core Subsystem PD to ON State */

	/* No IDLE: BUG in SDC */
	//sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	//while(((*(volatile int*)CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);
	*(volatile int*)(OMAP44XX_EMIF1_BASE + EMIF_PWR_MGMT_CTRL) = 0x80000000;
	*(volatile int*)(OMAP44XX_EMIF2_BASE + EMIF_PWR_MGMT_CTRL) = 0x80000000;

	/*
	 * DMM : DMM_LISA_MAP_0(Section_0)
	 * [31:24] SYS_ADDR 		0x80
	 * [22:20] SYS_SIZE		0x7 - 2Gb
	 * [19:18] SDRC_INTLDMM		0x1 - 128 byte
	 * [17:16] SDRC_ADDRSPC 	0x0
	 * [9:8] SDRC_MAP 		0x3
	 * [7:0] SDRC_ADDR		0X0
	 */
	reset_phy(OMAP44XX_EMIF1_BASE);
	reset_phy(OMAP44XX_EMIF2_BASE);

	*((volatile int *)0x80000000) = 0;
	*((volatile int *)0x80000080) = 0;
}

void omap4_power_i2c_send(u32 r)
{
	u32 val;

	writel(r, OMAP44XX_PRM_VC_VAL_BYPASS);

	val = readl(OMAP44XX_PRM_VC_VAL_BYPASS);
	val |= 0x1000000;
	writel(val, OMAP44XX_PRM_VC_VAL_BYPASS);

	while (readl(OMAP44XX_PRM_VC_VAL_BYPASS) & 0x1000000)
		;

	val = readl(OMAP44XX_PRM_IRQSTATUS_MPU_A9);
	writel(val, OMAP44XX_PRM_IRQSTATUS_MPU_A9);
}

static unsigned int cortex_a9_rev(void)
{

	unsigned int i;

	asm ("mrc p15, 0, %0, c0, c0, 0" : "=r" (i));

	return i;
}

unsigned int omap4_revision(void)
{
	unsigned int chip_rev = 0;
	unsigned int rev = cortex_a9_rev();

	switch(rev) {
	case 0x410FC091:
		return OMAP4430_ES1_0;
	case 0x411FC092:
		chip_rev = (readl(OMAP44XX_CTRL_BASE + 0x204)  >> 28) & 0xF;
		if (chip_rev == 3)
			return OMAP4430_ES2_1;
		else if (chip_rev >= 4)
			return OMAP4430_ES2_2;
		else
			return OMAP4430_ES2_0;
	}
	return OMAP4430_SILICON_ID_INVALID;
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	void __iomem *wd2_base = (void *)OMAP44XX_WDT2_BASE;

	writel(WD_UNLOCK1, wd2_base + WATCHDOG_WSPR);
	wait_for_command_complete();
	writel(WD_UNLOCK2, wd2_base + WATCHDOG_WSPR);

	return 0;
}
late_initcall(watchdog_init);

static int omap_vector_init(void)
{
	__asm__ __volatile__ (
		"mov    r0, #0;"
		"mcr    p15, #0, r0, c12, c0, #0;"
		:
		:
		: "r0"
	);

	return 0;
}
core_initcall(omap_vector_init);

#define OMAP4_TRACING_VECTOR3 0x4030d048

enum omap_boot_src omap4_bootsrc(void)
{
	u32 bootsrc = readl(OMAP4_TRACING_VECTOR3);

	if (bootsrc & (1 << 5))
		return OMAP_BOOTSRC_MMC1;
	if (bootsrc & (1 << 3))
		return OMAP_BOOTSRC_NAND;
	return OMAP_BOOTSRC_UNKNOWN;
}
