#include <linux/linkage.h>
#include <asm/assembler.h>

.section ".text_bare_init","ax"

/*
 * Get the offset between the link address and the address
 * we are currently running at.
 */
ENTRY(get_runtime_offset)
1:	adr r0, 1b
	ldr r1, linkadr
	subs r0, r0, r1
THUMB(	adds r0, r0, #1)
	mov pc, lr

linkadr:
.word get_runtime_offset
ENDPROC(get_runtime_offset)
