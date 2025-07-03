/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_STM32_BSEC_H__
#define __MACH_STM32_BSEC_H__

#include <mach/stm32mp/smc.h>

/* Return status */
enum bsec_smc {
	BSEC_SMC_OK		= 0,
	BSEC_SMC_ERROR		= -1,
	BSEC_SMC_DISTURBED	= -2,
	BSEC_SMC_INVALID_PARAM	= -3,
	BSEC_SMC_PROG_FAIL	= -4,
	BSEC_SMC_LOCK_FAIL	= -5,
	BSEC_SMC_WRITE_FAIL	= -6,
	BSEC_SMC_SHADOW_FAIL	= -7,
	BSEC_SMC_TIMEOUT	= -8,
};

/* Service for BSEC */
enum bsec_op {
	BSEC_SMC_READ_SHADOW	= 1,
	BSEC_SMC_PROG_OTP	= 2,
	BSEC_SMC_WRITE_SHADOW	= 3,
	BSEC_SMC_READ_OTP	= 4,
	BSEC_SMC_READ_ALL	= 5,
	BSEC_SMC_WRITE_ALL	= 6,
	BSEC_SMC_WRLOCK_OTP	= 7,
};

static inline enum bsec_smc bsec_read_field(unsigned field, unsigned *val)
{
	return stm32mp_smc(STM32_SMC_BSEC, BSEC_SMC_READ_SHADOW,
			   field, 0, val);
}

static inline enum bsec_smc bsec_write_field(unsigned field, unsigned val)
{
	return stm32mp_smc(STM32_SMC_BSEC, BSEC_SMC_WRITE_SHADOW,
			   field, val, NULL);
}

#endif
