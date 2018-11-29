/*
 * Copyright (C) 2014, 2015 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __HABV4_H
#define __HABV4_H

#include <errno.h>

#ifdef CONFIG_HABV4
extern bool habv4_need_rng_software_self_test;
int imx28_hab_get_status(void);
int imx6_hab_get_status(void);
#else
static inline int imx28_hab_get_status(void)
{
	return -EPERM;
}
static inline int imx6_hab_get_status(void)
{
	return -EPERM;
}
#endif

#ifdef CONFIG_HABV3
int imx25_hab_get_status(void);
#else
static inline int imx25_hab_get_status(void)
{
	return -EPERM;
}
#endif

#define SRK_HASH_SIZE	32

/* Force writing of key, even when a key is already written */
#define IMX_SRK_HASH_FORCE		(1 << 0)
/* Permanently write fuses, without this flag only the shadow registers
 * are written.
 */
#define IMX_SRK_HASH_WRITE_PERMANENT	(1 << 1)
/* When writing the super root key hash, also burn the write protection
 * fuses so that the key hash can not be modified.
 */
#define IMX_SRK_HASH_WRITE_LOCK		(1 << 2)

bool imx_hab_srk_hash_valid(const void *buf);
int imx_hab_write_srk_hash(const void *buf, unsigned flags);
int imx_hab_write_srk_hash_hex(const char *srkhash, unsigned flags);
int imx_hab_write_srk_hash_file(const char *filename, unsigned flags);
int imx_hab_read_srk_hash(void *buf);
int imx_hab_lockdown_device(unsigned flags);
int imx_hab_device_locked_down(void);

#endif /* __HABV4_H */
