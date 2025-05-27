// SPDX-License-Identifier: GPL-2.0-only
/*
 * The barebox random number generator provides mainly two APIs:
 *
 *   - get_noncrypto_bytes: Xorshift* seeded initially with the cycle counter
 *     - https://en.wikipedia.org/wiki/Xorshift#xorshift*)
 *     - https://forum.pjrc.com/index.php?threads/teensy-4-1-random-number-generator.61125/#post-243895
 *
 *   - get_crypto_bytes: Randomness directly from a HWRNG.
 *     PRNG fallback only possible with debugging option
 *     CONFIG_ALLOW_PRNG_FALLBACK set, which will emit a warning at runtime.
 */

#define pr_fmt(fmt) "random: " fmt

#include <common.h>
#include <stdlib.h>
#include <linux/hw_random.h>

static u64 prng_state = 1;

/**
 * rand_r - return next pseudo-random number depending only on input
 *
 * @x: RNG state
 *
 * This function runs the Xorshift* algorithm on the state input,
 * updates the state and returns the next number in the PRNG
 * sequence.
 *
 * Return: a 32 bit pseudo-random number
 */
static u32 rand_r(u64 *x)
{
	*x ^= *x >> 12;
	*x ^= *x << 25;
	*x ^= *x >> 27;

	/*
	 * Xorshift* fails only the MatrixRank test of BigCrush, however if the
	 * generator is modified to return only the high 32 bits, then it passes
	 * BigCrush with zero failures
	 */
	return (*x * 0x2545F4914F6CDD1DULL) >> 32;
}

/**
 * randbuf_r - fills pseudo-random numbers into buffer depending only on input
 *
 * @x:   RNG state
 * @buf: buffer to fill
 * @len: size of buffer
 *
 * This function runs the Xorshift* algorithm on the state input,
 * updates the state and fills the buffer with pseudo-random numbers.
 *
 * Only use this when you are using a fixed seed and the sequence
 * should be reproducible (e.g. for NAND test).
 */
void randbuf_r(u64 *x, void *buf, size_t len)
{
	for (size_t i = 0; i < len; i += 4) {
		u32 val = rand_r(x);
		memcpy(buf + i, &val, min_t(size_t, 4, len - i));
	}
}

/**
 * srand_xor - Xor a 64-bit into the existing RNG state
 *
 * @entropy: additional 64-bit of entropy
 *
 * This function mixes 64-bit of entropy into the exising
 * state by means of an Xor operation.
 *
 * Only use for independent entropy sources.
 */
void srand_xor(u64 entropy)
{
	/* The PRNG shouldn't be used for crypto anyway, but still let's not
	 * divulge the whole state in secure configurations.
	 */
	if (IS_ENABLED(CONFIG_INSECURE))
		pr_debug("PRNG seeded with %llu\n", entropy);

	prng_state ^= entropy;
	/* Ensure prng_state is never zero */
	prng_state += !prng_state;
	rand_r(&prng_state);
}

/**
 * get_noncrypto_bytes - get pseudo random numbers.
 *
 * @buf: buffer to fill
 * @len: length of buffer
 *
 * This interface can be good enough to generate MAC address
 * or use for NAND test. Use get_crypto_bytes for cryptographic
 * applications.
 */
void get_noncrypto_bytes(void *buf, size_t len)
{
	randbuf_r(&prng_state, buf, len);
}

u32 random32(void)
{
	return rand_r(&prng_state);
}

int hwrng_get_crypto_bytes(struct hwrng *rng, void *buf, int len)
{
	while (len) {
		int bytes = hwrng_get_data(rng, buf, len, true);
		if (!bytes)
			return -ENOMEDIUM;

		if (bytes < 0)
			return bytes;

		len -= bytes;
		buf = buf + bytes;
	}

	return 0;
}

/**
 * get_crypto_bytes - get random numbers suitable for cryptographic needs.
 */
int get_crypto_bytes(void *buf, int len)
{
	struct hwrng *rng;
	int err;

	rng = hwrng_get_first();
	err = PTR_ERR_OR_ZERO(rng);
	if (!err) {
		err = hwrng_get_crypto_bytes(rng, buf, len);
		if (!err)
			return 0;
	}

	if (!IS_ENABLED(CONFIG_ALLOW_PRNG_FALLBACK)) {
		pr_err("%ps: no HWRNG available!\n", (void *)_RET_IP_);
		return err;
	}

	pr_warn("%ps: falling back to Pseudo RNG source!\n", (void *)_RET_IP_);

	get_noncrypto_bytes(buf, len);

	return 0;
}
