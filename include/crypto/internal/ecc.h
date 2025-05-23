/*
 * Copyright (c) 2013, Kenneth MacKay
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _CRYPTO_ECC_H
#define _CRYPTO_ECC_H

#include <crypto/ecc_curve.h>
#include <asm/unaligned.h>

/* One digit is u64 qword. */
#define ECC_CURVE_NIST_P192_DIGITS  3
#define ECC_CURVE_NIST_P256_DIGITS  4
#define ECC_CURVE_NIST_P384_DIGITS  6
#define ECC_CURVE_NIST_P521_DIGITS  9
#define ECC_MAX_DIGITS              DIV_ROUND_UP(521, 64) /* NIST P521 */

#define ECC_DIGITS_TO_BYTES_SHIFT 3

#define ECC_MAX_BYTES (ECC_MAX_DIGITS << ECC_DIGITS_TO_BYTES_SHIFT)

#define ECC_POINT_INIT(x, y, ndigits)	(struct ecc_point) { x, y, ndigits }

/**
 * ecc_swap_digits() - Copy ndigits from big endian array to native array
 * @in:       Input array
 * @out:      Output array
 * @ndigits:  Number of digits to copy
 */
static inline void ecc_swap_digits(const void *in, u64 *out, unsigned int ndigits)
{
	const __be64 *src = (__force __be64 *)in;
	int i;

	for (i = 0; i < ndigits; i++)
		out[i] = get_unaligned_be64(&src[ndigits - 1 - i]);
}

/**
 * ecc_digits_from_bytes() - Create ndigits-sized digits array from byte array
 * @in:       Input byte array
 * @nbytes    Size of input byte array
 * @out       Output digits array
 * @ndigits:  Number of digits to create from byte array
 */
void ecc_digits_from_bytes(const u8 *in, unsigned int nbytes,
			   u64 *out, unsigned int ndigits);

/**
 * ecc_is_key_valid() - Validate a given ECDH private key
 *
 * @curve_id:		id representing the curve to use
 * @ndigits:		curve's number of digits
 * @private_key:	private key to be used for the given curve
 * @private_key_len:	private key length
 *
 * Returns 0 if the key is acceptable, a negative value otherwise
 */
int ecc_is_key_valid(unsigned int curve_id, unsigned int ndigits,
		     const u64 *private_key, unsigned int private_key_len);

/**
 * ecc_make_pub_key() - Compute an ECC public key
 *
 * @curve_id:		id representing the curve to use
 * @ndigits:		curve's number of digits
 * @private_key:	pregenerated private key for the given curve
 * @public_key:		buffer for storing the generated public key
 *
 * Returns 0 if the public key was generated successfully, a negative value
 * if an error occurred.
 */
int ecc_make_pub_key(const unsigned int curve_id, unsigned int ndigits,
		     const u64 *private_key, u64 *public_key);

/**
 * crypto_ecdh_shared_secret() - Compute a shared secret
 *
 * @curve_id:		id representing the curve to use
 * @ndigits:		curve's number of digits
 * @private_key:	private key of part A
 * @public_key:		public key of counterpart B
 * @secret:		buffer for storing the calculated shared secret
 *
 * Note: It is recommended that you hash the result of crypto_ecdh_shared_secret
 * before using it for symmetric encryption or HMAC.
 *
 * Returns 0 if the shared secret was generated successfully, a negative value
 * if an error occurred.
 */
int crypto_ecdh_shared_secret(unsigned int curve_id, unsigned int ndigits,
			      const u64 *private_key, const u64 *public_key,
			      u64 *secret);

/**
 * ecc_is_pubkey_valid_partial() - Partial public key validation
 *
 * @curve:		elliptic curve domain parameters
 * @pk:			public key as a point
 *
 * Valdiate public key according to SP800-56A section 5.6.2.3.4 ECC Partial
 * Public-Key Validation Routine.
 *
 * Note: There is no check that the public key is in the correct elliptic curve
 * subgroup.
 *
 * Return: 0 if validation is successful, -EINVAL if validation is failed.
 */
int ecc_is_pubkey_valid_partial(const struct ecc_curve *curve,
				struct ecc_point *pk);

