/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 *
 * This file contains the declarations for external variables from the main file.
 * See other comments in that file.
 *
 */

#include <types.h>

/* extern declarations. */

extern int memtester_use_phys;
extern off_t memtester_physaddrbase;

