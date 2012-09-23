#ifndef _IMX1_REGS_H
#define _IMX1_REGS_H

#ifndef _IMX_REGS_H
#error "Please do not include directly"
#endif

#define MX1_IO_BASE_ADDR	0x00200000
#define MX1_IO_SIZE		SZ_1M

#define MX1_CS0_PHYS		0x10000000
#define MX1_CS0_SIZE		0x02000000

#define MX1_CS1_PHYS		0x12000000
#define MX1_CS1_SIZE		0x01000000

#define MX1_CS2_PHYS		0x13000000
#define MX1_CS2_SIZE		0x01000000

#define MX1_CS3_PHYS		0x14000000
#define MX1_CS3_SIZE		0x01000000

#define MX1_CS4_PHYS		0x15000000
#define MX1_CS4_SIZE		0x01000000

#define MX1_CS5_PHYS		0x16000000
#define MX1_CS5_SIZE		0x01000000

/*
 *  Register BASEs, based on OFFSETs
 */
#define MX1_AIPI1_BASE_ADDR		(0x00000 + MX1_IO_BASE_ADDR)
#define MX1_WDT_BASE_ADDR		(0x01000 + MX1_IO_BASE_ADDR)
#define MX1_TIM1_BASE_ADDR		(0x02000 + MX1_IO_BASE_ADDR)
#define MX1_TIM2_BASE_ADDR		(0x03000 + MX1_IO_BASE_ADDR)
#define MX1_RTC_BASE_ADDR		(0x04000 + MX1_IO_BASE_ADDR)
#define MX1_LCDC_BASE_ADDR		(0x05000 + MX1_IO_BASE_ADDR)
#define MX1_UART1_BASE_ADDR		(0x06000 + MX1_IO_BASE_ADDR)
#define MX1_UART2_BASE_ADDR		(0x07000 + MX1_IO_BASE_ADDR)
#define MX1_PWM_BASE_ADDR		(0x08000 + MX1_IO_BASE_ADDR)
#define MX1_DMA_BASE_ADDR		(0x09000 + MX1_IO_BASE_ADDR)
#define MX1_AIPI2_BASE_ADDR		(0x10000 + MX1_IO_BASE_ADDR)
#define MX1_SIM_BASE_ADDR		(0x11000 + MX1_IO_BASE_ADDR)
#define MX1_USBD_BASE_ADDR		(0x12000 + MX1_IO_BASE_ADDR)
#define MX1_CSPI1_BASE_ADDR		(0x13000 + MX1_IO_BASE_ADDR)
#define MX1_MMC_BASE_ADDR		(0x14000 + MX1_IO_BASE_ADDR)
#define MX1_ASP_BASE_ADDR		(0x15000 + MX1_IO_BASE_ADDR)
#define MX1_BTA_BASE_ADDR		(0x16000 + MX1_IO_BASE_ADDR)
#define MX1_I2C_BASE_ADDR		(0x17000 + MX1_IO_BASE_ADDR)
#define MX1_SSI_BASE_ADDR		(0x18000 + MX1_IO_BASE_ADDR)
#define MX1_CSPI2_BASE_ADDR		(0x19000 + MX1_IO_BASE_ADDR)
#define MX1_MSHC_BASE_ADDR		(0x1A000 + MX1_IO_BASE_ADDR)
#define MX1_CCM_BASE_ADDR		(0x1B000 + MX1_IO_BASE_ADDR)
#define MX1_SCM_BASE_ADDR		(0x1B800 + MX1_IO_BASE_ADDR)
#define MX1_GPIO_BASE_ADDR		(0x1C000 + MX1_IO_BASE_ADDR)
#define MX1_GPIO1_BASE_ADDR		(0x1C000 + MX1_IO_BASE_ADDR)
#define MX1_GPIO2_BASE_ADDR		(0x1C100 + MX1_IO_BASE_ADDR)
#define MX1_GPIO3_BASE_ADDR		(0x1C200 + MX1_IO_BASE_ADDR)
#define MX1_GPIO4_BASE_ADDR		(0x1C300 + MX1_IO_BASE_ADDR)
#define MX1_EIM_BASE_ADDR		(0x20000 + MX1_IO_BASE_ADDR)
#define MX1_SDRAMC_BASE_ADDR		(0x21000 + MX1_IO_BASE_ADDR)
#define MX1_MMA_BASE_ADDR		(0x22000 + MX1_IO_BASE_ADDR)
#define MX1_AVIC_BASE_ADDR		(0x23000 + MX1_IO_BASE_ADDR)
#define MX1_CSI_BASE_ADDR		(0x24000 + MX1_IO_BASE_ADDR)

