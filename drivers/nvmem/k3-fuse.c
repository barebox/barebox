// SPDX-License-Identifier: GPL-2.0-only

#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/regmap.h>
#include <linux/nvmem-provider.h>
#include <linux/arm-smccc.h>
#include <linux/bitmap.h>

/*
 * These SIP calls are currently only supported in the TI downstream
 * TF-A
 */
#define K3_SIP_OTP_READ		0xC2000002
#define K3_SIP_OTP_WRITE	0xC2000001

struct ti_k3_otp_driver_data {
	unsigned int skip_init;
	unsigned int bits_per_row;
	unsigned int nrows;
};

struct ti_k3_otp {
	struct device *dev;
	uint32_t *map;
	const struct ti_k3_otp_driver_data *data;
};

static int ti_k3_otp_read_raw(unsigned int word, unsigned int *val)
{
	struct arm_smccc_res res;
	unsigned int bank = 0;

	/* TF-A ignores bank argument */
	arm_smccc_smc(K3_SIP_OTP_READ, bank, word,
		      0, 0, 0, 0, 0, &res);

	if ((long)res.a0 == -1) /* SMC_UNK */
		return -EOPNOTSUPP;

	if (res.a0 != 0)
		return -EIO;

	*val = res.a1;

	return 0;
}

/*
 * Fuses are organized in rows where each row has a SoC specific number
 * of fuses (25 on most K3 devices). When writing a fuse we always write
 * to a single row of fuses. This means the upper 7 bits of each 32 bit word
 * are unused. When reading from fuses these gaps are skipped, meaning the first
 * word we read has 25 bits from row0 in the lower bits and 7 bits from row1
 * in the upper bits.
 * Additionally on some SoCs the very first n fuses are reserved. These bits
 * cannot be written and are skipped while reading.
 * These effects are reversed here which means that we actually provide a
 * consistent register map between writing and reading.
 *
 * Rather than adjusting the write map we adjust the read map, because this
 * way we provide one fuse row in each 32bit word and a fuse row is the granularity
 * for write protection.
 *
 * The TI-SCI firmware updates the registers we read from only after a reset,
 * so it doesn't hurt us when we read all registers upfront, you can't read
 * back the values you've just written anyway.
 */
static int ti_k3_otp_read_map(struct ti_k3_otp *priv)
{
	uint32_t *map_raw;
	int i, ret;
	unsigned int bits_per_row = priv->data->bits_per_row;
	unsigned int mask = (1 << bits_per_row) - 1;
	unsigned long *bitmap = NULL;
	int nbits = 32 * 32;
	int nrows = priv->data->nrows;

	map_raw = xzalloc(sizeof(uint32_t) * nrows);

	for (i = 0; i < 32; i++) {
		unsigned int val;

		ret = ti_k3_otp_read_raw(i, &val);
		if (ret)
			goto out;

		map_raw[i] = val;
	}

	bitmap = bitmap_xzalloc(nbits);
	bitmap_from_arr32(bitmap, map_raw, nbits);

	if (priv->data->skip_init)
		bitmap_shift_left(bitmap, bitmap, priv->data->skip_init, nbits);

	for (i = 0; i < priv->data->nrows; i++) {
		priv->map[i] = bitmap[0] & mask;
		bitmap_shift_right(bitmap, bitmap, bits_per_row, nbits);
	}

	ret = 0;

out:
	free(bitmap);
	free(map_raw);
	return ret;
}

/*
 * offset and size are assumed aligned to the size of the fuses (32-bit).
 */
static int ti_k3_otp_read(void *ctx, unsigned int offset, unsigned int *val)
{
	struct ti_k3_otp *priv = ctx;
	unsigned int word = offset >> 2;

	*val = priv->map[word];

	return 0;
}

static int ti_k3_otp_write(void *ctx, unsigned int offset, unsigned int val)
{
	struct ti_k3_otp *priv = ctx;
	struct arm_smccc_res res;
	unsigned int bank = 0;
	unsigned int word = offset >> 2;
	unsigned int mask = val;

	if (word == 0 && priv->data->skip_init) {
		unsigned int skip_mask = GENMASK(priv->data->skip_init, 0);
		if (val & skip_mask) {
			dev_err(priv->dev, "Lower %d bits of word 0 cannot be written\n",
				priv->data->skip_init);
			return -EINVAL;
		}
	}

	if (val & GENMASK(31, priv->data->bits_per_row)) {
		dev_err(priv->dev, "Each row only has %d bits",
			priv->data->bits_per_row);
		return -EINVAL;
	}

	arm_smccc_smc(K3_SIP_OTP_WRITE, bank, word,
		      val, mask, 0, 0, 0, &res);

	if (res.a0 != 0) {
		dev_err(priv->dev, "Writing fuse 0x%08x failed with: %lu\n",
			offset, res.a0);
		return -EIO;
	}

	return 0;
}

static struct regmap_bus ti_k3_otp_regmap_bus = {
	.reg_read = ti_k3_otp_read,
	.reg_write = ti_k3_otp_write,
};

static int ti_k3_otp_probe(struct device *dev)
{
	struct ti_k3_otp *priv;
	struct regmap_config config = {};
	struct regmap *map;
	int ret;

	priv = xzalloc(sizeof(*priv));
	priv->data = device_get_match_data(dev);
	priv->dev = dev;
	priv->map = xzalloc(sizeof(uint32_t) * priv->data->nrows);

	config.name = "k3-otp";
	config.reg_bits = 32;
	config.val_bits = 32;
	config.reg_stride = 4;
	config.max_register = sizeof(uint32_t) * (priv->data->nrows - 1);

	ret = ti_k3_otp_read_map(priv);
	if (ret) {
		/*
		 * Reading fuses is only supported by TI downstream TF-A and
		 * only on AM62L. Do not bother the user with error messages
		 * when it's not supported.
		 */
		if (ret == -EOPNOTSUPP) {
			dev_info(dev, "TF-A doesn't support reading OTP");
			return -ENODEV;
		}
		return ret;
	}

	map = regmap_init(dev, &ti_k3_otp_regmap_bus, priv, &config);
	if (IS_ERR(map))
		return PTR_ERR(map);

	return PTR_ERR_OR_ZERO(nvmem_regmap_register(map, "ti-k3-otp"));
}

struct ti_k3_otp_driver_data am62x_data = {
	.skip_init = 2,
	.bits_per_row = 25,
	.nrows = 42,
};

struct ti_k3_otp_driver_data am62l_data = {
	.skip_init = 0,
	.bits_per_row = 25,
	.nrows = 42,
};

static struct of_device_id ti_k3_otp_dt_ids[] = {
	{ .compatible = "ti,am62x-otp", .data = &am62x_data },
	{ .compatible = "ti,am62l-otp", .data = &am62l_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ti_k3_otp_dt_ids);

static struct driver ti_k3_otp_driver = {
	.name	= "ti-k3-otp",
	.probe	= ti_k3_otp_probe,
	.of_compatible = ti_k3_otp_dt_ids,
};
device_platform_driver(ti_k3_otp_driver);
