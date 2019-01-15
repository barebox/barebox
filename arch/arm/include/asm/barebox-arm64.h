/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _BAREBOX_ARM64_H_
#define _BAREBOX_ARM64_H_

#include <linux/linkage.h>

/*
 * ENTRY_PROC - mark start of entry procedure
 */
#define ENTRY_PROC(name) \
	.section .text_head_entry_##name; \
	ENTRY(##name); \
		b 2f; \
		nop; \
		nop; \
		nop; \
		nop; \
		nop; \
		nop; \
		nop; \
		.asciz "barebox"; \
		.word 0xffffffff; \
		.word _barebox_image_size; \
		.rept 8; \
		.word 0x55555555; \
		.endr; \
		2:

/*
 * ENTRY_PROC_END - mark end of entry procedure
 */
#define ENTRY_PROC_END(name) \
	END(##name)

#endif /* _BAREBOX_ARM64_H_ */
