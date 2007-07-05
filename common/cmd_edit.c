#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fs.h>
#include <linux/ctype.h>
#include <fcntl.h>
#include <errno.h>

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

static int cursx  = 0;	/* position on screen */
static int cursy  = 0;

static int textx  = 0; /* position in text */

static struct line *curline;	/* line where the cursor is */

static struct line *scrline;	/* the first line on screen */
int scrcol = 0;			/* the first column on screen */

// Misc. non-Ascii keys that report an escape sequence
#define VI_K_UP			(char)128	// cursor key Up
#define VI_K_DOWN		(char)129	// cursor key Down
#define VI_K_RIGHT		(char)130	// Cursor Key Right
#define VI_K_LEFT		(char)131	// cursor key Left
#define VI_K_HOME		(char)132	// Cursor Key Home
#define VI_K_END		(char)133	// Cursor Key End
#define VI_K_INSERT		(char)134	// Cursor Key Insert
#define VI_K_PAGEUP		(char)135	// Cursor Key Page Up
#define VI_K_PAGEDOWN		(char)136	// Cursor Key Page Down
#define VI_K_DEL		(char)137	// Cursor Key Del

struct esc_cmds {
	const char *seq;
	char val;
};

static const struct esc_cmds esccmds[] = {
	{"OA", VI_K_UP},       // cursor key Up
	{"OB", VI_K_DOWN},     // cursor key Down
	{"OC", VI_K_RIGHT},    // Cursor Key Right
	{"OD", VI_K_LEFT},     // cursor key Left
	{"OH", VI_K_HOME},     // Cursor Key Home
	{"OF", VI_K_END},      // Cursor Key End
	{"[A", VI_K_UP},       // cursor key Up
	{"[B", VI_K_DOWN},     // cursor key Down
	{"[C", VI_K_RIGHT},    // Cursor Key Right
	{"[D", VI_K_LEFT},     // cursor key Left
	{"[H", VI_K_HOME},     // Cursor Key Home
	{"[F", VI_K_END},      // Cursor Key End
	{"[1~", VI_K_HOME},    // Cursor Key Home
	{"[2~", VI_K_INSERT},  // Cursor Key Insert
	{"[3~", VI_K_DEL},     // Cursor Key Delete
	{"[4~", VI_K_END},     // Cursor Key End
	{"[5~", VI_K_PAGEUP},  // Cursor Key Page Up
	{"[6~", VI_K_PAGEDOWN},// Cursor Key Page Down
};

static char readit(void)
{
	char c;
	char esc[5];
	c = getc();

	if (c == 27) {
		int i = 0;
		esc[i++] = getc();
		esc[i++] = getc();
		if (isdigit(esc[1])) {
			while(1) {
				esc[i] = getc();
				if (esc[i++] == '~')
					break;
			}
		}
		esc[i] = 0;
		for (i = 0; i < 18; i++){
			if (!strcmp(esc, esccmds[i].seq))
				return esccmds[i].val;
		}
		return -1;
	}
	return c;
}

static void pos(int x, int y)
{
	printf("%c[%d;%dH", 27, y + 1, x + 1);
}

static char *screenline(char *line, int *pos)
{
	int i, outpos = 0;
	static char lbuf[1024];

	memset(lbuf, 0, 1024);

	if (!line) {
		lbuf[0] = '~';
		return lbuf;
	}

	for (i = 0; outpos < 1024; i++) {
		if (i == textx && pos)
			*pos = outpos;
		if (!line[i])
			break;
		if (line[i] == '\t') {
			lbuf[outpos++] = ' ';
			while (outpos % TABSPACE)
				lbuf[outpos++] = ' ';
			continue;
		}
		lbuf[outpos++] = line[i];
	}

	return lbuf;
}

int setpos(char *line, int position)
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
	pos(0, ypos);
	str[screenwidth] = 0;
	printf("%-*s", screenwidth, str);
	pos(cursx, cursy);
}

