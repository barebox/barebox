#include <common.h>
#include <readkey.h>
#include <init.h>
#include <xfuncs.h>
#include <complete.h>
#include <linux/ctype.h>

/*
 * cmdline-editing related codes from vivi.
 * Author: Janghoon Lyu <nandy@mizi.com>
 */

#define putnstr(str,n)	do {			\
		printf ("%.*s", n, str);	\
	} while (0)

#define MAX_CMDBUF_SIZE		256

#define CTL_BACKSPACE		('\b')
#define DEL			255
#define DEL7			127
#define CREAD_HIST_CHAR		('!')

#define getcmd_putch(ch)	putchar(ch)
#define getcmd_getch()		getc()
#define getcmd_cbeep()		getcmd_putch('\a')

#define HIST_MAX		20
#define HIST_SIZE		MAX_CMDBUF_SIZE

static int hist_max = 0;
static int hist_add_idx = 0;
static int hist_cur = -1;
static unsigned hist_num = 0;

static char* hist_list[HIST_MAX];
static char hist_lines[HIST_MAX][HIST_SIZE];

#define add_idx_minus_one() ((hist_add_idx == 0) ? hist_max : hist_add_idx-1)

static int hist_init(void)
{
	int i;

	hist_max = 0;
	hist_add_idx = 0;
	hist_cur = -1;
	hist_num = 0;

	for (i = 0; i < HIST_MAX; i++) {
		hist_list[i] = hist_lines[i];
		hist_list[i][0] = '\0';
	}
	return 0;
}

postcore_initcall(hist_init);

static void cread_add_to_hist(char *line)
{
	strcpy(hist_list[hist_add_idx], line);

	if (++hist_add_idx >= HIST_MAX)
		hist_add_idx = 0;

	if (hist_add_idx > hist_max)
		hist_max = hist_add_idx;

	hist_num++;
}

