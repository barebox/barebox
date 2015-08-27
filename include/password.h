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

#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#define PASSWD_FILE		"/env/etc/passwd"
#define PASSWD_DIR		"/env/etc/"

#define HIDE	(0 << 0)
#define STAR	(1 << 1)
#define CLEAR	(1 << 2)

int password(unsigned char *passwd, size_t length, int flags, int timeout);
int passwd_env_disable(void);
int set_env_passwd(unsigned char *passwd, size_t length);

#ifdef CONFIG_PASSWORD
void login(void);
#else
static inline void login(void)
{
}
#endif

#endif /* __PASSWORD_H__ */
