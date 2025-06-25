// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xilinx Zynq MPSoC PL loading
 *
 * Copyright (c) 2018 Thomas Haemmerle <thomas.haemmerle at wolfvision.net>
 *
 * based on U-Boot zynqmppl code
 *
 * (C) Copyright 2015 - 2016, Xilinx, Inc,
 * Michal Simek <michal.simek at xilinx.com>
 * Siva Durga Prasad <siva.durga.paladugu at xilinx.com> *
 */

#include <firmware.h>
#include <xilinx-firmware.h>
#include <common.h>
#include <init.h>
#include <dma.h>
#include <mach/zynqmp/zynqmp-pcap.h>
#include <mach/zynq/zynq-pcap.h>

struct xilinx_fpga_devdata zynqmp_devdata = {
	.bus_width_auto_detect1_offset = ZYNQMP_BUS_WIDTH_AUTO_DETECT1_OFFSET,
	.bus_width_auto_detect2_offset = ZYNQMP_BUS_WIDTH_AUTO_DETECT2_OFFSET,
	.sync_word_offset = ZYNQMP_SYNC_WORD_OFFSET,
	.bin_header_length = ZYNQMP_BIN_HEADER_LENGTH,
	.dev_init = zynqmp_init,
	.dev_programmed_get = zynqmp_programmed_get,
	.dev_fpga_load = zynqmp_fpga_load,
};

static struct xilinx_fpga_devdata zynq_devdata = {
	.bus_width_auto_detect1_offset = ZYNQ_BUS_WIDTH_AUTO_DETECT1_OFFSET,
	.bus_width_auto_detect2_offset = ZYNQ_BUS_WIDTH_AUTO_DETECT2_OFFSET,
	.sync_word_offset = ZYNQ_SYNC_WORD_OFFSET,
	.bin_header_length = ZYNQ_BIN_HEADER_LENGTH,
	.dev_init = zynq_init,
	.dev_programmed_get = zynq_programmed_get,
	.dev_fpga_load = zynq_fpga_load,
};

static void copy_words_swapped(u32 *dst, const u32 *src, size_t size)
{
	int i;

	for (i = 0; i < size; i++)
		dst[i] = __swab32(src[i]);
}

static int get_byte_order(const struct xilinx_fpga_devdata *devdata,
			  const u32 *bin_header, size_t size,
			  enum xilinx_byte_order *byte_order)
{
	if (size < devdata->bin_header_length * sizeof(*bin_header))
		return -EINVAL;

	if (bin_header[devdata->sync_word_offset] == SYNC_WORD) {
		*byte_order = XILINX_BYTE_ORDER_BIT;
		return 0;
	}

	if (bin_header[devdata->sync_word_offset] == __swab32(SYNC_WORD)) {
		*byte_order = XILINX_BYTE_ORDER_BIN;
		return 0;
	}

	return -EINVAL;
}

static bool is_bin_header_valid(const struct xilinx_fpga_devdata *devdata,
				const u32 *bin_header, size_t size,
				enum xilinx_byte_order byte_order)
{
	size_t i;

	if (size < devdata->bin_header_length * sizeof(*bin_header))
		return false;

	for (i = 0; i < devdata->bin_header_length; i++, bin_header++) {
		u32 current;
		u32 expected;

		if (byte_order == XILINX_BYTE_ORDER_BIT)
			current = *bin_header;
		else
			current = __swab32(*bin_header);

		if (i == devdata->bus_width_auto_detect1_offset)
			expected = BUS_WIDTH_AUTO_DETECT1;
		else if (i == devdata->bus_width_auto_detect2_offset)
			expected = BUS_WIDTH_AUTO_DETECT2;
		else if (i == devdata->sync_word_offset)
			expected = SYNC_WORD;
		else
			expected = DUMMY_WORD;

		if (current != expected)
			return false;
	}

	return true;
}

