/*
 * (C) Copyright 2002
 * Daniel Engström, Omicron Ceti AB, daniel@omicron.se.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/i8254.h>
#include <asm/ibmpc.h>


static volatile unsigned long system_ticks;
static int timer_init_done =0;

static void timer_isr(void *unused)
{
	system_ticks++;
}

unsigned long get_system_ticks(void)
{
	return system_ticks;
}

#define TIMER0_VALUE 0x04aa /* 1kHz 1.9318MHz / 1000 */
#define TIMER2_VALUE 0x0a8e /* 440Hz */

int timer_init(void)
{
	system_ticks = 0;

	irq_install_handler(0, timer_isr, NULL);

	/* initialize timer 0 and 2
	 *
	 * Timer 0 is used to increment system_tick 1000 times/sec
	 * Timer 1 was used for DRAM refresh in early PC's
	 * Timer 2 is used to drive the speaker
	 * (to stasrt a beep: write 3 to port 0x61,
	 * to stop it again: write 0)
	 */

	outb(PIT_CMD_CTR0|PIT_CMD_BOTH|PIT_CMD_MODE2, PIT_BASE + PIT_COMMAND);
	outb(TIMER0_VALUE&0xff, PIT_BASE + PIT_T0);
	outb(TIMER0_VALUE>>8, PIT_BASE + PIT_T0);

	outb(PIT_CMD_CTR2|PIT_CMD_BOTH|PIT_CMD_MODE3, PIT_BASE + PIT_COMMAND);
	outb(TIMER2_VALUE&0xff, PIT_BASE + PIT_T2);
	outb(TIMER2_VALUE>>8, PIT_BASE + PIT_T2);

	timer_init_done = 1;

	return 0;
}


#ifdef CFG_TIMER_GENERIC

/* the unit for these is CFG_HZ */

/* FixMe: implement these */
void reset_timer (void)
{
	system_ticks = 0;
}

ulong get_timer (ulong base)
{
	return (system_ticks - base);
}

void set_timer (ulong t)
{
	system_ticks = t;
}

static u16 read_pit(void)
{
	u8 low;
	outb(PIT_CMD_LATCH, PIT_BASE + PIT_COMMAND);
	low = inb(PIT_BASE + PIT_T0);
	return ((inb(PIT_BASE + PIT_T0) << 8) | low);
}

#endif
