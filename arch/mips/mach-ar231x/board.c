// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Based on Linux driver:
 *  Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *  Copyright (C) 2006 FON Technology, SL.
 *  Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *  Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */


#include <common.h>
#include <io.h>
#include <mach/ar2312_regs.h>
#include <mach/ar231x_platform.h>
#include <linux/stddef.h>
#include <net.h>

#define HDR_SIZE 6

#define TAB_1 "\t"
#define TAB_2 "\t\t"

extern struct ar231x_board_data ar231x_board;

static void
ar231x_print_mac(unsigned char *mac)
{
	int i;
	for (i = 0; i < 5; i++)
		printf("%02x:", mac[i]);
	printf("%02x\n", mac[5]);
}

#ifdef DEBUG_BOARD
static void
ar231x_print_board_config(struct ar231x_board_config *config)
{
	printf("board config:\n");
	printf(TAB_1 "chsum:"		TAB_2 "%04x\n", config->cksum);
	printf(TAB_1 "rev:"		TAB_2 "%04x\n", config->rev);
	printf(TAB_1 "name:"		TAB_2 "%s\n", config->board_name);
	printf(TAB_1 "maj:"		TAB_2 "%04x\n", config->major);
	printf(TAB_1 "min:"		TAB_2 "%04x\n", config->minor);
	printf(TAB_1 "board flags:" TAB_1 "%08x\n", config->flags);

	if (config->flags & BD_ENET0)
		printf(TAB_2 "ENET0 is stuffed\n");
	if (config->flags & BD_ENET1)
		printf(TAB_2 "ENET1 is stuffed\n");
	if (config->flags & BD_UART1)
		printf(TAB_2 "UART1 is stuffed\n");
	if (config->flags & BD_UART0)
		printf(TAB_2 "UART0 is stuffed (dma)\n");
	if (config->flags & BD_RSTFACTORY)
		printf(TAB_2 "Reset factory defaults stuffed\n");
	if (config->flags & BD_SYSLED)
		printf(TAB_2 "System LED stuffed\n");
	if (config->flags & BD_EXTUARTCLK)
		printf(TAB_2 "External UART clock\n");
	if (config->flags & BD_CPUFREQ)
		printf(TAB_2 "cpu freq is valid in nvram\n");
	if (config->flags & BD_SYSFREQ)
		printf(TAB_2 "sys freq is set in nvram\n");
	if (config->flags & BD_WLAN0)
		printf(TAB_2 "Enable WLAN0\n");
	if (config->flags & BD_MEMCAP)
		printf(TAB_2 "CAP SDRAM @ memCap for testing\n");
	if (config->flags & BD_DISWATCHDOG)
		printf(TAB_2 "disable system watchdog\n");
	if (config->flags & BD_WLAN1)
		printf(TAB_2 "Enable WLAN1 (ar5212)\n");
	if (config->flags & BD_ISCASPER)
		printf(TAB_2 "FLAG for AR2312\n");
	if (config->flags & BD_WLAN0_2G_EN)
		printf(TAB_2 "FLAG for radio0_2G\n");
	if (config->flags & BD_WLAN0_5G_EN)
		printf(TAB_2 "FLAG for radio0_5G\n");
	if (config->flags & BD_WLAN1_2G_EN)
		printf(TAB_2 "FLAG for radio1_2G\n");
	if (config->flags & BD_WLAN1_5G_EN)
		printf(TAB_2 "FLAG for radio1_5G\n");

	printf(TAB_1 "ResetConf GPIO pin:" TAB_1 "%04x\n",
	       config->reset_config_gpio);
	printf(TAB_1 "Sys LED GPIO pin:" TAB_1 "%04x\n", config->sys_led_gpio);
	printf(TAB_1 "CPU Freq:" TAB_2 "%u\n", config->cpu_freq);
	printf(TAB_1 "Sys Freq:" TAB_2 "%u\n", config->sys_freq);
	printf(TAB_1 "Calculated Freq:" TAB_1 "%u\n", config->cnt_freq);

	printf(TAB_1 "wlan0 mac:" TAB_2);
	ar231x_print_mac(config->wlan0_mac);
	printf(TAB_1 "wlan1 mac:" TAB_2);
	ar231x_print_mac(config->wlan1_mac);
	printf(TAB_1 "eth0 mac:" TAB_2);
	ar231x_print_mac(config->enet0_mac);
	printf(TAB_1 "eth1 mac:" TAB_2);
	ar231x_print_mac(config->enet1_mac);

	printf(TAB_1 "Pseudo PCIID:" TAB_2 "%04x\n", config->pci_id);
	printf(TAB_1 "Mem capacity:" TAB_2 "%u\n", config->mem_cap);
}
#endif

static u16
ar231x_cksum16(u8 *data, int size)
{
	int i;
	u16 sum = 0;

	for (i = 0; i < size; i++)
		sum += data[i];

	return sum;
}

static void
ar231x_check_mac(u8 *addr)
{
	if (!is_valid_ether_addr(addr)) {
		pr_warn("config: no valid mac was found. "
				"Generating random mac: ");
		random_ether_addr(addr);
		ar231x_print_mac(addr);
	}
}

static u8 *
ar231x_find_board_config(u8 *flash_limit)
{
	u8 *addr;
	int found = 0;

	for (addr = flash_limit - 0x1000;
		addr >= flash_limit - 0x30000;
		addr -= 0x1000) {

		if (*((u32 *)addr) == AR231X_BD_MAGIC) {
			found = 1;
			pr_debug("config at %x\n", addr);
			break;
		}
	}

	if (!found)
		addr = NULL;

	return addr;
}

void
ar231x_find_config(u8 *flash_limit)
{
	struct ar231x_board_config *config;
	u8 *bcfg, bsize;
	u8 brocken;

	bcfg = ar231x_find_board_config(flash_limit);

	bsize = sizeof(struct ar231x_board_config);
	config = xzalloc(bsize);
	ar231x_board.config = config;

	if (bcfg) {
		u16 cksum;
		/* Copy the board and radio data to RAM.
		 * If flash will go to CFI mode, we won't
		 * be able to read to from mapped memory area */
		memcpy(config, bcfg, bsize);
		cksum = 0xca + ar231x_cksum16((unsigned char *)config + HDR_SIZE,
				sizeof(struct ar231x_board_config) - HDR_SIZE);
		if (cksum != config->cksum) {
			pr_err("config: checksum is invalid: %04x != %04x\n",
					cksum, config->cksum);
			brocken = 1;
		}
		/* ar231x_print_board_config(config); */
	}

	/* We do not care about wlans here */
	ar231x_check_mac(config->enet0_mac);
	ar231x_check_mac(config->enet1_mac);
}
