/*
 * Copyright (c) 2013, Google Inc.
 *
 * This code is ported from U-Boot code.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <of.h>
#include <digest.h>
#include <asm/types.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <rsa.h>
#include <asm/types.h>
#include <asm/unaligned.h>

#define UINT64_MULT32(v, multby)  (((uint64_t)(v)) * ((uint32_t)(multby)))

#define get_unaligned_be32(a) fdt32_to_cpu(*(uint32_t *)a)
#define put_unaligned_be32(a, b) (*(uint32_t *)(b) = cpu_to_fdt32(a))

/* Default public exponent for backward compatibility */
#define RSA_DEFAULT_PUBEXP	65537

/* This is the minimum/maximum key size we support, in bits */
#define RSA_MIN_KEY_BITS	1024
#define RSA_MAX_KEY_BITS	4096

/**
 * subtract_modulus() - subtract modulus from the given value
 *
 * @key:	Key containing modulus to subtract
 * @num:	Number to subtract modulus from, as little endian word array
 */
static void subtract_modulus(const struct rsa_public_key *key, uint32_t num[])
{
	int64_t acc = 0;
	uint i;

	for (i = 0; i < key->len; i++) {
		acc += (uint64_t)num[i] - key->modulus[i];
		num[i] = (uint32_t)acc;
		acc >>= 32;
	}
}

/**
 * greater_equal_modulus() - check if a value is >= modulus
 *
 * @key:	Key containing modulus to check
 * @num:	Number to check against modulus, as little endian word array
 * @return 0 if num < modulus, 1 if num >= modulus
 */
static int greater_equal_modulus(const struct rsa_public_key *key,
				 uint32_t num[])
{
	int i;

	for (i = (int)key->len - 1; i >= 0; i--) {
		if (num[i] < key->modulus[i])
			return 0;
		if (num[i] > key->modulus[i])
			return 1;
	}

	return 1;  /* equal */
}

/**
 * montgomery_mul_add_step() - Perform montgomery multiply-add step
 *
 * Operation: montgomery result[] += a * b[] / n0inv % modulus
 *
 * @key:	RSA key
 * @result:	Place to put result, as little endian word array
 * @a:		Multiplier
 * @b:		Multiplicand, as little endian word array
 */
static void montgomery_mul_add_step(const struct rsa_public_key *key,
		uint32_t result[], const uint32_t a, const uint32_t b[])
{
	uint64_t acc_a, acc_b;
	uint32_t d0;
	uint i;

	acc_a = (uint64_t)a * b[0] + result[0];
	d0 = (uint32_t)acc_a * key->n0inv;
	acc_b = (uint64_t)d0 * key->modulus[0] + (uint32_t)acc_a;
	for (i = 1; i < key->len; i++) {
		acc_a = (acc_a >> 32) + (uint64_t)a * b[i] + result[i];
		acc_b = (acc_b >> 32) + (uint64_t)d0 * key->modulus[i] +
				(uint32_t)acc_a;
		result[i - 1] = (uint32_t)acc_b;
	}

	acc_a = (acc_a >> 32) + (acc_b >> 32);

	result[i - 1] = (uint32_t)acc_a;

	if (acc_a >> 32)
		subtract_modulus(key, result);
}

/**
 * montgomery_mul() - Perform montgomery mutitply
 *
 * Operation: montgomery result[] = a[] * b[] / n0inv % modulus
 *
 * @key:	RSA key
 * @result:	Place to put result, as little endian word array
 * @a:		Multiplier, as little endian word array
 * @b:		Multiplicand, as little endian word array
 */
static void montgomery_mul(const struct rsa_public_key *key,
		uint32_t result[], uint32_t a[], const uint32_t b[])
{
	uint i;

	for (i = 0; i < key->len; ++i)
		result[i] = 0;
	for (i = 0; i < key->len; ++i)
		montgomery_mul_add_step(key, result, a[i], b);
}

/**
 * num_pub_exponent_bits() - Number of bits in the public exponent
 *
 * @key:	RSA key
 * @num_bits:	Storage for the number of public exponent bits
 */
static int num_public_exponent_bits(const struct rsa_public_key *key,
		int *num_bits)
{
	uint64_t exponent;
	int exponent_bits;
	const uint max_bits = (sizeof(exponent) * 8);

	exponent = key->exponent;
	exponent_bits = 0;

	if (!exponent) {
		*num_bits = exponent_bits;
		return 0;
	}

	for (exponent_bits = 1; exponent_bits < max_bits + 1; ++exponent_bits)
		if (!(exponent >>= 1)) {
			*num_bits = exponent_bits;
			return 0;
		}

	return -EINVAL;
}

/**
 * is_public_exponent_bit_set() - Check if a bit in the public exponent is set
 *
 * @key:	RSA key
 * @pos:	The bit position to check
 */
