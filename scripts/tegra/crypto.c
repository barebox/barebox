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
 * crypto.c - Cryptography support
 */
#include "crypto.h"
#include "parse.h"
#include "nvaes_ref.h"
#include <stdio.h>

/* Local function declarations */
static void
apply_cbc_chain_data(u_int8_t *cbc_chain_data,
			u_int8_t *src,
			u_int8_t *dst);

static void
generate_key_schedule(u_int8_t *key, u_int8_t *key_schedule);

static void
encrypt_object( u_int8_t	*key_schedule,
		u_int8_t	*src,
		u_int8_t	*dst,
		u_int32_t	num_aes_blocks);

static int
encrypt_and_sign(u_int8_t		*key,
		 u_int8_t		*src,
		 u_int32_t		length,
		 u_int8_t		*sig_dst);

u_int8_t enable_debug_crypto = 0;

/* Implementation */
static u_int8_t zero_key[16] = { 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0 };

static void
print_vector(char *name, u_int32_t num_bytes, u_int8_t *data)
{
	u_int32_t i;

	printf("%s [%d] @%p", name, num_bytes, data);
	for (i=0; i<num_bytes; i++) {
		if ( i % 16 == 0 )
			printf(" = ");
		printf("%02x", data[i]);
		if ( (i+1) % 16 != 0 )
			printf(" ");
	}
	printf("\n");
}


static void
apply_cbc_chain_data(u_int8_t *cbc_chain_data,
			u_int8_t *src,
			u_int8_t *dst)
{
	int i;

	for (i = 0; i < 16; i++) {
		*dst++ = *src++ ^ *cbc_chain_data++;
	}
}

static void
generate_key_schedule(u_int8_t *key, u_int8_t *key_schedule)
{
	/*
	 * The only need for a key is for signing/checksum purposes, so
	 * expand a key of 0's.
	 */
	nv_aes_expand_key(zero_key, key_schedule);
}

static void
encrypt_object(u_int8_t	*key_schedule,
		u_int8_t	*src,
		u_int8_t	*dst,
		u_int32_t	num_aes_blocks)
{
	u_int32_t i;
	u_int8_t *cbc_chain_data;
	u_int8_t  tmp_data[KEY_LENGTH];

	cbc_chain_data = zero_key; /* Convenient array of 0's for IV */

	for (i = 0; i < num_aes_blocks; i++) {
		if (enable_debug_crypto) {
			printf("encrypt_object: block %d of %d\n", i,
							num_aes_blocks);
			print_vector("AES Src", KEY_LENGTH, src);
		}

		/* Apply the chain data */
		apply_cbc_chain_data(cbc_chain_data, src, tmp_data);

		if (enable_debug_crypto)
			print_vector("AES Xor", KEY_LENGTH,
							tmp_data);

		/* encrypt the AES block */
		nv_aes_encrypt(tmp_data, key_schedule, dst);

		if (enable_debug_crypto)
			print_vector("AES Dst", KEY_LENGTH, dst);
		/* Update pointers for next loop. */
		cbc_chain_data = dst;
		src += KEY_LENGTH;
		dst += KEY_LENGTH;
	}
}

static void
left_shift_vector(u_int8_t *in,
		  u_int8_t	*out,
		  u_int32_t size)
{
	u_int32_t i;
	u_int8_t carry = 0;

	for (i=0; i<size; i++) {
		u_int32_t j = size-1-i;

		out[j] = (in[j] << 1) | carry;
		carry = in[j] >> 7; /* get most significant bit */
	}
}

