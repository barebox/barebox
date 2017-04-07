/*
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */

#ifndef __CRYPTO_KEYSTORE_H
#define __CRYPTO_KEYSTORE_H

#ifdef CONFIG_CRYPTO_KEYSTORE
int keystore_get_secret(const char *name, const u8 **secret, int *secret_len);
int keystore_set_secret(const char *name, const u8 *secret, int secret_len);
void keystore_forget_secret(const char *name);
#else
static inline int keystore_get_secret(const char *name, const u8 **secret, int *secret_len)
{
	return -EINVAL;
}
static inline int keystore_set_secret(const char *name, const u8 *secret, int secret_len)
{
	return 0;
}
static inline void keystore_forget_secret(const char *name)
{
}
#endif

#endif
