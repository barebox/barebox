// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Raghavendra KH <r-khandenahally@ti.com>, Texas Instruments (http://www.ti.com/)

/**
 * @file
 * @brief exported generic APIs which various board files implement
 *
 * This file will not contain any board specific implementations.
 */

#ifndef __BOARD_OMAP_H_
#define __BOARD_OMAP_H_

/** Generic Board initialization called from platform.S */
void board_init(void);

#endif         /* __BOARD_OMAP_H_ */