/**
 * ecc_is_pubkey_valid_full() - Full public key validation
 *
 * @curve:		elliptic curve domain parameters
 * @pk:			public key as a point
 *
 * Valdiate public key according to SP800-56A section 5.6.2.3.3 ECC Full
 * Public-Key Validation Routine.
 *
 * Return: 0 if validation is successful, -EINVAL if validation is failed.
 */
int ecc_is_pubkey_valid_full(const struct ecc_curve *curve,
			     struct ecc_point *pk);

/**
 * vli_is_zero() - Determine is vli is zero
 *
 * @vli:		vli to check.
 * @ndigits:		length of the @vli
 */
bool vli_is_zero(const u64 *vli, unsigned int ndigits);

/**
 * vli_cmp() - compare left and right vlis
 *
 * @left:		vli
 * @right:		vli
 * @ndigits:		length of both vlis
 *
 * Returns sign of @left - @right, i.e. -1 if @left < @right,
 * 0 if @left == @right, 1 if @left > @right.
 */
int vli_cmp(const u64 *left, const u64 *right, unsigned int ndigits);

/**
 * vli_sub() - Subtracts right from left
 *
 * @result:		where to write result
 * @left:		vli
 * @right		vli
 * @ndigits:		length of all vlis
 *
 * Note: can modify in-place.
 *
 * Return: carry bit.
 */
u64 vli_sub(u64 *result, const u64 *left, const u64 *right,
	    unsigned int ndigits);

/**
 * vli_from_be64() - Load vli from big-endian u64 array
 *
 * @dest:		destination vli
 * @src:		source array of u64 BE values
 * @ndigits:		length of both vli and array
 */
void vli_from_be64(u64 *dest, const void *src, unsigned int ndigits);

/**
 * vli_from_le64() - Load vli from little-endian u64 array
 *
 * @dest:		destination vli
 * @src:		source array of u64 LE values
 * @ndigits:		length of both vli and array
 */
void vli_from_le64(u64 *dest, const void *src, unsigned int ndigits);

/**
 * vli_mod_inv() - Modular inversion
 *
 * @result:		where to write vli number
 * @input:		vli value to operate on
 * @mod:		modulus
 * @ndigits:		length of all vlis
 */
void vli_mod_inv(u64 *result, const u64 *input, const u64 *mod,
		 unsigned int ndigits);

/**
 * vli_mod_mult_slow() - Modular multiplication
 *
 * @result:		where to write result value
 * @left:		vli number to multiply with @right
 * @right:		vli number to multiply with @left
 * @mod:		modulus
 * @ndigits:		length of all vlis
 *
 * Note: Assumes that mod is big enough curve order.
 */
void vli_mod_mult_slow(u64 *result, const u64 *left, const u64 *right,
		       const u64 *mod, unsigned int ndigits);

/**
 * vli_num_bits() - Counts the number of bits required for vli.
 *
 * @vli:		vli to check.
 * @ndigits:		Length of the @vli
 *
 * Return: The number of bits required to represent @vli.
 */
unsigned int vli_num_bits(const u64 *vli, unsigned int ndigits);

/**
 * ecc_aloc_point() - Allocate ECC point.
 *
 * @ndigits:		Length of vlis in u64 qwords.
 *
 * Return: Pointer to the allocated point or NULL if allocation failed.
 */
struct ecc_point *ecc_alloc_point(unsigned int ndigits);

/**
 * ecc_free_point() - Free ECC point.
 *
 * @p:			The point to free.
 */
void ecc_free_point(struct ecc_point *p);

/**
 * ecc_point_is_zero() - Check if point is zero.
 *
 * @p:			Point to check for zero.
 *
 * Return: true if point is the point at infinity, false otherwise.
 */
bool ecc_point_is_zero(const struct ecc_point *point);

/**
 * ecc_point_mult_shamir() - Add two points multiplied by scalars
 *
 * @result:		resulting point
 * @x:			scalar to multiply with @p
 * @p:			point to multiply with @x
 * @y:			scalar to multiply with @q
 * @q:			point to multiply with @y
 * @curve:		curve
 *
 * Returns result = x * p + x * q over the curve.
 * This works faster than two multiplications and addition.
 */
void ecc_point_mult_shamir(const struct ecc_point *result,
			   const u64 *x, const struct ecc_point *p,
			   const u64 *y, const struct ecc_point *q,
			   const struct ecc_curve *curve);

#endif
