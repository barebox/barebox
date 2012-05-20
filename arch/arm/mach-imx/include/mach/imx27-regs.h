#ifndef _IMX27_REGS_H
#define _IMX27_REGS_H

#ifndef _IMX_REGS_H
#error "Please do not include directly"
#endif

#define IMX_IO_BASE		0x10000000

#define IMX_AIPI1_BASE             (0x00000 + IMX_IO_BASE)
#define IMX_WDT_BASE               (0x02000 + IMX_IO_BASE)
#define IMX_TIM1_BASE              (0x03000 + IMX_IO_BASE)
#define IMX_TIM2_BASE              (0x04000 + IMX_IO_BASE)
#define IMX_TIM3_BASE              (0x05000 + IMX_IO_BASE)
#define IMX_UART1_BASE             (0x0a000 + IMX_IO_BASE)
#define IMX_UART2_BASE             (0x0b000 + IMX_IO_BASE)
#define IMX_UART3_BASE             (0x0c000 + IMX_IO_BASE)
#define IMX_UART4_BASE             (0x0d000 + IMX_IO_BASE)
#define IMX_SPI1_BASE              (0x0e000 + IMX_IO_BASE)
#define IMX_SPI2_BASE              (0x0f000 + IMX_IO_BASE)
#define IMX_I2C1_BASE              (0x12000 + IMX_IO_BASE)
#define IMX_SDHC1_BASE             (0x13000 + IMX_IO_BASE)
#define IMX_SDHC2_BASE             (0x14000 + IMX_IO_BASE)
#define IMX_GPIO_BASE              (0x15000 + IMX_IO_BASE)
#define IMX_TIM4_BASE              (0x19000 + IMX_IO_BASE)
#define IMX_TIM5_BASE              (0x1a000 + IMX_IO_BASE)
#define IMX_UART5_BASE             (0x1b000 + IMX_IO_BASE)
#define IMX_UART6_BASE             (0x1c000 + IMX_IO_BASE)
#define IMX_I2C2_BASE              (0x1d000 + IMX_IO_BASE)
#define IMX_SDHC3_BASE             (0x1e000 + IMX_IO_BASE)
#define IMX_TIM6_BASE              (0x1f000 + IMX_IO_BASE)
#define IMX_AIPI2_BASE             (0x20000 + IMX_IO_BASE)
#define IMX_FB_BASE                (0x21000 + IMX_IO_BASE)
#define IMX_PLL_BASE               (0x27000 + IMX_IO_BASE)
#define IMX_SYSTEM_CTL_BASE        (0x27800 + IMX_IO_BASE)
#define IMX_IIM_BASE               (0x28000 + IMX_IO_BASE)
#define IMX_OTG_BASE               (0x24000 + IMX_IO_BASE)
#define IMX_FEC_BASE               (0x2b000 + IMX_IO_BASE)
#define IMX_MAX_BASE               (0x3f000 + IMX_IO_BASE)

#define IMX_NFC_BASE               (0xd8000000)
#define IMX_ESD_BASE               (0xd8001000)
#define IMX_WEIM_BASE              (0xd8002000)
#define IMX_M3IF_BASE		(0xd8003000)
#define IMX_PCMCIA_CTL_BASE	(0xd8004000)

#define PCMCIA_PIPR		(IMX_PCMCIA_CTL_BASE + 0x00)
#define PCMCIA_PSCR		(IMX_PCMCIA_CTL_BASE + 0x04)
#define PCMCIA_PER		(IMX_PCMCIA_CTL_BASE + 0x08)
#define PCMCIA_PBR(x)		(IMX_PCMCIA_CTL_BASE + 0x0c + ((x) << 2))
#define PCMCIA_POR(x)		(IMX_PCMCIA_CTL_BASE + 0x28 + ((x) << 2))
#define PCMCIA_POFR(x)		(IMX_PCMCIA_CTL_BASE + 0x44 + ((x) << 2))
#define PCMCIA_PGCR		(IMX_PCMCIA_CTL_BASE + 0x60)
#define PCMCIA_PGSR		(IMX_PCMCIA_CTL_BASE + 0x64)

/* AIPI */
#define AIPI1_PSR0	__REG(IMX_AIPI1_BASE + 0x00)
#define AIPI1_PSR1	__REG(IMX_AIPI1_BASE + 0x04)
#define AIPI2_PSR0	__REG(IMX_AIPI2_BASE + 0x00)
#define AIPI2_PSR1	__REG(IMX_AIPI2_BASE + 0x04)

