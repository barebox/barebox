#ifndef _EARLY_PRINTF_
#define _EARLY_PRINTF_

#include <asm/nios2-io.h>

void early_putc(char ch);
void early_puts(const char *s);
int early_printf(const char *fmt, ...);

#endif  /* _EARLY_PRINTF_ */
