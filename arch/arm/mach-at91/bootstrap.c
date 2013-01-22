/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <bootstrap.h>
#include <mach/bootstrap.h>
#include <sizes.h>
#include <malloc.h>
#include <init.h>

#if defined(CONFIG_MCI_ATMEL)
#define is_mmc() 1
#else
#define is_mmc() 0
#endif

#ifdef CONFIG_NAND_ATMEL
#define is_nand() 1
#else
#define is_nand() 0
#endif

#ifdef CONFIG_MTD_M25P80
#define is_m25p80() 1
#else
#define is_m25p80() 0
#endif

#ifdef CONFIG_MTD_DATAFLASH
#define is_dataflash() 1
#else
#define is_dataflash() 0
#endif

static void boot_seq(bool is_barebox)
{
	char *name = is_barebox ? "barebox" : "unknown";
	int (*func)(void) = NULL;

	if (is_m25p80()) {
		func = bootstrap_board_read_m25p80();
		printf("Boot %s from m25p80\n", name);
		bootstrap_boot(func, is_barebox);
		bootstrap_err("... failed\n");
		free(func);
	}
	if (is_dataflash()) {
		printf("Boot %s from dataflash\n", name);
		func = bootstrap_board_read_dataflash();
		bootstrap_boot(func, is_barebox);
		bootstrap_err("... failed\n");
		free(func);
	}
	if (is_nand()) {
		printf("Boot %s from nand\n", name);
		func = bootstrap_read_devfs("nand0", true, SZ_128K, SZ_256K, SZ_1M);
		bootstrap_boot(func, is_barebox);
		bootstrap_err("... failed\n");
		free(func);
	}
}

static int at91_bootstrap(void)
{
	int (*func)(void) = NULL;

	if (is_mmc()) {
		printf("Boot from mmc\n");
		func = bootstrap_read_disk("disk0.0", NULL);
		bootstrap_boot(func, false);
		bootstrap_err("... failed\n");
		free(func);
	}

	/* First only bootstrap_boot a barebox */
	boot_seq(true);
	/* Second bootstrap_boot any */
	boot_seq(false);

	bootstrap_err("bootstrap_booting failed\n");
	while (1);

	return 0;
}

static int at91_set_bootstrap(void)
{
	barebox_main = at91_bootstrap;

	return 0;
}
late_initcall(at91_set_bootstrap);
