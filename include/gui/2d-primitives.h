#ifndef __2D_PRIMITIVES__
#define __2D_PRIMITIVES__

#include <fb.h>

void gu_draw_line(struct screen *sc,
		  int x1, int y1, int x2, int y2,
		  u8 r, u8 g, u8 b, u8 a,
		  unsigned int dash);

void gu_draw_circle(struct screen *sc,
		    int x0, int y0, int radius,
		    u8 r, u8 g, u8 b, u8 a);

#endif
