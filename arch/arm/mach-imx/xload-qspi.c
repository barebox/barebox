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
 * @start: Whether to directly start the loaded image
 *
 * This uses imx8m_qspi_load_image() to load an image from QSPI. It is assumed
 * that the image is the currently running barebox image (This information
 * is used to calculate the length of the image).
 * The image is started afterwards.
 *
 * Return: If successful, this function does not return (if directly started)
 * or 0. A negative error code is returned when this function fails.
 */
static
int imx8m_qspi_load_image(int instance, bool start, off_t offset, off_t ivt_offset)
{
	void __iomem *qspi_ahb = IOMEM(IMX8M_QSPI_MMAP);

	return imx_load_image(MX8M_DDR_CSD1_BASE_ADDR, MX8M_ATF_BL33_BASE_ADDR,
			      offset, ivt_offset, start, 0,
			      imx8m_qspi_read, qspi_ahb);
}

int imx8mm_qspi_load_image(int instance, bool start)
{
	return imx8m_qspi_load_image(instance, start, 0, SZ_4K);
}

int imx8mn_qspi_load_image(int instance, bool start)
{
	return imx8m_qspi_load_image(instance, start, SZ_4K, 0);
}

int imx8mp_qspi_load_image(int instance, bool start)
	__alias(imx8mn_qspi_load_image);
