/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on Linux driver:
 *  Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *  Copyright (C) 2006 FON Technology, SL.
 *  Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *  Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#ifndef __AR231X_PLATFORM_H
#define __AR231X_PLATFORM_H

/*
 * This is board-specific data that is stored in a "fixed" location in flash.
 * It is shared across operating systems, so it should not be changed lightly.
 * The main reason we need it is in order to extract the ethernet MAC
 * address(es).
 */
struct ar231x_board_config {
	u32 magic;			/* board data is valid */
#define AR231X_BD_MAGIC	0x35333131	/* "5311", for all 531x platforms */
	u16 cksum;			/* checksum (starting with BD_REV 2) */
	u16 rev;			/* revision of this struct */
#define BD_REV  4
	char boardName[64];		/* Name of board */
	u16 major;			/* Board major number */
	u16 minor;			/* Board minor number */
	u32 flags;			/* Board configuration */
#define BD_ENET0	0x00000001	/* ENET0 is stuffed */
#define BD_ENET1	0x00000002	/* ENET1 is stuffed */
#define BD_UART1	0x00000004	/* UART1 is stuffed */
#define BD_UART0	0x00000008	/* UART0 is stuffed (dma) */
#define BD_RSTFACTORY	0x00000010	/* Reset factory defaults stuffed */
#define BD_SYSLED	0x00000020	/* System LED stuffed */
#define BD_EXTUARTCLK	0x00000040	/* External UART clock */
#define BD_CPUFREQ	0x00000080	/* cpu freq is valid in nvram */
#define BD_SYSFREQ	0x00000100	/* sys freq is set in nvram */
#define BD_WLAN0	0x00000200	/* Enable WLAN0 */
#define BD_MEMCAP	0x00000400	/* CAP SDRAM @ memCap for testing */
#define BD_DISWATCHDOG	0x00000800	/* disable system watchdog */
#define BD_WLAN1	0x00001000	/* Enable WLAN1 (ar5212) */
#define BD_ISCASPER	0x00002000	/* FLAG for AR2312 */
#define BD_WLAN0_2G_EN	0x00004000	/* FLAG for radio0_2G */
#define BD_WLAN0_5G_EN	0x00008000	/* FLAG for radio0_2G */
#define BD_WLAN1_2G_EN	0x00020000	/* FLAG for radio0_2G */
#define BD_WLAN1_5G_EN	0x00040000	/* FLAG for radio0_2G */
	u16 resetConfigGpio;		/* Reset factory GPIO pin */
	u16 sysLedGpio;			/* System LED GPIO pin */

	u32 cpuFreq;			/* CPU core frequency in Hz */
	u32 sysFreq;			/* System frequency in Hz */
	u32 cntFreq;			/* Calculated C0_COUNT frequency */

	u8  wlan0_mac[6];
	u8  enet0_mac[6];
	u8  enet1_mac[6];

	u16 pciId;			/* Pseudo PCIID for common code */
	u16 memCap;			/* cap bank1 in MB */

	/* version 3 */
	u8  wlan1_mac[6];		/* (ar5212) */
};

#define BOARD_CONFIG_BUFSZ		0x1000

/*
 * Platform device information for the Ethernet MAC
 */
enum reset_state {
	SET,
	REMOVE,
};

struct ar231x_eth_platform_data {
	u32 base_reset;
	u32 reset_mac;
	u32 reset_phy;

	u8 *mac;

	void (*reset_bit)(u32 val, enum reset_state state);
};

struct ar231x_board_data {
	u16 devid;

	/* board config data */
	struct ar231x_board_config *config;

	struct ar231x_eth_platform_data eth_pdata;
};

void ar231x_find_config(u8 *flash_limit);

void ar231x_reset_bit(u32 val, enum reset_state state);

#endif /* __AR231X_PLATFORM_H */
