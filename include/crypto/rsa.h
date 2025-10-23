/*
 * Copyright (c) 2013, Google Inc.
 *
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RSA_H
#define _RSA_H

#include <linux/types.h>
#include <errno.h>
#include <digest.h>

/**
 * struct rsa_public_key - holder for a public key
 *
 * An RSA public key consists of a modulus (typically called N), the inverse
 * and R^2, where R is 2^(# key bits).
 */

struct rsa_public_key {
	uint len;		/* len of modulus[] in number of uint32_t */
	uint32_t n0inv;		/* -1 / modulus[0] mod 2^32 */
	const uint32_t *modulus;/* modulus as little endian array */
	const uint32_t *rr;	/* R^2 as little endian array */
	uint64_t exponent;	/* public exponent */
};

/* This is the maximum signature length that we support, in bits */
#define RSA_MAX_SIG_BITS	4096

struct device_node;

struct public_key *rsa_of_read_key(struct device_node *node);

#ifdef CONFIG_CRYPTO_RSA
/**
 * rsa_verify() - Verify a signature against some data
 *
 * Verify a RSA PKCS1.5 signature against an expected hash.
 *
 * @info:	Specifies key and FIT information
 * @sig:	Signature
 * @sig_len:	Number of bytes in signature
 * @hash:	hash over payload
 * @algo:	hashing algo
 * @return 0 if verified, -ve on error
 */
int rsa_verify(const struct rsa_public_key *key, const uint8_t *sig,
			  const uint32_t sig_len, const uint8_t *hash,
			  enum hash_algo algo);
#else
static inline int rsa_verify(const struct rsa_public_key *key, const uint8_t *sig,
			  const uint32_t sig_len, const uint8_t *hash,
			  enum hash_algo algo)
{
	return -ENOSYS;
}
#endif

#endif