/* FIXME: get rid of these */
#define IMX_TIM1_BASE	MX1_CCM_BASE_ADDR
#define IMX_WDT_BASE	MX1_WDT_BASE_ADDR
#define IMX_GPIO_BASE	MX1_GPIO_BASE_ADDR

/* SYSCTRL Registers */
#define SIDR   __REG(MX1_SCM_BASE_ADDR + 0x4) /* Silicon ID Register		    */
#define FMCR   __REG(MX1_SCM_BASE_ADDR + 0x8) /* Function Multiplex Control Register */
#define GPCR   __REG(MX1_SCM_BASE_ADDR + 0xC) /* Function Multiplex Control Register */

/* SDRAM controller registers */

#define SDCTL0 __REG(MX1_SDRAMC_BASE_ADDR)        /* SDRAM 0 Control Register */
#define SDCTL1 __REG(MX1_SDRAMC_BASE_ADDR + 0x4)  /* SDRAM 1 Control Register */
#define SDMISC __REG(MX1_SDRAMC_BASE_ADDR + 0x14) /* Miscellaneous Register */
#define SDRST  __REG(MX1_SDRAMC_BASE_ADDR + 0x18) /* SDRAM Reset Register */

/* PLL registers */
#define CSCR   __REG(MX1_CCM_BASE_ADDR)        /* Clock Source Control Register */
#define MPCTL0 __REG(MX1_CCM_BASE_ADDR + 0x4)  /* MCU PLL Control Register 0 */
#define MPCTL1 __REG(MX1_CCM_BASE_ADDR + 0x8)  /* MCU PLL and System Clock Register 1 */
#define SPCTL0 __REG(MX1_CCM_BASE_ADDR + 0xc)  /* System PLL Control Register 0 */
#define SPCTL1 __REG(MX1_CCM_BASE_ADDR + 0x10) /* System PLL Control Register 1 */
#define PCDR   __REG(MX1_CCM_BASE_ADDR + 0x20) /* Peripheral Clock Divider Register */

#define CSCR_MPLL_RESTART (1<<21)

/* Chip Select Registers */
#define CS0U __REG(MX1_EIM_BASE_ADDR)        /* Chip Select 0 Upper Register */
#define CS0L __REG(MX1_EIM_BASE_ADDR + 0x4)  /* Chip Select 0 Lower Register */
#define CS1U __REG(MX1_EIM_BASE_ADDR + 0x8)  /* Chip Select 1 Upper Register */
#define CS1L __REG(MX1_EIM_BASE_ADDR + 0xc)  /* Chip Select 1 Lower Register */
#define CS2U __REG(MX1_EIM_BASE_ADDR + 0x10) /* Chip Select 2 Upper Register */
#define CS2L __REG(MX1_EIM_BASE_ADDR + 0x14) /* Chip Select 2 Lower Register */
#define CS3U __REG(MX1_EIM_BASE_ADDR + 0x18) /* Chip Select 3 Upper Register */
#define CS3L __REG(MX1_EIM_BASE_ADDR + 0x1c) /* Chip Select 3 Lower Register */
#define CS4U __REG(MX1_EIM_BASE_ADDR + 0x20) /* Chip Select 4 Upper Register */
#define CS4L __REG(MX1_EIM_BASE_ADDR + 0x24) /* Chip Select 4 Lower Register */
#define CS5U __REG(MX1_EIM_BASE_ADDR + 0x28) /* Chip Select 5 Upper Register */
#define CS5L __REG(MX1_EIM_BASE_ADDR + 0x2c) /* Chip Select 5 Lower Register */
#define EIM  __REG(MX1_EIM_BASE_ADDR + 0x30) /* EIM Configuration Register */

/* assignements for GPIO alternate/primary functions */

/* FIXME: This list is not completed. The correct directions are
 * missing on some (many) pins
 */
