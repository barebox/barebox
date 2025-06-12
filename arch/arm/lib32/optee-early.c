// SPDX-License-Identifier: GPL-2.0
/*
 * optee-early.c - start OP-TEE during PBL
 *
 * Copyright (c) 2019 Rouven Czerwinski <r.czerwinski@pengutronix.de>, Pengutronix
 *
 */
#include <asm/cache.h>
#include <asm/setjmp.h>
#include <asm/system.h>
#include <tee/optee.h>
#include <linux/bug.h>
#include <debug_ll.h>
#include <string.h>

static jmp_buf tee_buf;

int start_optee_early(void *fdt, void *tee)
{
	void (*tee_start)(void *r0, void *r1, void *r2);
	struct optee_header *hdr;
	int ret;

	/* We expect this function to be called with data caches disabled */
	BUG_ON(get_cr() & CR_C);

	hdr = tee;
	ret = optee_verify_header(hdr);
	if (ret < 0)
		return ret;

	memcpy((void *)hdr->init_load_addr_lo, tee + sizeof(*hdr), hdr->init_size);
	tee_start = (void *) hdr->init_load_addr_lo;

	/* We use setjmp/longjmp here because OP-TEE clobbers most registers */
	ret = setjmp(tee_buf);
	if (ret == 0) {
		tee_start(0, 0, fdt);
		longjmp(tee_buf, 1);
	}

	return 0;
}
