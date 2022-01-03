// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/compiler.h>
#include <asm/barebox-arm-head.h>

void go(void);

void __naked __section(.flash_header_start) go(void)
{
        barebox_arm_head();
}
