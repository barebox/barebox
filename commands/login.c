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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <password.h>

#define PASSWD_MAX_LENGTH	(128 + 1)

#if defined(CONFIG_PASSWD_MODE_STAR)
#define LOGIN_MODE STAR
#elif defined(CONFIG_PASSWD_MODE_CLEAR)
#define LOGIN_MODE CLEAR
#else
#define LOGIN_MODE HIDE
#endif

static int do_login(struct command *cmdtp, int argc, char *argv[])
{
	unsigned char passwd[PASSWD_MAX_LENGTH];
	int passwd_len;

	if (!is_passwd_enable()) {
		puts("login: password not set\n");
		return 0;
	}

	do {
		puts("Password: ");
		passwd_len = password(passwd, PASSWD_MAX_LENGTH, LOGIN_MODE);

		if (check_passwd(passwd, passwd_len))
			return 0;
	} while(1);

	return 0;
}

static const __maybe_unused char cmd_login_help[] =
"";

BAREBOX_CMD_START(login)
	.cmd		= do_login,
	.usage		= "login",
	BAREBOX_CMD_HELP(cmd_login_help)
BAREBOX_CMD_END
