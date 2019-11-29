#include <linux/linkage.h>

ENTRY(start_kernel_optee)
	/*
	 * r0 = optee
	 * r1 = kernel
	 * r2 = oftree
	 */
	mov r4, r0
	mov r0, #0
	mov lr, r1
	mov r1, #0
	bx r4
ENDPROC(start_kernel_optee)
