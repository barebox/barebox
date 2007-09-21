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
 *  Command Processor Table
 */

#include <common.h>
#include <command.h>
#include <xfuncs.h>
#include <malloc.h>
#include <environment.h>

int
do_version (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	extern char version_string[];
	printf ("\n%s\n", version_string);
	return 0;
}

U_BOOT_CMD_START(version)
	.maxargs	= 1,
	.cmd		= do_version,
	.usage		= "print monitor version",
U_BOOT_CMD_END

int
do_true (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return 0;
}

U_BOOT_CMD_START(true)
	.maxargs	= 1,
	.cmd		= do_true,
	.usage		= "do nothing, successfully",
U_BOOT_CMD_END

int
do_false (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return 1;
}

U_BOOT_CMD_START(false)
	.maxargs	= 1,
	.cmd		= do_false,
	.usage		= "do nothing, unsuccessfully",
U_BOOT_CMD_END

#ifdef CONFIG_HUSH_PARSER

int
do_readline (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *buf = xzalloc(CONFIG_CBSIZE);

	if (argc < 3) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	if (readline(argv[1], buf, CONFIG_CBSIZE) < 0) {
		free(buf);
		return 1;
	}

	setenv(argv[2], buf);
	free(buf);

	return 0;
}

static __maybe_unused char cmd_readline_help[] =
"Usage: readline <prompt> VAR\n"
"readline reads a line of user input into variable VAR.\n";

U_BOOT_CMD_START(readline)
	.maxargs	= 3,
	.cmd		= do_readline,
	.usage		= "prompt for user input",
	U_BOOT_CMD_HELP(cmd_readline_help)
U_BOOT_CMD_END

int
do_exit (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r;

	r = 0;
	if (argc > 1)
		r = simple_strtoul(argv[1], NULL, 0);

	return -r - 2;
}

U_BOOT_CMD_START(exit)
	.maxargs	= 2,
	.cmd		= do_exit,
	.usage		= "exit script",
U_BOOT_CMD_END

#endif

void u_boot_cmd_usage(cmd_tbl_t *cmdtp)
{
#ifdef	CONFIG_LONGHELP
		/* found - print (long) help info */
		if (cmdtp->help) {
			puts (cmdtp->help);
		} else {
			puts (cmdtp->name);
			putchar (' ');
			puts ("- No help available.\n");
		}
		putchar ('\n');
#else	/* no long help available */
		if (cmdtp->usage) {
			puts (cmdtp->usage);
			puts("\n");
		}
#endif	/* CONFIG_LONGHELP */
}

/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
int do_help (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	if (argc == 1) {	/*show list of commands */
		int cmd_items = &__u_boot_cmd_end -
				&__u_boot_cmd_start;	/* pointer arith! */
		int i;

		/* No need to sort the command list. The linker already did
		 * this for us.
		 */
		cmdtp = &__u_boot_cmd_start;
		for (i = 0; i < cmd_items; i++) {
			/* print short help (usage) */

			/* allow user abort */
			if (ctrlc ())
				return 1;
			if (!cmdtp->usage)
				continue;
			printf("%10s - %s\n", cmdtp->name, cmdtp->usage);
			cmdtp++;
		}
		return 0;
	}
	/*
	 * command help (long version)
	 */
	if ((cmdtp = find_cmd (argv[1])) != NULL) {
		u_boot_cmd_usage(cmdtp);
		return 0;
	} else {
		printf ("Unknown command '%s' - try 'help'"
			" without arguments for list of all"
			" known commands\n\n", argv[1]
				);
		return 1;
	}
}

static __maybe_unused char cmd_help_help[] =
"Show help information (for 'command')\n"
"'help' prints online help for the monitor commands.\n\n"
"Without arguments, it prints a short usage message for all commands.\n\n"
"To get detailed help information for specific commands you can type\n"
"'help' with one or more command names as arguments.\n";

static char *help_aliases[] = { "?", NULL};

U_BOOT_CMD_START(help)
	.maxargs	= 2,
	.cmd		= do_help,
	.aliases	= help_aliases,
	.usage		= "print online help",
	U_BOOT_CMD_HELP(cmd_help_help)