static int get_header_length(const char *header, size_t size)
{
	u32 *buf_u32;
	int p;

	for (p = 0; p < size; p++) {
		buf_u32 = (u32 *)&header[p];
		if (*buf_u32 == DUMMY_WORD)
			return p;
	}
	return -EINVAL;
}

static void xilinx_fpga_show_header(const struct device *dev,
				    struct bs_header *header, size_t size)
{
	struct bs_header_entry *entry;
	unsigned int i;
	unsigned int length;

	for (i = 0; i < size - sizeof(*header); i += sizeof(*entry) + length) {
		entry = (struct bs_header_entry *)&header->entries[i];
		length = __be16_to_cpu(entry->length);

		switch (entry->type) {
		case 'a':
			printf("Design: %s\n", entry->data);
			break;
		case 'b':
			printf("Part number: %s\n", entry->data);
			break;
		case 'c':
			printf("Date: %s\n", entry->data);
			break;
		case 'd':
			printf("Time: %s\n", entry->data);
			break;
		case 'e':
			/* Size entry does not have a length but is be32 int */
			printf("Size: %d bytes\n",
			      (length << 16) + (entry->data[0] << 8)
			      + entry->data[1]);
			return;
		default:
			dev_warn(dev, "Invalid header entry: %c", entry->type);
			return;
		}
	}
}

static int fpgamgr_program_finish(struct firmware_handler *fh)
{
	struct fpgamgr *mgr = container_of(fh, struct fpgamgr, fh);
	struct device *hw_dev = mgr->dev.parent;
	u32 *buf_aligned;
	u32 buf_size;
	u32 *body;
	size_t body_length;
	int header_length = 0;
	enum xilinx_byte_order byte_order;
	dma_addr_t addr;
	int status = 0;
	u8 flags = 0;

	if (!mgr->buf) {
		status = -ENOBUFS;
		dev_err(&mgr->dev, "buffer is NULL\n");
		goto err_free;
	}

	header_length = get_header_length(mgr->buf, mgr->size);
	if (header_length < 0) {
		status = header_length;
		goto err_free;
	}
	xilinx_fpga_show_header(&mgr->dev,
			      (struct bs_header *)mgr->buf, header_length);

	body = (u32 *)&mgr->buf[header_length];
	body_length = mgr->size - header_length;

	status = get_byte_order(mgr->devdata, body, body_length, &byte_order);
	if (status < 0)
		goto err_free;

	if (!is_bin_header_valid(mgr->devdata, body, body_length, byte_order)) {
		dev_err(&mgr->dev, "Invalid bitstream header\n");
		status =  -EINVAL;
		goto err_free;
	}

	/*
	 * The PMU FW variants without the ZYNQMP_PM_FEATURE_SIZE_NOT_NEEDED
	 * feature expect a pointer to the bitstream size, which is stored in
	 * memory. Allocate extra space at the end of the buffer for the
	 * bitstream size.
	 */
	buf_aligned = dma_alloc(body_length + sizeof(buf_size));
	if (!buf_aligned) {
		status = -ENOBUFS;
		goto err_free;
	}

	if (!(mgr->features & ZYNQMP_PM_FEATURE_BYTE_ORDER_IRREL) &&
	    byte_order == XILINX_BYTE_ORDER_BIN)
		copy_words_swapped((u32 *)buf_aligned, body,
				   body_length / sizeof(u32));
	else
		memcpy((u32 *)buf_aligned, body, body_length);
	buf_aligned[body_length / sizeof(*buf_aligned)] = body_length;

	addr = dma_map_single(hw_dev, buf_aligned,
			      body_length + sizeof(buf_size), DMA_TO_DEVICE);
	if (dma_mapping_error(hw_dev, addr)) {
		status = -EFAULT;
		goto err_free_dma;
	}

	if (mgr->features & ZYNQMP_PM_FEATURE_SIZE_NOT_NEEDED)
		buf_size = body_length;
	else
		buf_size = addr + body_length;

	status = mgr->devdata->dev_fpga_load(mgr, addr, buf_size, flags);
	dma_unmap_single(hw_dev, addr, body_length + sizeof(buf_size),
			 DMA_TO_DEVICE);
	if (status < 0)
		dev_err(&mgr->dev, "unable to load fpga\n");

err_free_dma:
	dma_free(buf_aligned);

 err_free:
	free(mgr->buf);

	return status;
}

