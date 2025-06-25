// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <linux/regmap.h>
#include <linux/nvmem-provider.h>

static int nvmem_regmap_write(void *ctx, unsigned offset, const void *val, size_t bytes)
{
	struct regmap *map = ctx;

	/*
	 * eFuse writes going through this function may be irreversible,
	 * so expect users to observe alignment.
	 */
	if (bytes % regmap_get_val_bytes(map))
		return -EINVAL;

	return regmap_bulk_write(map, offset, val,
				 bytes / regmap_get_val_bytes(map));
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

	if (roffset + rbytes > regmap_size_bytes(map))
		return -EINVAL;

	for (i = roffset; i < roffset + rbytes; i += stride) {
		ret = regmap_read(map, i, &val);
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

static int nvmem_regmap_protect(void *ctx, unsigned int offset, size_t bytes,
				int prot)
{
	unsigned int seal_flags = 0;
	struct regmap *map = ctx;
	size_t reg_val_bytes;
	unsigned int i;
	int ret = 0;

	reg_val_bytes = regmap_get_val_bytes(map);
	if (reg_val_bytes == 0) {
		dev_err(regmap_get_device(map), "Invalid regmap value byte size (0)\n");
		return -EINVAL;
	}

	/* NVMEM protect operations should typically be on aligned boundaries
	 * matching the hardware's lockable unit (which is regmap's val_bytes
	 * here).
	 */
	if ((offset % reg_val_bytes) != 0 || (bytes % reg_val_bytes) != 0) {
		dev_warn(regmap_get_device(map),
			 "NVMEM protect op for regmap: offset (0x%x) or size (0x%zx) not aligned to register size (%zu bytes).\n",
			 offset, bytes, reg_val_bytes);
		return -EINVAL;
	}

	switch (prot) {
	case PROTECT_ENABLE_WRITE:
		/* NVMEM protect mode 0 = Unlock/Make-writable
		 * Attempt to clear write protection.
		 * The underlying bus->reg_seal must support clearing.
		 * For BSEC OTPs, this will (and should) fail with -EOPNOTSUPP
		 * or -EPERM.
		 */
		seal_flags = REGMAP_SEAL_CLEAR | REGMAP_SEAL_WRITE_PROTECT;
		break;
	case PROTECT_DISABLE_WRITE:
		/* NVMEM protect mode 1 = Lock/Write-protect */
		/* For OTPs like BSEC, permanent is implied */
		seal_flags = REGMAP_SEAL_WRITE_PROTECT | REGMAP_SEAL_PERMANENT;
		break;
	default:
		dev_warn(regmap_get_device(map), "Unsupported NVMEM protect mode: %d\n",
			 prot);
		return -EOPNOTSUPP;
	}

	for (i = 0; i < bytes; i += reg_val_bytes) {
		unsigned int current_reg_offset = offset + i;

		ret = regmap_seal(map, current_reg_offset, seal_flags);
		if (ret) {
			dev_err(regmap_get_device(map), "regmap_seal failed for offset 0x%x: %pe\n",
				current_reg_offset, ERR_PTR(ret));
			/* No error handling for partial failures, we messed up
			 * the HW state and can't recover.
			 */
			return ret;
		}
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
	config.size = regmap_size_bytes(map);
	config.cell_post_process = cell_post_process;
	config.reg_write = nvmem_regmap_write;
	config.reg_read = nvmem_regmap_read;
	config.reg_protect = nvmem_regmap_protect;

	return nvmem_register(&config);
}

struct nvmem_device *nvmem_regmap_register(struct regmap *map, const char *name)
{
	return nvmem_regmap_register_with_pp(map, name, NULL);
}
