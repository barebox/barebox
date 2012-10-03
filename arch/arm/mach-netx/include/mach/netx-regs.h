/*
 * include/asm-arm/arch-netx/netx-regs.h
 *
 * Copyright (c) 2005 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_NETX_REGS_H
#define __ASM_ARCH_NETX_REGS_H

#define NETX_SDRAM_BASE 0x80000000
#define NETX_CS0_BASE 0xc0000000
#define NETX_CS1_BASE 0xc8000000
#define NETX_CS2_BASE 0xd0000000

#define NETX_IO_PHYS 0x00100000
#define io_p2v(x) (x)
#define __REG(base,ofs)       (*((volatile unsigned long *)(io_p2v(base) + ofs)))

#define XPEC_MEM_SIZE 0x4000
#define XMAC_MEM_SIZE 0x1000
#define SRAM_MEM_SIZE 0x8000

/* offsets relative to the beginning of the io space */
#define NETX_OFS_SYSTEM  0x00000
#define NETX_OFS_MEMCR   0x00100
#define NETX_OFS_DPRAM   0x03000
#define NETX_OFS_GPIO    0x00800
#define NETX_OFS_PIO     0x00900
#define NETX_OFS_UART0   0x00a00
#define NETX_OFS_UART1   0x00a40
#define NETX_OFS_UART2   0x00a80
#define NETX_OF_MIIMU    0x00b00
#define NETX_OFS_SPI     0x00c00
#define NETX_OFS_I2C     0x00d00
#define NETX_OFS_SYSTIME 0x01100
#define NETX_OFS_RTC     0x01200
#define NETX_OFS_LCD     0x04000
#define NETX_OFS_USB     0x20000
#define NETX_OFS_XMAC0   0x60000
#define NETX_OFS_XMAC1   0x61000
#define NETX_OFS_XMAC2   0x62000
#define NETX_OFS_XMAC3   0x63000
#define NETX_OFS_XMAC(no) (0x60000 + (no) * 0x1000)
#define NETX_OFS_PFIFO   0x64000
#define NETX_OFS_XPEC0   0x70000
#define NETX_OFS_XPEC1   0x74000
#define NETX_OFS_XPEC2   0x78000
#define NETX_OFS_XPEC3   0x7c000
#define NETX_OFS_XPEC(no) (0x70000 + (no) * 0x4000)
#define NETX_OFS_VIC     0xff000

#define NETX_PA_SYSTEM  (NETX_IO_PHYS + NETX_OFS_SYSTEM)
#define NETX_PA_MEMCR   (NETX_IO_PHYS + NETX_OFS_MEMCR)
#define NETX_PA_DPMAS   (NETX_IO_PHYS + NETX_OFS_DPRAM)
#define NETX_PA_GPIO    (NETX_IO_PHYS + NETX_OFS_GPIO)
#define NETX_PA_PIO     (NETX_IO_PHYS + NETX_OFS_PIO)
#define NETX_PA_UART0   (NETX_IO_PHYS + NETX_OFS_UART0)
#define NETX_PA_UART1   (NETX_IO_PHYS + NETX_OFS_UART1)
#define NETX_PA_UART2   (NETX_IO_PHYS + NETX_OFS_UART2)
#define NETX_PA_MIIMU   (NETX_IO_PHYS + NETX_OF_MIIMU)
#define NETX_PA_SPI     (NETX_IO_PHYS + NETX_OFS_SPI)
#define NETX_PA_I2C     (NETX_IO_PHYS + NETX_OFS_I2C)
#define NETX_PA_SYSTIME (NETX_IO_PHYS + NETX_OFS_SYSTIME)
#define NETX_PA_RTC     (NETX_IO_PHYS + NETX_OFS_RTC)
#define NETX_PA_LCD     (NETX_IO_PHYS + NETX_OFS_LCD)
#define NETX_PA_USB     (NETX_IO_PHYS + NETX_OFS_USB)
#define NETX_PA_XMAC0   (NETX_IO_PHYS + NETX_OFS_XMAC0)
#define NETX_PA_XMAC1   (NETX_IO_PHYS + NETX_OFS_XMAC1)
#define NETX_PA_XMAC2   (NETX_IO_PHYS + NETX_OFS_XMAC2)
#define NETX_PA_XMAC3   (NETX_IO_PHYS + NETX_OFS_XMAC3)
#define NETX_PA_XMAC(no) (NETX_IO_PHYS + NETX_OFS_XMAC(no))
#define NETX_PA_PFIFO   (NETX_IO_PHYS + NETX_OFS_PFIFO)
#define NETX_PA_XPEC0   (NETX_IO_PHYS + NETX_OFS_XPEC0)
#define NETX_PA_XPEC1   (NETX_IO_PHYS + NETX_OFS_XPEC1)
#define NETX_PA_XPEC2   (NETX_IO_PHYS + NETX_OFS_XPEC2)
#define NETX_PA_XPEC3   (NETX_IO_PHYS + NETX_OFS_XPEC3)
#define NETX_PA_XPEC(no) (NETX_IO_PHYS + NETX_OFS_XPEC(no))
#define NETX_PA_VIC     (NETX_IO_PHYS + NETX_OFS_VIC)

