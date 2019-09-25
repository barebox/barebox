// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Marvell 88E6xxx Switch Global (1) Registers support
 *
 * Copyright (c) 2008 Marvell Semiconductor
 *
 * Copyright (c) 2016-2017 Savoir-faire Linux Inc.
 *	Vivien Didelot <vivien.didelot@savoirfairelinux.com>
 */

#include <clock.h>
#include <linux/bitfield.h>

#include "chip.h"
#include "global1.h"

static int mv88e6xxx_g1_read(struct mv88e6xxx_chip *chip, int reg, u16 *val)
{
	int addr = chip->info->global1_addr;

	return mv88e6xxx_read(chip, addr, reg, val);
}

void mv88e6xxx_g1_wait_eeprom_done(struct mv88e6xxx_chip *chip)
{
	const uint64_t start   = get_time_ns();
	const uint64_t timeout = SECOND;
	u16 val;
	int err;

	/* Wait up to 1 second for the switch to finish reading the
	 * EEPROM.
	 */
	while (!is_timeout(start, timeout)) {
		err = mv88e6xxx_g1_read(chip, MV88E6XXX_G1_STS, &val);
		if (err) {
			dev_err(chip->dev, "Error reading status\n");
			return;
		}

		if (val != 0xFFFF && /* switch will return 0xffff until
				      * EEPROM is loaded
				      */
		    val & BIT(MV88E6XXX_G1_STS_IRQ_EEPROM_DONE))
			return;

		mdelay(2);
	}

	dev_err(chip->dev, "Timeout waiting for EEPROM done\n");
}
