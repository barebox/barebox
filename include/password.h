/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
