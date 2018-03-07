#include <linux/linkage.h>
#include <asm/assembler.h>

.section ".text_bare_init","ax"

/*
 * Get the offset between the link address and the address
 * we are currently running at.
 */
ENTRY(get_runtime_offset)
1:	adr x0, 1b
	ldr x1, linkadr
	subs x0, x0, x1
	ret

.align 3
linkadr:
.quad get_runtime_offset
ENDPROC(get_runtime_offset)