/*********************************
 * System functions              *
 *********************************/

#define SYSTEM_REG(x)          __REG(NETX_PA_SYSTEM, (x))
#define SYSTEM_BOO_SR          0x00
#define SYSTEM_IOC_CR          0x04
#define SYSTEM_IOC_MR          0x08
#define SYSTEM_RES_CR          0x0c
#define SYSTEM_PHY_CONTROL     0x10
#define SYSTEM_REV             0x34
#define SYSTEM_IOC_ACCESS_KEY  0x70
#define SYSTEM_WDG_TR          0x200
#define SYSTEM_WDG_CTR         0x204
#define SYSTEM_WDG_IRQ_TIMEOUT 0x208
#define SYSTEM_WDG_RES_TIMEOUT 0x20c

#define PHY_CONTROL_RESET            (1<<31)
#define PHY_CONTROL_SIM_BYP          (1<<30)
#define PHY_CONTROL_CLK_XLATIN       (1<<29)
#define PHY_CONTROL_PHY1_EN          (1<<21)
#define PHY_CONTROL_PHY1_NP_MSG_CODE
#define PHY_CONTROL_PHY1_AUTOMDIX    (1<<17)
#define PHY_CONTROL_PHY1_FIXMODE     (1<<16)
#define PHY_CONTROL_PHY1_MODE(mode)  (((mode) & 0x7) << 13)
#define PHY_CONTROL_PHY0_EN          (1<<12)
#define PHY_CONTROL_PHY0_NP_MSG_CODE
#define PHY_CONTROL_PHY0_AUTOMDIX    (1<<8)
#define PHY_CONTROL_PHY0_FIXMODE     (1<<7)
#define PHY_CONTROL_PHY0_MODE(mode)  (((mode) & 0x7) << 4)
#define PHY_CONTROL_PHY_ADDRESS(adr) ((adr) & 0xf)

#define PHY_MODE_10BASE_T_HALF      0
#define PHY_MODE_10BASE_T_FULL      1
#define PHY_MODE_100BASE_TX_FX_FULL 2
#define PHY_MODE_100BASE_TX_FX_HALF 3
#define PHY_MODE_100BASE_TX_HALF    4
#define PHY_MODE_REPEATER           5
#define PHY_MODE_POWER_DOWN         6
#define PHY_MODE_ALL                7

/*********************************
 * Vector interrupt controller   *
 *********************************/

/* Registers */
#define VIC_REG(x)			__REG(NETX_PA_VIC, (x))
#define VIC_IRQ_STATUS                  0x00
#define VIC_FIQ_STATUS                  0x04
#define VIC_IRQ_RAW_STATUS              0x08
#define VIC_INT_SELECT                  0x0C    /* 1 = FIQ, 0 = IRQ */
#define VIC_IRQ_ENABLE                  0x10    /* 1 = enable, 0 = disable */
#define VIC_IRQ_ENABLE_CLEAR            0x14
#define VIC_IRQ_SOFT                    0x18
#define VIC_IRQ_SOFT_CLEAR              0x1C
#define VIC_PROTECT                     0x20
#define VIC_VECT_ADDR                   0x30
#define VIC_DEF_VECT_ADDR               0x34
#define VIC_VECT_ADDR0                  0x100   /* 0 to 15 */
#define VIC_VECT_CNTL0                  0x200   /* 0 to 15 */
#define VIC_ITCR                        0x300   /* VIC test control register */

/* Bits */
#define VECT_CNTL_ENABLE               (1 << 5)

/*******************************
 * GPIO and timer module       *
 *******************************/

