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
			run_command(timeout_cmd, 0);
		}

		if (check_passwd(passwd, passwd_len))
			return 0;
	} while(1);

	return 0;
}

static const __maybe_unused char cmd_login_help[] =
"Usage: login [-t timeout [<command>]]\n"
"If a timeout is specified and expired the command will be executed;\n"
"\"boot\" by default\n"
;

BAREBOX_CMD_START(login)
	.cmd		= do_login,
	.usage		= "login",
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
