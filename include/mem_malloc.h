#ifndef __MEM_MALLOC_H
#define __MEM_MALLOC_H

void mem_malloc_init (ulong start, ulong end);
void *sbrk (ptrdiff_t increment);

#endif
