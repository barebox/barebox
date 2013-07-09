#include <config.h>
#include <linux/linkage.h>
#include <asm/assembler.h>

.section ".text_bare_init","ax"
.globl omap_bootinfo
omap_bootinfo:
	.word 0x0
	.word 0x0
	.word 0x0

.section ".text_bare_init","ax"
ENTRY(omap_save_bootinfo)
	/*
	* save data from rom boot loader
	*/
	adr     r2, omap_bootinfo
	ldr     r1, [r0, #0x00]
	str     r1, [r2, #0x00]
	ldr     r1, [r0, #0x04]
	str     r1, [r2, #0x04]
	ldr     r1, [r0, #0x08]
	str     r1, [r2, #0x08]
	mov	pc, lr
ENDPROC(omap_save_bootinfo)
