#ifndef _IMX21_REGS_H
#define _IMX21_REGS_H

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
#define MX21_IRAM_SIZE			0x00001800

/* AIPI (base MX21_AIPI_BASE_ADDR) */
#define MX21_AIPI1_PSR0	0x00
#define MX21_AIPI1_PSR1	0x04
#define MX21_AIPI2_PSR0	(0x20000 + 0x00)
#define MX21_AIPI2_PSR1	(0x20000 + 0x04)

/* System Control (base: MX21_SYSCTRL_BASE_ADDR) */
#define MX21_SUID0	0x4	/* Silicon ID Register (12 bytes) */
#define MX21_SUID1	0x8	/* Silicon ID Register (12 bytes) */
#define MX21_CID	0xC	/* Silicon ID Register (12 bytes) */
#define MX21_FMCR	0x14	/* Function Multeplexing Control Register */
#define MX21_GPCR	0x18	/* Global Peripheral Control Register */
#define MX21_WBCR	0x1C	/* Well Bias Control Register */
#define MX21_DSCR(x)	0x1C + ((x) << 2)	/* Driving Strength Control Register 1 - 13 */

#define MX21_GPCR_BOOT_SHIFT		16
#define MX21_GPCR_BOOT_MASK		(0xf << GPCR_BOOT_SHIFT)
#define MX21_GPCR_BOOT_UART_USB		0
#define MX21_GPCR_BOOT_8BIT_NAND_2k	2
#define MX21_GPCR_BOOT_16BIT_NAND_2k	3
#define MX21_GPCR_BOOT_16BIT_NAND_512	4
#define MX21_GPCR_BOOT_16BIT_CS0	5
#define MX21_GPCR_BOOT_32BIT_CS0	6
#define MX21_GPCR_BOOT_8BIT_NAND_512	7

/* SDRAM Controller registers bitfields (base: MX21_X_MEMC_BASE_ADDR) */
#define MX21_SDCTL0 0x00 /* SDRAM 0 Control Register */
#define MX21_SDCTL1 0x04 /* SDRAM 0 Control Register */
#define MX21_SDRST  0x18 /* SDRAM Reset Register */
#define MX21_SDMISC 0x14 /* SDRAM Miscellaneous Register */

/* PLL registers (base: MX21_CCM_BASE_ADDR) */
#define MX21_CSCR		0x00 /* Clock Source Control Register       */
#define MX21_MPCTL0		0x04 /* MCU PLL Control Register 0          */
#define MX21_MPCTL1		0x08 /* MCU PLL Control Register 1          */
#define MX21_SPCTL0		0x0c /* System PLL Control Register 0       */
#define MX21_SPCTL1		0x10 /* System PLL Control Register 1       */
#define MX21_OSC26MCTL		0x14 /* Oscillator 26M Register             */
#define MX21_PCDR0		0x18 /* Peripheral Clock Divider Register 0 */
#define MX21_PCDR1		0x1c /* Peripheral Clock Divider Register 1 */
#define MX21_PCCR0		0x20 /* Peripheral Clock Control Register 0 */
#define MX21_PCCR1		0x24 /* Peripheral Clock Control Register 1 */
#define MX21_CCSR		0x28 /* Clock Control Status Register       */

#define MX21_CSCR_MPEN		(1 << 0)
#define MX21_CSCR_SPEN		(1 << 1)
#define MX21_CSCR_FPM_EN	(1 << 2)
#define MX21_CSCR_OSC26M_DIS	(1 << 3)
#define MX21_CSCR_OSC26M_DIV1P5	(1 << 4)
#define MX21_CSCR_MCU_SEL	(1 << 16)
#define MX21_CSCR_SP_SEL	(1 << 17)
#define MX21_CSCR_SD_CNT(d)	(((d) & 0x3) << 24)
#define MX21_CSCR_USB_DIV(d)	(((d) & 0x7) << 26)
#define MX21_CSCR_PRESC(d)	(((d) & 0x7) << 29)

#define MX21_MPCTL1_BRMO	(1 << 6)
#define MX21_MPCTL1_LF		(1 << 15)

#define MX21_CCSR_32K_SR	(1 << 15)

#endif /* _IMX21_REGS_H */