static int fpgamgr_program_write_buf(struct firmware_handler *fh,
		const void *buf, size_t size)
{
	struct fpgamgr *mgr = container_of(fh, struct fpgamgr, fh);

	/* Since write() is called by copy_file, we only receive chuncks with
	 * size RW_BUF_SIZE of the bitstream.
	 * Buffer the chunks here and handle it in close()
	 */

	mgr->buf = realloc(mgr->buf, mgr->size + size);
	if (!mgr->buf)
		return -ENOBUFS;

	memcpy(&(mgr->buf[mgr->size]), buf, size);
	mgr->size += size;

	return 0;
}

static int fpgamgr_program_start(struct firmware_handler *fh)
{
	struct fpgamgr *mgr = container_of(fh, struct fpgamgr, fh);

	mgr->size = 0;
	mgr->buf = NULL;

	return 0;
}

static int programmed_get(struct param_d *p, void *priv)
{
	struct fpgamgr *mgr = priv;

	return mgr->devdata->dev_programmed_get(mgr);
}

static int xilinx_fpga_probe(struct device *dev)
{
	struct fpgamgr *mgr;
	struct firmware_handler *fh;
	const char *alias = of_alias_get(dev->of_node);
	const char *model = NULL;
	struct param_d *p;
	int ret;

	mgr = xzalloc(sizeof(*mgr));

	fh = &mgr->fh;

	if (alias)
		fh->id = xstrdup(alias);
	else
		fh->id = xstrdup("xilinx-fpga-manager");

	fh->open = fpgamgr_program_start;
	fh->write = fpgamgr_program_write_buf;
	fh->close = fpgamgr_program_finish;
	of_property_read_string(dev->of_node, "compatible", &model);
	if (model)
		fh->model = xstrdup(model);
	fh->dev = dev;

	mgr->devdata = device_get_match_data(dev);
	if (!mgr->devdata) {
		dev_err(dev, "could not get device data\n");
		ret = -EINVAL;
		goto out;
	}

	ret = mgr->devdata->dev_init(mgr, dev);
	if (ret) {
		dev_err(dev, "could not init device\n");
		goto out;
	}

	dev_dbg(dev, "Registering Xilinx FPGA programmer\n");
	mgr->dev.id = DEVICE_ID_SINGLE;
	dev_set_name(&mgr->dev, "xilinx_fpga");
	mgr->dev.parent = dev;
	ret = register_device(&mgr->dev);
	if (ret)
		goto out;

	p = dev_add_param_bool(&mgr->dev, "programmed", NULL, programmed_get,
			&mgr->programmed, mgr);
	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto out_unreg;
	}

	fh->dev = &mgr->dev;
	fh->device_node = dev->of_node;

	ret = firmwaremgr_register(fh);
	if (ret != 0) {
		free(mgr);
		goto out_unreg;
	}

	return 0;
out_unreg:
	unregister_device(&mgr->dev);
out:
	free(fh->id);
	if (mgr->private)
		free(mgr->private);
	free(mgr);

	return ret;
}

static struct of_device_id xilinx_fpga_id_table[] = {
	{
		.compatible = "xlnx,zynqmp-pcap-fpga",
		.data = &zynqmp_devdata,
	},
	{
		.compatible = "xlnx,zynq-devcfg-1.0",
		.data = &zynq_devdata,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, xilinx_fpga_id_table);

static struct driver xilinx_fpga_driver = {
	.name = "xilinx_fpga_manager",
	.of_compatible = DRV_OF_COMPAT(xilinx_fpga_id_table),
	.probe = xilinx_fpga_probe,
};
device_platform_driver(xilinx_fpga_driver);
