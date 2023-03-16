/*
 * Copyright (C) 2015 Lucas Stach <l.stach@pengutronix.de>
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

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE
int tegra_bbu_register_emmc_handler(const char *name, char *devicefile,
				    unsigned long flags);
#else
static int tegra_bbu_register_emmc_handler(const char *name, char *devicefile,
				    unsigned long flags)
{
	return 0;
};
#endif
