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
#include <errno.h>

#define PASSWD_MAX_LENGTH	(128 + 1)

#if defined(CONFIG_PASSWD_MODE_STAR)
#define PASSWD_MODE STAR
#elif defined(CONFIG_PASSWD_MODE_CLEAR)
#define PASSWD_MODE CLEAR
#else
#define PASSWD_MODE HIDE
#endif

static int do_passwd(int argc, char *argv[])
{
	unsigned char passwd2[PASSWD_MAX_LENGTH];
	unsigned char passwd1[PASSWD_MAX_LENGTH];
	int passwd1_len;
	int passwd2_len;
	int ret = 1;

	puts("Enter new password: ");
	passwd1_len = password(passwd1, PASSWD_MAX_LENGTH, PASSWD_MODE, 0);

	if (passwd1_len < 0)
		return 1;

	puts("Retype new password: ");
	passwd2_len = password(passwd2, PASSWD_MAX_LENGTH, PASSWD_MODE, 0);

	if (passwd2_len < 0)
		return 1;

	if (passwd2_len != passwd1_len) {
		goto err;
	} else {
		if (passwd1_len == 0) {
			ret = 0;
			goto disable;
		}

		if (strncmp(passwd1, passwd2, passwd1_len) != 0)
			goto err;
	}

	ret = set_env_passwd(passwd1, passwd1_len);

	if (ret < 0) {
		puts("Sorry, passwords write failed\n");
		ret = 1;
		goto disable;
	}

	return 0;
err:
	puts("Sorry, passwords do not match\n");
	puts("passwd: password unchanged\n");
	return 1;

disable:
	passwd_env_disable();
	puts("passwd: password disabled\n");
	return ret;
}

static const __maybe_unused char cmd_passwd_help[] =
"Usage: passwd\n"
"passwd allow you to specify a password in the env\n"
"to disable it put an empty password will still use the default password if set\n"
;

BAREBOX_CMD_START(passwd)
	.cmd		= do_passwd,
	.usage		= "passwd",
	BAREBOX_CMD_HELP(cmd_passwd_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
