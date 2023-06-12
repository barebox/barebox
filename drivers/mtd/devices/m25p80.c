// SPDX-License-Identifier: GPL-2.0-only
/*
 * MTD SPI driver for ST M25Pxx (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 * Copyright (c) 2005, Intec Automation Inc.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <of.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <linux/err.h>
#include <clock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <linux/spi/spi-mem.h>
#include <linux/mod_devicetable.h>

#define MAX_CMD_SIZE		6

struct m25p {
	struct spi_mem		*spimem;
	struct spi_nor		spi_nor;
	struct mtd_info		mtd;
	u8			command[MAX_CMD_SIZE];
};

static int m25p80_read_reg(struct spi_nor *nor, u8 code, u8 *val, int len)
{
	struct m25p *flash = nor->priv;
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(code, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_IN(len, val, 1));
	int ret;

	ret = spi_mem_exec_op(flash->spimem, &op);
	if (ret < 0)
		dev_err(&flash->spimem->spi->dev, "error %d reading %x\n", ret,
			code);

	return ret;
}

static int m25p80_write_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
	struct m25p *flash = nor->priv;
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_OUT(len, buf, 1));

	return spi_mem_exec_op(flash->spimem, &op);
}

static void m25p80_write(struct spi_nor *nor, loff_t to, size_t len,
			 size_t *retlen, const u_char *buf)
{
	struct m25p *flash = nor->priv;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(nor->program_opcode, 1),
			   SPI_MEM_OP_ADDR(nor->addr_width, to, 1),
			   SPI_MEM_OP_NO_DUMMY,
			   SPI_MEM_OP_DATA_OUT(len, buf, 1));
	int ret;

	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->write_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(nor->write_proto);
	op.data.buswidth = spi_nor_get_protocol_data_nbits(nor->write_proto);

	if (nor->program_opcode == SPINOR_OP_AAI_WP && nor->sst_write_second)
		op.addr.nbytes = 0;

	ret = spi_mem_adjust_op_size(flash->spimem, &op);
	if (ret)
		return;

	op.data.nbytes = len < op.data.nbytes ? len : op.data.nbytes;

	ret = spi_mem_exec_op(flash->spimem, &op);
	if (ret)
		return;

	*retlen = op.data.nbytes;
}

/*
 * Read an address range from the nor chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int m25p80_read(struct spi_nor *nor, loff_t from, size_t len,
		       size_t *retlen, u_char *buf)
{
	struct m25p *flash = nor->priv;
	struct spi_mem_op op =
		SPI_MEM_OP(SPI_MEM_OP_CMD(nor->read_opcode, 1),
			   SPI_MEM_OP_ADDR(nor->addr_width, from, 1),
			   SPI_MEM_OP_DUMMY(nor->read_dummy, 1),
			   SPI_MEM_OP_DATA_IN(len, buf, 1));
	size_t remaining = len;
	int ret;

	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->read_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(nor->read_proto);
	op.dummy.buswidth = op.addr.buswidth;
	op.data.buswidth = spi_nor_get_protocol_data_nbits(nor->read_proto);

	op.dummy.nbytes = (nor->read_dummy * op.dummy.buswidth) / 8;

	while (remaining) {
		op.data.nbytes = remaining < UINT_MAX ? remaining : UINT_MAX;
		ret = spi_mem_adjust_op_size(flash->spimem, &op);
		if (ret)
			return ret;
		ret = spi_mem_exec_op(flash->spimem, &op);
		if (ret)
			return ret;
		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	*retlen = len;

	return 0;
}

/*
 * Do NOT add to this array without reading the following:
 *
 * Historically, many flash devices are bound to this driver by their name. But
 * since most of these flash are compatible to some extent, and their
 * differences can often be differentiated by the JEDEC read-ID command, we
 * encourage new users to add support to the spi-nor library, and simply bind
 * against a generic string here (e.g., "nor-jedec").
 *
 * Many flash names are kept here in this list (as well as in spi-nor.c) to
 * keep them available as module aliases for existing platforms.
 */
