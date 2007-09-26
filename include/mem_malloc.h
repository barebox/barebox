#ifndef __MEM_MALLOC_H
#define __MEM_MALLOC_H

#include <linux/types.h>

void mem_malloc_init(void *start, void *end);
void *sbrk_no_zero(ptrdiff_t increment);

ulong mem_malloc_start(void);
ulong mem_malloc_end(void);

#endif
