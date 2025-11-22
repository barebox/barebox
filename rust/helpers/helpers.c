// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/helpers/helpers.c?id=ffd294d346d185b70e28b1a28abe367bbfe53c04
/*
 * Non-trivial C macros cannot be used in Rust. Similarly, inlined C functions
 * cannot be called either. This file explicitly creates functions ("helpers")
 * that wrap those so that they can be called from Rust.
 *
 * Sorted alphabetically.
 */

#include "alloc.c"
#include "bitops.c"
#include "bug.c"
#include "build_assert.c"
#include "build_bug.c"
#include "ktime.c"
#include "err.c"
#include "processor.c"
#include "time.c"
