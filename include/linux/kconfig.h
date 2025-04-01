/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_KCONFIG_H
#define __LINUX_KCONFIG_H

#include <generated/autoconf.h>
#include <linux/is_defined.h>

/*
 * IS_BUILTIN(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y', 0
 * otherwise. For boolean options, this is equivalent to
 * IS_ENABLED(CONFIG_FOO).
 */
#define IS_BUILTIN(option) __is_defined(option)

/*
 * IS_MODULE(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'm', 0
 * otherwise.
 */
#define IS_MODULE(option) __is_defined(option##_MODULE)

/*
 * IS_REACHABLE(CONFIG_FOO) evaluates to 1 if the currently compiled
 * code can call a function defined in code compiled based on CONFIG_FOO.
 * This is similar to IS_ENABLED(), but returns false when invoked from
 * built-in code when CONFIG_FOO is set to 'm'.
 */
#define IS_REACHABLE(option) __or(IS_BUILTIN(option), \
				__and(IS_MODULE(option), __is_defined(MODULE)))

/*
 * IS_ENABLED(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y' or 'm',
 * 0 otherwise.
 */
#define IS_ENABLED(option) __or(IS_BUILTIN(option), IS_MODULE(option))

#define IF_ENABLED__0(args...)
#define IF_ENABLED__1(args...)		args
#define IF_ENABLED__(optval, args...)	IF_ENABLED__##optval(args)
#define IF_ENABLED_(optval, args...)	IF_ENABLED__(optval, args)
#define IF_ENABLED(option, args...)	IF_ENABLED_(IS_ENABLED(option), args)

#ifdef __PBL__
#define IN_PBL		1
#define IN_PROPER	0
#else
#define IN_PBL		0
#define IN_PROPER	1
#endif

#endif /* __LINUX_KCONFIG_H */
