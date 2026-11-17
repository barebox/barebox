// SPDX-License-Identifier: GPL-2.0-only

#include <init.h>
#include <linux/printk.h>

static int rust_init(void)
{
	return rust_setup_log();
}
pure_initcall(rust_init);
