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
#ifdef CONFIG_8xx
#include <mpc8xx.h>
#endif
#ifdef CONFIG_5xx
#include <mpc5xx.h>
#endif
#ifdef CONFIG_MPC5xxx
#include <mpc5xxx.h>
#endif
#if (CONFIG_COMMANDS & CFG_CMD_IDE)
#include <ide.h>
#endif
#if (CONFIG_COMMANDS & CFG_CMD_SCSI)
#include <scsi.h>
#endif
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#include <kgdb.h>
#endif
#ifdef CONFIG_STATUS_LED
#include <status_led.h>
#endif
#include <net.h>
#include <serial.h>
#ifdef CFG_ALLOC_DPRAM
#if !defined(CONFIG_CPM2)
#include <commproc.h>
#endif
#endif
#include <version.h>
#if defined(CONFIG_BAB7xx)
#include <w83c553f.h>
#endif
#include <dtt.h>
#if defined(CONFIG_POST)
#include <post.h>
#endif
#if defined(CONFIG_LOGBUFFER)
#include <logbuff.h>
#endif
#if defined(CFG_INIT_RAM_LOCK) && defined(CONFIG_E500)
#include <asm/cache.h>
#endif
#ifdef CONFIG_PS2KBD
#include <keyboard.h>
#endif

#ifdef CFG_UPDATE_FLASH_SIZE
extern int update_flash_size (int flash_size);
#endif

#if (CONFIG_COMMANDS & CFG_CMD_DOC)
void doc_init (void);
#endif
#if defined(CONFIG_HARD_I2C) || \
    defined(CONFIG_SOFT_I2C)
#include <i2c.h>
#endif
#if (CONFIG_COMMANDS & CFG_CMD_NAND)
void nand_init (void);
#endif

static char *failed = "*** failed ***\n";

#if defined(CONFIG_OXC) || defined(CONFIG_PCU_E) || defined(CONFIG_RMU)
extern flash_info_t flash_info[];
#endif

#include <environment.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CFG_ENV_IS_EMBEDDED)
#define TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#elif ( ((CFG_ENV_ADDR+CFG_ENV_SIZE) < CFG_MONITOR_BASE) || \
	(CFG_ENV_ADDR >= (CFG_MONITOR_BASE + CFG_MONITOR_LEN)) ) || \
      defined(CFG_ENV_IS_IN_NVRAM)
#define	TOTAL_MALLOC_LEN	(CFG_MALLOC_LEN + CFG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#endif

extern ulong __init_end;
extern ulong _end;
ulong monitor_flash_len;

#if (CONFIG_COMMANDS & CFG_CMD_BEDBUG)
#include <bedbug/type.h>
#endif

int ppc_mem_alloc_init(void)
{
	ulong dest_addr = CFG_MONITOR_BASE + gd->reloc_off;

        mem_alloc_init(dest_addr - TOTAL_MALLOC_LEN,
			dest_addr);
}

core_initcall(ppc_mem_alloc_init);

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

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

/***********************************************************************/

#ifdef CONFIG_ADD_RAM_INFO
void board_add_ram_info(int);
#endif

