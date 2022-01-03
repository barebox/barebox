/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>

.section .text.ls1046a_init_l2_latency
ENTRY(ls1046a_init_l2_latency)
	/* Initialize the L2 RAM latency */
	mrs   x1, S3_1_c11_c0_2
	mov   x0, #0x1C7
	/* Clear L2 Tag RAM latency and L2 Data RAM latency */
	bic   x1, x1, x0
	/* Set L2 data ram latency bits [2:0] */
	orr   x1, x1, #0x2
	/* set L2 tag ram latency bits [8:6] */
	orr   x1,  x1, #0x80
	msr   S3_1_c11_c0_2, x1
	isb

	ret
ENDPROC(ls1046a_init_l2_latency);
