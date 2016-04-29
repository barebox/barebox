#ifndef __STRING_H
#define __STRING_H

#include <linux/string.h>

void *memdup(const void *, size_t);
int strtobool(const char *str, int *val);

#endif /* __STRING_H */
