#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

int ls1046a_esdhc_start_image(unsigned long r0, unsigned long r1, unsigned long r2);
int ls1046a_qspi_start_image(unsigned long r0, unsigned long r1,
					     unsigned long r2);
int ls1046a_xload_start_image(unsigned long r0, unsigned long r1,
			      unsigned long r2);

#endif /* __MACH_XLOAD_H */
