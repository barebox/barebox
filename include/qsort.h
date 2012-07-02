#ifndef __QSORT_H
#define __QSORT_H

void qsort(void  *base, size_t nel, size_t width,
	   int (*comp)(const void *, const void *));

int strcmp_compar(const void *p1, const void *p2);

#endif /* __QSORT_H */
