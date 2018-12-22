// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017 Oleksij Rempel <linux@rempel-privat.de>
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


