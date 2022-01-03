/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/asm.h>

	.section .data
EXPORT(input_data)
	.incbin	"arch/mips/pbl/piggy.lzo"
EXPORT(input_data_end)
