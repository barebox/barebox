/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STDLIB_H
#define __STDLIB_H

#include <linux/bug.h>
#include <types.h>
#include <malloc.h>

#define RAND_MAX 32767

/* return a pseudo-random integer in the range [0, RAND_MAX] */
unsigned int rand(void);

/* set the seed for rand () */
void srand(unsigned int seed);

struct hwrng;

/* fill a buffer with pseudo-random data */
#if IN_PROPER
void randbuf_r(u64 *x, void *buf, size_t len);
void srand_xor(u64 entropy);
void get_noncrypto_bytes(void *buf, size_t len);
void get_random_bytes(void *buf, int len);
int get_crypto_bytes(void *buf, int len);
int hwrng_get_crypto_bytes(struct hwrng *rng, void *buf, int len);
#else
static inline void randbuf_r(u64 *x, void *buf, size_t len)
{
	BUG();
}
static inline void srand_xor(u64 entropy)
{
	BUG();
}
static inline void get_noncrypto_bytes(void *buf, size_t len)
{
	BUG();
}
static inline void get_random_bytes(void *buf, int len)
{
	BUG();
}
static inline int get_crypto_bytes(void *buf, int len)
{
	return -ENOSYS;
}
static inline int hwrng_get_crypto_bytes(struct hwrng *rng, void *buf, int len)
{
	return -ENOSYS;
}
#endif

static inline u32 random32(void)
{
	u32 ret;

	get_random_bytes(&ret, 4);

	return ret;
}

static inline u32 prandom_u32_max(u32 ep_ro)
{
	return (u32)(((u64) random32() * ep_ro) >> 32);
}

#endif /* __STDLIB_H */
