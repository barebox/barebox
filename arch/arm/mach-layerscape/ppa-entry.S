/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>

.section .text.ppa_entry
ENTRY(ppa_entry)
	/*
	 * x0: Secure Firmware entry point
	 * x1: Exception return address Low
	 * x2: Exception return address High
	 */

	/* Save stack pointer for EL2 */
	mov	x3, sp
	msr	sp_el2, x3

	/* Set exception return address hold pointer */
	adr	x4, 1f
	mov	x3, x4
	rev	w3, w3
	str	w3, [x1]
	lsr	x3, x4, #32
	rev	w3, w3
	str	w3, [x2]

	/* Call SEC monitor */
	br	x0

1:
	mov	x0, #0
	ret
ENDPROC(ppa_entry)
