// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fs.h>
#include <linux/ctype.h>
#include <fcntl.h>
#include <libfile.h>
#include <readkey.h>
#include <errno.h>
#include <xfuncs.h>
#include <linux/stat.h>
#include <console.h>
#include <term.h>

#define TABSPACE 8

struct line {
	int length;
	struct line *next;
	struct line *prev;
	char *data;
};

static struct line *buffer;

static struct line *lastscrline;

static int screenwidth  = 80;
static int screenheight = 25;

static int cursx  = 0;		/* position on screen */
static int cursy  = 0;

static int textx  = 0;		/* position in text */

static struct line *curline;	/* line where the cursor is */

static struct line *scrline;	/* the first line on screen */
static int scrcol = 0;		/* the first column on screen */

static char *screenline(char *line, int *pos)
{
	int i, outpos = 0;
	static char lbuf[1024];

	if (!line) {
		lbuf[outpos++] = '~';
		goto out;
	}

	for (i = 0; outpos < sizeof(lbuf) - 1; i++) {
		if (i == textx && pos)
			*pos = outpos;
		if (!line[i])
			break;
		if (line[i] == '\t') {
			lbuf[outpos++] = ' ';
			while (outpos < sizeof(lbuf) - 1 && outpos % TABSPACE)
				lbuf[outpos++] = ' ';
			continue;
		}
		lbuf[outpos++] = line[i];
	}

out:
	lbuf[outpos] = 0;
	return lbuf;
}

static int setpos(char *line, int position)
{
	int i = 0;
	int linepos = 0;

	while(line[linepos]) {
		if (line[linepos] == '\t')
			while ((i + 1) % TABSPACE)
				i++;
		if (i >= position)
			return linepos;
		linepos++;
		i++;
	}

	return linepos;
}

static void refresh_line(struct line *line, int ypos)
{
	char *str = screenline(line->data, NULL) + scrcol;
	term_setpos(0, ypos);
	str[screenwidth] = 0;
	printf("%s\x1b[K", str);
	term_setpos(cursx, cursy);
}

/*
 * Most sane terminal programs can do ansi screen scrolling.
 * Unfortunately one of the most popular programs cannot:
 * minicom.
 * Grmpf!
 */
static int smartscroll = 0;

static void refresh(int full)
{
	int i;
	struct line *l = scrline;

	if (!full) {
		if (smartscroll) {
			if (scrline->next == lastscrline) {
				printf("\x1b[1T");
				refresh_line(scrline, 0);
				term_setpos(0, screenheight);
				printf("%*s", screenwidth, "");
				return;
			}

			if (scrline->prev == lastscrline) {
				printf("\x1b[1S");
				for (i = 0; i < screenheight - 1; i++) {
					l = l->next;
					if (!l)
						return;
				}
				refresh_line(l, screenheight - 1);
				return;
			}
		} else {
			refresh(1);
			return;
		}
	}

	for (i = 0; i < screenheight; i++) {
		refresh_line(l, i);
		l = l->next;
		if (!l)
			break;
	}

	i++;
	while (i < screenheight) {
		term_setpos(0, i++);
		printf("~");
	}
}

static void line_free(struct line *line)
{
	free(line->data);
	free(line);
}

static struct line *line_realloc(int len, struct line *line)
{
	int size = 32;

	if (!line)
		line = xzalloc(sizeof(struct line));

	while (size < len)
		size <<= 1;

	line->data = xrealloc(line->data, size);
	return line;
}

