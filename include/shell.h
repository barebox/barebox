/*
 * (C) Copyright 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
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

#endif /* __SHELL_H__ */
