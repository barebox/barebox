/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION */

#include "cbootimage.h"
#include "string.h"

#ifndef INCLUDED_NVAES_REF_H
#define INCLUDED_NVAES_REF_H

#define NVAES_STATECOLS	4
#define NVAES_KEYCOLS	4
#define NVAES_ROUNDS	10

void nv_aes_expand_key(u_int8_t *key, u_int8_t *expkey);
void nv_aes_encrypt(u_int8_t *in,
		u_int8_t *expkey,
		u_int8_t *out);

#endif // INCLUDED_NVAES_REF_H
