#ifndef __MEM_MALLOC_H
#define __MEM_MALLOC_H

#include <linux/types.h>

void mem_malloc_init (void *start, void *end);
void *sbrk (ptrdiff_t increment);

#endif
