#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>
#include <errno.h>

#ifdef CONFIG_BAREBOX_UPDATE_NAND_S3C24XX
int s3c24x0_bbu_nand_register_handler(void);
#else
static inline int s3c24x0_bbu_nand_register_handler(void)
{
	return -ENOSYS;
}
#endif

#endif
