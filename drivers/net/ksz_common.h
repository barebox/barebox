/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NET_KSZ_COMMON_H_
#define NET_KSZ_COMMON_H_

#include <linux/swab.h>
#include <linux/regmap.h>
#include <linux/bitops.h>
#include <platform_data/ksz9477_reg.h>

struct ksz_switch {
	struct spi_device *spi;
	struct i2c_client *i2c;
	struct dsa_switch ds;
	struct device *dev;
	int phy_port_cnt;
	u32 chip_id;
	u8 features;
	struct regmap *regmap[3];
};

static inline int ksz_read8(struct ksz_switch *priv, u32 reg, u8 *val)
{
	unsigned int value = 0;
	int ret = regmap_read(priv->regmap[0], reg, &value);

	*val = value;
	return ret;
}

static inline int ksz_read16(struct ksz_switch *priv, u32 reg, u16 *val)
{
	unsigned int value = 0;
	int ret = regmap_read(priv->regmap[1], reg, &value);

	*val = value;
	return ret;
}

static inline int ksz_read32(struct ksz_switch *priv, u32 reg, u32 *val)
{
	unsigned int value = 0;
	int ret = regmap_read(priv->regmap[2], reg, &value);

	*val = value;
	return ret;
}

static inline int ksz_read64(struct ksz_switch *priv, u32 reg, u64 *val)
{
	u32 value[2];
	int ret;

	ret = regmap_bulk_read(priv->regmap[2], reg, value, 2);
	if (!ret)
		*val = (u64)value[0] << 32 | value[1];

	return ret;
}

static inline int ksz_write8(struct ksz_switch *priv, u32 reg, u8 value)
{
	return regmap_write(priv->regmap[0], reg, value);
}

static inline int ksz_write16(struct ksz_switch *priv, u32 reg, u16 value)
{
	return regmap_write(priv->regmap[1], reg, value);
}

static inline int ksz_write32(struct ksz_switch *priv, u32 reg, u32 value)
{
	return regmap_write(priv->regmap[2], reg, value);
}

static inline int ksz_write64(struct ksz_switch *priv, u32 reg, u64 value)
{
	u32 val[2];

	/* Ick! ToDo: Add 64bit R/W to regmap on 32bit systems */
	value = swab64(value);
	val[0] = swab32(value & 0xffffffffULL);
	val[1] = swab32(value >> 32ULL);

	return regmap_bulk_write(priv->regmap[2], reg, val, 2);
}

static inline int ksz_pread8(struct ksz_switch *priv, int port, int reg, u8 *val)
{
       return ksz_read8(priv, PORT_CTRL_ADDR(port, reg), val);
}

static inline int ksz_pwrite8(struct ksz_switch *priv, int port, int reg, u8 val)
{
       return ksz_write8(priv, PORT_CTRL_ADDR(port, reg), val);
}

static inline int ksz_pread16(struct ksz_switch *priv, int port, int reg, u16 *val)
{
       return ksz_read16(priv, PORT_CTRL_ADDR(port, reg), val);
}

static inline int ksz_pwrite16(struct ksz_switch *priv, int port, int reg, u16 val)
{
       return ksz_write16(priv, PORT_CTRL_ADDR(port, reg), val);
}

static inline int ksz_pwrite32(struct ksz_switch *priv, int port, int reg, u32 val)
{
       return ksz_write32(priv, PORT_CTRL_ADDR(port, reg), val);
}

static void ksz_cfg(struct ksz_switch *priv, u32 addr, u8 bits, bool set)
{
	regmap_update_bits(priv->regmap[0], addr, bits, set ? bits : 0);
}

/* Regmap tables generation */
#define KSZ_SPI_OP_RD		3
#define KSZ_SPI_OP_WR		2

#define swabnot_used(x)		0

#define KSZ_SPI_OP_FLAG_MASK(opcode, swp, regbits, regpad)		\
	swab##swp((opcode) << ((regbits) + (regpad)))

#define KSZ_REGMAP_ENTRY_COUNT	3

#define KSZ_REGMAP_ENTRY(width, swp, regbits, regpad, regalign)		\
	{								\
		.name = #width,						\
		.val_bits = (width),					\
		.reg_stride = 1,					\
		.reg_bits = (regbits) + (regalign),			\
		.pad_bits = (regpad),					\
		.max_register = BIT(regbits) - 1,			\
		.read_flag_mask =					\
			KSZ_SPI_OP_FLAG_MASK(KSZ_SPI_OP_RD, swp,	\
					     regbits, regpad),		\
		.write_flag_mask =					\
			KSZ_SPI_OP_FLAG_MASK(KSZ_SPI_OP_WR, swp,	\
					     regbits, regpad),		\
		.reg_format_endian = REGMAP_ENDIAN_BIG,			\
		.val_format_endian = REGMAP_ENDIAN_BIG			\
	}

#define KSZ_REGMAP_TABLE(ksz, swp, regbits, regpad, regalign)		\
	static const struct regmap_config ksz##_regmap_config[KSZ_REGMAP_ENTRY_COUNT] = {	\
		KSZ_REGMAP_ENTRY(8, swp, (regbits), (regpad), (regalign)), \
		KSZ_REGMAP_ENTRY(16, swp, (regbits), (regpad), (regalign)), \
		KSZ_REGMAP_ENTRY(32, swp, (regbits), (regpad), (regalign)), \
	}


#endif
