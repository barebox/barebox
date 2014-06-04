/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

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
