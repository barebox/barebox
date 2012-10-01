#ifndef _IMX27_REGS_H
#define _IMX27_REGS_H

#define MX27_AIPI_BASE_ADDR		0x10000000
#define MX27_AIPI_SIZE			SZ_1M
#define MX27_DMA_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x01000)
#define MX27_WDOG_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x02000)
#define MX27_GPT1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x03000)
#define MX27_GPT2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x04000)
#define MX27_GPT3_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x05000)
#define MX27_PWM_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x06000)
#define MX27_RTC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x07000)
#define MX27_KPP_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x08000)
#define MX27_OWIRE_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x09000)
#define MX27_UART1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0a000)
#define MX27_UART2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0b000)
#define MX27_UART3_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0c000)
#define MX27_UART4_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0d000)
#define MX27_CSPI1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0e000)
#define MX27_CSPI2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x0f000)
#define MX27_SSI1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x10000)
#define MX27_SSI2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x11000)
#define MX27_I2C1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x12000)
#define MX27_SDHC1_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x13000)
#define MX27_SDHC2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x14000)
#define MX27_GPIO_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x15000)
#define MX27_GPIO1_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x000)
#define MX27_GPIO2_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x100)
#define MX27_GPIO3_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x200)
#define MX27_GPIO4_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x300)
#define MX27_GPIO5_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x400)
#define MX27_GPIO6_BASE_ADDR			(MX27_GPIO_BASE_ADDR + 0x500)
#define MX27_AUDMUX_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x16000)
#define MX27_CSPI3_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x17000)
#define MX27_MSHC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x18000)
#define MX27_GPT4_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x19000)
#define MX27_GPT5_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1a000)
#define MX27_UART5_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1b000)
#define MX27_UART6_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1c000)
#define MX27_I2C2_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1d000)
#define MX27_SDHC3_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1e000)
#define MX27_GPT6_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x1f000)
#define MX27_LCDC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x21000)
#define MX27_SLCDC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x22000)
#define MX27_VPU_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x23000)
#define MX27_USB_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x24000)
#define MX27_USB_OTG_BASE_ADDR			(MX27_USB_BASE_ADDR + 0x0000)
#define MX27_USB_HS1_BASE_ADDR			(MX27_USB_BASE_ADDR + 0x0200)
#define MX27_USB_HS2_BASE_ADDR			(MX27_USB_BASE_ADDR + 0x0400)
#define MX27_SAHARA_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x25000)
#define MX27_EMMAPP_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x26000)
#define MX27_EMMAPRP_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x26400)
#define MX27_CCM_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x27000)
#define MX27_SYSCTRL_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x27800)
#define MX27_IIM_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x28000)
#define MX27_RTIC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x2a000)
#define MX27_FEC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x2b000)
#define MX27_SCC_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x2c000)
#define MX27_ETB_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x3b000)
#define MX27_ETB_RAM_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x3c000)
#define MX27_JAM_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x3e000)
#define MX27_MAX_BASE_ADDR			(MX27_AIPI_BASE_ADDR + 0x3f000)

#define MX27_AVIC_BASE_ADDR		0x10040000

/* ROM patch */
#define MX27_ROMP_BASE_ADDR		0x10041000

#define MX27_SAHB1_BASE_ADDR		0x80000000
#define MX27_SAHB1_SIZE			SZ_1M
#define MX27_CSI_BASE_ADDR			(MX27_SAHB1_BASE_ADDR + 0x0000)
#define MX27_ATA_BASE_ADDR			(MX27_SAHB1_BASE_ADDR + 0x1000)

/* Memory regions and CS */
#define MX27_CSD0_BASE_ADDR		0xa0000000
#define MX27_CSD1_BASE_ADDR		0xb0000000

#define MX27_CS0_BASE_ADDR		0xc0000000
#define MX27_CS1_BASE_ADDR		0xc8000000
#define MX27_CS2_BASE_ADDR		0xd0000000
#define MX27_CS3_BASE_ADDR		0xd2000000
#define MX27_CS4_BASE_ADDR		0xd4000000
#define MX27_CS5_BASE_ADDR		0xd6000000

