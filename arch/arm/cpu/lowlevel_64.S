#include <linux/linkage.h>
#include <init.h>
#include <asm/system.h>

.section ".text_bare_init_","ax"
ENTRY(arm_cpu_lowlevel_init)
	adr	x0, vectors
	mrs	x1, CurrentEL
	cmp	x1, #0xC		/* Check EL3 state */
	b.eq	1f
	cmp	x1, #0x8		/* Check EL2 state */
	b.eq	2f
	cmp	x1, #0x4		/* Check EL1 state */
	b.eq	3f

1:
	msr	vbar_el3, x0
	mov	x0, #1			/* Non-Secure EL0/1 */
	orr	x0, x0, #(1 << 10)	/* 64-bit EL2 */
	msr	scr_el3, x0
	msr	cptr_el3, xzr
	b	done

2:
	msr	vbar_el2, x0
	mov	x0, #0x33ff		/* Enable FP/SIMD */
	msr	cptr_el2, x0
	b	done


3:
	msr	vbar_el1, x0
	mov	x0, #(3 << 20)		/* Enable FP/SIMD */
	msr	cpacr_el1, x0
	b	done

done:
	ret

ENDPROC(arm_cpu_lowlevel_init)
