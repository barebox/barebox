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
#define IMX_GPIO_BASE              (0x15000 + IMX_IO_BASE)
#define IMX_TIM4_BASE              (0x19000 + IMX_IO_BASE)
#define IMX_TIM5_BASE              (0x1a000 + IMX_IO_BASE)
#define IMX_UART5_BASE             (0x1b000 + IMX_IO_BASE)
#define IMX_UART6_BASE             (0x1c000 + IMX_IO_BASE)
#define IMX_TIM6_BASE              (0x1f000 + IMX_IO_BASE)
#define IMX_AIPI2_BASE             (0x20000 + IMX_IO_BASE)
#define IMX_PLL_BASE               (0x27000 + IMX_IO_BASE)
#define IMX_SYSTEM_CTL_BASE        (0x27800 + IMX_IO_BASE)
#define IMX_OTG_BASE               (0x24000 + IMX_IO_BASE)

#define IMX_NFC_BASE               (0xd8000000)
#define IMX_ESD_BASE               (0xd8001000)
#define IMX_WEIM_BASE              (0xd8002000)

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
#define CS0U __REG(IMX_WEIM_BASE + 0x00) /* Chip Select 0 Upper Register    */
#define CS0L __REG(IMX_WEIM_BASE + 0x04) /* Chip Select 0 Lower Register    */
#define CS0A __REG(IMX_WEIM_BASE + 0x08) /* Chip Select 0 Addition Register */
#define CS1U __REG(IMX_WEIM_BASE + 0x10) /* Chip Select 1 Upper Register    */
#define CS1L __REG(IMX_WEIM_BASE + 0x14) /* Chip Select 1 Lower Register    */
#define CS1A __REG(IMX_WEIM_BASE + 0x18) /* Chip Select 1 Addition Register */
#define CS2U __REG(IMX_WEIM_BASE + 0x20) /* Chip Select 2 Upper Register    */
#define CS2L __REG(IMX_WEIM_BASE + 0x24) /* Chip Select 2 Lower Register    */
#define CS2A __REG(IMX_WEIM_BASE + 0x28) /* Chip Select 2 Addition Register */
#define CS3U __REG(IMX_WEIM_BASE + 0x30) /* Chip Select 3 Upper Register    */
#define CS3L __REG(IMX_WEIM_BASE + 0x34) /* Chip Select 3 Lower Register    */
#define CS3A __REG(IMX_WEIM_BASE + 0x38) /* Chip Select 3 Addition Register */
#define CS4U __REG(IMX_WEIM_BASE + 0x40) /* Chip Select 4 Upper Register    */
#define CS4L __REG(IMX_WEIM_BASE + 0x44) /* Chip Select 4 Lower Register    */
#define CS4A __REG(IMX_WEIM_BASE + 0x48) /* Chip Select 4 Addition Register */
#define CS5U __REG(IMX_WEIM_BASE + 0x50) /* Chip Select 5 Upper Register    */
#define CS5L __REG(IMX_WEIM_BASE + 0x54) /* Chip Select 5 Lower Register    */
#define CS5A __REG(IMX_WEIM_BASE + 0x58) /* Chip Select 5 Addition Register */
#define EIM  __REG(IMX_WEIM_BASE + 0x60) /* WEIM Configuration Register     */

#include "esdctl.h"

/* Watchdog Registers*/
#define WCR  __REG(IMX_WDT_BASE + 0x00) /* Watchdog Control Register */
#define WSR  __REG(IMX_WDT_BASE + 0x04) /* Watchdog Service Register */
#define WSTR __REG(IMX_WDT_BASE + 0x08) /* Watchdog Status Register  */

/* important definition of some bits of WCR */
#define WCR_WDE 0x04

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

#define ESDMISC_RST		(1 << 1)
#define ESDMISC_MDDREN		(1 << 2)
#define ESDMISC_MDDR_DL_RST	(1 << 3)
#define ESDMISC_MDDR_MDIS	(1 << 4)
#define ESDMISC_LHD		(1 << 5)
#define ESDMISC_MA10_SHARE	(1 << 6)
#define ESDMISC_SDRAM_RDY	(1 << 6)

