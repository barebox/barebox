#include <common.h>
#include <io.h>
#include <spi/imx-spi.h>
#include <mach/imx6-regs.h>
#include <mach/generic.h>
#include <bootsource.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <mach/xload.h>

static int cspi_2_3_read_data(void __iomem *base, u32 *data)
{
	u32 r;

	while (1) {
		if (readl(base + CSPI_2_3_STAT) & CSPI_2_3_STAT_RR)
			break;
	}

	r = swab32(readl(base + CSPI_2_3_RXDATA));
	if (data)
		*data = r;

	return 0;
}

static int cspi_2_3_load(void __iomem *base, unsigned int flash_offset, void *buf, int len)
{
	int transfer_size = 256;
	u32 val;
	int words, adr = 0;
	int ret;

	val = readl(base + CSPI_2_3_CTRL);
	val &= ~(0xfff << CSPI_2_3_CTRL_BL_OFFSET);
	val |= CSPI_2_3_CTRL_ENABLE;
	writel(val, base + CSPI_2_3_CTRL);

	writel(val, base + CSPI_2_3_CTRL);

	for (adr = 0; adr < len; adr += transfer_size) {

		val |= ((transfer_size + 4) * 8 - 1) << CSPI_2_3_CTRL_BL_OFFSET;
		writel(val, base + CSPI_2_3_CTRL);

		/* address */
		writel(swab32(0x3) | (adr + flash_offset), base + CSPI_2_3_TXDATA);
		writel(val | CSPI_2_3_CTRL_XCH, base + CSPI_2_3_CTRL);

		ret = cspi_2_3_read_data(base, NULL);
		if (ret)
			return ret;

		words = 0;

		for (words = 0; words < transfer_size >> 2; words++) {
			writel(0, base + CSPI_2_3_TXDATA);
			cspi_2_3_read_data(base, buf);
			buf += 4;
		}
	}

	return 0;
}

/**
 * imx6_spi_load_image - load an image from SPI NOR
 * @instance: The SPI controller instance (0..4)
 * @flash_offset: The offset in flash where the image starts
 * @buf: The buffer to load the image to
 * @len: The size to load
 *
 * This function loads data from SPI NOR flash on i.MX6. This assumes the
 * SPI controller has already been initialized and the pinctrl / clocks are
 * configured correctly. This is the case when the ROM has loaded the initial
 * portion of the boot loader from exactly this controller.
 *
 * Return: 0 if successful, negative error code otherwise
 */
int imx6_spi_load_image(int instance, unsigned int flash_offset, void *buf, int len)
{
	void *base;

	switch (instance) {
	case 0:
		base = IOMEM(MX6_ECSPI1_BASE_ADDR);
		break;
	case 1:
		base = IOMEM(MX6_ECSPI2_BASE_ADDR);
		break;
	case 2:
		base = IOMEM(MX6_ECSPI3_BASE_ADDR);
		break;
	case 3:
		base = IOMEM(MX6_ECSPI4_BASE_ADDR);
		break;
	case 4:
		base = IOMEM(MX6_ECSPI5_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	cspi_2_3_load(base, flash_offset, buf, len);

	return 0;
}

/**
 * imx6_spi_start_image - Load and start an image from SPI NOR flash
 * @instance: The SPI controller instance (0..4)
 *
 * This uses imx6_spi_load_image() to load an image from SPI NOR flash.
 * It is assumed that the image is the currently running barebox image
 * (This information is used to calculate the length of the image). The
 * image is started afterwards.
 *
 * Return: If successul, this function does not return. A negative error
 * code is returned when this function fails.
 */
int imx6_spi_start_image(int instance)
{
	void *buf = (void *)0x10000000;
	int ret, len;
	void __noreturn (*bb)(void);

	len = imx_image_size();

	ret = imx6_spi_load_image(instance, 0, buf, len);
	if (ret)
		return ret;

	bb = buf;

	bb();
}
