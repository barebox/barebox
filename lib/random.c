#include <common.h>
#include <stdlib.h>

static unsigned int random_seed;

#if RAND_MAX > 32767
#error this rand implementation is for RAND_MAX < 32678 only.
#endif

unsigned int rand(void)
{
	random_seed = random_seed * 1103515245 + 12345;
	return (random_seed / 65536) % (RAND_MAX + 1);
}

void srand(unsigned int seed)
{
	random_seed = seed;
}

void get_random_bytes(void *_buf, int len)
{
	char *buf = _buf;

	while (len--)
		*buf++ = rand() % 256;
}

