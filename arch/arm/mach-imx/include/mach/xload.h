#ifndef __MACH_XLOAD_H
#define __MACH_XLOAD_H

int imx53_nand_start_image(void);
int imx6_spi_load_image(int instance, unsigned int flash_offset, void *buf, int len);
int imx6_spi_start_image(int instance);
int imx6_esdhc_load_image(int instance, void *buf, int len);
int imx6_esdhc_start_image(int instance);

int imx_image_size(void);

#endif /* __MACH_XLOAD_H */
