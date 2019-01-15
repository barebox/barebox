#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <restart.h>
#include <io.h>
#include <mach/omap4-clock.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-mux.h>
#include <mach/syslib.h>
#include <mach/generic.h>
#include <mach/gpmc.h>
#include <mach/omap4_rom_usb.h>
#include <mach/omap4-generic.h>

/*
 *  The following several lines are taken from U-Boot to support
 * recognizing more revisions of OMAP4 chips.
 */

#define MIDR_CORTEX_A9_R0P1	0x410FC091
#define MIDR_CORTEX_A9_R1P2	0x411FC092
#define MIDR_CORTEX_A9_R1P3	0x411FC093
#define MIDR_CORTEX_A9_R2P10	0x412FC09A

#define CONTROL_ID_CODE         0x4A002204

#define OMAP4_CONTROL_ID_CODE_ES1_0     0x0B85202F
#define OMAP4_CONTROL_ID_CODE_ES2_0     0x1B85202F
#define OMAP4_CONTROL_ID_CODE_ES2_1     0x3B95C02F
#define OMAP4_CONTROL_ID_CODE_ES2_2     0x4B95C02F
#define OMAP4_CONTROL_ID_CODE_ES2_3     0x6B95C02F
#define OMAP4460_CONTROL_ID_CODE_ES1_0  0x0B94E02F
#define OMAP4460_CONTROL_ID_CODE_ES1_1  0x2B94E02F

/* EMIF_L3_CONFIG register value */
#define EMIF_L3_CONFIG_VAL_SYS_10_LL_0		0x0A0000FF
#define EMIF_L3_CONFIG_VAL_SYS_10_MPU_3_LL_0	0x0A300000

static void __noreturn omap4_restart_soc(struct restart_handler *rst)
{
	writel(OMAP44XX_PRM_RSTCTRL_RESET, OMAP44XX_PRM_RSTCTRL);

	hang();
}

void omap4_set_warmboot_order(u32 *device_list)
{
	const u32 CH[] = {
		0xCF00AA01,
		0x0000000C,
		(device_list[0] << 16) | 0x0000,
		(device_list[2] << 16) | device_list[1],
		0x0000 | device_list[3],
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(CH); i++)
		writel(CH[i], OMAP44XX_SAR_CH_START + i*sizeof(CH[0]));
	writel(OMAP44XX_SAR_CH_START, OMAP44XX_SAR_CH_ADDRESS);
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

static int omap4_emif_config(unsigned int base, const struct ddr_regs *ddr_regs)
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
	unsigned int val = readl(base + IODFT_TLGC);
	val |= (1 << 10);
	writel(val, base + IODFT_TLGC);
}

