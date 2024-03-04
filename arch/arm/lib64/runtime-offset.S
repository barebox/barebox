/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/assembler.h>

.section ".text_bare_init","ax"

/*
 * Get the offset between the link address and the address
 * we are currently running at.
 */
ENTRY(get_runtime_offset)
	adr x0, _text
	ldr_l x1, linkadr
	subs x0, x0, x1
	ret
ENDPROC(get_runtime_offset)

.section ".data.runtime_offset","a"
.align 3
linkadr:
/*
 * With older gcc versions (gcc5) function pointers will not be filled
 * into the binary during compile time and instead rely on relocation
 * during runtime. In the binary we'll always have 0x0 here. We deliberately
 * use _text here since that is 0x0 and is correct without relocation.
 */
.quad _text