static void
sign_objext(
	u_int8_t	*key,
	u_int8_t	*key_schedule,
	u_int8_t	*src,
	u_int8_t	*dst,
	u_int32_t	num_aes_blocks)
{
	u_int32_t i;
	u_int8_t *cbc_chain_data;

	u_int8_t	l[KEY_LENGTH];
	u_int8_t	k1[KEY_LENGTH];
	u_int8_t	tmp_data[KEY_LENGTH];

	cbc_chain_data = zero_key; /* Convenient array of 0's for IV */

	/* compute k1 constant needed by AES-CMAC calculation */

	for (i=0; i<KEY_LENGTH; i++)
		tmp_data[i] = 0;

	encrypt_object(key_schedule, tmp_data, l, 1);

	if (enable_debug_crypto)
		print_vector("AES(key, nonce)", KEY_LENGTH, l);

	left_shift_vector(l, k1, sizeof(l));

	if (enable_debug_crypto)
		print_vector("L", KEY_LENGTH, l);

	if ( (l[0] >> 7) != 0 ) /* get MSB of L */
		k1[KEY_LENGTH-1] ^= AES_CMAC_CONST_RB;

	if (enable_debug_crypto)
		print_vector("K1", KEY_LENGTH, k1);

	/* compute the AES-CMAC value */
	for (i = 0; i < num_aes_blocks; i++) {
		/* Apply the chain data */
		apply_cbc_chain_data(cbc_chain_data, src, tmp_data);

		/* for the final block, XOR k1 into the IV */
		if ( i == num_aes_blocks-1 )
			apply_cbc_chain_data(tmp_data, k1, tmp_data);

		/* encrypt the AES block */
		nv_aes_encrypt(tmp_data, key_schedule, (u_int8_t*)dst);

		if (enable_debug_crypto) {
			printf("sign_objext: block %d of %d\n", i,
							num_aes_blocks);
			print_vector("AES-CMAC Src", KEY_LENGTH, src);
			print_vector("AES-CMAC Xor", KEY_LENGTH, tmp_data);
			print_vector("AES-CMAC Dst",
				KEY_LENGTH,
				(u_int8_t*)dst);
		}

		/* Update pointers for next loop. */
		cbc_chain_data = (u_int8_t*)dst;
		src += KEY_LENGTH;
	}

	if (enable_debug_crypto)
		print_vector("AES-CMAC Hash", KEY_LENGTH, (u_int8_t*)dst);
}

static int
encrypt_and_sign(u_int8_t		*key,
		u_int8_t		*src,
		u_int32_t		length,
		u_int8_t		*sig_dst)
{
	u_int32_t	num_aes_blocks;
	u_int8_t	key_schedule[4*NVAES_STATECOLS*(NVAES_ROUNDS+1)];

	if (enable_debug_crypto) {
		printf("encrypt_and_sign: length = %d\n", length);
		print_vector("AES key", KEY_LENGTH, key);
	}

	generate_key_schedule(key, key_schedule);

	num_aes_blocks = ICEIL(length, KEY_LENGTH);

	if (enable_debug_crypto)
		printf("encrypt_and_sign: begin signing\n");

	/* encrypt the data, overwriting the result in signature. */
	sign_objext(key, key_schedule, src, sig_dst, num_aes_blocks);

	if (enable_debug_crypto)
		printf("encrypt_and_sign: end signing\n");

	return 0;
}

int
sign_data_block(u_int8_t *source,
		u_int32_t length,
		u_int8_t *signature)
{
	return encrypt_and_sign(zero_key,
				source,
				length,
				signature);
}

int
sign_bct(build_image_context *context,
		u_int8_t *bct)
{
	u_int32_t Offset;
	u_int32_t length;
	u_int32_t hash_size;
	u_int8_t *hash_buffer = NULL;
	int e = 0;

	assert(bct != NULL);

	if (g_soc_config->get_value(token_hash_size,
					&hash_size,
					bct) != 0)
		return -ENODATA;
	if (g_soc_config->get_value(token_crypto_offset,
					&Offset,
					bct) != 0)
		return -ENODATA;
	if (g_soc_config->get_value(token_crypto_length,
					&length,
					bct) != 0)
		return -ENODATA;

	hash_buffer = calloc(1, hash_size);
	if (hash_buffer == NULL)
		return -ENOMEM;
	e = sign_data_block(bct + Offset, length, hash_buffer);
	if (e != 0)
		goto fail;
	e = g_soc_config->set_data(token_crypto_hash,
						hash_buffer,
						hash_size,
						bct);
	if (e != 0)
		goto fail;

 fail:
	free(hash_buffer);
	return e;
}
