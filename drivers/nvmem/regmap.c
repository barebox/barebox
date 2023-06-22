// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <regmap.h>
#include <linux/nvmem-provider.h>

static int nvmem_regmap_write(void *ctx, unsigned offset, const void *val, size_t bytes)
{
	return regmap_bulk_write(ctx, offset, val, bytes);
}

static int nvmem_regmap_read(void *ctx, unsigned offset, void *buf, size_t bytes)
{
	struct regmap *map = ctx;
	size_t rbytes, stride, skip_bytes;
	u32 roffset, val;
	u8 *buf8 = buf, *val8 = (u8 *)&val;
	int i, j = 0, ret, size;

	stride = regmap_get_reg_stride(map);

	roffset = rounddown(offset, stride);
	skip_bytes = offset & (stride - 1);
	rbytes = roundup(bytes + skip_bytes, stride);

	if (roffset + rbytes > stride * regmap_get_max_register(map))
		return -EINVAL;

	for (i = roffset; i < roffset + rbytes; i += stride) {
		ret = regmap_bulk_read(map, i, &val, stride);
		if (ret) {
			dev_err(regmap_get_device(map), "Can't read data%d (%d)\n", i, ret);
			return ret;
		}

		/* skip first bytes in case of unaligned read */
		if (skip_bytes)
			size = min(bytes, stride - skip_bytes);
		else
			size = min(bytes, stride);

		memcpy(&buf8[j], &val8[skip_bytes], size);
		bytes -= size;
		j += size;
		skip_bytes = 0;
	}

	return 0;
}

struct nvmem_device *
nvmem_regmap_register_with_pp(struct regmap *map, const char *name,
			      nvmem_cell_post_process_t cell_post_process)
{
	struct nvmem_config config = {};

	/* Can be retrofitted if needed */
	if (regmap_get_reg_stride(map) != regmap_get_val_bytes(map))
		return ERR_PTR(-EINVAL);

	config.name = name;
	config.dev = regmap_get_device(map);
	config.priv = map;
	config.stride = 1;
	config.word_size = 1;
	config.size = regmap_get_max_register(map) * regmap_get_reg_stride(map);
	config.cell_post_process = cell_post_process;
	config.reg_write = nvmem_regmap_write;
	config.reg_read = nvmem_regmap_read;

	return nvmem_register(&config);
}

struct nvmem_device *nvmem_regmap_register(struct regmap *map, const char *name)
{
	return nvmem_regmap_register_with_pp(map, name, NULL);
}
