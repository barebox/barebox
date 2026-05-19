/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __TERM_H
#define __TERM_H

struct console_device;

void term_setpos(int x, int y);
int term_getsize(int *screenwidth, int *screenheight);

int term_cdev_get_size(struct console_device *cdev,
		       int *screenwidth, int *screenheight);

#endif /* __LIBBB_H */
