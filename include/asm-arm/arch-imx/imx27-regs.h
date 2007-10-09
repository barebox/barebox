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

#define IMX_ESD_BASE               (0xd8001000)
#define IMX_WEIM_BASE              (0xd8002000)

/* AIPI */
#define AIPI1_PSR0	__REG(IMX_AIPI1_BASE + 0x00)
#define AIPI1_PSR1	__REG(IMX_AIPI1_BASE + 0x04)
#define AIPI2_PSR0	__REG(IMX_AIPI2_BASE + 0x00)
#define AIPI2_PSR1	__REG(IMX_AIPI2_BASE + 0x04)

/* System Control */
#define GPCR	__REG(IMX_SYSTEM_CTL_BASE + 0x18)

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

/* SDRAM Controller registers */
#define ESDCTL0 __REG(IMX_ESD_BASE + 0x00) /* Enhanced SDRAM Control Register 0       */
#define ESDCFG0 __REG(IMX_ESD_BASE + 0x04) /* Enhanced SDRAM Configuration Register 0 */
#define ESDCTL1 __REG(IMX_ESD_BASE + 0x08) /* Enhanced SDRAM Control Register 1       */
#define ESDCFG1 __REG(IMX_ESD_BASE + 0x10) /* Enhanced SDRAM Configuration Register 1 */
#define ESDMISC __REG(IMX_ESD_BASE + 0x14) /* Enhanced SDRAM Miscellanious Register   */

/* Watchdog Registers*/
#define WCR  __REG(IMX_WDT_BASE + 0x00) /* Watchdog Control Register */
#define WSR  __REG(IMX_WDT_BASE + 0x04) /* Watchdog Service Register */
#define WSTR __REG(IMX_WDT_BASE + 0x08) /* Watchdog Status Register  */

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
#define CSCR_AHB_DIV
#define CSCR_ARM_DIV
#define CSCR_ARM_SRC_MPLL	(1 << 15)
#define CSCR_MCU_SEL		(1 << 16)
#define CSCR_SP_SEL		(1 << 17)
#define CSCR_MPLL_RESTART	(1 << 18)
#define CSCR_SPLL_RESTART	(1 << 19)
#define CSCR_MSHC_SEL		(1 << 20)
#define CSCR_H264_SEL		(1 << 21)
#define CSCR_SSI1_SEL		(1 << 22)
#define CSCR_SSI2_SEL		(1 << 23)
#define CSCR_SD_CNT
#define CSCR_USB_DIV
#define CSCR_UPDATE_DIS		(1 << 31)

#define MPCTL1_BRMO		(1 << 6)
#define MPCTL1_LF		(1 << 15)

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
#define PD12_AOUT_FEC_RXD0	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 12)
#define PD14_AOUT_FEC_CLR	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 14)
#define PD11_AOUT_FEC_TX_CLK	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 11)
#define PD13_AOUT_FEC_RX_DV	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 13)
#define PD15_AOUT_FEC_COL	(GPIO_PORTD | GPIO_IN | GPIO_AOUT | 15)
#define PD16_AIN_FEC_TX_ER	(GPIO_PORTD | GPIO_OUT | GPIO_AIN | 16)
#define PF23_AIN_FEC_TX_EN	(GPIO_PORTF | GPIO_OUT | GPIO_AIN | 23)

#define PD17_PF_I2C_DATA	(GPIO_PORTD | GPIO_OUT | GPIO_PF | 17)
#define PD18_PF_I2C_CLK		(GPIO_PORTD | GPIO_OUT | GPIO_PF | 18)
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

#endif /* _IMX27_REGS_H */