/* Registers */
#define GPIO_REG(x)                        __REG(NETX_PA_GPIO, (x))
#define GPIO_CFG(gpio)                     (0x0  + ((gpio)<<2))
#define GPIO_THRESHOLD_CAPTURE(gpio)       (0x40 + ((gpio)<<2))
#define GPIO_COUNTER_CTRL(counter)         (0x80 + ((counter)<<2))
#define GPIO_COUNTER_MAX(counter)          (0x94 + ((counter)<<2))
#define GPIO_COUNTER_CURRENT(counter)      (0xa8 + ((counter)<<2))
#define GPIO_IRQ_ENABLE                    (0xbc)
#define GPIO_IRQ_DISABLE                   (0xc0)
#define GPIO_SYSTIME_NS_CMP                (0xc4)
#define GPIO_LINE                          (0xc8)
#define GPIO_IRQ                           (0xd0)

/* Bits */
#define CFG_IOCFG_GP_INPUT (0x0)
#define CFG_IOCFG_GP_OUTPUT (0x1)
#define CFG_IOCFG_GP_UART   (0x2)
#define CFG_INV (1<<2)
#define CFG_MODE_INPUT_READ (0<<3)
#define CFG_MODE_INPUT_CAPTURE_CONT_RISING (1<<3)
#define CFG_MODE_INPUT_CAPTURE_ONCE_RISING (2<<3)
#define CFG_MODE_INPUT_CAPTURE_HIGH_LEVEL  (3<<3)
#define CFG_COUNT_REF_COUNTER0             (0<<5)
#define CFG_COUNT_REF_COUNTER1             (1<<5)
#define CFG_COUNT_REF_COUNTER2             (2<<5)
#define CFG_COUNT_REF_COUNTER3             (3<<5)
#define CFG_COUNT_REF_COUNTER4             (4<<5)
#define CFG_COUNT_REF_SYSTIME              (7<<5)

#define COUNTER_CTRL_RUN (1<<0)
#define COUNTER_CTRL_SYM (1<<1)
#define COUNTER_CTRL_ONCE (1<<2)
#define COUNTER_CTRL_IRQ_EN (1<<3)
#define COUNTER_CTRL_CNT_EVENT (1<<4)
#define COUNTER_CTRL_RST_EN (1<<5)
#define COUNTER_CTRL_SEL_EVENT (1<<6)
#define COUNTER_CTRL_GPIO_REF /* FIXME */

#define GPIO_BIT(gpio)                     (1<<(gpio))
#define COUNTER_BIT(counter)               ((1<<16)<<(counter))

/*******************************
 * PIO                         *
 *******************************/

/* Registers */
#define NETX_PIO_REG(ofs)        __REG(NETX_PA_PIO, ofs)
#define NETX_PIO_INPIO       NETX_PIO_REG(0x0)
#define NETX_PIO_OUTPIO      NETX_PIO_REG(0x4)
#define NETX_PIO_OEPIO       NETX_PIO_REG(0x8)

/*******************************
 * MII Unit                    *
 *******************************/
#define MIIMU_REG                       __REG(NETX_PA_MIIMU, 0)
/* Bits */
#define MIIMU_SNRDY        (1<<0)
#define MIIMU_PREAMBLE     (1<<1)
#define MIIMU_OPMODE_WRITE (1<<2)
#define MIIMU_MDC_PERIOD   (1<<3)
#define MIIMU_PHY_NRES     (1<<4)
#define MIIMU_RTA          (1<<5)
#define MIIMU_REGADDR(adr) (((adr) & 0x1f) << 6)
#define MIIMU_PHYADDR(adr) (((adr) & 0x1f) << 11)
#define MIIMU_DATA(data)   (((data) & 0xffff) << 16)

/*******************************
 * xmac / xpec                 *
 *******************************/
#define XPEC_REG(no, reg)      __REG(NETX_PA_XPEC(no), (reg))
#define XPEC_R0           0x00
#define XPEC_R1           0x04
#define XPEC_R2           0x08
#define XPEC_R3           0x0c
#define XPEC_R4           0x10
#define XPEC_R5           0x14
#define XPEC_R6           0x18
#define XPEC_R7           0x1c
#define XPEC_RANGE01      0x20
#define XPEC_RANGE23      0x24
#define XPEC_RANGE45      0x28
#define XPEC_RANGE67      0x2c
#define XPEC_PC           0x48
#define XPEC_TIMER(timer) (0x30 + ((timer)<<2))
#define XPEC_IRQ          0x8c
#define XPEC_SYSTIME_NS   0x90
#define XPEC_FIFO_DATA    0x94
#define XPEC_SYSTIME_S    0x98
#define XPEC_ADC          0x9c
#define XPEC_URX_COUNT    0x40
#define XPEC_UTX_COUNT    0x44
#define XPEC_PC           0x48
#define XPEC_ZERO         0x4c
#define XPEC_STATCFG      0x50
#define XPEC_EC_MASKA     0x54
#define XPEC_EC_MASKB     0x58
#define XPEC_EC_MASK0     0x5c
#define XPEC_EC_MASK8     0x7c
#define XPEC_EC_MASK9     0x80
#define XPEC_XPU_HOLD_PC  0x100
#define XPEC_RAM_START    0x2000

