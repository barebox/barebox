/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * based on linux/include/asm-mips/mach-jz4750d/regs.h
 *
 * JZ4750D register definition.
 *
 * Copyright (C) 2008 Ingenic Semiconductor Inc.
 */

#ifndef __JZ4750D_REGS_H__
#define __JZ4750D_REGS_H__

#define TCU_BASE        0xb0002000
#define WDT_BASE        0xb0002000
#define RTC_BASE        0xb0003000
#define UART0_BASE      0xb0030000
#define UART1_BASE      0xb0031000
#define UART2_BASE      0xb0032000

/*************************************************************************
 * TCU (Timer Counter Unit)
 *************************************************************************/
#define TCU_TESR        (TCU_BASE + 0x14) /* Timer Counter Enable Set Register */
 #define TCU_TESR_OSTST		(1 << 15)
 #define TCU_TESR_TCST5		(1 << 5)
 #define TCU_TESR_TCST4		(1 << 4)
 #define TCU_TESR_TCST3		(1 << 3)
 #define TCU_TESR_TCST2		(1 << 2)
 #define TCU_TESR_TCST1		(1 << 1)
 #define TCU_TESR_TCST0		(1 << 0)

#define TCU_TSCR        (TCU_BASE + 0x3c) /* Timer Stop Clear Register */
 #define TCU_TSCR_WDTSC		(1 << 16)
 #define TCU_TSCR_OSTSC		(1 << 15)
 #define TCU_TSCR_STPC5		(1 << 5)
 #define TCU_TSCR_STPC4		(1 << 4)
 #define TCU_TSCR_STPC3		(1 << 3)
 #define TCU_TSCR_STPC2		(1 << 2)
 #define TCU_TSCR_STPC1		(1 << 1)
 #define TCU_TSCR_STPC0		(1 << 0)

/* Operating System Timer */
#define TCU_OSTDR	(TCU_BASE + 0xe0)
#define TCU_OSTCNT      (TCU_BASE + 0xe8)
#define TCU_OSTCSR	(TCU_BASE + 0xec)
#define TCU_OSTCSR_PRESCALE_BIT		3
#define TCU_OSTCSR_PRESCALE_MASK	(0x7 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE1		(0x0 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE4		(0x1 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE16		(0x2 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE64		(0x3 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE256		(0x4 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE1024	(0x5 << TCU_OSTCSR_PRESCALE_BIT)
#define TCU_OSTCSR_EXT_EN		(1 << 2) /* select extal as the timer clock input */
#define TCU_OSTCSR_RTC_EN		(1 << 1) /* select rtcclk as the timer clock input */
#define TCU_OSTCSR_PCK_EN		(1 << 0) /* select pclk as the timer clock input */

/*************************************************************************
 * RTC
 *************************************************************************/
#define RTC_RCR		(RTC_BASE + 0x00) /* RTC Control Register */
#define RTC_RSR		(RTC_BASE + 0x04) /* RTC Second Register */
#define RTC_RSAR	(RTC_BASE + 0x08) /* RTC Second Alarm Register */
#define RTC_RGR		(RTC_BASE + 0x0c) /* RTC Regulator Register */

#define RTC_HCR		(RTC_BASE + 0x20) /* Hibernate Control Register */
#define RTC_HWFCR	(RTC_BASE + 0x24) /* Hibernate Wakeup Filter Counter Reg */
#define RTC_HRCR	(RTC_BASE + 0x28) /* Hibernate Reset Counter Register */
#define RTC_HWCR	(RTC_BASE + 0x2c) /* Hibernate Wakeup Control Register */
#define RTC_HWRSR	(RTC_BASE + 0x30) /* Hibernate Wakeup Status Register */
#define RTC_HSPR	(RTC_BASE + 0x34) /* Hibernate Scratch Pattern Register */

/* RTC Control Register */
#define RTC_RCR_WRDY_BIT 7
#define RTC_RCR_WRDY	(1 << 7)  /* Write Ready Flag */
#define RTC_RCR_1HZ_BIT	6
#define RTC_RCR_1HZ	(1 << RTC_RCR_1HZ_BIT)  /* 1Hz Flag */
#define RTC_RCR_1HZIE	(1 << 5)  /* 1Hz Interrupt Enable */
#define RTC_RCR_AF_BIT	4
#define RTC_RCR_AF	(1 << RTC_RCR_AF_BIT)  /* Alarm Flag */
#define RTC_RCR_AIE	(1 << 3)  /* Alarm Interrupt Enable */
#define RTC_RCR_AE	(1 << 2)  /* Alarm Enable */
#define RTC_RCR_RTCE	(1 << 0)  /* RTC Enable */

/* Hibernate Control Register */
#define RTC_HCR_PD		(1 << 0)  /* Power Down */

#endif /* __JZ4750D_REGS_H__ */
