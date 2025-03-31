// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021 David Jander, Protonic Holland
// SPDX-FileCopyrightText: 2021 Oleksij Rempel, Pengutronix

#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <gpio.h>
#include <init.h>
#include <linux/bitfield.h>
#include <linux/tee_drv.h>
#include <mach/stm32mp/bbu.h>
#include <mach/stm32mp/bsec.h>
#include <of_device.h>
#include <soc/stm32/gpio.h>
#include <soc/stm32/stm32-bsec-optee-ta.h>

/* board specific flags */
#define PRT_STM32_BOOTSRC_SD		BIT(2)
#define PRT_STM32_BOOTSRC_EMMC		BIT(1)
#define PRT_STM32_BOOTSRC_SPI_NOR	BIT(0)

#define PRT_STM32_GPIO_HWID_PL_N	13 /* PA13 */
#define PRT_STM32_GPIO_HWID_CP		14 /* PA14 */
#define PRT_STM32_GPIO_HWID_Q7		45 /* PC13 */

/* board specific serial number length is 10 characters without '\0' */
#define PRT_STM32_SERIAL_LEN		10
#define PRT_STM32_SERIAL_OFFSET		60

#define PRT_STM32_REVISION_ID_MASK	GENMASK(2, 0)
#define PRT_STM32_BOARD_ID_MASK		GENMASK(7, 3)

/* defines for 74HC165BQ 8-bit parallel-in/serial out shift register */
#define PRT_STM32_SHIFT_REG_SIZE	8

struct prt_stm32_machine_data {
	u32 flags;
};

struct prt_stm32_boot_dev {
	char *name;
	char *env;
	char *dev;
	int flags;
	int boot_idx;
	enum bootsource boot_src;
};

static const struct prt_stm32_boot_dev prt_stm32_boot_devs[] = {
	{
		.name = "emmc",
		.env = "/chosen/environment-emmc",
		.dev = "/dev/mmc1.ssbl",
		.flags = PRT_STM32_BOOTSRC_EMMC,
		.boot_src = BOOTSOURCE_MMC,
		.boot_idx = 1,
	}, {
		.name = "qspi",
		.env = "/chosen/environment-qspi",
		.dev = "/dev/flash.ssbl",
		.flags = PRT_STM32_BOOTSRC_SPI_NOR,
		.boot_src = BOOTSOURCE_SPI_NOR,
		.boot_idx = -1,
	}, {
		/* SD is optional boot source and should be last device in the
		 * list.
		 */
		.name = "sd",
		.env = "/chosen/environment-sd",
		.dev = "/dev/mmc0.ssbl",
		.flags = PRT_STM32_BOOTSRC_SD,
		.boot_src = BOOTSOURCE_MMC,
		.boot_idx = 0,
	},
};

static int prt_stm32_set_serial(struct device *dev, char *serial)
{
	dev_info(dev, "Serial number: %s\n", serial);
	barebox_set_serial_number(serial);

	return 0;
}

