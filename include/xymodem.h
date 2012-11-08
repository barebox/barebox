/*
 * Handles the X-Modem, Y-Modem and Y-Modem/G protocols
 *
 * Copyright (C) 2008 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _XYMODEM_
#define _XYMODEM_
struct xyz_ctxt;
struct console_device;

int do_load_serial_xmodem(struct console_device *cdev, int fd);
int do_load_serial_ymodem(struct console_device *cdev);
int do_load_serial_ymodemg(struct console_device *cdev);
#endif
