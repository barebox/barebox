/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <bootstrap.h>
#include <mach/bootstrap.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <restart.h>
#include <init.h>
#include <menu.h>

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

#if defined(CONFIG_MENU) && !defined(CONFIG_NONE)
#define is_menu() 1
#else
#define is_menu() 0
#endif

static char* is_barebox_to_str(bool is_barebox)
{
	return is_barebox ? "barebox" : "unknown";
}

static void at91bootstrap_boot_m25p80(bool is_barebox)
{
	char *name = is_barebox_to_str(is_barebox);
	kernel_entry_func func = NULL;

	func = bootstrap_board_read_m25p80();
	printf("Boot %s from m25p80\n", name);
	bootstrap_boot(func, is_barebox);
	bootstrap_err("... failed\n");
	free(func);
}

static void at91bootstrap_boot_dataflash(bool is_barebox)
{
	char *name = is_barebox_to_str(is_barebox);
	kernel_entry_func func = NULL;

	printf("Boot %s from dataflash\n", name);
	func = bootstrap_board_read_dataflash();
	bootstrap_boot(func, is_barebox);
	bootstrap_err("... failed\n");
	free(func);
}

static void at91bootstrap_boot_nand(bool is_barebox)
{
	char *name = is_barebox_to_str(is_barebox);
	kernel_entry_func func = NULL;

	printf("Boot %s from nand\n", name);
	func = bootstrap_read_devfs("nand0", true, SZ_128K, SZ_256K, SZ_1M);
	bootstrap_boot(func, is_barebox);
	bootstrap_err("... failed\n");
	free(func);
}

static void at91bootstrap_boot_mmc(void)
{
	kernel_entry_func func = NULL;

	printf("Boot from mmc\n");
	func = bootstrap_read_disk("disk0.0", NULL);
	bootstrap_boot(func, false);
	bootstrap_err("... failed\n");
	free(func);
}

static void boot_nand_barebox_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_nand(true);

	getchar();
}

static void boot_nand_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_nand(false);

	getchar();
}

static void boot_m25p80_barebox_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_nand(true);

	getchar();
}

static void boot_m25p80_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_nand(false);

	getchar();
}

static void boot_dataflash_barebox_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_dataflash(true);

	getchar();
}

static void boot_dataflash_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_dataflash(false);

	getchar();
}

static void boot_mmc_disk_action(struct menu *m, struct menu_entry *me)
{
	at91bootstrap_boot_mmc();

	getchar();
}

static void boot_reset_action(struct menu *m, struct menu_entry *me)
{
	restart_machine();
}

static void at91_bootstrap_menu(void)
{
	struct menu *m;
	struct menu_entry *me;

	m = menu_alloc();
	m->name = "boot";
	menu_add_title(m, m->name);

	menu_add(m);

	if (is_mmc()) {
		me = menu_entry_alloc();
		me->action = boot_mmc_disk_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "mmc";
		menu_add_entry(m, me);
	}

	if (is_m25p80()) {
		me = menu_entry_alloc();
		me->action = boot_m25p80_barebox_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "m25p80 (barebox)";
		menu_add_entry(m, me);

		me = menu_entry_alloc();
		me->action = boot_m25p80_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "m25p80";
		menu_add_entry(m, me);
	}

	if (is_dataflash()) {
		me = menu_entry_alloc();
		me->action = boot_dataflash_barebox_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "dataflash (barebox)";
		menu_add_entry(m, me);

		me = menu_entry_alloc();
		me->action = boot_dataflash_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "dataflash";
		menu_add_entry(m, me);
	}

	if (is_nand()) {
		me = menu_entry_alloc();
		me->action = boot_nand_barebox_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "nand (barebox)";
		menu_add_entry(m, me);

		me = menu_entry_alloc();
		me->action = boot_nand_action;
		me->type = MENU_ENTRY_NORMAL;
		me->display = "nand";
		menu_add_entry(m, me);
	}

	me = menu_entry_alloc();
	me->action = boot_reset_action;
	me->type = MENU_ENTRY_NORMAL;
	me->display = "reset";
	menu_add_entry(m, me);

	menu_show(m);
}

static void boot_seq(bool is_barebox)
{
	if (is_m25p80())
		at91bootstrap_boot_m25p80(is_barebox);

	if (is_dataflash())
		at91bootstrap_boot_dataflash(is_barebox);

	if (is_nand())
		at91bootstrap_boot_nand(is_barebox);
}

static int at91_bootstrap(void)
{
	if (is_menu()) {
		printf("press 'm' to start the menu\n");
		if (tstc() && getchar() == 'm')
			at91_bootstrap_menu();
	}

	if (is_mmc())
		at91bootstrap_boot_mmc();

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
