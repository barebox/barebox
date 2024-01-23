/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MAGIC_VARS_H
#define __MAGIC_VARS_H

#include <linux/stringify.h>
#include <linux/compiler_types.h>

struct magicvar {
	const char *name;
	const char *description;
};

extern struct magicvar __barebox_magicvar_start;
extern struct magicvar __barebox_magicvar_end;

#ifdef CONFIG_CMD_MAGICVAR_HELP
#define MAGICVAR_DESCRIPTION(d)		(d)
#else
#define MAGICVAR_DESCRIPTION(d)		NULL
#endif

#ifdef CONFIG_CMD_MAGICVAR
#define __BAREBOX_MAGICVAR_NAMED(_name, _varname, _description)	\
static const struct magicvar _name				\
        __ll_elem(.barebox_magicvar_##_name) = {		\
        .name	= #_varname,					\
	.description = MAGICVAR_DESCRIPTION(_description),	\
};

#define BAREBOX_MAGICVAR(_name, _description)			\
	__BAREBOX_MAGICVAR_NAMED(__UNIQUE_ID(magicvar), _name, _description)
#else
#define BAREBOX_MAGICVAR(_name, _description)
#endif

#endif /* __MAGIC_VARS_H */