/* NAND, SDRAM, WEIM, M3IF, EMI controllers */
#define MX27_X_MEMC_BASE_ADDR		0xd8000000
#define MX27_X_MEMC_SIZE		SZ_1M
#define MX27_NFC_BASE_ADDR		(MX27_X_MEMC_BASE_ADDR)
#define MX27_SDRAMC_BASE_ADDR		(MX27_X_MEMC_BASE_ADDR + 0x1000)
#define MX27_WEIM_BASE_ADDR		(MX27_X_MEMC_BASE_ADDR + 0x2000)
#define MX27_M3IF_BASE_ADDR		(MX27_X_MEMC_BASE_ADDR + 0x3000)
#define MX27_PCMCIA_CTL_BASE_ADDR	(MX27_X_MEMC_BASE_ADDR + 0x4000)

#define MX27_WEIM_CSCRx_BASE_ADDR(cs)	(MX27_WEIM_BASE_ADDR + (cs) * 0x10)
#define MX27_WEIM_CSCRxU(cs)		(MX27_WEIM_CSCRx_BASE_ADDR(cs))
#define MX27_WEIM_CSCRxL(cs)		(MX27_WEIM_CSCRx_BASE_ADDR(cs) + 0x4)
#define MX27_WEIM_CSCRxA(cs)		(MX27_WEIM_CSCRx_BASE_ADDR(cs) + 0x8)

#define MX27_PCMCIA_MEM_BASE_ADDR	0xdc000000

/* IRAM */
#define MX27_IRAM_BASE_ADDR		0xffff4c00	/* internal ram */

/* FIXME: get rid of these */
#define IMX_GPIO_BASE		MX27_GPIO_BASE_ADDR
#define IMX_NFC_BASE		MX27_NFC_BASE_ADDR
#define IMX_WDT_BASE		MX27_WDOG_BASE_ADDR
#define IMX_ESD_BASE		MX27_SDRAMC_BASE_ADDR

#define PCMCIA_PIPR		(MX27_PCMCIA_CTL_BASE_ADDR + 0x00)
#define PCMCIA_PSCR		(MX27_PCMCIA_CTL_BASE_ADDR + 0x04)
#define PCMCIA_PER		(MX27_PCMCIA_CTL_BASE_ADDR + 0x08)
#define PCMCIA_PBR(x)		(MX27_PCMCIA_CTL_BASE_ADDR + 0x0c + ((x) << 2))
#define PCMCIA_POR(x)		(MX27_PCMCIA_CTL_BASE_ADDR + 0x28 + ((x) << 2))
#define PCMCIA_POFR(x)		(MX27_PCMCIA_CTL_BASE_ADDR + 0x44 + ((x) << 2))
#define PCMCIA_PGCR		(MX27_PCMCIA_CTL_BASE_ADDR + 0x60)
#define PCMCIA_PGSR		(MX27_PCMCIA_CTL_BASE_ADDR + 0x64)

/* AIPI */
#define AIPI1_PSR0	__REG(MX27_AIPI_BASE_ADDR + 0x00)
#define AIPI1_PSR1	__REG(MX27_AIPI_BASE_ADDR + 0x04)
#define AIPI2_PSR0	__REG(MX27_AIPI_BASE_ADDR + 0x20000 + 0x00)
#define AIPI2_PSR1	__REG(MX27_AIPI_BASE_ADDR + 0x20000 + 0x04)

/* System Control */
#define CID     __REG(MX27_SYSCTRL_BASE_ADDR + 0x0)		/* Chip ID Register */
#define FMCR    __REG(MX27_SYSCTRL_BASE_ADDR + 0x14)		/* Function Multeplexing Control Register */
#define GPCR	__REG(MX27_SYSCTRL_BASE_ADDR + 0x18)		/* Global Peripheral Control Register */
#define WBCR	__REG(MX27_SYSCTRL_BASE_ADDR + 0x1C)		/* Well Bias Control Register */
#define DSCR(x)	__REG(MX27_SYSCTRL_BASE_ADDR + 0x1C + ((x) << 2))	/* Driving Strength Control Register 1 - 13 */

#include "esdctl.h"

