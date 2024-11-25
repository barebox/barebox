/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * (C) Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#ifndef __SHELL_H__
#define __SHELL_H__

int shell_get_last_return_code(void);

int run_shell(void);

#ifdef CONFIG_SHELL_HUSH
char *shell_expand(char *str);
#else
static inline char *shell_expand(char *str)
{
	return strdup(str);
}
#endif

#ifdef CONFIG_BAREBOX_CMDLINE
const char *barebox_cmdline_get(void);
#else
static inline const char *barebox_cmdline_get(void)
{
	return NULL;
}
#endif

#endif /* __SHELL_H__ */
