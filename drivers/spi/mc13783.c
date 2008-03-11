#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <xfuncs.h>

static int pmic_probe(struct device_d *dev)
{
	printf("%s\n", __FUNCTION__);

	return 0;
}

static struct driver_d pmic_driver = {
	.name  = "mc13783",
	.probe = pmic_probe,
};

static int pmic_init(void)
{
	printf("%s\n", __FUNCTION__);
        register_driver(&pmic_driver);
        return 0;
}

device_initcall(pmic_init);

