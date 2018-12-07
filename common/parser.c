#include <common.h>
#include <command.h>
#include <password.h>
#include <environment.h>
#include <shell.h>

/*
 * not yet supported
 */
int shell_get_last_return_code(void)
{
	return 0;
}

static int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	pr_debug("parse_line: \"%s\"\n", line);

	while (nargs < CONFIG_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;

		pr_debug("parse_line: nargs=%d\n", nargs);

			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;

		pr_debug("parse_line: nargs=%d\n", nargs);

			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf ("** Too many args (max. %d) **\n", CONFIG_MAXARGS);
	pr_debug("parse_line: nargs=%d\n", nargs);

	return (nargs);
}

static void process_macros (const char *input, char *output)
{
	char c, prev;
	const char *varname_start = NULL;
	int inputcnt = strlen (input);
	int outputcnt = CONFIG_CBSIZE;
	int state = 0;		/* 0 = waiting for '$'  */

	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */
	char __maybe_unused *output_start = output;

	pr_debug("[PROCESS_MACROS] INPUT len %zu: \"%s\"\n", strlen (input),
		input);

	prev = '\0';		/* previous character   */

	while (inputcnt && outputcnt) {
		c = *input++;
		inputcnt--;

		if (state != 3) {
			/* remove one level of escape characters */
			if ((c == '\\') && (prev != '\\')) {
				if (inputcnt-- == 0)
					break;
				prev = c;
				c = *input++;
			}
		}

		switch (state) {
		case 0:	/* Waiting for (unescaped) $    */
			if ((c == '\'') && (prev != '\\')) {
				state = 3;
				break;
			}
			if ((c == '$') && (prev != '\\')) {
				state++;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		case 1:	/* Waiting for (        */
			if (c == '(' || c == '{') {
				state++;
				varname_start = input;
			} else {
				state = 0;
				*(output++) = '$';
				outputcnt--;

				if (outputcnt) {
					*(output++) = c;
					outputcnt--;
				}
			}
			break;
		case 2:	/* Waiting for )        */
			if (c == ')' || c == '}') {
				int i;
				char envname[CONFIG_CBSIZE];
				const char *envval;
				int envcnt = input - varname_start - 1;	/* Varname # of chars */

				/* Get the varname */
				for (i = 0; i < envcnt; i++) {
					envname[i] = varname_start[i];
				}
				envname[i] = 0;

				/* Get its value */
				envval = getenv (envname);

				/* Copy into the line if it exists */
				if (envval != NULL)
					while ((*envval) && outputcnt) {
						*(output++) = *(envval++);
						outputcnt--;
					}
				/* Look for another '$' */
				state = 0;
			}
			break;
		case 3:	/* Waiting for '        */
			if ((c == '\'') && (prev != '\\')) {
				state = 0;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		}
		prev = c;
	}

	if (outputcnt)
		*output = 0;

	pr_debug("[PROCESS_MACROS] OUTPUT len %zu: \"%s\"\n",
		strlen (output_start), output_start);
}

/****************************************************************************
 * returns:
 *	0  - command executed
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CONFIG_CBSIZE-1 it is
 *           considered unrecognized)
 *
 * WARNING:
 *
 * We must create a temporary copy of the command since the command we get
 * may be the result from getenv(), which returns a pointer directly to
 * the environment data, which may change magicly when the command we run
 * creates or modifies environment variables (like "bootp" does).
 */

int run_command(const char *cmd)
{
	char cmdbuf[CONFIG_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CONFIG_CBSIZE];
	char *str = cmdbuf;
	char *argv[CONFIG_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int rc = 0;

#ifdef DEBUG
	pr_debug("[RUN_COMMAND] cmd[%p]=\"", cmd);
	puts (cmd ? cmd : "NULL");	/* use puts - string may be loooong */
	puts ("\"\n");
#endif

	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CONFIG_CBSIZE) {
		puts ("## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/*
	 * Process separators and check for invalid
	 * repeatable commands
	 */

	pr_debug("[PROCESS_SEPARATORS] %s\n", cmd);

	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else {
			str = sep;	/* no more commands for next pass */
		}

		pr_debug("token: \"%s\"\n", token);

		/* find macros in this token and replace them */
		process_macros (token, finaltoken);

		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}

		if (execute_command(argc, argv) != COMMAND_SUCCESS)
			rc = -1;
	}

	return rc;
}

static char console_buffer[CONFIG_CBSIZE];		/* console I/O buffer	*/

int run_shell(void)
{
	static char lastcommand[CONFIG_CBSIZE] = { 0, };
	int len;

	login();

	for (;;) {
		len = readline (CONFIG_PROMPT, console_buffer, CONFIG_CBSIZE);

		if (len > 0)
			strcpy (lastcommand, console_buffer);

		if (len == -1) {
			puts ("<INTERRUPT>\n");
		} else {
			const int rc = run_command(lastcommand);
			if (rc < 0) {
				/* invalid command or not repeatable, forget it */
				lastcommand[0] = 0;
			}
		}
	}
	return 0;
}
