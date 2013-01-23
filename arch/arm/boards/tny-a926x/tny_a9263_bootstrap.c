/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <bootstrap.h>
#include <mach/bootstrap.h>

#ifdef CONFIG_MTD_DATAFLASH
void * bootstrap_board_read_dataflash(void)
{
	return bootstrap_read_devfs("dataflash0", false, 0xffc0, 204864, 204864);
}
#endif
