/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2010 Charles Cazabon <charlesc-memtester@pyropus.ca>
 *
 * This file contains typedefs, structure, and union definitions.
 *
 */

#include "sizes.h"

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

struct test {
    char *name;
    int (*fp)(ulv *, ulv *, size_t);
};

typedef union {
    unsigned char bytes[UL_LEN/8];
    ul val;
} mword8_t;

typedef union {
    unsigned short u16s[UL_LEN/16];
    ul val;
} mword16_t;
