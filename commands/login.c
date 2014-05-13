/*
 * Copyright (c) 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <password.h>
#include <getopt.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <init.h>
#include <console.h>

#define PASSWD_MAX_LENGTH	(128 + 1)

#if defined(CONFIG_PASSWD_MODE_STAR)
#define LOGIN_MODE STAR
#elif defined(CONFIG_PASSWD_MODE_CLEAR)
#define LOGIN_MODE CLEAR
#else
#define LOGIN_MODE HIDE
#endif

static int login_timeout = 0;

static int do_login(int argc, char *argv[])
{
	unsigned char passwd[PASSWD_MAX_LENGTH];
	int passwd_len, opt;
	int timeout = login_timeout;
	char *timeout_cmd = "boot";

	console_allow_input(true);
	if (!is_passwd_enable()) {
		puts("login: password not set\n");
		return 0;
	}

	while((opt = getopt(argc, argv, "t:")) > 0) {
		switch(opt) {
		case 't':
			timeout = simple_strtoul(optarg, NULL, 10);
			break;
		}
	}

	if (optind != argc)
		timeout_cmd = argv[optind];

	do {
		puts("Password: ");
		passwd_len = password(passwd, PASSWD_MAX_LENGTH, LOGIN_MODE, timeout);

		if (passwd_len < 0) {
			console_allow_input(false);
			run_command(timeout_cmd);
		}

		if (check_passwd(passwd, passwd_len))
			return 0;
	} while(1);

	return 0;
}

BAREBOX_CMD_HELP_START(login)
BAREBOX_CMD_HELP_TEXT("Asks for a password from the console before script execution continues.")
BAREBOX_CMD_HELP_TEXT("The password can be set with the 'passwd' command. Instead of specifying")
BAREBOX_CMD_HELP_TEXT("a TIMEOUT the magic variable 'global.login.timeout' could be set.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-t TIMEOUT", "Execute COMMAND if no login withing TIMEOUT seconds")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(login)
	.cmd		= do_login,
	BAREBOX_CMD_DESC("ask for a password")
	BAREBOX_CMD_OPTS("[-t TIMEOUT] COMMAND")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_login_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

static int login_global_init(void)
{
	globalvar_add_simple_int("login.timeout", &login_timeout, "%d");

	return 0;
}
late_initcall(login_global_init);

BAREBOX_MAGICVAR_NAMED(global_login_timeout, global.login.timeout, "timeout to type the password");