static void refresh(int full)
{
	int i;
	struct line *l = scrline;
	char *str;

	if (!full) {
		if (scrline->next == lastscrline) {
			printf("%c[1T", 27);
			refresh_line(scrline, 0);
			pos(0, screenheight);
			printf("%*s", screenwidth, "");
			return;
		}

		if (scrline->prev == lastscrline) {
			printf("%c[1S", 27);
			for (i = 0; i < screenheight - 1; i++) {
				l = l->next;
				if (!l)
					return;
			}
			refresh_line(l, screenheight - 1);
			return;
		}
	}

	for (i = 0; i < screenheight; i++) {
		refresh_line(l, i);
		l = l->next;
		if (!l)
			break;
	}

	while (i < screenheight) {
		pos(0, i++);
		printf("%-*s", screenwidth, "~");
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

	if (!line) {
		line = malloc(sizeof(struct line));
		memset(line, 0, sizeof(struct line));
		line->data = malloc(32);
	}

	while(size < len)
		size <<= 1;

	line->data = realloc(line->data, size);
	return line;
}

static int read_file(const char *path)
{
	int fd;
	static char rbuf[1024];
	struct line *line;
	struct line *lastline = NULL;
	int r = 1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	while (r) {
		char *tmp = rbuf;
		while((r = read(fd, tmp, 1)) == 1) {
			if (*tmp == '\n')
				break;
			tmp++;
		}
		*tmp = 0;
		line = line_realloc(strlen(rbuf + 1), NULL);
		if (!buffer)
			buffer = line;
		memcpy(line->data, rbuf, strlen(rbuf) + 1);
		line->prev = lastline;
		if (lastline)
			lastline->next = line;
		line->next = 0;
		lastline = line;
	}

	close(fd);
	return 0;
}

static void insert_char(char c)
{
	int pos = textx;
	char *line = curline->data;
	int end = strlen(line);

	line_realloc(strlen(curline->data) + 2, curline);

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

#if 0
static void getwinsize(void) {
	int y, yy = 25, xx = 80, i, n, r;
	char buf[100];
	char *endp;

	for (y = 25; y < 320; y++) {
		pos(y, y);
		printf("%c[6n", 27);
		i = 0;
		while ((r = getc()) != 'R') {
			buf[i] = r;
			i++;
		}
		n = simple_strtoul(buf + 2, &endp, 10);
		if (n == y + 1)
			yy = y + 1;
		n = simple_strtoul(endp + 1, NULL, 10);
		if (n == y + 1)
			xx = y + 1;
	}
	pos(0,0);
	printf("x: %d y: %d\n", xx, yy);
}
#endif

int do_edit(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int lastscrcol;
	int i;
	int linepos;
	unsigned char c;
	struct line *line, *tmp;

	buffer = NULL;
	read_file(argv[1]);
	cursx  = 0;
	cursy  = 0;
	textx  = 0;
	scrcol = 0;
	curline = buffer;
	scrline = curline;
	lastscrline = scrline;
	lastscrcol = 0;

	printf("%c[2J", 27);
	printf("%c[=3h", 27);
	refresh(1);

	while (1) {
		int curlen = strlen(curline->data);

		if (textx > curlen)
			textx = curlen;
		if (textx < 0)
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
		pos(cursx, cursy);

		c = readit();
		switch (c) {
		case VI_K_UP:
			if (!curline->prev)
				continue;

			curline = curline->prev;
			cursy--;
			textx = setpos(curline->data, linepos);
			break;
		case VI_K_DOWN:
			if (!curline->next)
				continue;

			curline = curline->next;
			cursy++;
			textx = setpos(curline->data, linepos);
			break;
		case VI_K_RIGHT:
			textx++;
			break;
		case VI_K_LEFT:
			textx--;
			break;
		case VI_K_HOME:
			textx = 0;
			break;
		case VI_K_END:
			textx = curlen;
			break;
		case VI_K_PAGEUP:
			for (i = 0; i < screenheight - 1; i++) {
				if (!curline->prev)
					break;
				cursy--;
				curline = curline->prev;
			}
			textx = setpos(curline->data, linepos);
			break;
		case VI_K_PAGEDOWN:
			for (i = 0; i < screenheight - 1; i++) {
				if (!curline->next)
					break;
				cursy++;
				curline = curline->next;
			}
			textx = setpos(curline->data, linepos);
			break;
		case VI_K_DEL:
			if (textx == curlen) {
				if (curline->next)
					merge_line(curline);
			} else
				delete_char(textx);
			break;
		case 13:
			break;
		case 10:
			split_line();
			break;
		case 127:
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
		case 4:
			printf("<ctrl-d>\n");
			goto out;
		case 3:
			printf("<???????\n");
			goto out;
		default:
			if ((signed char)c != -1)
				insert_char(c);
		}
	}
out:
	line = buffer;

	while(line) {
		tmp = line->next;
		line_free(line);
		line = tmp;
	}

	return 0;
}

int do_edit (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
U_BOOT_CMD(
	edit,	2,	0,	do_edit,
	"edit     - edit a file\n",
	"<filename> - edit <filename>\n"
);
