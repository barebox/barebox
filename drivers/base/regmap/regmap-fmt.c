// SPDX-License-Identifier: GPL-2.0-only
/*
 * Formatted register map access API
 *
 * Copyright 2022 Ahmad Fatoum <a.fatoum@pengutronix.de>
 *
 * based on Kernel code:
 *
 * Copyright 2011 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 */

#include <common.h>
#include <linux/regmap.h>
#include <linux/log2.h>
#include <asm/unaligned.h>

#include "internal.h"

static void regmap_format_12_20_write(struct regmap *map,
				     unsigned int reg, unsigned int val)
{
	u8 *out = map->work_buf;

	out[0] = reg >> 4;
	out[1] = (reg << 4) | (val >> 16);
	out[2] = val >> 8;
	out[3] = val;
}


static void regmap_format_2_6_write(struct regmap *map,
				     unsigned int reg, unsigned int val)
{
	u8 *out = map->work_buf;

	*out = (reg << 6) | val;
}

static void regmap_format_4_12_write(struct regmap *map,
				     unsigned int reg, unsigned int val)
{
	__be16 *out = map->work_buf;
	*out = cpu_to_be16((reg << 12) | val);
}

static void regmap_format_7_9_write(struct regmap *map,
				    unsigned int reg, unsigned int val)
{
	__be16 *out = map->work_buf;
	*out = cpu_to_be16((reg << 9) | val);
}

static void regmap_format_7_17_write(struct regmap *map,
				    unsigned int reg, unsigned int val)
{
	u8 *out = map->work_buf;

	out[2] = val;
	out[1] = val >> 8;
	out[0] = (val >> 16) | (reg << 1);
}

static void regmap_format_10_14_write(struct regmap *map,
				    unsigned int reg, unsigned int val)
{
	u8 *out = map->work_buf;

	out[2] = val;
	out[1] = (val >> 8) | (reg << 6);
	out[0] = reg >> 2;
}

static void regmap_format_8(void *buf, unsigned int val, unsigned int shift)
{
	u8 *b = buf;

	b[0] = val << shift;
}

static void regmap_format_16_be(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_be16(val << shift, buf);
}

static void regmap_format_16_le(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_le16(val << shift, buf);
}

static void regmap_format_16_native(void *buf, unsigned int val,
				    unsigned int shift)
{
	u16 v = val << shift;

	memcpy(buf, &v, sizeof(v));
}

static void regmap_format_24(void *buf, unsigned int val, unsigned int shift)
{
	u8 *b = buf;

	val <<= shift;

	b[0] = val >> 16;
	b[1] = val >> 8;
	b[2] = val;
}

static void regmap_format_32_be(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_be32(val << shift, buf);
}

static void regmap_format_32_le(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_le32(val << shift, buf);
}

static void regmap_format_32_native(void *buf, unsigned int val,
				    unsigned int shift)
{
	u32 v = val << shift;

	memcpy(buf, &v, sizeof(v));
}

#ifdef CONFIG_64BIT
static void regmap_format_64_be(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_be64((u64) val << shift, buf);
}

static void regmap_format_64_le(void *buf, unsigned int val, unsigned int shift)
{
	put_unaligned_le64((u64) val << shift, buf);
}

static void regmap_format_64_native(void *buf, unsigned int val,
				    unsigned int shift)
{
	u64 v = (u64) val << shift;

	memcpy(buf, &v, sizeof(v));
}
#endif

static unsigned int regmap_parse_8(const void *buf)
{
	const u8 *b = buf;

	return b[0];
}

static unsigned int regmap_parse_16_be(const void *buf)
{
	return get_unaligned_be16(buf);
}

static unsigned int regmap_parse_16_le(const void *buf)
{
	return get_unaligned_le16(buf);
}

static unsigned int regmap_parse_16_native(const void *buf)
{
	u16 v;

	memcpy(&v, buf, sizeof(v));
	return v;
}

static unsigned int regmap_parse_24(const void *buf)
{
	const u8 *b = buf;
	unsigned int ret = b[2];
	ret |= ((unsigned int)b[1]) << 8;
	ret |= ((unsigned int)b[0]) << 16;

	return ret;
}

static unsigned int regmap_parse_32_be(const void *buf)
{
	return get_unaligned_be32(buf);
}

static unsigned int regmap_parse_32_le(const void *buf)
{
	return get_unaligned_le32(buf);
}

