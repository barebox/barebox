/*
 * EEPROM driver for RAVE SP
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_device.h>
#include <malloc.h>
#include <linux/sizes.h>
#include <linux/mfd/rave-sp.h>
#include <linux/nvmem-provider.h>

/**
 * enum rave_sp_eeprom_access_type - Supported types of EEPROM access
 *
 * @RAVE_SP_EEPROM_WRITE:	EEPROM write
 * @RAVE_SP_EEPROM_READ:	EEPROM read
 *
 * Note that values of both constant are chose so that they'd match
 * similar values from RAVE SP ICD.
 */
enum rave_sp_eeprom_access_type {
	RAVE_SP_EEPROM_WRITE = 0,
	RAVE_SP_EEPROM_READ  = 1,
};

/**
 * enum rave_sp_eeprom_header_size - EEPROM command header sizes
 *
 * @RAVE_SP_EEPROM_HEADER_SMALL:	EEPROM header size for "small" devices (< 8K)
 * @RAVE_SP_EEPROM_HEADER_BIG:		EEPROM header size for "big" devices (> 8K, < 2M)
 *
 * Note that values of both constant are chose so that they'd match
 * similar values from RAVE SP ICD.
 */
enum rave_sp_eeprom_header_size {
	RAVE_SP_EEPROM_HEADER_SMALL = 4U,
	RAVE_SP_EEPROM_HEADER_BIG   = 5U,
};
#define RAVE_SP_EEPROM_HEADER_MAX	RAVE_SP_EEPROM_HEADER_BIG

#define	RAVE_SP_EEPROM_PAGE_SIZE	32U

/**
 * struct rave_sp_eeprom_page - RAVE SP EEPROM page
 *
 * @type:	Access type (see enum rave_sp_eeprom_access_type)
 * @success:	Success flag (Success = 1, Failure = 0)
 * @data:	Read data
 *
 * Note that the layout of this structure is made to follow the layout
 * of RSP_*_EEPROM from device's ICD (only the data portion of it due
 * to how rave_sp_exec() returns data)
 */
struct rave_sp_eeprom_page {
	u8  type;
	u8  success;
	u8  data[RAVE_SP_EEPROM_PAGE_SIZE];
} __packed;

/**
 * struct rave_sp_eeprom - RAVE SP EEPROM device
 *
 * @sp:			Pointer to parent RAVE SP device
 * @mutex:		Lock protecting access to EEPROM
 * @address:		EEPROM device address (first byte of the command to access it)
 * @header_size:	Size of EEPROM command header for this device
 */
struct rave_sp_eeprom {
	struct rave_sp *sp;
	u8 address;
	unsigned int header_size;
};

/**
 * rave_sp_eeprom_io - Low-level part of EEPROM page access
 *
 * @eeprom:	EEPROM device to write to
 * @type:	EEPROM access type (read or write)
 * @idx:	number of the EEPROM page
 * @page:	Data to write or buffer to store result (via page->data)
 *
 * This function does all of the low-lever work required to perform a
 * EEPROM access. This includes formatting corredct command payload,
 * sending it and checking received results.
 *
 * Returns zero in case of success or negative error code in
 * case of failure.
 */
static int rave_sp_eeprom_io(struct rave_sp_eeprom *eeprom,
			     enum rave_sp_eeprom_access_type type,
			     u16 idx,
			     struct rave_sp_eeprom_page *page)
{
	const bool is_write = type == RAVE_SP_EEPROM_WRITE;
	const unsigned int data_size = is_write ? sizeof(page->data) : 0;
	const unsigned int cmd_size = eeprom->header_size + data_size;
	const unsigned int rsp_size =
		is_write ? sizeof(*page) - sizeof(page->data) : sizeof(*page);
	unsigned int offset = 0;
	u8 cmd[RAVE_SP_EEPROM_HEADER_MAX + sizeof(page->data)];
	int ret;

