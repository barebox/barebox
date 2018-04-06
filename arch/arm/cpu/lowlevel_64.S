#include <linux/linkage.h>
#include <init.h>
#include <asm/system.h>
#include <asm/assembler64.h>

.section ".text_bare_init_","ax"
ENTRY(arm_cpu_lowlevel_init)
	switch_el x1, 3f, 2f, 1f

3:
	mov	x0, #1			/* Non-Secure EL0/1 */
	orr	x0, x0, #(1 << 10)	/* 64-bit EL2 */
	msr	scr_el3, x0
	msr	cptr_el3, xzr
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
