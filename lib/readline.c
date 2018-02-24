#include <common.h>
#include <readkey.h>
#include <init.h>
#include <libbb.h>
#include <poller.h>
#include <ratp_bb.h>
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

#define CTL_BACKSPACE		('\b')
#define DEL			255
#define DEL7			127
#define CREAD_HIST_CHAR		('!')

#define getcmd_putch(ch)	putchar(ch)
#define getcmd_getch()		getc()
#define getcmd_cbeep()		getcmd_putch('\a')

struct history {
	char *line;
	struct list_head list;
};

static LIST_HEAD(history_list);

static struct list_head *history_current;
static int history_num_entries;

static void cread_add_to_hist(char *line)
{
	struct history *history;
	char *newline;

	if (!list_empty(&history_list)) {
		history = list_last_entry(&history_list, struct history, list);

		if (!strcmp(line, history->line))
			return;
	}

	newline = strdup(line);
	if (!newline)
		return;

	if (history_num_entries < 32) {
		history = xzalloc(sizeof(*history));
		history_num_entries++;
	} else {
		history = list_first_entry(&history_list, struct history, list);
		free(history->line);
		list_del(&history->list);
	}

	history->line = newline;

	list_add_tail(&history->list, &history_list);
}

static const char *hist_prev(void)
{
	struct history *history;

	if (history_current->prev == &history_list) {
		getcmd_cbeep();

		if (list_empty(&history_list))
			return "";

		history = list_entry(history_current, struct history, list);
		return history->line;
	}

	history = list_entry(history_current->prev, struct history, list);

	history_current = &history->list;

	return history->line;
}

static const char *hist_next(void)
{
	struct history *history;

	if (history_current->next == &history_list) {
		history_current = &history_list;
		return "";
	}

	if (history_current == &history_list) {
		getcmd_cbeep();
		return "";
	}

	history = list_entry(history_current->next, struct history, list);

	history_current = &history->list;

	return history->line;
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
		if (*eol_num > len - 2) {
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
#ifdef CONFIG_AUTO_COMPLETE
	char tmp;
	int reprint, i;
	char *completestr;

	complete_reset();
#endif
	history_current = &history_list;

	puts (prompt);

	while (1) {
		while (!tstc()) {
			poller_call();
			if (IS_ENABLED(CONFIG_CONSOLE_RATP))
				barebox_ratp_command_run();
		}

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

		case BB_KEY_HOME:
			BEGINNING_OF_LINE();
			break;
		case CTL_CH('c'):	/* ^C - break */
			*buf = 0;	/* discard input */
			return -1;
		case BB_KEY_RIGHT:
			if (num < eol_num) {
				getcmd_putch(buf[num]);
				num++;
			}
			break;
		case BB_KEY_LEFT:
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
		case CTL_CH('l'):
			printf(ANSI_CLEAR_SCREEN);
			buf[eol_num] = 0;
			printf("%s%s", prompt, buf);
			wlen = eol_num - num;
			while (wlen--)
				getcmd_putch(CTL_BACKSPACE);
			break;
		case BB_KEY_ERASE_TO_EOL:
			ERASE_TO_EOL();
			break;
		case BB_KEY_REFRESH_TO_EOL:
		case BB_KEY_END:
			REFRESH_TO_EOL();
			break;
		case BB_KEY_INSERT:
			insert = !insert;
			break;
		case BB_KEY_ERASE_LINE:
			BEGINNING_OF_LINE();
			ERASE_TO_EOL();
			break;
		case DEL:
		case BB_KEY_DEL7:
		case 8:
			if (num) {
				DO_BACKSPACE();
			}
			break;
		case BB_KEY_DEL:
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
		case BB_KEY_UP:
		case BB_KEY_DOWN:
		{
			const char *hline;

			if (ichar == BB_KEY_UP)
				hline = hist_prev();
			else
				hline = hist_next();

			/* nuke the current line */
			/* first, go home */
			BEGINNING_OF_LINE();

			/* erase to end of line */
			ERASE_TO_EOL();

			/* copy new line into place and display */
			safe_strncpy(buf, hline, len);
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

	if (buf[0])
		cread_add_to_hist(buf);

	return len;
}