	if (WARN_ON(cmd_size > sizeof(cmd)))
		return -EINVAL;

	cmd[offset++] = eeprom->address;
	cmd[offset++] = 0;
	cmd[offset++] = type;
	cmd[offset++] = idx;

	if (offset < eeprom->header_size)
		cmd[offset++] = idx >> 8;
	/*
	 * Copy our data to write to command buffer first. In case of
	 * a read data_size should be zero and memcpy would become a
	 * no-op
	 */
	memcpy(&cmd[offset], page->data, data_size);

	ret = rave_sp_exec(eeprom->sp, cmd, cmd_size, page, rsp_size);
	if (ret)
		return ret;

	if (page->type != type)
		return -EPROTO;

	if (!page->success)
		return -EIO;

	return 0;
}

/**
 * rave_sp_eeprom_page_access - Access single EEPROM page
 *
 * @eeprom:	EEPROM device to access
 * @type:	Access type to perform (read or write)
 * @offset:	Offset within EEPROM to access
 * @data:	Data buffer
 * @data_len:	Size of the data buffer
 *
 * This function performs a generic access to a single page or a
 * portion thereof. Requested access MUST NOT cross the EEPROM page
 * boundary.
 *
 * Returns zero in case of success or negative error code in
 * case of failure.
 */
static int
rave_sp_eeprom_page_access(struct rave_sp_eeprom *eeprom,
			   enum rave_sp_eeprom_access_type type,
			   unsigned int offset, u8 *data,
			   size_t data_len)
{
	const unsigned int page_offset = offset % RAVE_SP_EEPROM_PAGE_SIZE;
	const unsigned int page_nr     = offset / RAVE_SP_EEPROM_PAGE_SIZE;
	struct rave_sp_eeprom_page page;
	int ret;

	/*
	 * This function will not work if data access we've been asked
	 * to do is crossing EEPROM page boundary. Normally this
	 * should never happen and getting here would indicate a bug
	 * in the code.
	 */
	if (WARN_ON(data_len > sizeof(page.data) - page_offset))
		return -EINVAL;

	if (type == RAVE_SP_EEPROM_WRITE) {
		/*
		 * If doing a partial write we need to do a read first
		 * to fill the rest of the page with correct data.
		 */
		if (data_len < RAVE_SP_EEPROM_PAGE_SIZE) {
			ret = rave_sp_eeprom_io(eeprom, RAVE_SP_EEPROM_READ,
						page_nr, &page);
			if (ret)
				return ret;
		}

		memcpy(&page.data[page_offset], data, data_len);
	}

	ret = rave_sp_eeprom_io(eeprom, type, page_nr, &page);
	if (ret)
		return ret;

	/*
	 * Since we receive the result of the read via 'page.data'
	 * buffer we need to copy that to 'data'
	 */
	if (type == RAVE_SP_EEPROM_READ)
		memcpy(data, &page.data[page_offset], data_len);

	return 0;
}

/**
 * rave_sp_eeprom_access - Access EEPROM data
 *
 * @eeprom:	EEPROM device to access
 * @type:	Access type to perform (read or write)
 * @offset:	Offset within EEPROM to access
 * @data:	Data buffer
 * @data_len:	Size of the data buffer
 *
 * This function performs a generic access (either read or write) at
 * arbitrary offset (not necessary page aligned) of arbitrary length
 * (is not constrained by EEPROM page size).
 *
 * Returns zero in case of success or negative error code in case of
 * failure.
 */
