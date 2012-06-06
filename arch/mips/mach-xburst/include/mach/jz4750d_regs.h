/*
 * based on linux/include/asm-mips/mach-jz4750d/regs.h
 *
 * JZ4750D register definition.
 *
 * Copyright (C) 2008 Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ4750D_REGS_H__
#define __JZ4750D_REGS_H__

#define TCU_BASE        0xb0002000
#define WDT_BASE        0xb0002000
#define UART1_BASE      0xb0031000

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
 * WDT (WatchDog Timer)
 *************************************************************************/
#define WDT_TDR		(WDT_BASE + 0x00)
#define WDT_TCER	(WDT_BASE + 0x04)
#define WDT_TCNT	(WDT_BASE + 0x08)
#define WDT_TCSR	(WDT_BASE + 0x0c)

#define WDT_TCSR_PRESCALE_BIT	3
#define WDT_TCSR_PRESCALE_MASK	(0x7 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE1	(0x0 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE4	(0x1 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE16	(0x2 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE64	(0x3 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE256	(0x4 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE1024	(0x5 << WDT_TCSR_PRESCALE_BIT)
#define WDT_TCSR_EXT_EN		(1 << 2)
#define WDT_TCSR_RTC_EN		(1 << 1)
#define WDT_TCSR_PCK_EN		(1 << 0)

#define WDT_TCER_TCEN		(1 << 0)

#endif /* __JZ4750D_REGS_H__ */
