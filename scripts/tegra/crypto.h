/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION */

/*
 * crypto.h - Definitions for the crypto support.
 */

#ifndef INCLUDED_CRYPTO_H_N
#define INCLUDED_CRYPTO_H_N

#include "cbootimage.h"

/* lengths, in bytes */
#define KEY_LENGTH (128/8)

#define ICEIL(a, b) (((a) + (b) - 1)/(b))

#define AES_CMAC_CONST_RB 0x87  // from RFC 4493, Figure 2.2

/* Function prototypes */

int
sign_bct(build_image_context *context,
                  u_int8_t *bct);

int
sign_data_block(u_int8_t *source,
		u_int32_t length,
		u_int8_t *signature);

#endif /* #ifndef INCLUDED_CRYPTO_H */
