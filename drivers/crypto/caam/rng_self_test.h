/*
 * CAAM RNG self test
 *
 * Copyright (C) 2018 Pengutronix, Roland Hieber <r.hieber@pengutronix.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef RNG_SELF_TEST_H
#define RNG_SELF_TEST_H

int caam_rng_self_test(struct device_d *dev, const u8 caam_era, const u8 rngvid, const u8 rngrev);

#endif /* RNG_SELF_TEST_H */
