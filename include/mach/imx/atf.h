/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IMX_ATF_H__
#define __IMX_ATF_H__

#include <linux/sizes.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/system.h>

#define MX8M_ATF_BL31_SIZE_LIMIT	SZ_64K

#define MX8MM_ATF_BL31_BASE_ADDR	0x00920000
#define MX8MN_ATF_BL31_BASE_ADDR	0x00960000
#define MX8MP_ATF_BL31_BASE_ADDR	0x00970000
#define MX8MQ_ATF_BL31_BASE_ADDR	0x00910000
#define MX8M_ATF_BL33_BASE_ADDR		0x40200000
#define MX8MM_ATF_BL33_BASE_ADDR	MX8M_ATF_BL33_BASE_ADDR
#define MX8MQ_ATF_BL33_BASE_ADDR	MX8M_ATF_BL33_BASE_ADDR

void __noreturn imx8mm_atf_load_bl31(const void *fw, size_t fw_size);
void __noreturn imx8mn_atf_load_bl31(const void *fw, size_t fw_size);
void __noreturn imx8mp_atf_load_bl31(const void *fw, size_t fw_size);
void __noreturn imx8mq_atf_load_bl31(const void *fw, size_t fw_size);

#define imx8m_load_and_start_tfa(soc,fw_name) do { \
	size_t __bl31_size; \
	const u8 *__bl31; \
	get_builtin_firmware(fw_name, &__bl31, &__bl31_size); \
	soc##_atf_load_bl31(__bl31, __bl31_size); \
} while (0)

#define imx8mm_load_and_start_tfa(fw_name) imx8m_load_and_start_tfa(imx8mm, fw_name)
#define imx8mn_load_and_start_tfa(fw_name) imx8m_load_and_start_tfa(imx8mn, fw_name)
#define imx8mp_load_and_start_tfa(fw_name) imx8m_load_and_start_tfa(imx8mp, fw_name)
#define imx8mq_load_and_start_tfa(fw_name) imx8m_load_and_start_tfa(imx8mq, fw_name)

#endif
