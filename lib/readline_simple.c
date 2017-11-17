#include <common.h>

static char erase_seq[] = "\b \b";		/* erase sequence	*/
static char   tab_seq[] = "        ";		/* used to expand TABs	*/

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putchar (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}

/*
 * Prompt for input and read a line.
 * Return:	number of read characters
 *		-1 if break
 */
int readline (const char *prompt, char *line, int len)
{
	char   *p = line;
	int	n = 0;				/* buffer index		*/
	int	plen = 0;			/* prompt length	*/
	int	col;				/* output column cnt	*/
	char	c;

	/* print prompt */
	if (prompt) {
		plen = strlen (prompt);
		puts (prompt);
	}
	col = plen;

	for (;;) {
		c = getchar();
		if (c < 0)
			return (-1);

		/*
		 * Special character handling
		 */
		switch (c) {
		case '\r':				/* Enter		*/
		case '\n':
			*p = '\0';
			puts ("\r\n");
			return (p - line);

		case '\0':				/* nul			*/
			continue;

		case 0x03:				/* ^C - break		*/
			line[0] = '\0';	/* discard input */
			return (-1);

		case 0x15:				/* ^U - erase line	*/
			while (col > plen) {
				puts (erase_seq);
				--col;
			}
			p = line;
			n = 0;
			continue;

		case 0x17:				/* ^W - erase word 	*/
			p=delete_char(line, p, &col, &n, plen);
			while ((n > 0) && (*p != ' ')) {
				p=delete_char(line, p, &col, &n, plen);
			}
			continue;

		case 0x08:				/* ^H  - backspace	*/
		case 0x7F:				/* DEL - backspace	*/
			p=delete_char(line, p, &col, &n, plen);
			continue;

		default:
			/*
			 * Must be a normal character then
			 */
			if (n < len-2) {
				if (c == '\t') {	/* expand TABs		*/
					puts (tab_seq+(col&07));
					col += 8 - (col&07);
				} else {
					++col;		/* echo input		*/
					putchar (c);
				}
				*p++ = c;
				++n;
			} else {			/* Buffer full		*/
				putchar ('\a');
			}
		}
	}
}

/**
 * @file
 * @brief Primitiv Line Parser
 */

/** @page readline_parser Primitive Line Parser
 *
 * There is still a primtive line parser as a alternative to the hush shell
 * environment available. This is for persons who like the old fashion way of
 * edititing and command entering.
 *
 * Enable the "Simple parser" in "General Settings", "Select your shell" to
 * get back the old console feeling.
 *
 */
