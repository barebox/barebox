/*
 * CAAM RNG self test
 *
 * Copyright (C) 2018 Pengutronix, Roland Hieber <r.hieber@pengutronix.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef RNG_SELF_TEST_H
#define RNG_SELF_TEST_H

int caam_rng_self_test(struct device_d *dev, const u8 caam_era, const u8 rngvid, const u8 rngrev);

#endif /* RNG_SELF_TEST_H */
