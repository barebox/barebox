// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Message SMC/HVC
 * Transport driver
 *
 * Copyright 2020 NXP
 */

#include <common.h>
#include <linux/arm-smccc.h>
#include <driver.h>
#include <linux/err.h>
#include <of.h>
#include <of_address.h>

#include "common.h"

/**
 * struct scmi_smc - Structure representing a SCMI smc transport
 *
 * @cinfo: SCMI channel info
 * @shmem: Transmit/Receive shared memory area
 * @func_id: smc/hvc call function id
 */

struct scmi_smc {
	struct scmi_chan_info *cinfo;
	struct scmi_shared_mem __iomem *shmem;
	u32 func_id;
};

static bool smc_chan_available(struct device_d *dev, int idx)
{
	return of_parse_phandle(dev->device_node, "shmem", 0) != NULL;
}

static int smc_chan_setup(struct scmi_chan_info *cinfo, struct device_d *dev,
			  bool tx)
{
	struct device_d *cdev = cinfo->dev;
	struct scmi_smc *scmi_info;
	resource_size_t size;
	struct resource res;
	struct device_node *np;
	u32 func_id;
	int ret;

	if (!tx)
		return -ENODEV;

	scmi_info = kzalloc(sizeof(*scmi_info), GFP_KERNEL);
	if (!scmi_info)
		return -ENOMEM;

	np = of_parse_phandle(cdev->device_node, "shmem", 0);
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(cdev, "failed to get SCMI Tx shared memory\n");
		return ret;
	}

	size = resource_size(&res);
	scmi_info->shmem = IOMEM(res.start);

	ret = of_property_read_u32(dev->device_node, "arm,smc-id", &func_id);
	if (ret < 0)
		return ret;

	scmi_info->func_id = func_id;
	scmi_info->cinfo = cinfo;
	cinfo->transport_info = scmi_info;

	return 0;
}

static int smc_chan_free(int id, void *p, void *data)
{
	struct scmi_chan_info *cinfo = p;
	struct scmi_smc *scmi_info = cinfo->transport_info;

	cinfo->transport_info = NULL;
	scmi_info->cinfo = NULL;

	scmi_free_channel(cinfo, data, id);

	return 0;
}

static int smc_send_message(struct scmi_chan_info *cinfo,
			    struct scmi_xfer *xfer)
{
	struct scmi_smc *scmi_info = cinfo->transport_info;
	struct arm_smccc_res res;

	shmem_tx_prepare(scmi_info->shmem, xfer);

	arm_smccc_1_1_invoke(scmi_info->func_id, 0, 0, 0, 0, 0, 0, 0, &res);

	scmi_rx_callback(scmi_info->cinfo, shmem_read_header(scmi_info->shmem));

	/* Only SMCCC_RET_NOT_SUPPORTED is valid error code */
	if (res.a0)
		return -EOPNOTSUPP;
	return 0;
}

static void smc_fetch_response(struct scmi_chan_info *cinfo,
			       struct scmi_xfer *xfer)
{
	struct scmi_smc *scmi_info = cinfo->transport_info;

	shmem_fetch_response(scmi_info->shmem, xfer);
}

static bool
smc_poll_done(struct scmi_chan_info *cinfo, struct scmi_xfer *xfer)
{
	struct scmi_smc *scmi_info = cinfo->transport_info;

	return shmem_poll_done(scmi_info->shmem, xfer);
}

static const struct scmi_transport_ops scmi_smc_ops = {
	.chan_available = smc_chan_available,
	.chan_setup = smc_chan_setup,
	.chan_free = smc_chan_free,
	.send_message = smc_send_message,
	.fetch_response = smc_fetch_response,
	.poll_done = smc_poll_done,
};

const struct scmi_desc scmi_smc_desc = {
	.ops = &scmi_smc_ops,
	.max_rx_timeout_ms = 30,
	.max_msg = 20,
	.max_msg_size = 128,
};