static int edit_read_file(const char *path)
{
	struct line *line;
	struct line *lastline = NULL;
	char *filebuffer;
	char *linestr, *lineend;
	struct stat s;

	if (!stat(path, &s)) {
		filebuffer = read_file(path, NULL);
		if (!filebuffer) {
			printf("could not read %s: %m\n", path);
			return -1;
		}

		linestr = filebuffer;
		while (1) {
			if (!*linestr)
				break;

			lineend = strchr(linestr, '\n');

			if (!lineend && !*linestr)
				break;

			if (lineend)
				*lineend = 0;

			line = line_realloc(strlen(linestr) + 1, NULL);
			if (!buffer)
				buffer = line;
			memcpy(line->data, linestr, strlen(linestr) + 1);
			line->prev = lastline;
			if (lastline)
				lastline->next = line;
			line->next = NULL;
			lastline = line;

			if (!lineend)
				break;

			linestr = lineend + 1;
		}
		free(filebuffer);
	}

	if (!buffer) {
		buffer = line_realloc(0, NULL);
		buffer->data[0] = 0;
	}

	return 0;
}

static void free_buffer(void)
{
	struct line *line, *tmp;

	line = buffer;

	while(line) {
		tmp = line->next;
		line_free(line);
		line = tmp;
	}
}

static int save_file(const char *path)
{
	struct line *line, *tmp;
	int fd;
	int ret = 0;

	fd = open(path, O_WRONLY | O_TRUNC | O_CREAT);
	if (fd < 0) {
		printf("could not open file for writing: %m\n");
		return fd;
	}

	line = buffer;

	while(line) {
		tmp = line->next;
		ret = write_full(fd, line->data, strlen(line->data));
		if (ret < 0)
			goto out;
		ret = write_full(fd, "\n", 1);
		if (ret < 0)
			goto out;
		line = tmp;
	}

	ret = 0;

out:
	close(fd);
	return ret;
}

static void insert_char(char c)
{
	int pos = textx;
	char *line;
	int end = strlen(curline->data);

	line_realloc(strlen(curline->data) + 2, curline);
	line = curline->data;

	while (end >= pos) {
		line[end + 1] = line[end];
		end--;
	}
	line[pos] = c;
	textx++;
	refresh_line(curline, cursy);
}

static void delete_char(int pos)
{
	char *line = curline->data;
	int end = strlen(line);

	while (pos < end) {
		line[pos] = line[pos + 1];
		pos++;
	}
	refresh_line(curline, cursy);
}

static void split_line(void)
{
	int length = strlen(curline->data + textx);
	struct line *newline = line_realloc(length + 1, NULL);
	struct line *tmp;

	memcpy(newline->data, curline->data + textx, length + 1);

	curline->data[textx] = 0;

	tmp = curline->next;
	curline->next = newline;
	newline->prev = curline;
	newline->next = tmp;
	if (tmp)
		tmp->prev = newline;

	textx = 0;
	cursy++;
	curline = curline->next;
	refresh(1);
}

static void merge_line(struct line *line)
{
	struct line *tmp;

	line_realloc(strlen(line->data) + strlen(line->next->data) + 1, line);

	tmp = line->next;

	line->next = line->next->next;
	if (line->next)
		line->next->prev = line;
	strcat(line->data, tmp->data);

	line_free(tmp);

	refresh(1);
}

#define ESC "\033"

static void statusbar(const char *str)
{
	term_setpos(0, screenheight+1);
	printf("%*c\r%s", screenwidth, ' ', str);
	term_setpos(cursx, cursy);
}

