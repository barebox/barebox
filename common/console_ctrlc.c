// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <console.h>
#include <sched.h>
#include <globalvar.h>
#include <magicvar.h>

static int ctrlc_abort;
static int ctrlc_allowed;

void ctrlc_handled(void)
{
	ctrlc_abort = 0;
}

int ctrlc_non_interruptible(void)
{
	int ret = 0;

	if (!ctrlc_allowed)
		return 0;

	if (ctrlc_abort)
		return 1;

#ifdef CONFIG_ARCH_HAS_CTRLC
	ret = arch_ctrlc();
#else
	if (tstc() && getchar() == 3)
		ret = 1;
#endif

	if (ret)
		ctrlc_abort = 1;

	return ret;
}
EXPORT_SYMBOL(ctrlc_non_interruptible);

/* test if ctrl-c was pressed */
int ctrlc(void)
{
	resched();
	return ctrlc_non_interruptible();
}
EXPORT_SYMBOL(ctrlc);

static int console_ctrlc_init(void)
{
	globalvar_add_simple_bool("console.ctrlc_allowed", &ctrlc_allowed);
	return 0;
}
device_initcall(console_ctrlc_init);

void console_ctrlc_allow(void)
{
	ctrlc_allowed = 1;
}

void console_ctrlc_forbid(void)
{
	ctrlc_allowed = 0;
}

BAREBOX_MAGICVAR(global.console.ctrlc_allowed,
		"If true, scripts can be aborted with ctrl-c");