static unsigned int regmap_parse_32_native(const void *buf)
{
	u32 v;

	memcpy(&v, buf, sizeof(v));
	return v;
}

#ifdef CONFIG_64BIT
static unsigned int regmap_parse_64_be(const void *buf)
{
	return get_unaligned_be64(buf);
}

static unsigned int regmap_parse_64_le(const void *buf)
{
	return get_unaligned_le64(buf);
}

static unsigned int regmap_parse_64_native(const void *buf)
{
	u64 v;

	memcpy(&v, buf, sizeof(v));
	return v;
}
#endif


static enum regmap_endian regmap_get_reg_endian(const struct regmap_bus *bus,
					const struct regmap_config *config)
{
	enum regmap_endian endian;

	/* Retrieve the endianness specification from the regmap config */
	endian = config->reg_format_endian;

	/* If the regmap config specified a non-default value, use that */
	if (endian != REGMAP_ENDIAN_DEFAULT)
		return endian;

	/* Retrieve the endianness specification from the bus config */
	if (bus && bus->reg_format_endian_default)
		endian = bus->reg_format_endian_default;

	/* If the bus specified a non-default value, use that */
	if (endian != REGMAP_ENDIAN_DEFAULT)
		return endian;

	/* Use this if no other value was found */
	return REGMAP_ENDIAN_BIG;
}

static void regmap_set_work_buf_flag_mask(struct regmap *map, int max_bytes,
					  unsigned long mask)
{
	u8 *buf;
	int i;

	if (!mask || !map->work_buf)
		return;

	buf = map->work_buf;

	for (i = 0; i < max_bytes; i++)
		buf[i] |= (mask >> (8 * i)) & 0xff;
}

static int _regmap_raw_read(struct regmap *map, unsigned int reg, void *val,
			    unsigned int val_len, bool noinc)
{
	const struct regmap_bus *bus = map->bus;

	if (!bus->read)
		return -EINVAL;

	map->format.format_reg(map->work_buf, reg, map->reg_shift);
	regmap_set_work_buf_flag_mask(map, map->format.reg_bytes,
				      map->read_flag_mask);

	return bus->read(map->bus_context, map->work_buf,
			 map->format.reg_bytes + map->format.pad_bytes,
			 val, val_len);

}

static int _regmap_bus_read(void *context, unsigned int reg,
			    unsigned int *val)
{
	int ret;
	struct regmap *map = context;
	void *work_val = map->work_buf + map->format.reg_bytes +
		map->format.pad_bytes;

	if (!map->format.parse_val)
		return -EINVAL;

	ret = _regmap_raw_read(map, reg, work_val, map->format.val_bytes, false);
	if (ret == 0)
		*val = map->format.parse_val(work_val);

	return ret;
}

static int _regmap_bus_formatted_write(void *context, unsigned int reg,
				       unsigned int val)
{
	struct regmap *map = context;

	map->format.format_write(map, reg, val);

	return map->bus->write(map->bus_context, map->work_buf,
			      map->format.buf_size);
}

static int _regmap_raw_write_impl(struct regmap *map, unsigned int reg,
				  const void *val, size_t val_len, bool noinc)
{
	void *work_val = map->work_buf + map->format.reg_bytes +
		map->format.pad_bytes;

	map->format.format_reg(map->work_buf, reg, map->reg_shift);
	regmap_set_work_buf_flag_mask(map, map->format.reg_bytes,
				      map->write_flag_mask);

	/*
	 * Essentially all I/O mechanisms will be faster with a single
	 * buffer to write.  Since register syncs often generate raw
	 * writes of single registers optimise that case.
	 */
	if (val != work_val && val_len == map->format.val_bytes) {
		memcpy(work_val, val, map->format.val_bytes);
		val = work_val;
	}

	/* If we're doing a single register write we can probably just
	 * send the work_buf directly, otherwise try to do a gather
	 * write.
	 */
	return map->bus->write(map->bus_context, map->work_buf,
			       map->format.reg_bytes +
			       map->format.pad_bytes +
			       val_len);

}

static int _regmap_bus_raw_write(void *context, unsigned int reg,
				 unsigned int val)
{
	struct regmap *map = context;

	WARN_ON(!map->format.format_val);

	map->format.format_val(map->work_buf + map->format.reg_bytes
			       + map->format.pad_bytes, val, 0);
	return _regmap_raw_write_impl(map, reg,
				      map->work_buf +
				      map->format.reg_bytes +
				      map->format.pad_bytes,
				      map->format.val_bytes,
				      false);
}

