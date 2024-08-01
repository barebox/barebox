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

void term_getsize(int *screenwidth, int *screenheight)
{
	int n;
	char *endp;
	struct console_device *cdev;
	const char esc[] = "\e7" "\e[r" "\e[999;999H" "\e[6n";
	char buf[64];

	if (screenwidth)
		*screenwidth = 256;
	if (screenheight)
		*screenheight = 256;

	for_each_console(cdev) {
		int width, height;
		uint64_t start;

		if (!(cdev->f_active & CONSOLE_STDIN))
			continue;
		if (!(cdev->f_active & CONSOLE_STDOUT))
			continue;

		memset(buf, 0, sizeof(buf));

		cdev->puts(cdev, esc, sizeof(esc));

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

		height = simple_strtoul(buf + 2, &endp, 10);
		width = simple_strtoul(endp + 1, NULL, 10);

		if (screenwidth)
			*screenwidth = min(*screenwidth, width);
		if (screenheight)
			*screenheight = min(*screenheight, height);
	}

	term_setpos(0, 0);
}