static int is_public_exponent_bit_set(const struct rsa_public_key *key,
		int pos)
{
	return key->exponent & (1ULL << pos);
}

/**
 * pow_mod() - in-place public exponentiation
 *
 * @key:	RSA key
 * @inout:	Big-endian word array containing value and result
 */
static int pow_mod(const struct rsa_public_key *key, void *__inout)
{
	uint32_t *inout = __inout;
	uint32_t *result, *ptr;
	uint i;
	int j, k;
	uint32_t val[RSA_MAX_KEY_BITS / 32], acc[RSA_MAX_KEY_BITS / 32], tmp[RSA_MAX_KEY_BITS / 32];
	uint32_t a_scaled[RSA_MAX_KEY_BITS / 32];

	/* Sanity check for stack size - key->len is in 32-bit words */
	if (key->len > RSA_MAX_KEY_BITS / 32) {
		debug("RSA key words %u exceeds maximum %d\n", key->len,
		      RSA_MAX_KEY_BITS / 32);
		return -EINVAL;
	}

	result = tmp;  /* Re-use location. */

	/* Convert from big endian byte array to little endian word array. */
	for (i = 0, ptr = inout + key->len - 1; i < key->len; i++, ptr--)
		val[i] = get_unaligned_be32(ptr);

	if (0 != num_public_exponent_bits(key, &k))
		return -EINVAL;

	if (k < 2) {
		debug("Public exponent is too short (%d bits, minimum 2)\n",
		      k);
		return -EINVAL;
	}

	if (!is_public_exponent_bit_set(key, 0)) {
		debug("LSB of RSA public exponent must be set.\n");
		return -EINVAL;
	}

	/* the bit at e[k-1] is 1 by definition, so start with: C := M */
	montgomery_mul(key, acc, val, key->rr); /* acc = a * RR / R mod n */
	/* retain scaled version for intermediate use */
	memcpy(a_scaled, acc, key->len * sizeof(a_scaled[0]));

	for (j = k - 2; j > 0; --j) {
		montgomery_mul(key, tmp, acc, acc); /* tmp = acc^2 / R mod n */

		if (is_public_exponent_bit_set(key, j)) {
			/* acc = tmp * val / R mod n */
			montgomery_mul(key, acc, tmp, a_scaled);
		} else {
			/* e[j] == 0, copy tmp back to acc for next operation */
			memcpy(acc, tmp, key->len * sizeof(acc[0]));
		}
	}

	/* the bit at e[0] is always 1 */
	montgomery_mul(key, tmp, acc, acc); /* tmp = acc^2 / R mod n */
	montgomery_mul(key, acc, tmp, val); /* acc = tmp * a / R mod M */
	memcpy(result, acc, key->len * sizeof(result[0]));

	/* Make sure result < mod; result is at most 1x mod too large. */
	if (greater_equal_modulus(key, result))
		subtract_modulus(key, result);

	/* Convert to bigendian byte array */
	for (i = key->len - 1, ptr = inout; (int)i >= 0; i--, ptr++)
		put_unaligned_be32(result[i], ptr);
	return 0;
}

/*
 * Hash algorithm OIDs plus ASN.1 DER wrappings [RFC4880 sec 5.2.2].
 */
static const u8 RSA_digest_info_MD5[] = {
	0x30, 0x20, 0x30, 0x0C, 0x06, 0x08,
	0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, /* OID */
	0x05, 0x00, 0x04, 0x10
};

static const u8 RSA_digest_info_SHA1[] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05,
	0x2B, 0x0E, 0x03, 0x02, 0x1A,
	0x05, 0x00, 0x04, 0x14
};

static const u8 RSA_digest_info_RIPE_MD_160[] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05,
	0x2B, 0x24, 0x03, 0x02, 0x01,
	0x05, 0x00, 0x04, 0x14
};

static const u8 RSA_digest_info_SHA224[] = {
	0x30, 0x2d, 0x30, 0x0d, 0x06, 0x09,
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,
	0x05, 0x00, 0x04, 0x1C
};

static const u8 RSA_digest_info_SHA256[] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09,
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
	0x05, 0x00, 0x04, 0x20
};

static const u8 RSA_digest_info_SHA384[] = {
	0x30, 0x41, 0x30, 0x0d, 0x06, 0x09,
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
	0x05, 0x00, 0x04, 0x30
};

static const u8 RSA_digest_info_SHA512[] = {
	0x30, 0x51, 0x30, 0x0d, 0x06, 0x09,
	0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
	0x05, 0x00, 0x04, 0x40
};