#define PA0_PF_USBH2_CLK        (GPIO_PORTA | GPIO_PF | 0)
#define PA1_PF_USBH2_DIR        (GPIO_PORTA | GPIO_PF | 1)
#define PA2_PF_USBH2_DATA7      (GPIO_PORTA | GPIO_PF | 2)
#define PA3_PF_USBH2_NXT        (GPIO_PORTA | GPIO_PF | 3)
#define PA4_PF_USBH2_STP        (GPIO_PORTA | GPIO_PF | 4)
#define PA5_PF_LSCLK            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 5)
#define PA6_PF_LD0              (GPIO_PORTA | GPIO_PF | GPIO_OUT | 6)
#define PA7_PF_LD1              (GPIO_PORTA | GPIO_PF | GPIO_OUT | 7)
#define PA8_PF_LD2              (GPIO_PORTA | GPIO_PF | GPIO_OUT | 8)
#define PA9_PF_LD3              (GPIO_PORTA | GPIO_PF | GPIO_OUT | 9)
#define PA10_PF_LD4             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 10)
#define PA11_PF_LD5             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 11)
#define PA12_PF_LD6             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 12)
#define PA13_PF_LD7             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 13)
#define PA14_PF_LD8             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 14)
#define PA15_PF_LD9             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 15)
#define PA16_PF_LD10            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 16)
#define PA17_PF_LD11            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 17)
#define PA18_PF_LD12            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 18)
#define PA19_PF_LD13            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 19)
#define PA20_PF_LD14            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 20)
#define PA21_PF_LD15            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 21)
#define PA22_PF_LD16            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 22)
#define PA23_PF_LD17            (GPIO_PORTA | GPIO_PF | GPIO_OUT | 23)
#define PA24_PF_REV             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 24)
#define PA25_PF_CLS             (GPIO_PORTA | GPIO_PF | GPIO_OUT | 25)
#define PA26_PF_PS              (GPIO_PORTA | GPIO_PF | GPIO_OUT | 26)
#define PA27_PF_SPL_SPR         (GPIO_PORTA | GPIO_PF | GPIO_OUT | 27)
#define PA28_PF_HSYNC           (GPIO_PORTA | GPIO_PF | GPIO_OUT | 28)
#define PA29_PF_VSYNC           (GPIO_PORTA | GPIO_PF | GPIO_OUT | 29)
#define PA30_PF_CONTRAST        (GPIO_PORTA | GPIO_PF | GPIO_OUT | 30)
#define PA31_PF_OE_ACD          (GPIO_PORTA | GPIO_PF | GPIO_OUT | 31)
#define PD0_AIN_FEC_TXD0	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 0)
#define PD1_AIN_FEC_TXD1	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 1)
#define PD2_AIN_FEC_TXD2	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 2)
#define PD3_AIN_FEC_TXD3	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 3)
#define PD4_AOUT_FEC_RX_ER	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 4)
#define PD5_AOUT_FEC_RXD1	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 5)
#define PD6_AOUT_FEC_RXD2	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 6)
#define PD7_AOUT_FEC_RXD3	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 7)
#define PD8_AF_FEC_MDIO		(GPIO_PORTD | GPIO_IN | GPIO_AF | 8)
#define PD9_AIN_FEC_MDC		(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 9)
#define PD10_AOUT_FEC_CRS	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 10)
#define PD11_AOUT_FEC_TX_CLK	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 11)
#define PD12_AOUT_FEC_RXD0	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 12)
#define PD13_AOUT_FEC_RX_DV	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 13)
#define PD14_AOUT_FEC_CLR	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 14)
#define PD15_AOUT_FEC_COL	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 15)
#define PD16_AIN_FEC_TX_ER	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 16)
#define PD17_PF_I2C_DATA	(GPIO_PORTD | GPIO_OUT | GPIO_PF | 17)
#define PD18_PF_I2C_CLK		(GPIO_PORTD | GPIO_OUT | GPIO_PF | 18)
#define PD19_AF_USBH2_DATA4     (GPIO_PORTD | GPIO_AF | 19)
#define PD20_AF_USBH2_DATA3     (GPIO_PORTD | GPIO_AF | 20)
#define PD21_AF_USBH2_DATA6     (GPIO_PORTD | GPIO_AF | 21)
#define PD22_AF_USBH2_DATA0     (GPIO_PORTD | GPIO_AF | 22)
#define PD23_AF_USBH2_DATA2     (GPIO_PORTD | GPIO_AF | 23)
#define PD24_AF_USBH2_DATA1     (GPIO_PORTD | GPIO_AF | 24)
#define PD25_PF_CSPI1_RDY	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 25)
#define PD26_PF_CSPI1_SS2	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 26)
#define PD26_AF_USBH2_DATA5     (GPIO_PORTD | GPIO_AF | 26)
#define PD27_PF_CSPI1_SS1	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 27)
#define PD28_PF_CSPI1_SS0	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 28)
#define PD29_PF_CSPI1_SCLK	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 29)
#define PD30_PF_CSPI1_MISO	(GPIO_PORTD | GPIO_IN | GPIO_PF  | 30)
#define PD31_PF_CSPI1_MOSI	(GPIO_PORTD | GPIO_OUT | GPIO_PF  | 31)
#define PF23_AIN_FEC_TX_EN	(GPIO_PORTF | GPIO_OUT | GPIO_AIN | 23)
#define PE3_PF_UART2_CTS	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 3)
#define PE4_PF_UART2_RTS	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 4)
#define PE6_PF_UART2_TXD	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 6)
#define PE7_PF_UART2_RXD	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 7)
#define PE8_PF_UART3_TXD	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 8)
#define PE9_PF_UART3_RXD	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 9)
#define PE10_PF_UART3_CTS	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 10)
#define PE11_PF_UART3_RTS	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 11)
#define PE12_PF_UART1_TXD	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 12)
#define PE13_PF_UART1_RXD	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 13)
#define PE14_PF_UART1_CTS	(GPIO_PORTE | GPIO_OUT | GPIO_PF | 14)
#define PE15_PF_UART1_RTS	(GPIO_PORTE | GPIO_IN  | GPIO_PF | 15)
#define PC7_PF_USBOTG_DATA5     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 7)
#define PC8_PF_USBOTG_DATA6     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 8)
#define PC9_PF_USBOTG_DATA0     (GPIO_PORTC | GPIO_PF | GPIO_OUT | 9)
#define PC10_PF_USBOTG_DATA2    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 10)
#define PC11_PF_USBOTG_DATA1    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 11)
#define PC12_PF_USBOTG_DATA4    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 12)
#define PC13_PF_USBOTG_DATA3    (GPIO_PORTC | GPIO_PF | GPIO_OUT | 13)
#define PE0_PF_USBOTG_NXT       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 0)
#define PE1_PF_USBOTG_STP       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 1)
#define PE2_PF_USBOTG_DIR       (GPIO_PORTE | GPIO_PF | GPIO_OUT | 2)
#define PE24_PF_USBOTG_CLK      (GPIO_PORTE | GPIO_PF | GPIO_OUT | 24)
#define PE25_PF_USBOTG_DATA7    (GPIO_PORTE | GPIO_PF | GPIO_OUT | 25)

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

#endif /* _IMX27_REGS_H */