static char* hist_prev(void)
{
	char *ret;
	int old_cur;

	if (hist_cur < 0)
		return NULL;

	old_cur = hist_cur;
	if (--hist_cur < 0)
		hist_cur = hist_max;

	if (hist_cur == hist_add_idx) {
		hist_cur = old_cur;
		ret = NULL;
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

static char* hist_next(void)
{
	char *ret;

	if (hist_cur < 0)
		return NULL;

	if (hist_cur == hist_add_idx)
		return NULL;

	if (++hist_cur > hist_max)
		hist_cur = 0;

	if (hist_cur == hist_add_idx) {
		ret = "";
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

#define BEGINNING_OF_LINE() {			\
	while (num) {				\
		getcmd_putch(CTL_BACKSPACE);	\
		num--;				\
	}					\
}

#define ERASE_TO_EOL() {				\
	if (num < eol_num) {				\
		int t;					\
		for (t = num; t < eol_num; t++)		\
			getcmd_putch(' ');		\
		while (t-- > num)			\
			getcmd_putch(CTL_BACKSPACE);	\
		eol_num = num;				\
	}						\
}

#define REFRESH_TO_EOL() {			\
	if (num < eol_num) {			\
		wlen = eol_num - num;		\
		putnstr(buf + num, wlen);	\
		num = eol_num;			\
	}					\
}

#define DO_BACKSPACE()						\
		wlen = eol_num - num;				\
		num--;						\
		memmove(buf + num, buf + num + 1, wlen);	\
		getcmd_putch(CTL_BACKSPACE);			\
		putnstr(buf + num, wlen);			\
		getcmd_putch(' ');				\
		do {						\
			getcmd_putch(CTL_BACKSPACE);		\
		} while (wlen--);				\
		eol_num--;

static void cread_add_char(char ichar, int insert, unsigned long *num,
	       unsigned long *eol_num, char *buf, unsigned long len)
{
	unsigned wlen;

	/* room ??? */
	if (insert || *num == *eol_num) {
		if (*eol_num > len - 1) {
			getcmd_cbeep();
			return;
		}
		(*eol_num)++;
	}

	if (insert) {
		wlen = *eol_num - *num;
		if (wlen > 1) {
			memmove(&buf[*num+1], &buf[*num], wlen-1);
		}

		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
		while (--wlen) {
			getcmd_putch(CTL_BACKSPACE);
		}
	} else {
		/* echo the character */
		wlen = 1;
		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
	}
}

int readline(const char *prompt, char *buf, int len)
{
	unsigned long num = 0;
	unsigned long eol_num = 0;
	unsigned wlen;
	int ichar;
	int insert = 1;
	int rc = 0;
#ifdef CONFIG_AUTO_COMPLETE
	char tmp;
	int reprint, i;
	char *completestr;

	complete_reset();
#endif
	puts (prompt);

	while (1) {
		ichar = read_key();

		if ((ichar == '\n') || (ichar == '\r')) {
			putchar('\n');
			break;
		}

		switch (ichar) {
		case '\t':
#ifdef CONFIG_AUTO_COMPLETE
			buf[eol_num] = 0;
			tmp = buf[num];

			buf[num] = 0;
			reprint = complete(buf, &completestr);
			buf[num] = tmp;

			if (reprint) {
				printf("%s%s", prompt, buf);

				if (tmp)
					for (i = 0; i < eol_num - num; i++)
						getcmd_putch(CTL_BACKSPACE);
			}

			i = 0;
			while (completestr[i])
				cread_add_char(completestr[i++], insert, &num,
						&eol_num, buf, len);
#endif
			break;

		case KEY_HOME:
			BEGINNING_OF_LINE();
			break;
		case CTL_CH('c'):	/* ^C - break */
			*buf = 0;	/* discard input */
			return -1;
		case KEY_RIGHT:
			if (num < eol_num) {
				getcmd_putch(buf[num]);
				num++;
			}
			break;
		case KEY_LEFT:
			if (num) {
				getcmd_putch(CTL_BACKSPACE);
				num--;
			}
			break;
		case CTL_CH('d'):
			if (num < eol_num) {
				wlen = eol_num - num - 1;
				if (wlen) {
					memmove(&buf[num], &buf[num+1], wlen);
					putnstr(buf + num, wlen);
				}

				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case KEY_ERASE_TO_EOL:
			ERASE_TO_EOL();
			break;
		case KEY_REFRESH_TO_EOL:
		case KEY_END:
			REFRESH_TO_EOL();
			break;
		case KEY_INSERT:
			insert = !insert;
			break;
		case KEY_ERASE_LINE:
			BEGINNING_OF_LINE();
			ERASE_TO_EOL();
			break;
		case DEL:
		case KEY_DEL7:
		case 8:
			if (num) {
				DO_BACKSPACE();
			}
			break;
		case KEY_DEL:
			if (num < eol_num) {
				wlen = eol_num - num;
				memmove(buf + num, buf + num + 1, wlen);
				putnstr(buf + num, wlen - 1);
				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (--wlen);
				eol_num--;
			}
			break;
		case KEY_UP:
		case KEY_DOWN:
		{
			char * hline;

			if (ichar == KEY_UP)
				hline = hist_prev();
			else
				hline = hist_next();

			if (!hline) {
				getcmd_cbeep();
				continue;
			}

			/* nuke the current line */
			/* first, go home */
			BEGINNING_OF_LINE();

			/* erase to end of line */
			ERASE_TO_EOL();

			/* copy new line into place and display */
			strcpy(buf, hline);
			eol_num = strlen(buf);
			REFRESH_TO_EOL();
			continue;
		}
		case CTL_CH('w'):
			while ((num >= 1) && (buf[num - 1] == ' ')) {
				DO_BACKSPACE();
			}

			while ((num >= 1) && (buf[num - 1] != ' ')) {
				DO_BACKSPACE();
			}

			break;
		default:
			if (isascii(ichar) && isprint(ichar))
				cread_add_char(ichar, insert, &num, &eol_num, buf, len);
			break;
		}
	}
	len = eol_num;
	buf[eol_num] = '\0';	/* lose the newline */

	if (buf[0] && buf[0] != CREAD_HIST_CHAR)
		cread_add_to_hist(buf);
	hist_cur = hist_add_idx;

	return rc < 0 ? rc : len;
}
