// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Intel Corporation

/* Author: H. Peter Anvin */

/**
 * @file
 * @brief Simple helper function for initializing a register set.
 *
 * Note that this sets EFLAGS_CF in the input register set; this
 * makes it easier to catch functions which do nothing but don't
 * explicitly set CF.
 */

#include <asm/segment.h>
#include "boot.h"

/* be aware of: */
THIS_IS_REALMODE_CODE

void __bootcode initregs(struct biosregs *reg)
{
	memset(reg, 0, sizeof *reg);
	reg->eflags |= X86_EFLAGS_CF;
	reg->ds = ds();
	reg->es = ds();
	reg->fs = fs();
	reg->gs = gs();
}
