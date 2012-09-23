#ifndef __MACH_WEIM_H
#define __MACH_WEIM_H

void imx27_setup_weimcs(size_t cs, unsigned upper, unsigned lower,
		unsigned additional);

void imx1_setup_eimcs(size_t cs, unsigned upper, unsigned lower);

#endif /* __MACH_WEIM_H */
