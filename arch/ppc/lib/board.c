/*
 * (C) Copyright 2000-2006
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

#include <debug_ll.h>
#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <malloc.h>
#include <mem_malloc.h>
#include <init.h>
#include <devices.h>
#include <net.h>
#include <serial.h>

gd_t *gd;

char *strmhz (char *buf, long hz)
{
	long l, n;
	long m;

	n = hz / 1000000L;
	l = sprintf (buf, "%ld", n);
	m = (hz % 1000000L) / 1000L;
	if (m != 0)
		sprintf (buf + l, ".%03ld", m);
	return (buf);
}

/***********************************************************************/

static void init_bd(bd_t *bd, ulong bootflag) {
//	bd->bi_memstart  = CFG_SDRAM_BASE;	/* start of  DRAM memory	*/
//	bd->bi_memsize   = gd->ram_size;	/* size  of  DRAM memory in bytes */

#ifdef CONFIG_IP860
//	bd->bi_sramstart = SRAM_BASE;	/* start of  SRAM memory	*/
//	bd->bi_sramsize  = SRAM_SIZE;	/* size  of  SRAM memory	*/
#else
//	bd->bi_sramstart = 0;		/* FIXME */ /* start of  SRAM memory	*/
//	bd->bi_sramsize  = 0;		/* FIXME */ /* size  of  SRAM memory	*/
#endif

#if defined(CONFIG_8xx) || defined(CONFIG_8260) || defined(CONFIG_5xx) || \
    defined(CONFIG_E500) || defined(CONFIG_MPC86xx)
	bd->bi_immr_base = CFG_IMMR;	/* base  of IMMR register     */
#endif

	bd->bi_bootflags = bootflag;	/* boot / reboot flag (for LynxOS)    */

	WATCHDOG_RESET ();
	bd->bi_intfreq = gd->cpu_clk;	/* Internal Freq, in Hz */
	bd->bi_busfreq = gd->bus_clk;	/* Bus Freq,      in Hz */
#if defined(CONFIG_CPM2)
	bd->bi_cpmfreq = gd->cpm_clk;
	bd->bi_brgfreq = gd->brg_clk;
	bd->bi_sccfreq = gd->scc_clk;
	bd->bi_vco     = gd->vco_out;
#endif /* CONFIG_CPM2 */
	bd->bi_baudrate = gd->baudrate;	/* Console Baudrate     */

#ifdef CFG_EXTBDINFO
	strncpy ((char *)bd->bi_s_version, "1.2", sizeof (bd->bi_s_version));
	strncpy ((char *)bd->bi_r_version, U_BOOT_VERSION, sizeof (bd->bi_r_version));

	bd->bi_procfreq = gd->cpu_clk;	/* Processor Speed, In Hz */
	bd->bi_plb_busfreq = gd->bus_clk;
#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || defined(CONFIG_440EP) || defined(CONFIG_440GR)
	bd->bi_pci_busfreq = get_PCI_freq ();
	bd->bi_opbfreq = get_OPB_freq ();
#elif defined(CONFIG_XILINX_ML300)
	bd->bi_pci_busfreq = get_PCI_freq ();
#endif
#endif
}

/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */
void board_init_r (ulong end_of_ram)
{
	bd_t *bd;
        char *x;
	extern void malloc_bin_reloc (void);

        asm ("sync ; isync");

        mem_malloc_init((void *)(end_of_ram - 4096 - CFG_MALLOC_LEN), (void *)(end_of_ram - 4096));

        /* get gd and bd */
        gd = malloc(sizeof(gd_t));
        memset(gd, 0x0, sizeof(gd_t));
        bd = malloc(sizeof(bd_t));
        memset(bd, 0x0, sizeof(bd_t));

        gd->bd = bd;

	/*
	 * Setup trap handlers
	 */
	trap_init (0);

	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r ();

	/*
	 * Enable Interrupts
	 */
	interrupt_init ();

	/* Initialization complete - start the monitor */

	start_uboot();
}

