
/*
 *  barebox - cpu.h
 *
 *  Copyright (c) 2005 blackfin.uclinux.org
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _CPU_H_
#define _CPU_H_

#include <asm/ptrace.h>

#if defined(CONFIG_BF561)
#define page_descriptor_table_size (CONFIG_MEM_SIZE/4 + 1 + 4) /* SDRAM +L1 + ASYNC_Memory */
#else
#define page_descriptor_table_size (CONFIG_MEM_SIZE/4 + 2) /* SDRAM + L1 + ASYNC_Memory */
#endif

/* we cover everything with 4 meg pages, and need an extra for L1 */
extern unsigned int icplb_table[page_descriptor_table_size][2] ;
extern unsigned int dcplb_table[page_descriptor_table_size][2] ;

#define INTERNAL_IRQS (32)
#define NUM_IRQ_NODES 16
#define DEF_INTERRUPT_FLAGS 1
#define MAX_TIM_LOAD	0xFFFFFFFF

void blackfin_irq_panic(int reason, struct pt_regs *reg);
extern void dump_regs(struct pt_regs *regs);
void display_excp(void);
void evt_nmi(void);
void evt_exception(void);
void trap(void);
void evt_ivhw(void);
void evt_rst(void);
void evt_timer(void);
void evt_evt7(void);
void evt_evt8(void);
void evt_evt9(void);
void evt_evt10(void);
void evt_evt11(void);
void evt_evt12(void);
void evt_evt13(void);
void evt_soft_int1(void);
void evt_system_call(void);

void flush_data_cache(void);
void flush_instruction_cache(void);
void dcache_disable(void);
void icache_enable(void);
void dcache_enable(void);
int icache_status(void);
void icache_disable (void);
int dcache_status(void);

#endif