int regmap_formatted_init(struct regmap *map, const struct regmap_config *config)
{
	enum regmap_endian reg_endian, val_endian;
	const struct regmap_bus *bus = map->bus;

	map->format.buf_size = DIV_ROUND_UP(config->reg_bits +
			config->val_bits + config->pad_bits, 8);

	map->work_buf = xzalloc(map->format.buf_size);

	if (config->read_flag_mask || config->write_flag_mask) {
		map->read_flag_mask = config->read_flag_mask;
		map->write_flag_mask = config->write_flag_mask;
	} else {
		map->read_flag_mask = bus->read_flag_mask;
	}

	map->reg_read = _regmap_bus_read;

	reg_endian = regmap_get_reg_endian(bus, config);
	val_endian = regmap_get_val_endian(map->dev, bus, config);

	switch (config->reg_bits + config->pad_bits % 8) {
	case 2:
		switch (config->val_bits) {
		case 6:
			map->format.format_write = regmap_format_2_6_write;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 4:
		switch (config->val_bits) {
		case 12:
			map->format.format_write = regmap_format_4_12_write;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 7:
		switch (config->val_bits) {
		case 9:
			map->format.format_write = regmap_format_7_9_write;
			break;
		case 17:
			map->format.format_write = regmap_format_7_17_write;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 10:
		switch (config->val_bits) {
		case 14:
			map->format.format_write = regmap_format_10_14_write;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 12:
		switch (config->val_bits) {
		case 20:
			map->format.format_write = regmap_format_12_20_write;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 8:
		map->format.format_reg = regmap_format_8;
		break;

	case 16:
		switch (reg_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_reg = regmap_format_16_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_reg = regmap_format_16_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_reg = regmap_format_16_native;
			break;
		default:
			return -EINVAL;
		}
		break;

	case 24:
		if (reg_endian != REGMAP_ENDIAN_BIG)
			return -EINVAL;
		map->format.format_reg = regmap_format_24;
		break;

	case 32:
		switch (reg_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_reg = regmap_format_32_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_reg = regmap_format_32_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_reg = regmap_format_32_native;
			break;
		default:
			return -EINVAL;
		}
		break;

#ifdef CONFIG_64BIT
	case 64:
		switch (reg_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_reg = regmap_format_64_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_reg = regmap_format_64_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_reg = regmap_format_64_native;
			break;
		default:
			return -EINVAL;
		}
		break;
#endif

	default:
		return -EINVAL;
	}

	switch (config->val_bits) {
	case 8:
		map->format.format_val = regmap_format_8;
		map->format.parse_val = regmap_parse_8;
		break;
	case 16:
		switch (val_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_val = regmap_format_16_be;
			map->format.parse_val = regmap_parse_16_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_val = regmap_format_16_le;
			map->format.parse_val = regmap_parse_16_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_val = regmap_format_16_native;
			map->format.parse_val = regmap_parse_16_native;
			break;
		default:
			return -EINVAL;
		}
		break;
	case 24:
		if (val_endian != REGMAP_ENDIAN_BIG)
			return -EINVAL;
		map->format.format_val = regmap_format_24;
		map->format.parse_val = regmap_parse_24;
		break;
	case 32:
		switch (val_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_val = regmap_format_32_be;
			map->format.parse_val = regmap_parse_32_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_val = regmap_format_32_le;
			map->format.parse_val = regmap_parse_32_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_val = regmap_format_32_native;
			map->format.parse_val = regmap_parse_32_native;
			break;
		default:
			return -EINVAL;
		}
		break;
#ifdef CONFIG_64BIT
	case 64:
		switch (val_endian) {
		case REGMAP_ENDIAN_BIG:
			map->format.format_val = regmap_format_64_be;
			map->format.parse_val = regmap_parse_64_be;
			break;
		case REGMAP_ENDIAN_LITTLE:
			map->format.format_val = regmap_format_64_le;
			map->format.parse_val = regmap_parse_64_le;
			break;
		case REGMAP_ENDIAN_NATIVE:
			map->format.format_val = regmap_format_64_native;
			map->format.parse_val = regmap_parse_64_native;
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	}

	if (map->format.format_write)
		map->reg_write = _regmap_bus_formatted_write;
	else if (map->format.format_val)
		map->reg_write = _regmap_bus_raw_write;
	else
		return -EOPNOTSUPP;

	return 0;
}
