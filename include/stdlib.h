#ifndef __STDLIB_H
#define __STDLIB_H

#define RAND_MAX 32767

/* return a pseudo-random integer in the range [0, RAND_MAX] */
unsigned int rand(void);

/* set the seed for rand () */
void srand(unsigned int seed);

/* fill a buffer with pseudo-random data */
void get_random_bytes(char *buf, int len);


#endif /* __STDLIB_H */