void omap4_ddr_init(const struct ddr_regs *ddr_regs,
		const struct dpll_param *core)
{
	unsigned int rev = omap4_revision();

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

	writel(0x00000000, OMAP44XX_DMM_BASE + DMM_LISA_MAP_2);
	writel(0xFF020100, OMAP44XX_DMM_BASE + DMM_LISA_MAP_3);

	if (rev >= OMAP4460_ES1_0) {
		writel(0x80640300, OMAP44XX_MA_BASE + DMM_LISA_MAP_0);

		writel(0x00000000, OMAP44XX_MA_BASE + DMM_LISA_MAP_2);
		writel(0xFF020100, OMAP44XX_MA_BASE + DMM_LISA_MAP_3);
	}

	/* DDR needs to be initialised @ 19.2 MHz
	 * So put core DPLL in bypass mode
	 * Configure the Core DPLL but don't lock it
	 */
	omap4_configure_core_dpll_no_lock(core);

	/* No IDLE: BUG in SDC */
	sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	while ((readl(CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);

	writel(0x0, OMAP44XX_EMIF1_BASE + EMIF_PWR_MGMT_CTRL);
	writel(0x0, OMAP44XX_EMIF2_BASE + EMIF_PWR_MGMT_CTRL);

	omap4_emif_config(OMAP44XX_EMIF1_BASE, ddr_regs);
	omap4_emif_config(OMAP44XX_EMIF2_BASE, ddr_regs);

	/* Lock Core using shadow CM_SHADOW_FREQ_CONFIG1 */
	omap4_lock_core_dpll_shadow(core);

	/* Set DLL_OVERRIDE = 0 */
	writel(0x0, CM_DLL_CTRL);

	delay(200);

	/* Check for DDR PHY ready for EMIF1 & EMIF2 */
	while (((readl(OMAP44XX_EMIF1_BASE + EMIF_STATUS) & 0x04) != 0x04) \
		|| ((readl(OMAP44XX_EMIF2_BASE + EMIF_STATUS) & 0x04) != 0x04));

	/* Reprogram the DDR PYHY Control register */
	/* PHY control values */

	sr32(CM_MEMIF_EMIF_1_CLKCTRL, 0, 32, 0x1);
	sr32(CM_MEMIF_EMIF_2_CLKCTRL, 0, 32, 0x1);

	/* Put the Core Subsystem PD to ON State */

	/* No IDLE: BUG in SDC */
	//sr32(CM_MEMIF_CLKSTCTRL, 0, 32, 0x2);
	//while ((readl(CM_MEMIF_CLKSTCTRL) & 0x700) != 0x700);
	writel(0x80000000, OMAP44XX_EMIF1_BASE + EMIF_PWR_MGMT_CTRL);
	writel(0x80000000, OMAP44XX_EMIF2_BASE + EMIF_PWR_MGMT_CTRL);

	if (rev >= OMAP4460_ES1_0) {
		writel(EMIF_L3_CONFIG_VAL_SYS_10_MPU_3_LL_0,
				OMAP44XX_EMIF1_BASE + EMIF_L3_CONFIG);
		writel(EMIF_L3_CONFIG_VAL_SYS_10_MPU_3_LL_0,
				OMAP44XX_EMIF2_BASE + EMIF_L3_CONFIG);
	}

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

	writel(0, 0x80000000);
	writel(0, 0x80000080);
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

	/* Read Main ID Register (MIDR) */
	asm ("mrc p15, 0, %0, c0, c0, 0" : "=r" (i));

	return i;
}

unsigned int omap4_revision(void)
{
	unsigned int rev = cortex_a9_rev();

	switch(rev) {
	case MIDR_CORTEX_A9_R0P1:
		return OMAP4430_ES1_0;
	case MIDR_CORTEX_A9_R1P2:
		switch (readl(CONTROL_ID_CODE)) {
		case OMAP4_CONTROL_ID_CODE_ES2_0:
			return OMAP4430_ES2_0;
			break;
		case OMAP4_CONTROL_ID_CODE_ES2_1:
			return OMAP4430_ES2_1;
			break;
		case OMAP4_CONTROL_ID_CODE_ES2_2:
			return OMAP4430_ES2_2;
			break;
		default:
			return OMAP4430_ES2_0;
			break;
		}
		break;
	case MIDR_CORTEX_A9_R1P3:
		return OMAP4430_ES2_3;
		break;
	case MIDR_CORTEX_A9_R2P10:
		switch (readl(CONTROL_ID_CODE)) {
		case OMAP4460_CONTROL_ID_CODE_ES1_1:
			return OMAP4460_ES1_1;
			break;
		case OMAP4460_CONTROL_ID_CODE_ES1_0:
		default:
			return OMAP4460_ES1_0;
			break;
		}
		break;
	default:
		return OMAP4430_SILICON_ID_INVALID;
		break;
	}
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	void __iomem *wd2_base = (void *)OMAP44XX_WDT2_BASE;

	if (!cpu_is_omap4())
		return 0;

	writel(WD_UNLOCK1, wd2_base + WATCHDOG_WSPR);
	wait_for_command_complete();
	writel(WD_UNLOCK2, wd2_base + WATCHDOG_WSPR);

	return 0;
}
late_initcall(watchdog_init);

static int omap_vector_init(void)
{
	/*
	 * omap4 usbboot interfaces with the omap4 ROM to reuse the USB port
	 * used for booting.
	 * The ROM code uses interrupts for the transfers, so do not modify the
	 * interrupt vectors in this case.
	 */
	if (bootsource_get() != BOOTSOURCE_USB) {
		__asm__ __volatile__ (
			"mov    r0, #0;"
			"mcr    p15, #0, r0, c12, c0, #0;"
			:
			:
			: "r0"
		);
	}

	return 0;
}

static int omap4_bootsource(void)
{
	enum bootsource src;
	uint32_t *omap4_bootinfo = (void *)OMAP44XX_SRAM_SCRATCH_SPACE;

	switch (omap4_bootinfo[2] & 0xFF) {
	case OMAP44XX_SAR_BOOT_NAND:
		src = BOOTSOURCE_NAND;
		break;
	case OMAP44XX_SAR_BOOT_MMC1:
		src = BOOTSOURCE_MMC;
		break;
	case OMAP44XX_SAR_BOOT_USB_1:
		src = BOOTSOURCE_USB;
		break;
	default:
		src = BOOTSOURCE_UNKNOWN;
	}

	bootsource_set(src);
	bootsource_set_instance(0);

	omap_vector_init();

	return 0;
}

int omap4_init(void)
{
	omap_gpmc_base = (void *)OMAP44XX_GPMC_BASE;

	restart_handler_register_fn(omap4_restart_soc);

	return omap4_bootsource();
}

#define GPIO_MASK 0x1f

static void __iomem *omap4_get_gpio_base(unsigned gpio)
{
	void __iomem *base;

	if (gpio < 32)
		base = (void *)0x4a310000;
	else
		base = (void *)(0x48053000 + ((gpio & ~GPIO_MASK) << 8));

	return base;
}

#define I2C_SLAVE 0x12

noinline int omap4430_scale_vcores(void)
{
	unsigned int rev = omap4_revision();

	/* For VC bypass only VCOREx_CGF_FORCE  is necessary and
	 * VCOREx_CFG_VOLTAGE  changes can be discarded
	 */
	writel(0, OMAP44XX_PRM_VC_CFG_I2C_MODE);
	writel(0x6026, OMAP44XX_PRM_VC_CFG_I2C_CLK);

	/* set VCORE1 force VSEL
	 * 4430 : supplies vdd_mpu
	 * Setting a high voltage for Nitro mode as smart reflex is not enabled.
	 * We use the maximum possible value in the AVS range because the next
	 * higher voltage in the discrete range (code >= 0b111010) is way too
	 * high
	 */

	/* 0x55: i2c addr, 3A: ~ 1430 mvolts*/
	omap4_power_i2c_send((0x3A55 << 8) | I2C_SLAVE);

	/* FIXME: set VCORE2 force VSEL, Check the reset value */
	omap4_power_i2c_send((0x295B << 8) | I2C_SLAVE);

	/* set VCORE3 force VSEL */
	switch (rev) {
	case OMAP4430_ES2_0:
		omap4_power_i2c_send((0x2961 << 8) | I2C_SLAVE);
		break;
	case OMAP4430_ES2_1:
		omap4_power_i2c_send((0x2A61 << 8) | I2C_SLAVE);
		break;
	}

	return 0;
}

noinline int omap4460_scale_vcores(unsigned vsel0_pin, unsigned volt_mv)
{
	void __iomem *base;
	u32 val = 0;

	/* For VC bypass only VCOREx_CGF_FORCE  is necessary and
	 * VCOREx_CFG_VOLTAGE  changes can be discarded
	 */
	writel(0, OMAP44XX_PRM_VC_CFG_I2C_MODE);
	writel(0x6026, OMAP44XX_PRM_VC_CFG_I2C_CLK);

	/* TPS - supplies vdd_mpu on 4460
	 * Setup SET1 and don't touch SET0 it acts as boot voltage
	 * source after reset.
	 */

	omap4_do_scale_tps62361(TPS62361_REG_ADDR_SET1, volt_mv);

	/*
	 * Select SET1 in TPS62361:
	 * VSEL1 is grounded on board. So the following selects
	 * VSEL1 = 0 and VSEL0 = 1
	 */
	base = omap4_get_gpio_base(vsel0_pin);

	val = 1 << (vsel0_pin & GPIO_MASK);
	writel(val, base + 0x190);

	val =  readl(base + 0x134);
	val &= ~(1 << (vsel0_pin & GPIO_MASK));
	writel(val, base + 0x134);

	val = 1 << (vsel0_pin & GPIO_MASK);
	writel(val, base + 0x194);

	/* set VCORE1 force VSEL
	 * 4460 : supplies vdd_core
	 */

	/* 0x55: i2c addr, 28: ~ 1200 mvolts*/
	omap4_power_i2c_send((0x2855 << 8) | I2C_SLAVE);

	/* FIXME: set VCORE2 force VSEL, Check the reset value */
	omap4_power_i2c_send((0x295B << 8) | I2C_SLAVE);

	return 0;
}

void omap4_do_set_mux(u32 base, struct pad_conf_entry const *array, int size)
{
	int i;
	struct pad_conf_entry *pad = (struct pad_conf_entry *) array;

	for (i = 0; i < size; i++, pad++)
		writew(pad->val, base + pad->offset);
}

/* GPMC timing for OMAP4 nand device */
const struct gpmc_config omap4_nand_cfg = {
	.cfg = {
		0x00000800,	/* CONF1 */
		0x00050500,	/* CONF2 */
		0x00040400,	/* CONF3 */
		0x03000300,	/* CONF4 */
		0x00050808,	/* CONF5 */
		0x00000000,	/* CONF6 */
	},
	/* GPMC address map as small as possible */
	.base = 0x28000000,
	.size = GPMC_SIZE_16M,
};

static int omap4_gpio_init(void)
{
	add_generic_device("omap-gpio", 0, NULL, OMAP44XX_GPIO1_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 1, NULL, OMAP44XX_GPIO2_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 2, NULL, OMAP44XX_GPIO3_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 3, NULL, OMAP44XX_GPIO4_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 4, NULL, OMAP44XX_GPIO5_BASE,
				0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 5, NULL, OMAP44XX_GPIO6_BASE,
				0xf00, IORESOURCE_MEM, NULL);

	return 0;
}

int omap4_devices_init(void)
{
	omap4_gpio_init();
	add_generic_device("omap-32ktimer", 0, NULL, OMAP44XX_32KTIMER_BASE, 0x400,
			   IORESOURCE_MEM, NULL);
	return 0;
}
