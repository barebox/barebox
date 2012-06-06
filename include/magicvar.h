#ifndef __MAGIC_VARS_H
#define __MAGIC_VARS_H

#include <linux/stringify.h>

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
#define BAREBOX_MAGICVAR_NAMED(_name, _varname, _description)			\
extern const struct magicvar __barebox_magicvar_##_name;	\
const struct magicvar __barebox_magicvar_##_name		\
        __attribute__ ((unused,section (".barebox_magicvar_" __stringify(_name)))) = {	\
        .name	= #_varname,					\
	.description = MAGICVAR_DESCRIPTION(_description),	\
};

#define BAREBOX_MAGICVAR(_name, _description)			\
	BAREBOX_MAGICVAR_NAMED(_name, _name, _description)
#else
#define BAREBOX_MAGICVAR_NAMED(_name, _varname, _description)
#define BAREBOX_MAGICVAR(_name, _description)
#endif

#endif /* __MAGIC_VARS_H */
