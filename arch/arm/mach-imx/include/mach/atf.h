#ifndef __IMX_ATF_H__
#define __IMX_ATF_H__

#include <linux/sizes.h>
#include <asm/system.h>

#define MX8MQ_ATF_BL31_SIZE_LIMIT	SZ_64K
#define MX8MQ_ATF_BL31_BASE_ADDR	0x00910000
#define MX8MQ_ATF_BL33_BASE_ADDR	0x40200000

void imx8mq_atf_load_bl31(const void *fw, size_t fw_size);

#endif