/* System Control */
#define CID     __REG(IMX_SYSTEM_CTL_BASE + 0x0)		/* Chip ID Register */
#define FMCR    __REG(IMX_SYSTEM_CTL_BASE + 0x14)		/* Function Multeplexing Control Register */
#define GPCR	__REG(IMX_SYSTEM_CTL_BASE + 0x18)		/* Global Peripheral Control Register */
#define WBCR	__REG(IMX_SYSTEM_CTL_BASE + 0x1C)		/* Well Bias Control Register */
#define DSCR(x)	__REG(IMX_SYSTEM_CTL_BASE + 0x1C + ((x) << 2))	/* Driving Strength Control Register 1 - 13 */

#define GPCR_BOOT_SHIFT			16
#define GPCR_BOOT_MASK			(0xf << GPCR_BOOT_SHIFT)
#define GPCR_BOOT_UART_USB		0
#define GPCR_BOOT_8BIT_NAND_2k		2
#define GPCR_BOOT_16BIT_NAND_2k		3
#define GPCR_BOOT_16BIT_NAND_512	4
#define GPCR_BOOT_16BIT_CS0		5
#define GPCR_BOOT_32BIT_CS0		6
#define GPCR_BOOT_8BIT_NAND_512		7

/* Chip Select Registers */
#define CSxU(x) __REG(IMX_WEIM_BASE + (cs * 0x10) + 0x00) /* Chip Select x Upper Register    */
#define CSxL(x) __REG(IMX_WEIM_BASE + (cs * 0x10) + 0x04) /* Chip Select x Lower Register    */
#define CSxA(x) __REG(IMX_WEIM_BASE + (cs * 0x10) + 0x08) /* Chip Select x Addition Register */
#define EIM  __REG(IMX_WEIM_BASE + 0x60) /* WEIM Configuration Register     */

#include "esdctl.h"

/* PLL registers */
#define CSCR		__REG(IMX_PLL_BASE + 0x00) /* Clock Source Control Register       */
#define MPCTL0		__REG(IMX_PLL_BASE + 0x04) /* MCU PLL Control Register 0          */
#define MPCTL1		__REG(IMX_PLL_BASE + 0x08) /* MCU PLL Control Register 1          */
#define SPCTL0		__REG(IMX_PLL_BASE + 0x0c) /* System PLL Control Register 0       */
#define SPCTL1		__REG(IMX_PLL_BASE + 0x10) /* System PLL Control Register 1       */
#define OSC26MCTL	__REG(IMX_PLL_BASE + 0x14) /* Oscillator 26M Register             */
#define PCDR0		__REG(IMX_PLL_BASE + 0x18) /* Peripheral Clock Divider Register 0 */
#define PCDR1		__REG(IMX_PLL_BASE + 0x1c) /* Peripheral Clock Divider Register 1 */
#define PCCR0		__REG(IMX_PLL_BASE + 0x20) /* Peripheral Clock Control Register 0 */
#define PCCR1		__REG(IMX_PLL_BASE + 0x24) /* Peripheral Clock Control Register 1 */
#define CCSR		__REG(IMX_PLL_BASE + 0x28) /* Clock Control Status Register       */

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

/*
 * Definitions for the clocksource driver
 */
/* Part 1: Registers */
# define GPT_TCTL   0x00
# define GPT_TPRER  0x04
# define GPT_TCMP   0x08
# define GPT_TCR    0x0c
# define GPT_TCN    0x10
# define GPT_TSTAT  0x14

/* Part 2: Bitfields */
#define TCTL_SWR       (1<<15) /* Software reset */
#define TCTL_FRR       (1<<8)  /* Freerun / restart */
#define TCTL_CAP       (3<<6)  /* Capture Edge */
#define TCTL_OM        (1<<5)  /* output mode */
#define TCTL_IRQEN     (1<<4)  /* interrupt enable */
#define TCTL_CLKSOURCE (1)     /* Clock source bit position */
#define TCTL_TEN       (1)     /* Timer enable */
#define TPRER_PRES     (0xff)  /* Prescale */
#define TSTAT_CAPT     (1<<1)  /* Capture event */
#define TSTAT_COMP     (1)     /* Compare event */

#define IMX_CS0_BASE	0xC0000000
#define IMX_CS1_BASE	0xC8000000
#define IMX_CS2_BASE	0xD0000000
#define IMX_CS3_BASE	0xD2000000
#define IMX_CS4_BASE	0xD4000000
#define IMX_CS5_BASE	0xD6000000

#define IMX_PCMCIA_MEM_BASE	(0xdc000000)

#ifndef __ASSEMBLY__
static inline void imx27_setup_weimcs(size_t cs, unsigned upper, unsigned lower, unsigned addional)
{
	CSxU(cs) = upper;
	CSxL(cs) = lower;
	CSxA(cs) = addional;
}
#endif /* __ASSEMBLY__ */

#endif /* _IMX27_REGS_H */
