#ifndef _IMX21_REGS_H
#define _IMX21_REGS_H

#ifndef _IMX_REGS_H
#error "Please do not include directly"
#endif

#define MX21_AIPI_BASE_ADDR		0x10000000
#define MX21_AIPI_SIZE			SZ_1M
#define MX21_DMA_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x01000)
#define MX21_WDOG_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x02000)
#define MX21_GPT1_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x03000)
#define MX21_GPT2_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x04000)
#define MX21_GPT3_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x05000)
#define MX21_PWM_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x06000)
#define MX21_RTC_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x07000)
#define MX21_KPP_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x08000)
#define MX21_OWIRE_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x09000)
#define MX21_UART1_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0a000)
#define MX21_UART2_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0b000)
#define MX21_UART3_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0c000)
#define MX21_UART4_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0d000)
#define MX21_CSPI1_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0e000)
#define MX21_CSPI2_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x0f000)
#define MX21_SSI1_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x10000)
#define MX21_SSI2_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x11000)
#define MX21_I2C_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x12000)
#define MX21_SDHC1_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x13000)
#define MX21_SDHC2_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x14000)
#define MX21_GPIO_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x15000)
#define MX21_GPIO1_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x000)
#define MX21_GPIO2_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x100)
#define MX21_GPIO3_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x200)
#define MX21_GPIO4_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x300)
#define MX21_GPIO5_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x400)
#define MX21_GPIO6_BASE_ADDR			(MX21_GPIO_BASE_ADDR + 0x500)
#define MX21_AUDMUX_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x16000)
#define MX21_CSPI3_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x17000)
#define MX21_LCDC_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x21000)
#define MX21_SLCDC_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x22000)
#define MX21_USBOTG_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x24000)
#define MX21_EMMA_PP_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x26000)
#define MX21_EMMA_PRP_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x26400)
#define MX21_CCM_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x27000)
#define MX21_SYSCTRL_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x27800)
#define MX21_JAM_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x3e000)
#define MX21_MAX_BASE_ADDR			(MX21_AIPI_BASE_ADDR + 0x3f000)

#define MX21_AVIC_BASE_ADDR		0x10040000

#define MX21_SAHB1_BASE_ADDR		0x80000000
#define MX21_SAHB1_SIZE			SZ_1M
#define MX21_CSI_BASE_ADDR			(MX2x_SAHB1_BASE_ADDR + 0x0000)

/* Memory regions and CS */
#define MX21_SDRAM_BASE_ADDR		0xc0000000
#define MX21_CSD1_BASE_ADDR		0xc4000000

#define MX21_CS0_BASE_ADDR		0xc8000000
#define MX21_CS1_BASE_ADDR		0xcc000000
#define MX21_CS2_BASE_ADDR		0xd0000000
#define MX21_CS3_BASE_ADDR		0xd1000000
#define MX21_CS4_BASE_ADDR		0xd2000000
#define MX21_PCMCIA_MEM_BASE_ADDR	0xd4000000
#define MX21_CS5_BASE_ADDR		0xdd000000

/* NAND, SDRAM, WEIM etc controllers */
#define MX21_X_MEMC_BASE_ADDR		0xdf000000
#define MX21_X_MEMC_SIZE		SZ_256K

#define MX21_SDRAMC_BASE_ADDR		(MX21_X_MEMC_BASE_ADDR + 0x0000)
#define MX21_EIM_BASE_ADDR		(MX21_X_MEMC_BASE_ADDR + 0x1000)
#define MX21_PCMCIA_CTL_BASE_ADDR	(MX21_X_MEMC_BASE_ADDR + 0x2000)
#define MX21_NFC_BASE_ADDR		(MX21_X_MEMC_BASE_ADDR + 0x3000)

#define MX21_IRAM_BASE_ADDR		0xffffe800	/* internal ram */

/* FIXME: Get rid of these */
#define IMX_GPIO_BASE MX21_GPIO_BASE_ADDR
#define IMX_TIM1_BASE MX21_GPT1_BASE_ADDR
#define IMX_WDT_BASE MX21_WDOG_BASE_ADDR
#define IMX_SYSTEM_CTL_BASE MX21_SYSCTRL_BASE_ADDR

/* AIPI */
#define AIPI1_PSR0	__REG(MX21_AIPI_BASE_ADDR + 0x00)
#define AIPI1_PSR1	__REG(MX21_AIPI_BASE_ADDR + 0x04)
#define AIPI2_PSR0	__REG(MX21_AIPI_BASE_ADDR + 0x20000 + 0x00)
#define AIPI2_PSR1	__REG(MX21_AIPI_BASE_ADDR + 0x20000 + 0x04)

