/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2014, 2015 Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef __HABV4_H
#define __HABV4_H

#include <errno.h>
#include <linux/bits.h>

/* State definitions */
enum habv4_state {
	HAB_STATE_INITIAL = 0x33,	/* Initialising state (transitory) */
	HAB_STATE_CHECK = 0x55,		/* Check state (non-secure) */
	HAB_STATE_NONSECURE = 0x66,	/* Non-secure state */
	HAB_STATE_TRUSTED = 0x99,	/* Trusted state */
	HAB_STATE_SECURE = 0xaa,	/* Secure state */
	HAB_STATE_FAIL_SOFT = 0xcc,	/* Soft fail state */
	HAB_STATE_FAIL_HARD = 0xff,	/* Hard fail state (terminal) */
	HAB_STATE_NONE = 0xf0,		/* No security state machine */
};

#ifdef CONFIG_HABV4
int habv4_get_state(void);
#else
static inline int habv4_get_state(void)
{
	return -ENOSYS;
}
#endif

#define SRK_HASH_SIZE	32

/* Force writing of key, even when a key is already written */
#define IMX_SRK_HASH_FORCE		BIT(0)
/* Permanently write fuses, without this flag only the shadow registers
 * are written.
 */
#define IMX_SRK_HASH_WRITE_PERMANENT	BIT(1)
/* When writing the super root key hash, also burn the write protection
 * fuses so that the key hash can not be modified.
 */
#define IMX_SRK_HASH_WRITE_LOCK		BIT(2)

bool imx_hab_srk_hash_valid(const void *buf);
int imx_hab_write_srk_hash(const void *buf, unsigned flags);
int imx_hab_write_srk_hash_hex(const char *srkhash, unsigned flags);
int imx_hab_write_srk_hash_file(const char *filename, unsigned flags);
int imx_hab_read_srk_hash(void *buf);
int imx_hab_lockdown_device(unsigned flags);
int imx_hab_device_locked_down(void);
int imx_hab_print_status(void);

#endif /* __HABV4_H */