static const struct platform_device_id m25p_ids[] = {
	{"at25fs010"},  {"at25fs040"},  {"at25df041a"}, {"at25df321a"},
	{"at25df641"},  {"at26f004"},   {"at26df081a"}, {"at26df161a"},
	{"at26df321"},  {"at45db081d"},
	{"en25f32"},    {"en25p32"},    {"en25q32b"},   {"en25p64"},
	{"en25q64"},    {"en25qh128"},  {"en25qh256"},
	{"f25l32pa"},
	{"mr25h256"},   {"mr25h10"},    {"mr25h40"},
	{"gd25q32"},    {"gd25q64"},
	{"160s33b"},    {"320s33b"},    {"640s33b"},
	{"mx25l2005a"}, {"mx25l4005a"}, {"mx25l8005"},  {"mx25l1606e"},
	{"mx25l3205d"}, {"mx25l3255e"}, {"mx25l6405d"}, {"mx25l12805d"},
	{"mx25l12855e"},{"mx25l25635e"},{"mx25l25655e"},{"mx66l51235l"},
	{"mx66l1g55g"},
	{"n25q064"},    {"n25q128a11"}, {"n25q128a13"}, {"n25q256a"},
	{"n25q512a"},   {"n25q512ax3"}, {"n25q00"},
	{"pm25lv512"},  {"pm25lv010"},  {"pm25lq032"},
	{"s25sl032p"},  {"s25sl064p"},  {"s25fl256s0"}, {"s25fl256s1"},
	{"s25fl512s"},  {"s70fl01gs"},  {"s25sl12800"}, {"s25sl12801"},
	{"s25fl129p0"}, {"s25fl129p1"}, {"s25sl004a"},  {"s25sl008a"},
	{"s25sl016a"},  {"s25sl032a"},  {"s25sl064a"},  {"s25fl008k"},
	{"s25fl016k"},  {"s25fl064k"},  {"s25fl132k"},
	{"sst25vf040b"},{"sst25vf080b"},{"sst25vf016b"},{"sst25vf032b"},
	{"sst25vf064c"},{"sst25wf512"}, {"sst25wf010"}, {"sst25wf020"},
	{"sst25wf040"},
	{"m25p05"},     {"m25p10"},     {"m25p20"},     {"m25p40"},
	{"m25p80"},     {"m25p16"},     {"m25p32"},     {"m25p64"},
	{"m25p128"},    {"n25q032"},
	{"m25p05-nonjedec"},    {"m25p10-nonjedec"},    {"m25p20-nonjedec"},
	{"m25p40-nonjedec"},    {"m25p80-nonjedec"},    {"m25p16-nonjedec"},
	{"m25p32-nonjedec"},    {"m25p64-nonjedec"},    {"m25p128-nonjedec"},
	{"m45pe10"},    {"m45pe80"},    {"m45pe16"},
	{"m25pe20"},    {"m25pe80"},    {"m25pe16"},
	{"m25px16"},    {"m25px32"},    {"m25px32-s0"}, {"m25px32-s1"},
	{"m25px64"},    {"m25px80"},
	{"w25x10"},     {"w25x20"},     {"w25x40"},     {"w25x80"},
	{"w25x16"},     {"w25x32"},     {"w25q32"},     {"w25q32dw"},
	{"w25x64"},     {"w25q64"},     {"w25q80"},     {"w25q80bl"},
	{"w25q128"},    {"w25q256"},    {"cat25c11"},
	{"cat25c03"},   {"cat25c09"},   {"cat25c17"},   {"cat25128"},

	/*
	 * Generic support for SPI NOR that can be identified by the JEDEC READ
	 * ID opcode (0x9F). Use this, if possible.
	 */
	{"nor-jedec"},
	{ },
};

/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int m25p_probe(struct device *dev)
{
	struct spi_device *spi = (struct spi_device *)dev->type_data;
	struct spi_mem *spimem = spi->mem;
	struct m25p			*flash;
	struct spi_nor			*nor;
	struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_PP,
	};
	const char			*flash_name = NULL;
	int				device_id;
	bool				use_large_blocks;
	int ret;

	flash = xzalloc(sizeof *flash);

	nor = &flash->spi_nor;

	/* install the hooks */
	nor->read = m25p80_read;
	nor->write = m25p80_write;
	nor->write_reg = m25p80_write_reg;
	nor->read_reg = m25p80_read_reg;

	nor->dev = &spimem->spi->dev;
	nor->mtd = &flash->mtd;
	nor->priv = flash;

	flash->mtd.priv = nor;
	flash->mtd.dev.parent = &spi->dev;
	flash->spimem = spimem;

	if (spi->mode & SPI_RX_QUAD)
		hwcaps.mask |= SNOR_HWCAPS_READ_1_1_4;
	else if (spi->mode & SPI_RX_DUAL)
		hwcaps.mask |= SNOR_HWCAPS_READ_1_1_2;

	dev->priv = (void *)flash;

	if (dev->id_entry)
		flash_name = dev->id_entry->name;
	else
		flash_name = NULL; /* auto-detect */

	use_large_blocks = of_property_read_bool(dev->of_node,
						 "use-large-blocks");

	ret = spi_nor_scan(nor, flash_name, &hwcaps, use_large_blocks);
	if (ret)
		return ret;

	device_id = DEVICE_ID_SINGLE;
	if (dev->of_node)
		flash_name = of_alias_get(dev->of_node);

	if (!flash_name) {
		device_id = DEVICE_ID_DYNAMIC;
		flash_name = "m25p";
	}

	return add_mtd_device(&flash->mtd, flash_name, device_id);
}

static __maybe_unused struct of_device_id m25p80_dt_ids[] = {
	{
		.compatible = "m25p80",
	}, {
		.compatible = "jedec,spi-nor",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, m25p80_dt_ids);

static struct driver m25p80_driver = {
	.name	= "m25p80",
	.probe	= m25p_probe,
	.of_compatible = DRV_OF_COMPAT(m25p80_dt_ids),
	.id_table = (struct platform_device_id *)m25p_ids,
};
device_spi_driver(m25p80_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mike Lavender");
MODULE_DESCRIPTION("MTD SPI driver for ST M25Pxx flash chips");
