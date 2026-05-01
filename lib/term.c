// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#include <linux/kstrtox.h>
#include <linux/minmax.h>
#include <errno.h>
#include <clock.h>
#include <console.h>
#include <term.h>

void term_setpos(int x, int y)
{
	printf("\x1b[%d;%dH", y + 2, x + 1);
}

static void term_cdev_drain(struct console_device *cdev)
{
	while (cdev->tstc(cdev))
		cdev->getc(cdev);
}

/**
 * term_cdev_read_reply - read and parse a CSI response from a console
 * @cdev:	console device to read from
 * @params:	output array of parsed integer parameters
 * @num:	number of expected parameters
 * @end_char:	character that terminates the response (e.g. 'R')
 *
 * Reads a CSI response of the form ESC [ Pn ; Pn ; ... <end_char>
 * with a single 100ms timeout for the entire response.
 *
 * Return: 0 on success, -ETIMEDOUT or -EINVAL on failure.
 */
static int term_cdev_read_reply(struct console_device *cdev,
				int *params, int num, char end_char)
{
	char buf[64];
	uint64_t start;
	char *endp;
	int n = 0, i;

	start = get_time_ns();

	while (1) {
		if (is_timeout(start, 100 * MSECOND))
			return -ETIMEDOUT;

		if (!cdev->tstc(cdev))
			continue;

		buf[n] = cdev->getc(cdev);
		if (buf[n] == end_char)
			break;

		if (++n >= sizeof(buf) - 1)
			return -EINVAL;
	}

	if (n < 2 || buf[0] != '\e' || buf[1] != '[')
		return -EINVAL;

	endp = buf + 2;
	for (i = 0; i < num; i++) {
		params[i] = simple_strtoul(endp, &endp, 10);
		if (i < num - 1) {
			if (*endp != ';')
				return -EINVAL;
			endp++;
		}
	}

	return 0;
}

int term_cdev_get_size(struct console_device *cdev,
		       int *screenwidth, int *screenheight)
{
	const char esc[] = "\e7" "\e[r" "\e[999;999H" "\e[6n";
	const char restore[] = "\e8";
	int ret, params[2];

	if (!(cdev->f_active & CONSOLE_STDIN))
		return -ENOENT;
	if (!(cdev->f_active & CONSOLE_STDOUT))
		return -ENOENT;

	term_cdev_drain(cdev);

	console_puts(cdev, esc);

	ret = term_cdev_read_reply(cdev, params, 2, 'R');

	console_puts(cdev, restore);

	if (ret)
		return ret;

	*screenheight = params[0];
	*screenwidth = params[1];

	return 0;
}

int term_getsize(int *screenwidth, int *screenheight)
{
	int width = INT_MAX, height = INT_MAX;
	bool found = false;

	for_each_console(cdev) {
		int w, h;

		if (term_cdev_get_size(cdev, &w, &h))
			continue;

		width = min(w, width);
		height = min(h, height);
		found = true;
	}

	if (!found)
		return -ENOENT;

	if (screenwidth)
		*screenwidth = width;
	if (screenheight)
		*screenheight = height;

	return 0;
}
