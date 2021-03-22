/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: Copyright (c) 2021 Ahmad Fatoum, Pengutronix */
#include <linux/linkage.h>
#include <asm/sections.h>

.section ".text_bare_init","ax"
ENTRY(get_runtime_offset)
	lla	a0, _text	/* load addr */
	la	a1, _text	/* link addr */
	sub	a0, a0, a1
	ret
ENDPROC(get_runtime_offset)
