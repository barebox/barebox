// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <linux/kstrtox.h>
#include <linux/minmax.h>
#include <console.h>
#include <term.h>

void term_setpos(int x, int y)
{
	printf("\x1b[%d;%dH", y + 2, x + 1);
}

int term_getsize(int *screenwidth, int *screenheight)
{
	int n;
	int width = INT_MAX, height = INT_MAX;
	bool found = false;
	char *endp;
	const char esc[] = "\e7" "\e[r" "\e[999;999H" "\e[6n";
	char buf[64];

	for_each_console(cdev) {
		int w, h;
		uint64_t start;

		if (!(cdev->f_active & CONSOLE_STDIN))
			continue;
		if (!(cdev->f_active & CONSOLE_STDOUT))
			continue;

		memset(buf, 0, sizeof(buf));

		console_puts(cdev, esc);

		n = 0;

		start = get_time_ns();

		while (1) {
			if (is_timeout(start, 100 * MSECOND))
				break;

			if (!cdev->tstc(cdev))
				continue;

			buf[n] = cdev->getc(cdev);

			if (buf[n] == 'R')
				break;

			n++;
		}

		if (buf[0] != 27)
			continue;
		if (buf[1] != '[')
			continue;

		h = simple_strtoul(buf + 2, &endp, 10);
		w = simple_strtoul(endp + 1, NULL, 10);

		width = min(w, width);
		height = min(h, height);
		found = true;
	}

	term_setpos(0, 0);

	if (!found)
		return -ENOENT;

	if (screenwidth)
		*screenwidth = width;
	if (screenheight)
		*screenheight = height;

	return 0;
}
