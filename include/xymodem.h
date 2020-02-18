/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Handles the X-Modem, Y-Modem and Y-Modem/G protocols
 *
 * Copyright (C) 2008 Robert Jarzmik
 */

#ifndef _XYMODEM_
#define _XYMODEM_
struct xyz_ctxt;
struct console_device;

int do_load_serial_xmodem(struct console_device *cdev, int fd);
int do_load_serial_ymodem(struct console_device *cdev);
int do_load_serial_ymodemg(struct console_device *cdev);
#endif