/* PLL registers */
#define CSCR		__REG(MX27_CCM_BASE_ADDR + 0x00) /* Clock Source Control Register       */
#define MPCTL0		__REG(MX27_CCM_BASE_ADDR + 0x04) /* MCU PLL Control Register 0          */
#define MPCTL1		__REG(MX27_CCM_BASE_ADDR + 0x08) /* MCU PLL Control Register 1          */
#define SPCTL0		__REG(MX27_CCM_BASE_ADDR + 0x0c) /* System PLL Control Register 0       */
#define SPCTL1		__REG(MX27_CCM_BASE_ADDR + 0x10) /* System PLL Control Register 1       */
#define OSC26MCTL	__REG(MX27_CCM_BASE_ADDR + 0x14) /* Oscillator 26M Register             */
#define PCDR0		__REG(MX27_CCM_BASE_ADDR + 0x18) /* Peripheral Clock Divider Register 0 */
#define PCDR1		__REG(MX27_CCM_BASE_ADDR + 0x1c) /* Peripheral Clock Divider Register 1 */
#define PCCR0		__REG(MX27_CCM_BASE_ADDR + 0x20) /* Peripheral Clock Control Register 0 */
#define PCCR1		__REG(MX27_CCM_BASE_ADDR + 0x24) /* Peripheral Clock Control Register 1 */
#define CCSR		__REG(MX27_CCM_BASE_ADDR + 0x28) /* Clock Control Status Register       */

#define CSCR_MPEN		(1 << 0)
#define CSCR_SPEN		(1 << 1)
#define CSCR_FPM_EN		(1 << 2)
#define CSCR_OSC26M_DIS		(1 << 3)
#define CSCR_OSC26M_DIV1P5	(1 << 4)
#define CSCR_AHB_DIV(d)		(((d) & 0x3) << 8)
#define CSCR_ARM_DIV(d)		(((d) & 0x3) << 12)
#define CSCR_ARM_SRC_MPLL	(1 << 15)
#define CSCR_MCU_SEL		(1 << 16)
#define CSCR_SP_SEL		(1 << 17)
#define CSCR_MPLL_RESTART	(1 << 18)
#define CSCR_SPLL_RESTART	(1 << 19)
#define CSCR_MSHC_SEL		(1 << 20)
#define CSCR_H264_SEL		(1 << 21)
#define CSCR_SSI1_SEL		(1 << 22)
#define CSCR_SSI2_SEL		(1 << 23)
#define CSCR_SD_CNT(d)		(((d) & 0x3) << 24)
#define CSCR_USB_DIV(d)		(((d) & 0x7) << 28)
#define CSCR_UPDATE_DIS		(1 << 31)

#define MPCTL1_BRMO		(1 << 6)
#define MPCTL1_LF		(1 << 15)

#define PCCR0_SSI2_EN	(1 << 0)
#define PCCR0_SSI1_EN	(1 << 1)
#define PCCR0_SLCDC_EN	(1 << 2)
#define PCCR0_SDHC3_EN	(1 << 3)
#define PCCR0_SDHC2_EN	(1 << 4)
#define PCCR0_SDHC1_EN	(1 << 5)
#define PCCR0_SDC_EN	(1 << 6)
#define PCCR0_SAHARA_EN	(1 << 7)
#define PCCR0_RTIC_EN	(1 << 8)
#define PCCR0_RTC_EN	(1 << 9)
#define PCCR0_PWM_EN	(1 << 11)
#define PCCR0_OWIRE_EN	(1 << 12)
#define PCCR0_MSHC_EN	(1 << 13)
#define PCCR0_LCDC_EN	(1 << 14)
#define PCCR0_KPP_EN	(1 << 15)
#define PCCR0_IIM_EN	(1 << 16)
#define PCCR0_I2C2_EN	(1 << 17)
#define PCCR0_I2C1_EN	(1 << 18)
#define PCCR0_GPT6_EN	(1 << 19)
#define PCCR0_GPT5_EN	(1 << 20)
#define PCCR0_GPT4_EN	(1 << 21)
#define PCCR0_GPT3_EN	(1 << 22)
#define PCCR0_GPT2_EN	(1 << 23)
#define PCCR0_GPT1_EN	(1 << 24)
#define PCCR0_GPIO_EN	(1 << 25)
#define PCCR0_FEC_EN	(1 << 26)
#define PCCR0_EMMA_EN	(1 << 27)
#define PCCR0_DMA_EN	(1 << 28)
#define PCCR0_CSPI3_EN	(1 << 29)
#define PCCR0_CSPI2_EN	(1 << 30)
#define PCCR0_CSPI1_EN	(1 << 31)

