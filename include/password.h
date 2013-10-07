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

int read_passwd(unsigned char *sum, size_t length);
int check_passwd(unsigned char* passwd, size_t length);

int read_env_passwd(unsigned char *sum, size_t length);
int write_env_passwd(unsigned char *sum, size_t length);

int read_default_passwd(unsigned char *sum, size_t length);
int is_passwd_default_enable(void);
int check_default_passwd(unsigned char* passwd, size_t length);

int is_passwd_env_enable(void);
int passwd_env_disable(void);
int check_env_passwd(unsigned char* passwd, size_t length);
int set_env_passwd(unsigned char* passwd, size_t length);

static inline int is_passwd_enable(void)
{
	return is_passwd_default_enable() || is_passwd_env_enable();
}

#endif /* __PASSWORD_H__ */
