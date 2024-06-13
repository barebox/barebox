/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <init.h>
#include <asm/system.h>
#include <asm/assembler64.h>

.section ".text_bare_init_","ax"
ENTRY(arm_cpu_lowlevel_init)
	switch_el x1, 3f, 2f, 1f

3:
	mrs	x0, scr_el3
	orr	x0, x0, #0xf		/* SCR_EL3.NS|IRQ|FIQ|EA */
	msr	scr_el3, x0
	msr	cptr_el3, xzr

	mrs	x0, sctlr_el3
	ldr	x1, =SCTLR_ELx_FLAGS
	bic	x0, x0, x1
	msr	sctlr_el3, x0
	isb

	b	done

2:
	mov	x0, #0x33ff		/* Enable FP/SIMD */
	msr	cptr_el2, x0
	b	done


1:
	mov	x0, #(3 << 20)		/* Enable FP/SIMD */
	msr	cpacr_el1, x0
	b	done

done:
	ret

ENDPROC(arm_cpu_lowlevel_init)
