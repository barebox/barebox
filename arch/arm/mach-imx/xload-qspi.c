// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/sizes.h>
#include <mach/imx/atf.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/xload.h>

#define IMX8M_QSPI_MMAP		0x8000000

/* Make use of AHB reads */
static
int imx8m_qspi_read(void *dest, size_t len, void *priv)
{
	void __iomem *qspi_ahb = priv;

	memcpy(dest, qspi_ahb, len);

	return 0;
}

/**
 * imx8mm_qspi_start_image - Load and optionally start an image from the
 *                           FlexSPI controller.
 * @instance: The FlexSPI controller instance
 *
 * This uses imx8m_qspi_load_image() to load an image from QSPI. It is assumed
 * that the image is the currently running barebox image.
 * The image is not started afterwards.
 *
 * Return: If image successfully loaded, returns 0.
 * A negative error code is returned when this function fails.
 */
static
int imx8m_qspi_load_image(int instance, off_t offset, off_t ivt_offset, void *bl33)
{
	void __iomem *qspi_ahb = IOMEM(IMX8M_QSPI_MMAP);

	return imx_load_image(MX8M_DDR_CSD1_BASE_ADDR, (ptrdiff_t)bl33,
			      offset, ivt_offset, false, 0,
			      imx8m_qspi_read, qspi_ahb);
}

int imx8mm_qspi_load_image(int instance, void *bl33)
{
	return imx8m_qspi_load_image(instance, 0, SZ_4K, bl33);
}

int imx8mn_qspi_load_image(int instance, void *bl33)
{
	return imx8m_qspi_load_image(instance, SZ_4K, 0, bl33);
}

int imx8mp_qspi_load_image(int instance, void *bl33)
	__alias(imx8mn_qspi_load_image);
