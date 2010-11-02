#ifndef _IMX21_REGS_H
#define _IMX21_REGS_H

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
#define IMX_GPIO_BASE              (0x15000 + IMX_IO_BASE)
#define IMX_AIPI2_BASE             (0x20000 + IMX_IO_BASE)
#define IMX_PLL_BASE               (0x27000 + IMX_IO_BASE)
#define IMX_SYSTEM_CTL_BASE        (0x27800 + IMX_IO_BASE)

#define IMX_SDRAM_BASE             (0xdf000000)
#define IMX_EIM_BASE               (0xdf001000)
#define IMX_NFC_BASE               (0xdf003000)

/* AIPI */
#define AIPI1_PSR0	__REG(IMX_AIPI1_BASE + 0x00)
#define AIPI1_PSR1	__REG(IMX_AIPI1_BASE + 0x04)
#define AIPI2_PSR0	__REG(IMX_AIPI2_BASE + 0x00)
#define AIPI2_PSR1	__REG(IMX_AIPI2_BASE + 0x04)

/* System Control */
#define SUID0    __REG(IMX_SYSTEM_CTL_BASE + 0x4)		/* Silicon ID Register (12 bytes) */
#define SUID1    __REG(IMX_SYSTEM_CTL_BASE + 0x8)		/* Silicon ID Register (12 bytes) */
#define CID      __REG(IMX_SYSTEM_CTL_BASE + 0xC)		/* Silicon ID Register (12 bytes) */
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

/* SDRAM Controller registers bitfields */
#define SDCTL0 __REG(IMX_SDRAM_BASE + 0x00) /* SDRAM 0 Control Register */
#define SDCTL1 __REG(IMX_SDRAM_BASE + 0x04) /* SDRAM 0 Control Register */
#define SDRST  __REG(IMX_SDRAM_BASE + 0x18) /* SDRAM Reset Register */
#define SDMISC __REG(IMX_SDRAM_BASE + 0x14) /* SDRAM Miscellaneous Register */


/* Chip Select Registers */
#define CS0U __REG(IMX_EIM_BASE + 0x00) /* Chip Select 0 Upper Register    */
#define CS0L __REG(IMX_EIM_BASE + 0x04) /* Chip Select 0 Lower Register    */
#define CS1U __REG(IMX_EIM_BASE + 0x08) /* Chip Select 1 Upper Register    */
#define CS1L __REG(IMX_EIM_BASE + 0x0C) /* Chip Select 1 Lower Register    */
#define CS2U __REG(IMX_EIM_BASE + 0x10) /* Chip Select 2 Upper Register    */
#define CS2L __REG(IMX_EIM_BASE + 0x14) /* Chip Select 2 Lower Register    */
#define CS3U __REG(IMX_EIM_BASE + 0x18) /* Chip Select 3 Upper Register    */
#define CS3L __REG(IMX_EIM_BASE + 0x1C) /* Chip Select 3 Lower Register    */
#define CS4U __REG(IMX_EIM_BASE + 0x20) /* Chip Select 4 Upper Register    */
#define CS4L __REG(IMX_EIM_BASE + 0x24) /* Chip Select 4 Lower Register    */
#define CS5U __REG(IMX_EIM_BASE + 0x28) /* Chip Select 5 Upper Register    */
#define CS5L __REG(IMX_EIM_BASE + 0x2C) /* Chip Select 5 Lower Register    */
#define EIM  __REG(IMX_EIM_BASE + 0x30) /* EIM Configuration Register      */

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
#define CSCR_MCU_SEL		(1 << 16)
#define CSCR_SP_SEL		(1 << 17)
#define CSCR_SD_CNT(d)		(((d) & 0x3) << 24)
#define CSCR_USB_DIV(d)		(((d) & 0x7) << 26)
#define CSCR_PRESC(d)		(((d) & 0x7) << 29)

#define MPCTL1_BRMO		(1 << 6)
#define MPCTL1_LF		(1 << 15)

#define PCCR0_PERCLK3_EN	(1 << 18)
#define PCCR0_NFC_EN		(1 << 19)
#define PCCR0_HCLK_LCDC_EN	(1 << 26)

#define PCCR1_GPT1_EN		(1 << 25)

#define CCSR_32K_SR		(1 << 15)

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
#define TCTL_CC        (1<<10) /* counter clear */
#define TCTL_FRR       (1<<8)  /* Freerun / restart */
#define TCTL_CAP       (3<<6)  /* Capture Edge */
#define TCTL_CAPEN     (1<<5)  /* compare interrupt enable */
#define TCTL_COMPEN    (1<<4)  /* compare interrupt enable */
#define TCTL_CLKSOURCE (1)     /* Clock source bit position */
#define TCTL_TEN       (1)     /* Timer enable */
#define TPRER_PRES     (0xff)  /* Prescale */
#define TSTAT_CAPT     (1<<1)  /* Capture event */
#define TSTAT_COMP     (1)     /* Compare event */

#define IMX_CS0_BASE	0xC8000000
#define IMX_CS1_BASE	0xCC000000
#define IMX_CS2_BASE	0xD0000000
#define IMX_CS3_BASE	0xD1000000
#define IMX_CS4_BASE	0xD2000000
#define IMX_CS5_BASE	0xD3000000

#endif /* _IMX21_REGS_H */
