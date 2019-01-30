#include <linux/linkage.h>
#include <asm/assembler.h>

/*
 * The .section directive below intentionally omits "a", since that
 * appears to be the simplest way to force assembler to not generate
 * R_AARCH64_RELATIVE relocation for
 *
 * linkadr:
 *	.quad get_runtime_offset
 *
 * statement below. While having that relocating was relatively
 * harmless with GCC8, builging the code with GCC5 resulted in
 * "linkaddr" being initialized to 0 causing complete boot breakdown
 */
.section ".text_bare_init","x"

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
