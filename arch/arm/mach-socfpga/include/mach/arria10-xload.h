#ifndef __MACH_ARRIA10_XLOAD_H
#define __MACH_ARRIA10_XLOAD_H

void arria10_init_mmc(void);
int arria10_prepare_mmc(int barebox_part, int rbf_part);
int arria10_read_blocks(void *dst, int blocknum, size_t len);

struct partition {
	uint64_t first_sec;
	uint8_t type;
};

#endif /* __MACH_ARRIA10_XLOAD_H */
