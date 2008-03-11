#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <xfuncs.h>

static int imx_spi_probe(struct device_d *dev)
{
	struct spi_master *master;

	master = xmalloc(sizeof(struct spi_master));

	spi_register_master(master);

	return 0;
}

static struct driver_d imx_spi_driver = {
	.name  = "imx_spi",
	.probe = imx_spi_probe,
};

static int imx_spi_init(void)
{
        register_driver(&imx_spi_driver);
        return 0;
}

device_initcall(imx_spi_init);

