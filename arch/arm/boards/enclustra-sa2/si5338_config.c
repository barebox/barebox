// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt)     "enclustra-SA2-si5338: " fmt

#include <i2c/i2c.h>
#include <clock.h>
#include <linux/kernel.h>              /* ARRAY_SIZE */
#include <linux/regmap.h>
#include "si5338_config.h"
#include "Si5338-RevB-Registers.h"

struct si5338 {
	struct device *dev;
	struct regmap *map;
	struct i2c_client *client;
	int revision;
	int type;
} si_dev = { NULL, NULL, NULL, 0, 0 };


/**
 * @brief Get the device from the devicetree
 * @return A pointer to the device if found, or \t NULL otherwise.
 */
static struct device *get_dev(void)
{
	struct device *dev;
	struct i2c_client *client;

	dev = get_device_by_name("si53380");
	if (dev == NULL) {
		pr_err("can't find device SI5338\n");
		return NULL;
	}
	client = to_i2c_client(dev);
	pr_debug("found at I2C address 0x%02x\n", client->addr);

	return dev;
}

/**
 * @brief Write a single byte to a register in the SI5338
 * @param[in] ctx A pointer to a struct si5338.
 * @param[in] reg The register address.
 * @param[in] val The byte to be written to the register.
 * @return 0 on success, a negative value from `asm-generic/errno.h` on error.
 */
static int regmap_reg_write(void *ctx, unsigned int reg, unsigned int val)
{
	int ret;
	struct si5338 *si = ctx;
	u8 buf[] = { val };			/* register value */

	ret = i2c_write_reg(si->client, reg, buf, 1);

	return ret == 1 ? 0 : ret;
}

/**
 * @brief Read a single byte from a register in the SI5338
 * @param[in] ctx A pointer to a struct si5338.
 * @param[in] reg The register address.
 * @param[out] val The byte to be written to the register.
 * @return 0 on success, a negative value from `asm-generic/errno.h` on error.
 */
static int regmap_reg_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct si5338 *si = ctx;
	u8 buf[1];
	int ret;

	ret = i2c_read_reg(si->client, reg, buf, 1);
	*val = buf[0];

	return ret == 1 ? 0 : ret;
}

/**
 * @brief Validate input clock status
 * @param[in] dev The I²C device.
 *
 * Loop until the \c LOS_CLKIN bit is clear.
 *
 * @return 0 on success, a negative value from `asm-generic/errno.h` on error.
 */
static int check_input_clock(struct device *dev)
{
	// validate input clock status
	int ret;
	unsigned int val;

	do {
		ret = regmap_reg_read(&si_dev, 218, &val);
		if (ret) {
			pr_err("SI5338 read failed addr: 218\n");
			return ret;
		}
	} while (val & 0x04);

	return 0;
}

/**
 * @brief Check output PLL status
 * @param[in] dev The I²C device.
 *
 * Loop until the \c PLL_LOL, \c LOS_CLKIN and \c SYS_CAL bits are clear.
 *
 * @return 0 on success, a negative value from `asm-generic/errno.h` on error
 * (-EIO if too many trials).
 */
static int check_pll(struct device *dev)
{
	int ret;
	int try = 0;
	unsigned int val;

	do {
		ret = regmap_reg_read(&si_dev, 218, &val);
		if (ret < 0)
			return ret;
		mdelay(100);
		try++;
		if (try > 10) {
			pr_err("SI5338 PLL is not locking\n");
			return -EIO;
		}
	} while (val & 0x15);

	return 0;
}

int si5338_init(void)
{
	unsigned int val;
	struct device *dev;
	int ret;
	struct regmap_config rgmp_cfg = {
		.reg_bits = 8,          /* register addresses are coded on 8 bits */
		.reg_stride = 1,        /* register addresses increment by 1 */
		.pad_bits = 0,
		.val_bits = 8,
		.max_register = 0xff,   /* maximum register address */
		.reg_format_endian = REGMAP_ENDIAN_DEFAULT,
		.val_format_endian = REGMAP_ENDIAN_DEFAULT,
		.read_flag_mask = 0,
		.write_flag_mask = 0,
	};
	struct regmap_bus rgmp_i2c_bus = {
		.reg_write = regmap_reg_write,
		.reg_read = regmap_reg_read,
	};

	si_dev.dev = get_dev();
	if (si_dev.dev == NULL)
		return -ENODEV;
	si_dev.client = to_i2c_client(si_dev.dev);
	si_dev.map = regmap_init(si_dev.dev, &rgmp_i2c_bus,
							 &si_dev, &rgmp_cfg);

	dev = get_dev();
	if (dev == NULL)
		return -ENODEV;

	/* Set PAGE_SEL bit to 0. If bit is 1, registers with address
	 * greater than 255 can be addressed.
	 */
	if (regmap_reg_write(&si_dev, 255, 0x00))
		return -1;

	// disable outputs
	if (regmap_update_bits(si_dev.map, 230, 0x10, 0x10))
		return -1;

	// pause lol
	if (regmap_update_bits(si_dev.map, 241, 0x80, 0x80))
		return -1;

	// write new configuration
	for (int i = 0; i < NUM_REGS_MAX; i++)
		if (regmap_update_bits(si_dev.map, Reg_Store[i].Reg_Addr,
							   Reg_Store[i].Reg_Mask, Reg_Store[i].Reg_Val))
			return -1;

	ret = check_input_clock(dev);
	if (ret)
		return ret;

	// configure PLL for locking
	ret = regmap_update_bits(si_dev.map, 49, 0x80, 0x00);
	if (ret)
		return ret;

	// initiate locking of PLL
	ret = regmap_reg_write(&si_dev, 246, 0x02);
	if (ret)
		return ret;

	// wait 25ms (100ms to be on the safe side)
	mdelay(100);

	// restart lol
	ret = regmap_update_bits(si_dev.map, 241, 0xff, 0x65);
	if (ret)
		return ret;

	ret = check_pll(dev);
	if (ret)
		return ret;

	// copy fcal values to active registers: FCAL[17:16]
	ret = regmap_reg_read(&si_dev, 237, &val);
	if (ret)
		return ret;
	ret = regmap_update_bits(si_dev.map, 47, 0x03, val);
	if (ret)
		return ret;

	// copy fcal values to active registers: FCAL[15:8]
	ret = regmap_reg_read(&si_dev, 236, &val);
	if (ret)
		return ret;
	ret = regmap_reg_write(&si_dev, 46, val);
	if (ret)
		return ret;

	// copy fcal values to active registers: FCAL[7:0]
	ret = regmap_reg_read(&si_dev, 235, &val);
	if (ret)
		return ret;
	ret = regmap_reg_write(&si_dev, 45, val);
	if (ret)
		return ret;

	// Must write 000101b to these bits if the device is not factory programmed
	ret = regmap_update_bits(si_dev.map, 47, 0xFC, 0x14);
	if (ret)
		return ret;

	// set PLL to use FCAL values
	ret = regmap_update_bits(si_dev.map, 49, 0x80, 0x80);
	if (ret)
		return ret;

	// enable outputs
	ret = regmap_reg_write(&si_dev, 230, 0x00);
	if (ret)
		return ret;

	pr_info("initialized\n");

	return 0;
}