static int rave_sp_eeprom_access(struct rave_sp_eeprom *eeprom,
				 enum rave_sp_eeprom_access_type type,
				 unsigned int offset, u8 *data,
				 unsigned int data_len)
{
	unsigned int residue;
	unsigned int chunk;
	unsigned int head;
	int ret;

	head    = offset % RAVE_SP_EEPROM_PAGE_SIZE;
	residue = data_len;

	do {
		/*
		 * First iteration, if we are doing an access that is
		 * not 32-byte aligned, we need to access only data up
		 * to a page boundary to avoid corssing it in
		 * rave_sp_eeprom_page_access()
		 */
		if (head) {
			chunk = RAVE_SP_EEPROM_PAGE_SIZE - head;
			/*
			 * This can only happen once per
			 * rave_sp_eeprom_access() call, so we set
			 * head to zero to process all the other
			 * iterations normally.
			 */
			head  = 0;
		} else {
			chunk = RAVE_SP_EEPROM_PAGE_SIZE;
		}

		/*
		 * We should never read more that 'residue' bytes
		 */
		chunk = min(chunk, residue);
		ret = rave_sp_eeprom_page_access(eeprom, type, offset,
						 data, chunk);
		if (ret)
			return ret;

		residue -= chunk;
		offset  += chunk;
		data    += chunk;
	} while (residue);

	return 0;
}

static int rave_sp_eeprom_read(struct device_d *dev, const int offset,
			       void *val, int bytes)
{
	return rave_sp_eeprom_access(dev->parent->priv,
				     RAVE_SP_EEPROM_READ,
				     offset, val, bytes);
}

static int rave_sp_eeprom_write(struct device_d *dev, const int offset,
				const void *val, int bytes)
{
	return rave_sp_eeprom_access(dev->parent->priv,
				     RAVE_SP_EEPROM_WRITE,
				     offset, (void *)val, bytes);
}

static const struct nvmem_bus rave_sp_eeprom_nvmem_bus = {
	.write = rave_sp_eeprom_write,
	.read  = rave_sp_eeprom_read,
};

static int rave_sp_eeprom_probe(struct device_d *dev)
{
	struct rave_sp *sp = dev->parent->priv;
	struct nvmem_config config = { 0 };
	struct rave_sp_eeprom *eeprom;
	struct nvmem_device *nvmem;
	u32 reg[2], size;

	if (of_property_read_u32_array(dev->device_node,
				       "reg", reg, ARRAY_SIZE(reg))) {
		dev_err(dev, "Failed to parse \"reg\" property\n");
		return -EINVAL;
	}

	size = reg[1];
	/*
	 * Per ICD, we have no more than 2 bytes to specify EEPROM
	 * page.
	 */
	if (size > U16_MAX * RAVE_SP_EEPROM_PAGE_SIZE) {
		dev_err(dev, "Specified size is too big\n");
		return -EINVAL;
	}

	eeprom = xzalloc(sizeof(*eeprom));
	eeprom->address = reg[0];
	eeprom->sp      = sp;

	dev->priv = eeprom;

	if (size > SZ_8K)
		eeprom->header_size = RAVE_SP_EEPROM_HEADER_BIG;
	else
		eeprom->header_size = RAVE_SP_EEPROM_HEADER_SMALL;

	config.name      = dev_name(dev);
	/*
	 * If a name is specified via DT, override the above with it.
	 */
	of_property_read_string(dev->device_node, "zii,eeprom-name",
				&config.name);
	config.dev       = dev;
	config.word_size = 1;
	config.stride    = 1;
	config.size      = reg[1];
	config.bus       = &rave_sp_eeprom_nvmem_bus;

	nvmem = nvmem_register(&config);
	if (IS_ERR(nvmem)) {
		free(eeprom);
		return PTR_ERR(nvmem);
	}

	return 0;
}

static __maybe_unused const struct of_device_id rave_sp_eeprom_of_match[] = {
	{ .compatible = "zii,rave-sp-eeprom" },
	{}
};

static struct driver_d rave_sp_eeprom_driver = {
	.name = "rave-sp-eeprom",
	.probe = rave_sp_eeprom_probe,
	.of_compatible = DRV_OF_COMPAT(rave_sp_eeprom_of_match),
};
console_platform_driver(rave_sp_eeprom_driver);
