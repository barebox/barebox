/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARCH_ARM_BOARDS_LOWLEVEL_H__
#define __ARCH_ARM_BOARDS_LOWLEVEL_H__

#include <linux/types.h>
#include <linux/sizes.h>

#define VIDEOCORE_FDT_SZ SZ_1M
#define VIDEOCORE_FDT_ERROR 0xdeadfeed

ssize_t rpi_get_arm_mem(void);
int rpi_get_usbethaddr(u8 mac[6]);
int rpi_get_board_rev(void);

#endif /* __ARCH_ARM_BOARDS_LOWLEVEL_H__ */
