/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Add to readline cmdline-editing by
 * (C) Copyright 2005
 * JinHua Luo, GuangDong Linux Center, <luo.jinhua@gd-linux.com>
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

/* #define	DEBUG	*/

#include <common.h>
#include <watchdog.h>
#include <command.h>
#ifdef CONFIG_MODEM_SUPPORT
#include <malloc.h>		/* for free() prototype */
#endif

#ifdef CONFIG_HUSH_PARSER
#include <hush.h>
#endif

#include <post.h>

#if defined(CONFIG_BOOT_RETRY_TIME) && defined(CONFIG_RESET_TO_RETRY)
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);		/* for do_reset() prototype */
#endif

extern int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);


#define MAX_DELAY_STOP_STR 32

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
static int abortboot(int);
#endif

#undef DEBUG_PARSER

char        console_buffer[CONFIG_CBSIZE];		/* console I/O buffer	*/

#ifdef CONFIG_BOOT_RETRY_TIME
static uint64_t endtime = 0;  /* must be set, default is instant timeout */
static int      retry_time = -1; /* -1 so can call readline before main_loop */
#endif

#ifndef CONFIG_BOOT_RETRY_MIN
#define CONFIG_BOOT_RETRY_MIN CONFIG_BOOT_RETRY_TIME
#endif

#ifdef CONFIG_MODEM_SUPPORT
int do_mdm_init = 0;
extern void mdm_init(void); /* defined in board.c */
#endif

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot
 *          1 - got key string, abort
 */
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
# if defined(CONFIG_AUTOBOOT_KEYED)
static __inline__ int abortboot(int bootdelay)
{
	int abort = 0;
	ulong etime;

	struct {
		char* str;
		u_int len;
		int retry;
	}
	delaykey [] = {
		{ str: getenv ("bootdelaykey"),  retry: 1 },
		{ str: getenv ("bootdelaykey2"), retry: 1 },
		{ str: getenv ("bootstopkey"),   retry: 0 },
		{ str: getenv ("bootstopkey2"),  retry: 0 },
	};

	char presskey [MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT) {
		/* Restore serial console */
		console_assign (stdout, "serial");
		console_assign (stderr, "serial");
	}
#endif

#  ifdef CONFIG_AUTOBOOT_PROMPT
	printf (CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_DELAY_STR2
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_DELAY_STR2;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[2].str == NULL)
		delaykey[2].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR2
	if (delaykey[3].str == NULL)
		delaykey[3].str = CONFIG_AUTOBOOT_STOP_STR2;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen (delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

#  if DEBUG_BOOTKEYS
		printf("%s key:<%s>\n",
		       delaykey[i].retry ? "delay" : "stop",
		       delaykey[i].str ? delaykey[i].str : "NULL");
#  endif
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	etime = get_timer(0);
	while (!abort && get_timer(etime) <= bootdelay * CFG_HZ) {
		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
			    memcmp (presskey + presskey_len - delaykey[i].len,
				    delaykey[i].str,
				    delaykey[i].len) == 0) {
#  if DEBUG_BOOTKEYS
				printf("got %skey\n",
				       delaykey[i].retry ? "delay" : "stop");
#  endif

#  ifdef CONFIG_BOOT_RETRY_TIME
				/* don't retry auto boot */
				if (! delaykey[i].retry)
					retry_time = -1;
#  endif
				abort = 1;
			}
		}

		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey [presskey_len ++] = getc();
			}
			else {
				for (i = 0; i < presskey_max - 1; i ++)
					presskey [i] = presskey [i + 1];

				presskey [i] = getc();
			}
		}
	}
#  if DEBUG_BOOTKEYS
	if (!abort)
		puts ("key timeout\n");
#  endif

#ifdef CONFIG_SILENT_CONSOLE
	if (abort) {
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if (gd->flags & GD_FLG_SILENT) {
		/* Restore silent console */
		console_assign (stdout, "nulldev");
		console_assign (stderr, "nulldev");
	}
#endif

	return abort;
}

# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */

#ifdef CONFIG_MENUKEY
static int menukey = 0;
#endif

static __inline__ int abortboot(int bootdelay)
{
	int abort = 0;

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT) {
		/* Restore serial console */
		console_assign (stdout, "serial");
		console_assign (stderr, "serial");
	}
#endif

#ifdef CONFIG_MENUPROMPT
	printf(CONFIG_MENUPROMPT, bootdelay);
#else
	printf("Hit any key to stop autoboot: %2d ", bootdelay);
#endif

#if defined CONFIG_ZERO_BOOTDELAY_CHECK
	/*
	 * Check if key already pressed
	 * Don't check if bootdelay < 0
	 */
	if (bootdelay >= 0) {
		if (tstc()) {	/* we got a key press	*/
			(void) getc();  /* consume input	*/
			puts ("\b\b\b 0");
			abort = 1; 	/* don't auto boot	*/
		}
	}
#endif

	while ((bootdelay > 0) && (!abort)) {
		int i;

		--bootdelay;
		/* delay 100 * 10ms */
		for (i=0; !abort && i<100; ++i) {
			if (tstc()) {	/* we got a key press	*/
				abort  = 1;	/* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
# ifdef CONFIG_MENUKEY
				menukey = getc();
# else
				(void) getc();  /* consume input	*/
# endif
				break;
			}
			udelay (10000);
		}

		printf ("\b\b\b%2d ", bootdelay);
	}

	putc ('\n');