#define PCCR1_MSHC_BAUDEN	(1 << 2)
#define PCCR1_NFC_BAUDEN	(1 << 3)
#define PCCR1_SSI2_BAUDEN	(1 << 4)
#define PCCR1_SSI1_BAUDEN	(1 << 5)
#define PCCR1_H264_BAUDEN	(1 << 6)
#define PCCR1_PERCLK4_EN	(1 << 7)
#define PCCR1_PERCLK3_EN	(1 << 8)
#define PCCR1_PERCLK2_EN	(1 << 9)
#define PCCR1_PERCLK1_EN	(1 << 10)
#define PCCR1_HCLK_USB		(1 << 11)
#define PCCR1_HCLK_SLCDC	(1 << 12)
#define PCCR1_HCLK_SAHARA	(1 << 13)
#define PCCR1_HCLK_RTIC		(1 << 14)
#define PCCR1_HCLK_LCDC		(1 << 15)
#define PCCR1_HCLK_H264		(1 << 16)
#define PCCR1_HCLK_FEC		(1 << 17)
#define PCCR1_HCLK_EMMA		(1 << 18)
#define PCCR1_HCLK_EMI		(1 << 19)
#define PCCR1_HCLK_DMA		(1 << 20)
#define PCCR1_HCLK_CSI		(1 << 21)
#define PCCR1_HCLK_BROM		(1 << 22)
#define PCCR1_HCLK_ATA		(1 << 23)
#define PCCR1_WDT_EN		(1 << 24)
#define PCCR1_USB_EN		(1 << 25)
#define PCCR1_UART6_EN		(1 << 26)
#define PCCR1_UART5_EN		(1 << 27)
#define PCCR1_UART4_EN		(1 << 28)
#define PCCR1_UART3_EN		(1 << 29)
#define PCCR1_UART2_EN		(1 << 30)
#define PCCR1_UART1_EN		(1 << 31)

#define CCSR_32K_SR		(1 << 15)

/* SDRAM Controller registers bitfields */
#define ESDCTL_PRCT(x)		(((x) & 3f) << 0)
#define ESDCTL_BL		(1 << 7)
#define ESDCTL_FP		(1 << 8)
#define ESDCTL_PWDT(x)		(((x) & 3) << 10)
#define ESDCTL_SREFR(x)		(((x) & 7) << 13)
#define ESDCTL_DSIZ_16_UPPER	(0 << 16)
#define ESDCTL_DSIZ_16_LOWER	(0 << 16)
#define ESDCTL_DSIZ_32		(0 << 16)
#define ESDCTL_COL8		(0 << 20)
#define ESDCTL_COL9		(1 << 20)
#define ESDCTL_COL10		(2 << 20)
#define ESDCTL_ROW11		(0 << 24)
#define ESDCTL_ROW12		(1 << 24)
#define ESDCTL_ROW13		(2 << 24)
#define ESDCTL_ROW14		(3 << 24)
#define ESDCTL_ROW15		(4 << 24)
#define ESDCTL_SP		(1 << 27)
#define ESDCTL_SMODE_NORMAL	(0 << 28)
#define ESDCTL_SMODE_PRECHAGRE	(1 << 28)
#define ESDCTL_SMODE_AUTO_REF	(2 << 28)
#define ESDCTL_SMODE_LOAD_MODE	(3 << 28)
#define ESDCTL_SMODE_MAN_REF	(4 << 28)
#define ESDCTL_SDE		(1 << 31)

#define ESDCFG_TRC(x)		(((x) & 0xf) << 0)
#define ESDCFG_TRCD(x)		(((x) & 0x7) << 4)
#define ESDCFG_TCAS(x)		(((x) & 0x3) << 8)
#define ESDCFG_TRRD(x)		(((x) & 0x3) << 10)
#define ESDCFG_TRAS(x)		(((x) & 0x7) << 12)
#define ESDCFG_TWR		(1 << 15)
#define ESDCFG_TMRD(x)		(((x) & 0x3) << 16)
#define ESDCFG_TRP(x)		(((x) & 0x3) << 18)
#define ESDCFG_TWTR		(1 << 20)
#define ESDCFG_TXP(x)		(((x) & 0x3) << 21)

#endif /* _IMX27_REGS_H */
