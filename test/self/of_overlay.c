// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <of.h>

BSELFTEST_GLOBALS();

static void test_of_overlay(void)
{
}
bselftest(core, test_of_overlay);
