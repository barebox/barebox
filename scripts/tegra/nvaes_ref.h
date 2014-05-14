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