static int read_modal_key(bool is_modal)
{
	static enum { MODE_INSERT, MODE_NORMAL } mode = MODE_NORMAL;
	static int backlog[2] = { -1, -1 };
	int c;

	if (backlog[0] >= 0) {
		/* pop a character */
		c = backlog[0];
		backlog[0] = backlog[1];
		backlog[1] = -1;
	} else {
		c = read_key();
	}

	if (is_modal && mode == MODE_INSERT && (c == -1 || c == CTL_CH('c'))) {
		mode = MODE_NORMAL;
		statusbar("");
		return -EAGAIN;
	}

	if (!is_modal || mode == MODE_INSERT)
		return c;

	switch (c) {
	case -1: /* invalid escape, e.g. two escapes in a row */
		break;
	case 'i':
		statusbar("-- INSERT --");
		mode = MODE_INSERT;
		break;
	case 'h':
		return BB_KEY_LEFT;
	case 'j':
		return BB_KEY_DOWN;
	case 'k':
		return BB_KEY_UP;
	case 'a':
		statusbar("-- INSERT --");
		mode = MODE_INSERT;
		fallthrough;
	case 'l':
		return BB_KEY_RIGHT;
	case 'O':
		backlog[0] = '\n';
		backlog[1] = BB_KEY_UP;
		fallthrough;
	case 'I':
		statusbar("-- INSERT --");
		mode = MODE_INSERT;
		fallthrough;
	case '^':
	case '0':
		return BB_KEY_HOME;
	case 'g':
		c = read_key();
		if (c != 'g')
			break;
		backlog[0] = CTL_CH('u');
		backlog[1] = CTL_CH('u');
		fallthrough;
	case CTL_CH('u'):
		return BB_KEY_PAGEUP;
	case 'G':
		backlog[0] = CTL_CH('d');
		backlog[1] = CTL_CH('d');
		fallthrough;
	case CTL_CH('d'):
		return BB_KEY_PAGEDOWN;
	case 'o':
		backlog[0] = '\n';
		fallthrough;
	case 'A':
		statusbar("-- INSERT --");
		mode = MODE_INSERT;
		fallthrough;
	case '$':
		return BB_KEY_END;
	case CTL_CH('c'):
		statusbar("Type ZQ to abandon all changes and exit vi."
			  "Type ZZ to exit while saving them.");
		break;
	case 'x':
		return BB_KEY_DEL;
	case 'X':
		return '\b';
	case BB_KEY_PAGEUP:
	case BB_KEY_PAGEDOWN:
	case BB_KEY_HOME:
	case BB_KEY_END:
	case BB_KEY_UP:
	case BB_KEY_DOWN:
	case BB_KEY_RIGHT:
	case BB_KEY_LEFT:
		return c;
	case ':':
		statusbar("ERROR: command mode not supported");
		break;
	case 'Z':
		c = read_key();
		if (c == 'Z')
			return CTL_CH('d');
		if (c == 'Q')
			return CTL_CH('c');
		fallthrough;
	default:
		statusbar("ERROR: not implemented");
		break;
	}

	return -EAGAIN;
}

static bool is_efi_console_active(void)
{
	struct console_device *cdev;

	if (!IS_ENABLED(CONFIG_DRIVER_SERIAL_EFI_STDIO))
		return false;

	for_each_console(cdev) {
		if (!(cdev->f_active & CONSOLE_STDIN))
			continue;
		if (!(cdev->f_active & CONSOLE_STDOUT))
			continue;
		if (!strcmp(dev_name(cdev->dev), "efi-stdio"))
			return true;
	}

	return false;
}