#define PA0_PF_A24           ( GPIO_PORTA | GPIO_PF | 0 )
#define PA0_AIN_SPI2_CLK     ( GPIO_PORTA | GPIO_OUT | GPIO_AIN | 0 )
#define PA0_AF_ETMTRACESYNC  ( GPIO_PORTA | GPIO_AF | 0 )
#define PA1_AOUT_SPI2_RXD    ( GPIO_PORTA | GPIO_IN | GPIO_AOUT | 1 )
#define PA1_PF_TIN           ( GPIO_PORTA | GPIO_PF | 1 )
#define PA2_PF_PWM0          ( GPIO_PORTA | GPIO_OUT | GPIO_PF | 2 )
#define PA3_PF_CSI_MCLK      ( GPIO_PORTA | GPIO_PF | 3 )
#define PA4_PF_CSI_D0        ( GPIO_PORTA | GPIO_PF | 4 )
#define PA5_PF_CSI_D1        ( GPIO_PORTA | GPIO_PF | 5 )
#define PA6_PF_CSI_D2        ( GPIO_PORTA | GPIO_PF | 6 )
#define PA7_PF_CSI_D3        ( GPIO_PORTA | GPIO_PF | 7 )
#define PA8_PF_CSI_D4        ( GPIO_PORTA | GPIO_PF | 8 )
#define PA9_PF_CSI_D5        ( GPIO_PORTA | GPIO_PF | 9 )
#define PA10_PF_CSI_D6       ( GPIO_PORTA | GPIO_PF | 10 )
#define PA11_PF_CSI_D7       ( GPIO_PORTA | GPIO_PF | 11 )
#define PA12_PF_CSI_VSYNC    ( GPIO_PORTA | GPIO_PF | 12 )
#define PA13_PF_CSI_HSYNC    ( GPIO_PORTA | GPIO_PF | 13 )
#define PA14_PF_CSI_PIXCLK   ( GPIO_PORTA | GPIO_PF | 14 )
#define PA15_PF_I2C_SDA      ( GPIO_PORTA | GPIO_OUT | GPIO_PF | 15 )
#define PA16_PF_I2C_SCL      ( GPIO_PORTA | GPIO_OUT | GPIO_PF | 16 )
#define PA17_AF_ETMTRACEPKT4 ( GPIO_PORTA | GPIO_AF | 17 )
#define PA17_AIN_SPI2_SS     ( GPIO_PORTA | GPIO_AIN | 17 )
#define PA18_AF_ETMTRACEPKT5 ( GPIO_PORTA | GPIO_AF | 18 )
#define PA19_AF_ETMTRACEPKT6 ( GPIO_PORTA | GPIO_AF | 19 )
#define PA20_AF_ETMTRACEPKT7 ( GPIO_PORTA | GPIO_AF | 20 )
#define PA21_PF_A0           ( GPIO_PORTA | GPIO_PF | 21 )
#define PA22_PF_CS4          ( GPIO_PORTA | GPIO_PF | 22 )
#define PA23_PF_CS5          ( GPIO_PORTA | GPIO_PF | 23 )
#define PA24_PF_A16          ( GPIO_PORTA | GPIO_PF | 24 )
#define PA24_AF_ETMTRACEPKT0 ( GPIO_PORTA | GPIO_AF | 24 )
#define PA25_PF_A17          ( GPIO_PORTA | GPIO_PF | 25 )
#define PA25_AF_ETMTRACEPKT1 ( GPIO_PORTA | GPIO_AF | 25 )
#define PA26_PF_A18          ( GPIO_PORTA | GPIO_PF | 26 )
#define PA26_AF_ETMTRACEPKT2 ( GPIO_PORTA | GPIO_AF | 26 )
#define PA27_PF_A19          ( GPIO_PORTA | GPIO_PF | 27 )
#define PA27_AF_ETMTRACEPKT3 ( GPIO_PORTA | GPIO_AF | 27 )
#define PA28_PF_A20          ( GPIO_PORTA | GPIO_PF | 28 )
#define PA28_AF_ETMPIPESTAT0 ( GPIO_PORTA | GPIO_AF | 28 )
#define PA29_PF_A21          ( GPIO_PORTA | GPIO_PF | 29 )
#define PA29_AF_ETMPIPESTAT1 ( GPIO_PORTA | GPIO_AF | 29 )
#define PA30_PF_A22          ( GPIO_PORTA | GPIO_PF | 30 )
#define PA30_AF_ETMPIPESTAT2 ( GPIO_PORTA | GPIO_AF | 30 )
#define PA31_PF_A23          ( GPIO_PORTA | GPIO_PF | 31 )
#define PA31_AF_ETMTRACECLK  ( GPIO_PORTA | GPIO_AF | 31 )
#define PB8_PF_SD_DAT0       ( GPIO_PORTB | GPIO_PF | GPIO_PUEN | 8 )
#define PB8_AF_MS_PIO        ( GPIO_PORTB | GPIO_AF | 8 )
#define PB9_PF_SD_DAT1       ( GPIO_PORTB | GPIO_PF | GPIO_PUEN  | 9 )
#define PB9_AF_MS_PI1        ( GPIO_PORTB | GPIO_AF | 9 )
#define PB10_PF_SD_DAT2      ( GPIO_PORTB | GPIO_PF | GPIO_PUEN  | 10 )
#define PB10_AF_MS_SCLKI     ( GPIO_PORTB | GPIO_AF | 10 )
#define PB11_PF_SD_DAT3      ( GPIO_PORTB | GPIO_PF | GPIO_PUEN  | 11 )
#define PB11_AF_MS_SDIO      ( GPIO_PORTB | GPIO_AF | 11 )
#define PB12_PF_SD_CLK       ( GPIO_PORTB | GPIO_PF | GPIO_OUT | 12 )
#define PB12_AF_MS_SCLK0     ( GPIO_PORTB | GPIO_AF | 12 )
#define PB13_PF_SD_CMD       ( GPIO_PORTB | GPIO_PF | GPIO_OUT | GPIO_PUEN | 13 )
#define PB13_AF_MS_BS        ( GPIO_PORTB | GPIO_AF | 13 )
#define PB14_AF_SSI_RXFS     ( GPIO_PORTB | GPIO_AF | 14 )
#define PB15_AF_SSI_RXCLK    ( GPIO_PORTB | GPIO_AF | 15 )
#define PB16_AF_SSI_RXDAT    ( GPIO_PORTB | GPIO_IN | GPIO_AF | 16 )
#define PB17_AF_SSI_TXDAT    ( GPIO_PORTB | GPIO_OUT | GPIO_AF | 17 )
#define PB18_AF_SSI_TXFS     ( GPIO_PORTB | GPIO_AF | 18 )
#define PB19_AF_SSI_TXCLK    ( GPIO_PORTB | GPIO_AF | 19 )
#define PB20_PF_USBD_AFE     ( GPIO_PORTB | GPIO_PF | 20 )
#define PB21_PF_USBD_OE      ( GPIO_PORTB | GPIO_PF | 21 )
#define PB22_PFUSBD_RCV      ( GPIO_PORTB | GPIO_PF | 22 )
#define PB23_PF_USBD_SUSPND  ( GPIO_PORTB | GPIO_PF | 23 )
#define PB24_PF_USBD_VP      ( GPIO_PORTB | GPIO_PF | 24 )
#define PB25_PF_USBD_VM      ( GPIO_PORTB | GPIO_PF | 25 )
#define PB26_PF_USBD_VPO     ( GPIO_PORTB | GPIO_PF | 26 )
#define PB27_PF_USBD_VMO     ( GPIO_PORTB | GPIO_PF | 27 )
#define PB28_PF_UART2_CTS    ( GPIO_PORTB | GPIO_OUT | GPIO_PF | 28 )
#define PB29_PF_UART2_RTS    ( GPIO_PORTB | GPIO_IN | GPIO_PF | 29 )
#define PB30_PF_UART2_TXD    ( GPIO_PORTB | GPIO_OUT | GPIO_PF | 30 )
#define PB31_PF_UART2_RXD    ( GPIO_PORTB | GPIO_IN | GPIO_PF | 31 )
#define PC3_PF_SSI_RXFS      ( GPIO_PORTC | GPIO_PF | 3 )
#define PC4_PF_SSI_RXCLK     ( GPIO_PORTC | GPIO_PF | 4 )
#define PC5_PF_SSI_RXDAT     ( GPIO_PORTC | GPIO_IN | GPIO_PF | 5 )
#define PC6_PF_SSI_TXDAT     ( GPIO_PORTC | GPIO_OUT | GPIO_PF | 6 )
#define PC7_PF_SSI_TXFS      ( GPIO_PORTC | GPIO_PF | 7 )
#define PC8_PF_SSI_TXCLK     ( GPIO_PORTC | GPIO_PF | 8 )
#define PC9_PF_UART1_CTS     ( GPIO_PORTC | GPIO_OUT | GPIO_PF | 9 )
#define PC10_PF_UART1_RTS    ( GPIO_PORTC | GPIO_IN | GPIO_PF | 10 )
#define PC11_PF_UART1_TXD    ( GPIO_PORTC | GPIO_OUT | GPIO_PF | 11 )
#define PC12_PF_UART1_RXD    ( GPIO_PORTC | GPIO_IN | GPIO_PF | 12 )
#define PC13_PF_SPI1_SPI_RDY ( GPIO_PORTC | GPIO_PF | 13 )
#define PC14_PF_SPI1_SCLK    ( GPIO_PORTC | GPIO_PF | 14 )
#define PC15_PF_SPI1_SS      ( GPIO_PORTC | GPIO_PF | 15 )
#define PC16_PF_SPI1_MISO    ( GPIO_PORTC | GPIO_PF | 16 )
#define PC17_PF_SPI1_MOSI    ( GPIO_PORTC | GPIO_PF | 17 )
#define PD6_PF_LSCLK         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 6 )
#define PD7_PF_REV           ( GPIO_PORTD | GPIO_PF | 7 )
#define PD7_AF_UART2_DTR     ( GPIO_PORTD | GPIO_IN | GPIO_AF | 7 )
#define PD7_AIN_SPI2_SCLK    ( GPIO_PORTD | GPIO_AIN | 7 )
#define PD8_PF_CLS           ( GPIO_PORTD | GPIO_PF | 8 )
#define PD8_AF_UART2_DCD     ( GPIO_PORTD | GPIO_OUT | GPIO_AF | 8 )
#define PD8_AIN_SPI2_SS      ( GPIO_PORTD | GPIO_AIN | 8 )
#define PD9_PF_PS            ( GPIO_PORTD | GPIO_PF | 9 )
#define PD9_AF_UART2_RI      ( GPIO_PORTD | GPIO_OUT | GPIO_AF | 9 )
#define PD9_AOUT_SPI2_RXD    ( GPIO_PORTD | GPIO_IN | GPIO_AOUT | 9 )
#define PD10_PF_SPL_SPR      ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 10 )
#define PD10_AF_UART2_DSR    ( GPIO_PORTD | GPIO_OUT | GPIO_AF | 10 )
#define PD10_AIN_SPI2_TXD    ( GPIO_PORTD | GPIO_OUT | GPIO_AIN | 10 )
#define PD11_PF_CONTRAST     ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 11 )
#define PD12_PF_ACD_OE       ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 12 )
#define PD13_PF_LP_HSYNC     ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 13 )
#define PD14_PF_FLM_VSYNC    ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 14 )
#define PD15_PF_LD0          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 15 )
#define PD16_PF_LD1          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 16 )
#define PD17_PF_LD2          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 17 )
#define PD18_PF_LD3          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 18 )
#define PD19_PF_LD4          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 19 )
#define PD20_PF_LD5          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 20 )
#define PD21_PF_LD6          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 21 )
#define PD22_PF_LD7          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 22 )
#define PD23_PF_LD8          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 23 )
#define PD24_PF_LD9          ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 24 )
#define PD25_PF_LD10         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 25 )
#define PD26_PF_LD11         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 26 )
#define PD27_PF_LD12         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 27 )
#define PD28_PF_LD13         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 28 )
#define PD29_PF_LD14         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 29 )
#define PD30_PF_LD15         ( GPIO_PORTD | GPIO_OUT | GPIO_PF | 30 )
#define PD31_PF_TMR2OUT      ( GPIO_PORTD | GPIO_PF | 31 )
#define PD31_BIN_SPI2_TXD    ( GPIO_PORTD | GPIO_BIN | 31 )

#endif /* _IMX1_REGS_H */
