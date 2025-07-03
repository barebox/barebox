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
#include <mach/imx/imx6.h>
#include <mach/imx/tzasc.h>

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

int imx6q_start_optee_early(void *fdt, void *tee, void *data_location,
			    unsigned int data_location_size)
{
	if (imx6q_tzc380_is_bypassed())
		panic("TZC380 is bypassed, abort OP-TEE loading\n");

	/* Add early non-secure TZASC region1 to pass DTO */
	imx6q_tzc380_early_ns_region1();

	/*
	 * Set the OP-TEE <-> barebox exchange data location to zero.
	 * This is optional since recent OP-TEE versions perform the
	 * memset too.
	 */
	if (data_location)
		memset(data_location, 0, data_location_size);

	return start_optee_early(fdt, tee);
}

int imx6ul_start_optee_early(void *fdt, void *tee, void *data_location,
			     unsigned int data_location_size)
{
	if (imx6ul_tzc380_is_bypassed())
		panic("TZC380 is bypassed, abort OP-TEE loading\n");

	/* Add early non-secure TZASC region1 to pass DTO */
	imx6ul_tzc380_early_ns_region1();

	/*
	 * Set the OP-TEE <-> barebox exchange data location to zero.
	 * This is optional since recent OP-TEE versions perform the
	 * memset too.
	 */
	if (data_location)
		memset(data_location, 0, data_location_size);

	return start_optee_early(fdt, tee);
}
