/* SPDX-License-Identifier: GPL-2.0-only */

#define pr_fmt(fmt) "stackprot: " fmt

#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <init.h>
#include <stdlib.h>

#ifdef __PBL__
#define STAGE "PBL"
#else
#define STAGE "barebox"
#endif

void __stack_chk_fail(void);

volatile ulong __stack_chk_guard = (ulong)(0xfeedf00ddeadbeef & ~0UL);

/*
 * Called when gcc's -fstack-protector feature is used, and
 * gcc detects corruption of the on-stack canary value
 */
noinstr void __stack_chk_fail(void)
{
	panic("stack-protector: " STAGE " stack is corrupted in: %pS\n",
	      (void *)_RET_IP_);
}
EXPORT_SYMBOL(__stack_chk_fail);

static __no_stack_protector int stackprot_randomize_guard(void)
{
	ulong chk_guard;
	int ret;

	ret = get_crypto_bytes(&chk_guard, sizeof(chk_guard));
	if (ret)
		pr_warn("proceeding without randomized stack protector\n");
	else
		__stack_chk_guard = chk_guard;

	return 0;
}
late_initcall(stackprot_randomize_guard);
