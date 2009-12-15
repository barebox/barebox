/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Network definitions and prototypes for dBUG.
 */

#ifndef _MCFV4E_NET_H
#define _MCFV4E_NET_H

/*
 * Include information and prototypes for all protocols
 */
#include "eth.h"
#include "nbuf.h"

int netif_init(int channel);
int netif_setup(int channel);
int netif_done(int channel);

#endif  /* _MCFV4E_NET_H */

