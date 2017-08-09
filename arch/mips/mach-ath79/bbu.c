/*
 * Copyright (c) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <bbu.h>
#include <init.h>

static int ath79_init_bbu(void)
{
	bbu_register_std_file_update("barebox", BBU_HANDLER_FLAG_DEFAULT,
				     "/dev/spiflash.barebox",
				     filetype_mips_barebox);

	return 0;
}
postcore_initcall(ath79_init_bbu);


