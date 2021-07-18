/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/sections.h>

#include "asm/debug_ll.h"
#include "asm/riscv_nmon.h"

.section .text_bare_init

ENTRY(__barebox_nmon_entry)
	riscv_nmon
	ret
ENDPROC(__barebox_nmon_entry)