U_BOOT_CMD_END


/***************************************************************************
 * find command table entry for a command
 */
cmd_tbl_t *find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = &__u_boot_cmd_start;	/*Init value */
	int len;
	int n_found = 0;

	len = strlen (cmd);

	for (cmdtp = &__u_boot_cmd_start;
	     cmdtp != &__u_boot_cmd_end;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
		if (cmdtp->aliases) {
			char **aliases = cmdtp->aliases;
			while(*aliases) {
				if (strncmp (cmd, *aliases, len) == 0) {
					if (len == strlen (cmdtp->name))
						return cmdtp;	/* full match */

					cmdtp_temp = cmdtp;	/* abbreviated command ? */
					n_found++;
				}
				aliases++;
			}
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

#ifdef CONFIG_AUTO_COMPLETE

int var_complete(int argc, char *argv[], char last_char, int maxv, char *cmdv[])
{
	static char tmp_buf[512];
	int space;

	space = last_char == '\0' || last_char == ' ' || last_char == '\t';

	if (space && argc == 1)
		return env_complete("", maxv, cmdv, sizeof(tmp_buf), tmp_buf);

	if (!space && argc == 2)
		return env_complete(argv[1], maxv, cmdv, sizeof(tmp_buf), tmp_buf);

	return 0;
}

static void install_auto_complete_handler(const char *cmd,
		int (*complete)(int argc, char *argv[], char last_char, int maxv, char *cmdv[]))
{
	cmd_tbl_t *cmdtp;

	cmdtp = find_cmd(cmd);
	if (cmdtp == NULL)
		return;

	cmdtp->complete = complete;
}

void install_auto_complete(void)
{
	install_auto_complete_handler("printenv", var_complete);
	install_auto_complete_handler("setenv", var_complete);
#if (CONFIG_COMMANDS & CFG_CMD_RUN)
	install_auto_complete_handler("run", var_complete);
#endif
}

/*************************************************************************************/

static int complete_cmdv(int argc, char *argv[], char last_char, int maxv, char *cmdv[])
{
	cmd_tbl_t *cmdtp;
	const char *p;
	int len, clen;
	int n_found = 0;
	const char *cmd;

	/* sanity? */
	if (maxv < 2)
		return -2;

	cmdv[0] = NULL;

	if (argc == 0) {
		/* output full list of commands */
		for (cmdtp = &__u_boot_cmd_start; cmdtp != &__u_boot_cmd_end; cmdtp++) {
			if (n_found >= maxv - 2) {
				cmdv[n_found++] = "...";
				break;
			}
			cmdv[n_found++] = cmdtp->name;
		}
		cmdv[n_found] = NULL;
		return n_found;
	}

	/* more than one arg or one but the start of the next */
	if (argc > 1 || (last_char == '\0' || last_char == ' ' || last_char == '\t')) {
		cmdtp = find_cmd(argv[0]);
		if (cmdtp == NULL || cmdtp->complete == NULL) {
			cmdv[0] = NULL;
			return 0;
		}
		return (*cmdtp->complete)(argc, argv, last_char, maxv, cmdv);
	}

	cmd = argv[0];
	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	p = strchr(cmd, '.');
	if (p == NULL)
		len = strlen(cmd);
	else
		len = p - cmd;

	/* return the partial matches */
	for (cmdtp = &__u_boot_cmd_start; cmdtp != &__u_boot_cmd_end; cmdtp++) {

		clen = strlen(cmdtp->name);
		if (clen < len)
			continue;

		if (memcmp(cmd, cmdtp->name, len) != 0)
			continue;

		/* too many! */
		if (n_found >= maxv - 2) {
			cmdv[n_found++] = "...";
			break;
		}

		cmdv[n_found++] = cmdtp->name;
	}

	cmdv[n_found] = NULL;
	return n_found;
}

static int make_argv(char *s, int argvsz, char *argv[])
{
	int argc = 0;

	/* split into argv */
	while (argc < argvsz - 1) {

		/* skip any white space */
		while ((*s == ' ') || (*s == '\t'))
			++s;

		if (*s == '\0') 	/* end of s, no more args	*/
			break;

		argv[argc++] = s;	/* begin of argument string	*/

		/* find end of string */
		while (*s && (*s != ' ') && (*s != '\t'))
			++s;

		if (*s == '\0')		/* end of s, no more args	*/
			break;

		*s++ = '\0';		/* terminate current arg	 */
	}
	argv[argc] = NULL;

	return argc;
}

static void print_argv(const char *banner, const char *leader, const char *sep, int linemax, char *argv[])
{
	int ll = leader != NULL ? strlen(leader) : 0;
	int sl = sep != NULL ? strlen(sep) : 0;
	int len, i;

	if (banner) {
		puts("\n");
		puts(banner);
	}

	i = linemax;	/* force leader and newline */
	while (*argv != NULL) {
		len = strlen(*argv) + sl;
		if (i + len >= linemax) {
			puts("\n");
			if (leader)
				puts(leader);
			i = ll - sl;
		} else if (sep)
			puts(sep);
		puts(*argv++);
		i += len;
	}
	printf("\n");
}

static int find_common_prefix(char *argv[])
{
	int i, len;
	char *anchor, *s, *t;

	if (*argv == NULL)
		return 0;

	/* begin with max */
	anchor = *argv++;
	len = strlen(anchor);
	while ((t = *argv++) != NULL) {
		s = anchor;
		for (i = 0; i < len; i++, t++, s++) {
			if (*t != *s)
				break;
		}
		len = s - anchor;
	}
	return len;
}

static char tmp_buf[CONFIG_CBSIZE];	/* copy of console I/O buffer	*/

int cmd_auto_complete(const char *const prompt, char *buf, int *np, int *colp)
{
	int n = *np, col = *colp;
	char *argv[CONFIG_MAXARGS + 1];		/* NULL terminated	*/
	char *cmdv[20];
	char *s, *t;
	const char *sep;
	int i, j, k, len, seplen, argc;
	int cnt;
	char last_char;

	if (strcmp(prompt, CONFIG_PROMPT) != 0)
		return 0;	/* not in normal console */

	cnt = strlen(buf);
	if (cnt >= 1)
		last_char = buf[cnt - 1];
	else
		last_char = '\0';

	/* copy to secondary buffer which will be affected */
	strcpy(tmp_buf, buf);

	/* separate into argv */
	argc = make_argv(tmp_buf, sizeof(argv)/sizeof(argv[0]), argv);

	/* do the completion and return the possible completions */
	i = complete_cmdv(argc, argv, last_char, sizeof(cmdv)/sizeof(cmdv[0]), cmdv);

	/* no match; bell and out */
	if (i == 0) {
		if (argc > 1)	/* allow tab for non command */
			return 0;
		putchar('\a');
		return 1;
	}

	s = NULL;
	len = 0;
	sep = NULL;
	seplen = 0;
	if (i == 1) { /* one match; perfect */
		k = strlen(argv[argc - 1]);
		s = cmdv[0] + k;
		len = strlen(s);
		sep = " ";
		seplen = 1;
	} else if (i > 1 && (j = find_common_prefix(cmdv)) != 0) {	/* more */
		k = strlen(argv[argc - 1]);
		j -= k;
		if (j > 0) {
			s = cmdv[0] + k;
			len = j;
		}
	}

	if (s != NULL) {
		k = len + seplen;
		/* make sure it fits */
		if (n + k >= CONFIG_CBSIZE - 2) {
			putchar('\a');
			return 1;
		}

		t = buf + cnt;
		for (i = 0; i < len; i++)
			*t++ = *s++;
		if (sep != NULL)
			for (i = 0; i < seplen; i++)
				*t++ = sep[i];
		*t = '\0';
		n += k;
		col += k;
		puts(t - k);
		if (sep == NULL)
			putchar('\a');
		*np = n;
		*colp = col;
	} else {
		print_argv(NULL, "  ", " ", 78, cmdv);

		puts(prompt);
		puts(buf);
	}
	return 1;
}

#endif