static int prt_stm32_read_serial(struct device *dev)
{
	/* board specific serial number + one char for '\0' */
	char serial[PRT_STM32_SERIAL_LEN + 1];
	struct tee_context *ctx;
	int ret;

	/* the ctx pointer will be set in the stm32_bsec_optee_ta_open */
	ret = stm32_bsec_optee_ta_open(&ctx);
	if (ret) {
		dev_err(dev, "Failed to open BSEC TA: %pe\n", ERR_PTR(ret));
		return ret;
	}

	ret = stm32_bsec_optee_ta_read(ctx, PRT_STM32_SERIAL_OFFSET * 4,
				       &serial, sizeof(serial) - 1);
	if (ret)
		goto exit_pta_read;

	serial[PRT_STM32_SERIAL_LEN] = 0;

	stm32_bsec_optee_ta_close(&ctx);

	return prt_stm32_set_serial(dev, serial);

exit_pta_read:
	stm32_bsec_optee_ta_close(&ctx);
	dev_err(dev, "Failed to read serial: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_stm32_init_shift_reg(struct device *dev)
{
	int ret;

	ret = gpio_direction_output(PRT_STM32_GPIO_HWID_PL_N, 1);
	if (ret)
		goto error_out;

	ret = gpio_direction_output(PRT_STM32_GPIO_HWID_CP, 1);
	if (ret)
		goto error_out;

	ret = gpio_direction_input(PRT_STM32_GPIO_HWID_Q7);
	if (ret)
		goto error_out;

	__stm32_pmx_set_output_type((void __iomem *)0x50002000, 13,
					       STM32_PIN_OUT_PUSHPULL);
	__stm32_pmx_set_output_type((void __iomem *)0x50002000, 14,
					       STM32_PIN_OUT_PUSHPULL);

	return 0;

error_out:
	dev_err(dev, "Failed to init shift register: %pe\n", ERR_PTR(ret));
	return ret;
}

static int prt_stm32_of_fixup_hwrev(struct device *dev, uint8_t bid,
				    uint8_t rid)
{
	const char *compat;
	char *buf;

	compat = of_device_get_match_compatible(dev);

	buf = xasprintf("%s-m%u-r%u", compat, bid, rid);
	barebox_set_of_machine_compatible(buf);

	free(buf);

	return 0;
}


/**
 * prt_stm32_read_shift_reg - Reads board ID and hardware revision
 * @dev: The device structure for logging and potential device-specific
 *       operations.
 * 
 * This function reads an 8-bit value from a 74HC165BQ parallel-in/serial-out
 * shift register to extract the board ID and hardware revision.
 * 
 * GPIO pins used:
 * - PRT_STM32_GPIO_HWID_PL_N: Controls the latch operation.
 * - PRT_STM32_GPIO_HWID_CP: Controls the clock pulses for shifting data.
 * - PRT_STM32_GPIO_HWID_Q7: Reads the serial data output from the shift
 *   register.
 */
static void prt_stm32_read_shift_reg(struct device *dev)
{
	uint8_t rid, bid;
	uint8_t data = 0;
	int i;

	/* Initial state. PL (Parallel Load) is set in inactive state */
	gpio_set_value(PRT_STM32_GPIO_HWID_PL_N, 1);
	gpio_set_value(PRT_STM32_GPIO_HWID_CP, 0);
	mdelay(1);

	/* Activate PL to latch parallel interface */
	gpio_set_value(PRT_STM32_GPIO_HWID_PL_N, 0);
	/* Wait for the data to be stable. Works for me type of delay */
	mdelay(1);
	/* Deactivate PL */ 
	gpio_set_value(PRT_STM32_GPIO_HWID_PL_N, 1);

	/* Read data from the shift register using serial interface */
	for (i = PRT_STM32_SHIFT_REG_SIZE - 1; i >= 0; i--) {
		/* Shift the data */
		data += (gpio_get_value(PRT_STM32_GPIO_HWID_Q7) << i);

		/* Toggle the clock line */
		gpio_set_value(PRT_STM32_GPIO_HWID_CP, 1);
		mdelay(1);
		gpio_set_value(PRT_STM32_GPIO_HWID_CP, 0);
	}

	rid = FIELD_GET(PRT_STM32_REVISION_ID_MASK, data);
	bid = FIELD_GET(PRT_STM32_BOARD_ID_MASK, data);

	pr_info("  Board ID:    %d\n", bid);
	pr_info("  HW revision: %d\n", rid);
	prt_stm32_of_fixup_hwrev(dev, bid, rid);

	/* PL and CP pins are shared with LEDs. Make sure LEDs are turned off */
	gpio_set_value(PRT_STM32_GPIO_HWID_PL_N, 1);
	gpio_set_value(PRT_STM32_GPIO_HWID_CP, 1);
}

static int prt_stm32_probe(struct device *dev)
{
	const struct prt_stm32_machine_data *dcfg;
	char *env_path_back = NULL, *env_path = NULL;
	int ret, i;

	dcfg = of_device_get_match_data(dev);
	if (!dcfg) {
		ret = -EINVAL;
		goto exit_get_dcfg;
	}

	prt_stm32_read_serial(dev);
	prt_stm32_init_shift_reg(dev);
	prt_stm32_read_shift_reg(dev);

	for (i = 0; i < ARRAY_SIZE(prt_stm32_boot_devs); i++) {
		const struct prt_stm32_boot_dev *bd = &prt_stm32_boot_devs[i];
		int bbu_flags = 0;

		/* skip not supported boot sources */
		if (!(bd->flags & dcfg->flags))
			continue;

		/* first device is build-in device */
		if (!env_path_back)
			env_path_back = bd->env;

		if (bd->boot_src == bootsource_get() && (bd->boot_idx == -1 ||
		    bd->boot_idx  == bootsource_get_instance())) {
			bbu_flags = BBU_HANDLER_FLAG_DEFAULT;
			env_path = bd->env;
		}

		ret = stm32mp_bbu_mmc_register_handler(bd->name, bd->dev,
						       bbu_flags);
		if (ret < 0)
			dev_warn(dev, "Failed to enable %s bbu (%pe)\n",
				 bd->name, ERR_PTR(ret));
	}

	if (!env_path)
		env_path = env_path_back;
	ret = of_device_enable_path(env_path);
	if (ret < 0)
		dev_warn(dev, "Failed to enable environment partition '%s' (%pe)\n",
			 env_path, ERR_PTR(ret));

	return 0;

exit_get_dcfg:
	dev_err(dev, "Failed to get dcfg: %pe\n", ERR_PTR(ret));
	return ret;
}

static const struct prt_stm32_machine_data prt_stm32_prihmb = {
	.flags = PRT_STM32_BOOTSRC_SD | PRT_STM32_BOOTSRC_EMMC,
};

static const struct prt_stm32_machine_data prt_stm32_mect1sr1 = {
	.flags = PRT_STM32_BOOTSRC_SPI_NOR,
};

static const struct of_device_id prt_stm32_of_match[] = {
	{ .compatible = "pri,prihmb", .data = &prt_stm32_prihmb },
	{ .compatible = "prt,mect1sr1", .data = &prt_stm32_mect1sr1 },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(prt_stm32_of_match);

static struct driver prt_stm32_board_driver = {
	.name = "board-protonic-stm32",
	.probe = prt_stm32_probe,
	.of_compatible = prt_stm32_of_match,
};
postcore_platform_driver(prt_stm32_board_driver);