static void init_bd(bd_t *bd, ulong bootflag) {
	bd->bi_memstart  = CFG_SDRAM_BASE;	/* start of  DRAM memory	*/
	bd->bi_memsize   = gd->ram_size;	/* size  of  DRAM memory in bytes */

#ifdef CONFIG_IP860
	bd->bi_sramstart = SRAM_BASE;	/* start of  SRAM memory	*/
	bd->bi_sramsize  = SRAM_SIZE;	/* size  of  SRAM memory	*/
#else
	bd->bi_sramstart = 0;		/* FIXME */ /* start of  SRAM memory	*/
	bd->bi_sramsize  = 0;		/* FIXME */ /* size  of  SRAM memory	*/
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
 * This is the first part of the initialization sequence that is
 * implemented in C, but still running from ROM.
 *
 * The main purpose is to provide a (serial) console interface as
 * soon as possible (so we can see any error messages), and to
 * initialize the RAM so that we can relocate the monitor code to
 * RAM.
 *
 * Be aware of the restrictions: global data is read-only, BSS is not
 * initialized, and stack space is limited to a few kB.
 *
 ************************************************************************
 */

void board_init_f (ulong bootflag)
{
	bd_t *bd;
	ulong len, addr, addr_sp;
	ulong *s;
	gd_t *id;
	init_fnc_t **init_fnc_ptr;

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) (CFG_INIT_RAM_ADDR + CFG_GBL_DATA_OFFSET);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	/* Clear initial global data */
	memset ((void *) gd, 0, sizeof (gd_t));

        /*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 *
	 * Reserve memory at end of RAM for (top down in that order):
	 *  - kernel log buffer
	 *  - protected RAM
	 *  - LCD framebuffer
	 *  - monitor code
	 *  - board info struct
	 */
	len = (ulong)&_end - CFG_MONITOR_BASE;

	addr = CFG_SDRAM_BASE + initdram (0);

        /*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= len;
	addr &= ~(4096 - 1);
#ifdef CONFIG_E500
	/* round down to next 64 kB limit so that IVPR stays aligned */
	addr &= ~(65536 - 1);
#endif

PUTHEX_LL(addr);
PUTC(':');
	debug ("Reserving %ldk for U-Boot at: %08lx\n", len >> 10, addr);

	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug ("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug ("Reserving %d Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);
	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug ("Reserving %d Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;
	s = (ulong *)addr_sp;
	*s-- = 0;
	*s-- = 0;
	addr_sp = (ulong)s;
	debug ("Stack Pointer at: %08lx\n", addr_sp);

	/*
	 * Save local variables to board info struct
	 */
	init_bd(bd, bootflag);

	debug ("New Stack Pointer is: %08lx\n", addr_sp);

	WATCHDOG_RESET ();

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run (NULL, POST_ROM | post_bootmode_get(0));
#endif

	WATCHDOG_RESET();

//	memcpy (id, (void *)gd, sizeof (gd_t));
PUTHEX_LL(addr_sp);
PUTC(':');
PUTHEX_LL(id);
PUTC(':');
PUTHEX_LL(addr);
PUTC(':');

	relocate_code (addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
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
void board_init_r (gd_t *id, ulong dest_addr)
{
	cmd_tbl_t *cmdtp;
	char *s, *e;
	bd_t *bd;
	int i;
	extern void malloc_bin_reloc (void);
PUTC('B');
while(1);

	gd = id;		/* initialize RAM version of global data */
	bd = gd->bd;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */
	gd->reloc_off = dest_addr - CFG_MONITOR_BASE;

	debug ("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

	WATCHDOG_RESET ();

#if defined(CONFIG_BOARD_EARLY_INIT_R)
	board_early_init_r ();
#endif

	monitor_flash_len = (ulong)&__init_end - dest_addr;

	/*
	 * We have to relocate the command table manually
	 */
	for (cmdtp = &__u_boot_cmd_start; cmdtp !=  &__u_boot_cmd_end; cmdtp++) {
		ulong addr;
		addr = (ulong) (cmdtp->cmd) + gd->reloc_off;
		cmdtp->cmd =
			(int (*)(struct cmd_tbl_s *, int, int, char *[]))addr;

		addr = (ulong)(cmdtp->name) + gd->reloc_off;
		cmdtp->name = (char *)addr;

		if (cmdtp->usage) {
			addr = (ulong)(cmdtp->usage) + gd->reloc_off;
			cmdtp->usage = (char *)addr;
		}
#ifdef	CFG_LONGHELP
		if (cmdtp->help) {
			addr = (ulong)(cmdtp->help) + gd->reloc_off;
			cmdtp->help = (char *)addr;
		}
#endif
	}

	WATCHDOG_RESET ();

	asm ("sync ; isync");

	/*
	 * Setup trap handlers
	 */
	trap_init (dest_addr);

	WATCHDOG_RESET ();

	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r ();

	WATCHDOG_RESET ();

	malloc_bin_reloc ();

	WATCHDOG_RESET ();

	/*
	 * Enable Interrupts
	 */
	interrupt_init ();

#ifdef CONFIG_STATUS_LED
	status_led_set (STATUS_LED_BOOT, STATUS_LED_BLINKING);
#endif

	/* Initialization complete - start the monitor */

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		WATCHDOG_RESET ();
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
#ifdef CONFIG_SHOW_BOOT_PROGRESS
	show_boot_progress(-30);
#endif
	for (;;);
}
