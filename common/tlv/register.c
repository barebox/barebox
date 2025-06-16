// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <driver.h>
#include <linux/nvmem-consumer.h>
#include <of.h>
#include <tlv/tlv.h>
#include <libfile.h>
#include <fcntl.h>

#include <linux/err.h>

static int tlv_probe_from_magic(struct device *dev)
{
	struct tlv_device *tlvdev = to_tlv_device(dev);
	int ret;

	ret = tlv_parse(tlvdev, to_tlv_decoder(dev->driver));
	if (ret)
		return ret;

	return tlv_of_register_fixup(tlvdev);
}

static int tlv_probe_from_compatible(struct device *dev)
{
	struct tlv_decoder *decoder = to_tlv_decoder(dev->driver);
	struct tlv_header *header;
	struct tlv_device *tlvdev;
	struct cdev *cdev;
	char *backend_path;
	size_t size;
	u32 magic;
	int ret;

	cdev = cdev_by_device_node(dev->of_node);
	if (!cdev)
		return -EINVAL;

	backend_path = basprintf("/dev/%s", cdev_name(cdev));

	header = tlv_read(backend_path, &size);
	free(backend_path);

	if (IS_ERR(header))
		return PTR_ERR(header);

	magic = be32_to_cpu(header->magic);
	if (magic != decoder->magic) {
		dev_err(dev, "got magic %08x, but %08x expected\n",
			magic, decoder->magic);
		ret = -EILSEQ;
		goto err;
	}

	tlvdev = tlv_register_device(header, dev);
	if (IS_ERR(tlvdev)) {
		ret = PTR_ERR(tlvdev);
		goto err;
	}

	return 0;
err:
	free(header);
	return ret;
}

int tlv_register_decoder(struct tlv_decoder *decoder)
{
	int ret;

	if (decoder->driver.bus)
		return -EBUSY;

	if (!decoder->driver.probe)
		decoder->driver.probe = tlv_probe_from_magic;
	decoder->driver.bus = &tlv_bus;

	ret = register_driver(&decoder->driver);
	if (ret)
		return ret;

	if (!decoder->driver.of_compatible)
		return 0;

	decoder->_platform_driver.name = basprintf("%s-pltfm", decoder->driver.name);
	decoder->_platform_driver.of_compatible = decoder->driver.of_compatible;
	decoder->_platform_driver.probe = tlv_probe_from_compatible;

	return platform_driver_register(&decoder->_platform_driver);
}
