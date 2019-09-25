/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Marvell 88E6xxx Switch Global (1) Registers support
 *
 * Copyright (c) 2008 Marvell Semiconductor
 *
 * Copyright (c) 2016-2017 Savoir-faire Linux Inc.
 *	Vivien Didelot <vivien.didelot@savoirfairelinux.com>
 */

#ifndef _MV88E6XXX_GLOBAL1_H
#define _MV88E6XXX_GLOBAL1_H

#include "chip.h"

/* Offset 0x00: Switch Global Status Register */
#define MV88E6XXX_G1_STS				0x00
#define MV88E6352_G1_STS_PPU_STATE			0x8000
#define MV88E6185_G1_STS_PPU_STATE_MASK			0xc000
#define MV88E6185_G1_STS_PPU_STATE_DISABLED_RST		0x0000
#define MV88E6185_G1_STS_PPU_STATE_INITIALIZING		0x4000
#define MV88E6185_G1_STS_PPU_STATE_DISABLED		0x8000
#define MV88E6185_G1_STS_PPU_STATE_POLLING		0xc000
#define MV88E6XXX_G1_STS_INIT_READY			0x0800
#define MV88E6XXX_G1_STS_IRQ_AVB			8
#define MV88E6XXX_G1_STS_IRQ_DEVICE			7
#define MV88E6XXX_G1_STS_IRQ_STATS			6
#define MV88E6XXX_G1_STS_IRQ_VTU_PROB			5
#define MV88E6XXX_G1_STS_IRQ_VTU_DONE			4
#define MV88E6XXX_G1_STS_IRQ_ATU_PROB			3
#define MV88E6XXX_G1_STS_IRQ_ATU_DONE			2
#define MV88E6XXX_G1_STS_IRQ_TCAM_DONE			1
#define MV88E6XXX_G1_STS_IRQ_EEPROM_DONE		0

void mv88e6xxx_g1_wait_eeprom_done(struct mv88e6xxx_chip *chip);

#endif /* _MV88E6XXX_GLOBAL1_H */