static int do_edit(int argc, char *argv[])
{
	bool is_vi = false;
	int lastscrcol;
	int i;
	int linepos;
	int c;
	int ret = COMMAND_SUCCESS;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	buffer = NULL;
	if(edit_read_file(argv[1]))
		return 1;

	screenwidth = 80;
	screenheight = 25;

	/* check if we are not called as "edit" */
	if (*argv[0] != 'e') {
		smartscroll = 1;
		term_getsize(&screenwidth, &screenheight);

		/* check if we are called as "vi" */
		if (*argv[0] == 'v')
			is_vi = true;
	}

	if (is_efi_console_active()) {
		/*
		 * The EFI simple text output protocol wraps to the next line and
		 * scrolls down when we write to the right bottom screen position.
		 * Reduce the number of rows by one to work around this.
		 */
		screenheight--;

		/*
		 * Our console driver for the EFI simple text output protocol does
		 * not implement the "\e[1S" sequence we use for scrolling the
		 * screen.
		 */
		smartscroll = 0;
	}

	cursx  = 0;
	cursy  = 0;
	textx  = 0;
	scrcol = 0;
	curline = buffer;
	scrline = curline;
	lastscrline = scrline;
	lastscrcol = 0;

	printf("\x1b[2J");

	term_setpos(0, -1);

	if (is_vi) {
		screenheight -= 2;
		printf("\x1b[7m%*c\x1b[0m", screenwidth , ' ');
		term_setpos(0, screenheight-1);
		printf("\x1b[7m%*c\x1b[0m", screenwidth , ' ');
		printf("\r\x1b[7m%-25s\x1b[0m", argv[1]);
	} else {
		printf("\x1b[7m %-25s <ctrl-d>: Save and quit <ctrl-c>: quit \x1b[0m",
				argv[1]);
	}

	if (smartscroll)
		printf("\x1b[2;%dr", screenheight);

	term_setpos(0, 0);

	screenheight--; /* status line */

	refresh(1);

	while (1) {
		int curlen = strlen(curline->data);

		if (textx > curlen)
			textx = curlen;
		if (textx < 1)
			textx = 0;

		screenline(curline->data, &linepos);

		if (linepos > scrcol + screenwidth)
			scrcol = linepos - screenwidth;

		if (scrcol > linepos)
			scrcol = linepos;

		cursx = linepos - scrcol;

		while (cursy >= screenheight) {
			cursy--;
			scrline = scrline->next;
		}

		while (cursy < 0) {
			cursy++;
			scrline = scrline->prev;
		}

		if (scrline != lastscrline || scrcol != lastscrcol)
			refresh(0);

		lastscrcol  = scrcol;
		lastscrline = scrline;
		term_setpos(cursx, cursy);

again:
		c = read_modal_key(is_vi);
		if (c == -EAGAIN)
			goto again;

		switch (c) {
		case BB_KEY_UP:
			if (!curline->prev)
				continue;

			curline = curline->prev;
			cursy--;
			textx = setpos(curline->data, linepos);
			break;
		case BB_KEY_DOWN:
			if (!curline->next)
				continue;

			curline = curline->next;
			cursy++;
			textx = setpos(curline->data, linepos);
			break;
		case BB_KEY_RIGHT:
			textx++;
			break;
		case BB_KEY_LEFT:
			textx--;
			break;
		case BB_KEY_HOME:
			textx = 0;
			break;
		case BB_KEY_END:
			textx = curlen;
			break;
		case BB_KEY_PAGEUP:
			for (i = 0; i < screenheight - 1; i++) {
				if (!curline->prev)
					break;
				cursy--;
				curline = curline->prev;
			}
			textx = setpos(curline->data, linepos);
			break;
		case BB_KEY_PAGEDOWN:
			for (i = 0; i < screenheight - 1; i++) {
				if (!curline->next)
					break;
				cursy++;
				curline = curline->next;
			}
			textx = setpos(curline->data, linepos);
			break;
		case BB_KEY_DEL:
			if (textx == curlen) {
				if (curline->next)
					merge_line(curline);
			} else
				delete_char(textx);
			break;
		case '\r':
		case '\n':
			split_line();
			break;
		case 127:
		case '\b':
			if (textx > 0) {
				textx--;
				delete_char(textx);
			} else {
				if (!curline->prev)
					break;
				curline = curline->prev;
				cursy--;
				textx = strlen(curline->data);
				merge_line(curline);
			}
			break;
		case CTL_CH('d'):
			ret = save_file(argv[1]);
			goto out;
		case CTL_CH('c'):
			goto out;
		default:
			if ((signed char)c != -1)
				insert_char(c);
		}
	}
out:
	free_buffer();
	printf("\x1b[2J\x1b[r");
	printf("\n");
	return ret;
}

static const char * const edit_aliases[] = { "sedit", "vi", NULL};

BAREBOX_CMD_HELP_START(edit)
BAREBOX_CMD_HELP_TEXT("Use cursor keys, Ctrl-C to exit and Ctrl-D to exit-with-save.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(edit)
	.cmd		= do_edit,
	.aliases	= edit_aliases,
	BAREBOX_CMD_DESC("a small full-screen editor")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_edit_help)
BAREBOX_CMD_END