#define XPU_HOLD_PC (1<<0)

#define XMAC_REG(no, reg)      __REG(NETX_PA_XMAC(no), (reg))
#define XMAC_RPU_PROGRAM_START 0x000
#define XMAC_RPU_PROGRAM_END   0x3ff
#define XMAC_TPU_PROGRAM_START 0x400
#define XMAC_TPU_PROGRAM_END   0x7ff
#define XMAC_RPU_HOLD_PC       0xa00
#define XMAC_TPU_HOLD_PC       0xa04

#define RPU_HOLD_PC            (1<<15)
#define TPU_HOLD_PC            (1<<15)
/*******************************
 * Pointer FIFO                *
 *******************************/
#define PFIFO_REG(x)             __REG(NETX_PA_PFIFO, (x))
#define PFIFO_BASE(pfifo)        (0x00 + ((pfifo)<<2) )
#define PFIFO_BORDER_BASE(pfifo) (0x80 + ((pfifo)<<2) )
#define PFIFO_RESET              0x100
#define PFIFO_FULL               0x104
#define PFIFO_EMPTY              0x108
#define PFIFO_OVEFLOW            0x10c
#define PFIFO_UNDERRUN           0x110
#define PFIFO_FILL_LEVEL(pfifo)  (0x180 + ((pfifo)<<2))

/*******************************
 * Dual Port Memory            *
 *******************************/

/* Registers */
#define NETX_DPMAS_REG(ofs)           __REG(NETX_PA_DPMAS, (ofs))
#define NETX_DPMAS_IF_CONF0_REG           NETX_DPMAS_REG(0x608)
#define NETX_DPMAS_IF_CONF1_REG           NETX_DPMAS_REG(0x60c)
#define NETX_DPMAS_EXT_CONFIG0_REG        NETX_DPMAS_REG(0x610)
#define NETX_DPMAS_EXT_CONFIG1_REG        NETX_DPMAS_REG(0x614)
#define NETX_DPMAS_EXT_CONFIG2_REG        NETX_DPMAS_REG(0x618)
#define NETX_DPMAS_EXT_CONFIG3_REG        NETX_DPMAS_REG(0x61c)
#define NETX_DPMAS_IO_MODE0_REG           NETX_DPMAS_REG(0x620) /* I/O 32..63 */
#define NETX_DPMAS_DRV_EN0_REG            NETX_DPMAS_REG(0x624)
#define NETX_DPMAS_DATA0_REG              NETX_DPMAS_REG(0x628)
#define NETX_DPMAS_IO_MODE1_REG           NETX_DPMAS_REG(0x630) /* I/O 64..84 */
#define NETX_DPMAS_DRV_EN1_REG            NETX_DPMAS_REG(0x634)
#define NETX_DPMAS_DATA1_REG              NETX_DPMAS_REG(0x638)

/* Bits */
#define IF_CONF0_HIF_DISABLED (0<<28)
#define IF_CONF0_HIF_EXT_BUS  (1<<28)
#define IF_CONF0_HIF_UP_8BIT  (2<<28)
#define IF_CONF0_HIF_UP_16BIT (3<<28)
#define IF_CONF0_HIF_IO       (4<<28)

#define IO_MODE1_SAMPLE_NPOR   (0<<30)
#define IO_MODE1_SAMPLE_100MHZ (1<<30)
#define IO_MODE1_SAMPLE_NPIO36 (2<<30)
#define IO_MODE1_SAMPLE_PIO36  (3<<30)

/*******************************
 * I2C                         *
 *******************************/
#define NETX_I2C_REG(ofs)	__REG(NETX_PA_I2C, (ofs))
#define NETX_I2C_CTRL_REG	NETX_I2C_REG(0x0)
#define NETX_I2C_DATA_REG	NETX_I2C_REG(0x4)

#endif /* __ASM_ARCH_NETX_REGS_H */