static const struct {
	const u8 *data;
	size_t size;
} RSA_ASN1_templates[] = {
#define _(X) { RSA_digest_info_##X, sizeof(RSA_digest_info_##X) }
	[HASH_ALGO_MD5]		= _(MD5),
	[HASH_ALGO_SHA1]	= _(SHA1),
	[HASH_ALGO_RIPE_MD_160]	= _(RIPE_MD_160),
	[HASH_ALGO_SHA256]	= _(SHA256),
	[HASH_ALGO_SHA384]	= _(SHA384),
	[HASH_ALGO_SHA512]	= _(SHA512),
	[HASH_ALGO_SHA224]	= _(SHA224),
#undef _
};

int rsa_verify(const struct rsa_public_key *key, const uint8_t *sig,
			  const uint32_t sig_len, const uint8_t *hash,
			  enum hash_algo algo)
{
	int ret = 0;
	uint8_t buf[RSA_MAX_SIG_BITS / 8];
	int i;
	unsigned PS_end, T_offset;
	const u8 *asn1_template = RSA_ASN1_templates[algo].data;
	size_t asn1_size = RSA_ASN1_templates[algo].size;
	struct digest *d = digest_alloc_by_algo(algo);

	if (!d)
		return -EOPNOTSUPP;

	if (sig_len != (key->len * sizeof(uint32_t))) {
		debug("Signature is of incorrect length %d, should be %d\n", sig_len,
				key->len * sizeof(uint32_t));
		ret = -EINVAL;
		goto out_free_digest;
	}

	/* Sanity check for stack size */
	if (sig_len > RSA_MAX_SIG_BITS / 8) {
		debug("Signature length %u exceeds maximum %d\n", sig_len,
		      RSA_MAX_SIG_BITS / 8);
		ret = -EINVAL;
		goto out_free_digest;
	}

	memcpy(buf, sig, sig_len);

	ret = pow_mod(key, buf);
	if (ret)
		goto out_free_digest;

	T_offset = sig_len - (asn1_size + digest_length(d));

	PS_end = T_offset - 1;
	if (buf[PS_end] != 0x00) {
		pr_err(" = -EBADMSG [EM[T-1] == %02u]\n", buf[PS_end]);
		ret = -EBADMSG;
		goto out_free_digest;
	}

	for (i = 2; i < PS_end; i++) {
		if (buf[i] != 0xff) {
			pr_err(" = -EBADMSG [EM[PS%x] == %02u]\n", i - 2, buf[i]);
			ret = -EBADMSG;
			goto out_free_digest;
		}
	}

	if (memcmp(asn1_template, buf + T_offset, asn1_size) != 0) {
		pr_err(" = -EBADMSG [EM[T] ASN.1 mismatch]\n");
		ret = -EBADMSG;
		goto out_free_digest;
	}

	if (memcmp(hash, buf + T_offset + asn1_size, digest_length(d)) != 0) {
		pr_err(" = -EKEYREJECTED [EM[T] hash mismatch]\n");
		ret = -EKEYREJECTED;
		goto out_free_digest;
	}

 out_free_digest:
	digest_free(d);

	return ret;
}

static void rsa_convert_big_endian(uint32_t *dst, const uint32_t *src, int len)
{
	int i;

	for (i = 0; i < len; i++)
		dst[i] = fdt32_to_cpu(src[len - 1 - i]);
}

int rsa_of_read_key(struct device_node *node, struct rsa_public_key *key)
{
	const void *modulus, *rr;
	const uint64_t *public_exponent;
	int length;

	of_property_read_u32(node, "rsa,num-bits", &key->len);
	of_property_read_u32(node, "rsa,n0-inverse", &key->n0inv);

	public_exponent = of_get_property(node, "rsa,exponent", &length);
	if (!public_exponent || length < sizeof(*public_exponent))
		key->exponent = RSA_DEFAULT_PUBEXP;
	else
		key->exponent = fdt64_to_cpu(*public_exponent);

	modulus = of_get_property(node, "rsa,modulus", NULL);
	rr = of_get_property(node, "rsa,r-squared", NULL);

	if (!key->len || !modulus || !rr) {
		debug("%s: Missing RSA key info", __func__);
		return -EFAULT;
	}

	/* Sanity check for stack size */
	if (key->len > RSA_MAX_KEY_BITS || key->len < RSA_MIN_KEY_BITS) {
		debug("RSA key bits %u outside allowed range %d..%d\n",
		      key->len, RSA_MIN_KEY_BITS, RSA_MAX_KEY_BITS);
		return -EFAULT;
	}

	key->len /= sizeof(uint32_t) * 8;

	key->modulus = xzalloc(RSA_MAX_KEY_BITS / 8);
	key->rr = xzalloc(RSA_MAX_KEY_BITS / 8);

	rsa_convert_big_endian(key->modulus, modulus, key->len);
	rsa_convert_big_endian(key->rr, rr, key->len);

	return 0;
}
