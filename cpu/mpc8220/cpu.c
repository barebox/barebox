/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * CPU specific code for the MPC8220 CPUs
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <mpc8220.h>
#include <asm/processor.h>

DECLARE_GLOBAL_DATA_PTR;

int checkcpu (void)
{
	ulong clock = gd->cpu_clk;
	char buf[32];

	puts ("CPU:   ");

	printf (CPU_ID_STR);

	printf (" (JTAG ID %08lx)", *(vu_long *) (CFG_MBAR + 0x50));

	printf (" at %s MHz\n", strmhz (buf, clock));

	return 0;
}

/* ------------------------------------------------------------------------- */

int do_reset (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	volatile gptmr8220_t *gptmr = (volatile gptmr8220_t *) MMAP_GPTMR;
	ulong msr;

	/* Interrupts and MMU off */
	__asm__ __volatile__ ("mfmsr    %0":"=r" (msr):);

	msr &= ~(MSR_ME | MSR_EE | MSR_IR | MSR_DR);
	__asm__ __volatile__ ("mtmsr    %0"::"r" (msr));

	/* Charge the watchdog timer */
	gptmr->Prescl = 10;
	gptmr->Count = 1;

	gptmr->Mode = GPT_TMS_SGPIO;

	gptmr->Control = GPT_CTRL_WDEN | GPT_CTRL_CE;

	return 1;
}

/* ------------------------------------------------------------------------- */

/*
 * Get timebase clock frequency (like cpu_clk in Hz)
 *
 */
unsigned long get_tbclk (void)
{
	ulong tbclk;

	tbclk = (gd->bus_clk + 3L) / 4L;

	return (tbclk);
}

/* ------------------------------------------------------------------------- */

static int mpc8220_bd_init(void)
{
	bd_t *bd = gd->bd;

	bd->bi_sramstart = CFG_SRAM_BASE;	/* start of  SRAM memory	*/
	bd->bi_sramsize  = CFG_SRAM_SIZE;	/* size  of  SRAM memory	*/

	bd->bi_mbar_base = CFG_MBAR;	/* base of internal registers */
	bd->bi_inpfreq   = gd->inp_clk;
	bd->bi_pcifreq   = gd->pci_clk;
	bd->bi_vcofreq   = gd->vco_clk;
	bd->bi_pevfreq   = gd->pev_clk;
	bd->bi_flbfreq   = gd->flb_clk;

	/* store bootparam to sram (backward compatible), here? */
	{
		u32 *sram = (u32 *)CFG_SRAM_BASE;
		*sram++ = gd->ram_size;
		*sram++ = gd->bus_clk;
		*sram++ = gd->inp_clk;
		*sram++ = gd->cpu_clk;
		*sram++ = gd->vco_clk;
		*sram++ = gd->flb_clk;
		*sram++ = 0xb8c3ba11;  /* boot signature */
	}
}

bd_initcall(mpc8220_bd_init);
