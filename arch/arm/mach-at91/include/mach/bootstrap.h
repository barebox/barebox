/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __MACH_BOOTSTRAP_H__
#define __MACH_BOOTSTRAP_H__

#ifdef CONFIG_MTD_M25P80
void * bootstrap_board_read_m25p80(void);
#else
static inline void * bootstrap_board_read_m25p80(void)
{
	return NULL;
}
#endif

#ifdef CONFIG_MTD_DATAFLASH
void * bootstrap_board_read_dataflash(void);
#else
static inline void * bootstrap_board_read_dataflash(void)
{
	return NULL;
}
#endif

#endif /* __MACH_BOOTSTRAP_H__ */
