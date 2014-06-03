/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Register definitions */
#define TEGRA_SYSCTR0_CNTCR		0x00
#define TEGRA_SYSCTR0_CNTCR_ENABLE	(1 << 0)
#define TEGRA_SYSCTR0_CNTCR_HDBG	(1 << 1)

#define TEGRA_SYSCTR0_CNTSR		0x04

#define TEGRA_SYSCTR0_CNTCV0		0x08

#define TEGRA_SYSCTR0_CNTCV1		0x0c

#define TEGRA_SYSCTR0_CNTFID0		0x20

#define TEGRA_SYSCTR0_CNTFID1		0x24