#ifdef CONFIG_SILENT_CONSOLE
	if (abort) {
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if (gd->flags & GD_FLG_SILENT) {
		/* Restore silent console */
		console_assign (stdout, "nulldev");
		console_assign (stderr, "nulldev");
	}
#endif

	return abort;
}
# endif	/* CONFIG_AUTOBOOT_KEYED */
#endif	/* CONFIG_BOOTDELAY >= 0  */

/****************************************************************************/

void main_loop (void)
{
#ifndef CONFIG_HUSH_PARSER
	static char lastcommand[CONFIG_CBSIZE] = { 0, };
	int len;
	int rc = 1;
	int flag;
#endif

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	char *s;
	int bootdelay;
#endif
#ifdef CONFIG_PREBOOT
	char *p;
#endif
#ifdef CONFIG_BOOTCOUNT_LIMIT
	unsigned long bootcount = 0;
	unsigned long bootlimit = 0;
	char *bcs;
	char bcs_set[16];
#endif /* CONFIG_BOOTCOUNT_LIMIT */

#ifdef CONFIG_BOOTCOUNT_LIMIT
	bootcount = bootcount_load();
	bootcount++;
	bootcount_store (bootcount);
	sprintf (bcs_set, "%lu", bootcount);
	setenv ("bootcount", bcs_set);
	bcs = getenv ("bootlimit");
	bootlimit = bcs ? simple_strtoul (bcs, NULL, 10) : 0;
#endif /* CONFIG_BOOTCOUNT_LIMIT */

#ifdef CONFIG_HUSH_PARSER
	u_boot_hush_start ();
#endif

#ifdef CONFIG_AUTO_COMPLETE
	install_auto_complete();
#endif

#ifdef CONFIG_PREBOOT
	if ((p = getenv ("preboot")) != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

# ifndef CONFIG_HUSH_PARSER
		run_command (p, 0);
# else
		parse_string_outer(p, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	s = getenv ("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	debug ("### main_loop entered: bootdelay=%d\n\n", bootdelay);

# ifdef CONFIG_BOOT_RETRY_TIME
	init_cmd_timeout ();
# endif	/* CONFIG_BOOT_RETRY_TIME */

#ifdef CONFIG_BOOTCOUNT_LIMIT
	if (bootlimit && (bootcount > bootlimit)) {
		printf ("Warning: Bootlimit (%u) exceeded. Using altbootcmd.\n",
		        (unsigned)bootlimit);
		s = getenv ("altbootcmd");
	}
	else
#endif /* CONFIG_BOOTCOUNT_LIMIT */
		s = getenv ("bootcmd");

	debug ("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (bootdelay >= 0 && s && !abortboot (bootdelay)) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

# ifndef CONFIG_HUSH_PARSER
		run_command (s, 0);
# else
		parse_string_outer(s, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}

# ifdef CONFIG_MENUKEY
	if (menukey == CONFIG_MENUKEY) {
	    s = getenv("menucmd");
	    if (s) {
# ifndef CONFIG_HUSH_PARSER
		run_command (s, 0);
# else
		parse_string_outer(s, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif
	    }
	}
#endif /* CONFIG_MENUKEY */
#endif	/* CONFIG_BOOTDELAY */

	/*
	 * Main Loop for Monitor Command Processing
	 */
#ifdef CONFIG_HUSH_PARSER
	parse_file_outer();
	/* This point is never reached */
	for (;;);
#else
	for (;;) {
#ifdef CONFIG_BOOT_RETRY_TIME
		if (rc >= 0) {
			/* Saw enough of a valid command to
			 * restart the timeout.
			 */
			reset_cmd_timeout();
		}
#endif
		len = readline (CONFIG_PROMPT);

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (len == -2) {
			/* -2 means timed out, retry autoboot
			 */
			puts ("\nTimed out waiting for command\n");
# ifdef CONFIG_RESET_TO_RETRY
			/* Reinit board to run initialization code again */
			do_reset (NULL, 0, 0, NULL);
# else
			return;		/* retry autoboot */
# endif
		}
#endif

		if (len == -1)
			puts ("<INTERRUPT>\n");
		else
			rc = run_command (lastcommand, flag);

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
#endif /*CONFIG_HUSH_PARSER*/
}

#ifdef CONFIG_BOOT_RETRY_TIME
/***************************************************************************
 * initialize command line timeout
 */
void init_cmd_timeout(void)
{
	char *s = getenv ("bootretry");

	if (s != NULL)
		retry_time = (int)simple_strtol(s, NULL, 10);
	else
		retry_time =  CONFIG_BOOT_RETRY_TIME;

	if (retry_time >= 0 && retry_time < CONFIG_BOOT_RETRY_MIN)
		retry_time = CONFIG_BOOT_RETRY_MIN;
}

/***************************************************************************
 * reset command line timeout to retry_time seconds
 */
void reset_cmd_timeout(void)
{
	endtime = endtick(retry_time);
}
#endif

/****************************************************************************/

/****************************************************************************/

int do_run (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	for (i=1; i<argc; ++i) {
		char *arg;

		if ((arg = getenv (argv[i])) == NULL) {
			printf ("## Error: \"%s\" not defined\n", argv[i]);
			return 1;
		}
#ifndef CONFIG_HUSH_PARSER
		if (run_command (arg, flag) == -1)
			return 1;
#else
		if (parse_string_outer(arg,
		    FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0)
			return 1;
#endif
	}
	return 0;
}

int do_run (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
U_BOOT_CMD(
	run,	CONFIG_MAXARGS,	1,	do_run,
	"run     - run commands in an environment variable\n",
	"var [...]\n"
	"    - run the commands in the environment variable(s) 'var'\n"
);
