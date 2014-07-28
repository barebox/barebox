/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DIGIC_UART_H__
#define __DIGIC_UART_H__

/* Serial interface registers offsets */
#define DIGIC_UART_TX	0x0
#define DIGIC_UART_RX	0x4
#define DIGIC_UART_ST	0x14
# define DIGIC_UART_ST_RX_RDY	1
# define DIGIC_UART_ST_TX_RDY	2

#endif /* __DIGIC_UART_H__ */
