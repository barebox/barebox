/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX_XLOAD_H
#define __MACH_IMX_XLOAD_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <mach/imx/tzasc.h>

int imx53_nand_start_image(void);
int imx6_spi_load_image(int instance, unsigned int flash_offset, void *buf, int len);
int imx6_spi_start_image(int instance);
int imx6_esdhc_start_image(int instance);
int imx6_nand_start_image(void);
int imx7_esdhc_start_image(int instance);
int imx7_nand_start_image(void);

/* Below functions only load and don't start the image */
int imx8m_esdhc_load_image(int instance, void *bl33);
int imx8mn_esdhc_load_image(int instance, void *bl33);
int imx8mp_esdhc_load_image(int instance, void *bl33);
int imx8mm_qspi_load_image(int instance, void *bl33);
int imx8mn_qspi_load_image(int instance, void *bl33);
int imx8mp_qspi_load_image(int instance, void *bl33);

void imx8mm_load_bl33(void *bl33);
void imx8mn_load_bl33(void *bl33);
void imx8mp_load_bl33(void *bl33);
void imx8mq_load_bl33(void *bl33);

void __noreturn imx8mm_load_and_start_image_via_tfa(void);
void __noreturn imx8mn_load_and_start_image_via_tfa(void);
void __noreturn imx8mp_load_and_start_image_via_tfa(void);
void __noreturn imx8mq_load_and_start_image_via_tfa(void);
void __noreturn __imx8mm_load_and_start_image_via_tfa(void *bl33);
void __noreturn __imx8mn_load_and_start_image_via_tfa(void *bl33);
void __noreturn __imx8mp_load_and_start_image_via_tfa(void *bl33);
void __noreturn __imx8mq_load_and_start_image_via_tfa(void *bl33);

void __noreturn imx93_load_and_start_image_via_tfa(void);

int imx_load_image(ptrdiff_t address, ptrdiff_t entry, u32 offset,
		   u32 ivt_offset, bool start, unsigned int alignment,
		   int (*read)(void *dest, size_t len, void *priv),
		   void *priv);

int imx_image_size(void);
int piggydata_size(void);

extern unsigned char input_data[];
extern unsigned char input_data_end[];

#endif /* __MACH_IMX_XLOAD_H */