/* System Control */
#define SUID0    __REG(MX21_SYSCTRL_BASE_ADDR + 0x4)		/* Silicon ID Register (12 bytes) */
#define SUID1    __REG(MX21_SYSCTRL_BASE_ADDR + 0x8)		/* Silicon ID Register (12 bytes) */
#define CID      __REG(MX21_SYSCTRL_BASE_ADDR + 0xC)		/* Silicon ID Register (12 bytes) */
#define FMCR    __REG(MX21_SYSCTRL_BASE_ADDR + 0x14)		/* Function Multeplexing Control Register */
#define GPCR	__REG(MX21_SYSCTRL_BASE_ADDR + 0x18)		/* Global Peripheral Control Register */
#define WBCR	__REG(MX21_SYSCTRL_BASE_ADDR + 0x1C)		/* Well Bias Control Register */
#define DSCR(x)	__REG(MX21_SYSCTRL_BASE_ADDR + 0x1C + ((x) << 2))	/* Driving Strength Control Register 1 - 13 */

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
#define SDCTL0 __REG(MX21_X_MEMC_BASE_ADDR + 0x00) /* SDRAM 0 Control Register */
#define SDCTL1 __REG(MX21_X_MEMC_BASE_ADDR + 0x04) /* SDRAM 0 Control Register */
#define SDRST  __REG(MX21_X_MEMC_BASE_ADDR + 0x18) /* SDRAM Reset Register */
#define SDMISC __REG(MX21_X_MEMC_BASE_ADDR + 0x14) /* SDRAM Miscellaneous Register */


/* Chip Select Registers */
#define CS0U __REG(MX21_EIM_BASE_ADDR + 0x00) /* Chip Select 0 Upper Register    */
#define CS0L __REG(MX21_EIM_BASE_ADDR + 0x04) /* Chip Select 0 Lower Register    */
#define CS1U __REG(MX21_EIM_BASE_ADDR + 0x08) /* Chip Select 1 Upper Register    */
#define CS1L __REG(MX21_EIM_BASE_ADDR + 0x0C) /* Chip Select 1 Lower Register    */
#define CS2U __REG(MX21_EIM_BASE_ADDR + 0x10) /* Chip Select 2 Upper Register    */
#define CS2L __REG(MX21_EIM_BASE_ADDR + 0x14) /* Chip Select 2 Lower Register    */
#define CS3U __REG(MX21_EIM_BASE_ADDR + 0x18) /* Chip Select 3 Upper Register    */
#define CS3L __REG(MX21_EIM_BASE_ADDR + 0x1C) /* Chip Select 3 Lower Register    */
#define CS4U __REG(MX21_EIM_BASE_ADDR + 0x20) /* Chip Select 4 Upper Register    */
#define CS4L __REG(MX21_EIM_BASE_ADDR + 0x24) /* Chip Select 4 Lower Register    */
#define CS5U __REG(MX21_EIM_BASE_ADDR + 0x28) /* Chip Select 5 Upper Register    */
#define CS5L __REG(MX21_EIM_BASE_ADDR + 0x2C) /* Chip Select 5 Lower Register    */
#define EIM  __REG(MX21_EIM_BASE_ADDR + 0x30) /* EIM Configuration Register      */

/* PLL registers */
#define CSCR		__REG(MX21_CCM_BASE_ADDR + 0x00) /* Clock Source Control Register       */
#define MPCTL0		__REG(MX21_CCM_BASE_ADDR + 0x04) /* MCU PLL Control Register 0          */
#define MPCTL1		__REG(MX21_CCM_BASE_ADDR + 0x08) /* MCU PLL Control Register 1          */
#define SPCTL0		__REG(MX21_CCM_BASE_ADDR + 0x0c) /* System PLL Control Register 0       */
#define SPCTL1		__REG(MX21_CCM_BASE_ADDR + 0x10) /* System PLL Control Register 1       */
#define OSC26MCTL	__REG(MX21_CCM_BASE_ADDR + 0x14) /* Oscillator 26M Register             */
#define PCDR0		__REG(MX21_CCM_BASE_ADDR + 0x18) /* Peripheral Clock Divider Register 0 */
#define PCDR1		__REG(MX21_CCM_BASE_ADDR + 0x1c) /* Peripheral Clock Divider Register 1 */
#define PCCR0		__REG(MX21_CCM_BASE_ADDR + 0x20) /* Peripheral Clock Control Register 0 */
#define PCCR1		__REG(MX21_CCM_BASE_ADDR + 0x24) /* Peripheral Clock Control Register 1 */
#define CCSR		__REG(MX21_CCM_BASE_ADDR + 0x28) /* Clock Control Status Register       */

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

#endif /* _IMX21_REGS_H */
