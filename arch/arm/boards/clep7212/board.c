// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru>

#include <envfs.h>
#include <init.h>
#include <of.h>

static __init int clep7212_init(void)
{
	if (of_machine_is_compatible("cirrus,clep7212"))
		defaultenv_append_directory(defaultenv_clep7212);

	return 0;
}
device_initcall(clep7212_init);
