#ifndef __STDLIB_H
#define __STDLIB_H

#include <types.h>

#define RAND_MAX 32767

/* return a pseudo-random integer in the range [0, RAND_MAX] */
unsigned int rand(void);

/* set the seed for rand () */
void srand(unsigned int seed);

/* fill a buffer with pseudo-random data */
void get_random_bytes(void *buf, int len);
int get_crypto_bytes(void *buf, int len);

static inline u32 random32(void)
{
	u32 ret;

	get_random_bytes(&ret, 4);

	return ret;
}

#endif /* __STDLIB_H */
