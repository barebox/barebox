// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Message SMC/HVC
 * Transport driver
 *
 * Copyright 2020 NXP
 */

#include <common.h>
#include <linux/arm-smccc.h>
#include <linux/mutex.h>
#include <linux/processor.h>
#include <linux/sizes.h>
#include <linux/device.h>
#include <linux/err.h>
#include <of.h>
#include <of_address.h>

#include "common.h"

/*
 * The shmem address is split into 4K page and offset.
 * This is to make sure the parameters fit in 32bit arguments of the
 * smc/hvc call to keep it uniform across smc32/smc64 conventions.
 * This however limits the shmem address to 44 bit.
 *
 * These optional parameters can be used to distinguish among multiple
 * scmi instances that are using the same smc-id.
 * The page parameter is passed in r1/x1/w1 register and the offset parameter
 * is passed in r2/x2/w2 register.
 */

#define SHMEM_SIZE (SZ_4K)
#define SHMEM_SHIFT 12
#define SHMEM_PAGE(x) (_UL((x) >> SHMEM_SHIFT))
#define SHMEM_OFFSET(x) ((x) & (SHMEM_SIZE - 1))

/**
 * struct scmi_smc - Structure representing a SCMI smc transport
 *
 * @cinfo: SCMI channel info
 * @shmem: Transmit/Receive shared memory area
 * @func_id: smc/hvc call function id
 * @param_page: 4K page number of the shmem channel
 * @param_offset: Offset within the 4K page of the shmem channel
 */

struct scmi_smc {
	struct scmi_chan_info *cinfo;
	struct scmi_shared_mem __iomem *shmem;
	/* Protect access to shmem area */
	struct mutex shmem_lock;
	u32 func_id;
	u32 param_page;
	u32 param_offset;
};

static bool smc_chan_available(struct device_node *of_node, int idx)
{
	struct device_node *np = of_parse_phandle(of_node, "shmem", 0);
	if (!np)
		return false;

	of_node_put(np);
	return true;
}

static int smc_chan_setup(struct scmi_chan_info *cinfo, struct device *dev,
			  bool tx)
{
	struct device *cdev = cinfo->dev;
	struct scmi_smc *scmi_info;
	resource_size_t size;
	struct resource res;
	struct device_node *np;
	u32 func_id;
	int ret;

	if (!tx)
		return -ENODEV;

	scmi_info = devm_kzalloc(dev, sizeof(*scmi_info), GFP_KERNEL);
	if (!scmi_info)
		return -ENOMEM;

	np = of_parse_phandle(cdev->of_node, "shmem", 0);
	if (!of_device_is_compatible(np, "arm,scmi-shmem")) {
		of_node_put(np);
		return -ENXIO;
	}

	ret = of_address_to_resource(np, 0, &res);
	of_node_put(np);
	if (ret) {
		dev_err(cdev, "failed to get SCMI Tx shared memory\n");
		return ret;
	}

	size = resource_size(&res);
	scmi_info->shmem = devm_ioremap(dev, res.start, size);
	if (!scmi_info->shmem) {
		dev_err(dev, "failed to ioremap SCMI Tx shared memory\n");
		return -EADDRNOTAVAIL;
	}

	ret = of_property_read_u32(dev->of_node, "arm,smc-id", &func_id);
	if (ret < 0)
		return ret;

	if (of_device_is_compatible(dev->of_node, "arm,scmi-smc-param")) {
		scmi_info->param_page = SHMEM_PAGE(res.start);
		scmi_info->param_offset = SHMEM_OFFSET(res.start);
	}

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

	return 0;
}

static int smc_send_message(struct scmi_chan_info *cinfo,
			    struct scmi_xfer *xfer)
{
	struct scmi_smc *scmi_info = cinfo->transport_info;
	struct arm_smccc_res res;
	unsigned long page = scmi_info->param_page;
	unsigned long offset = scmi_info->param_offset;

	shmem_tx_prepare(scmi_info->shmem, xfer, cinfo);

	arm_smccc_1_1_invoke(scmi_info->func_id, page, offset, 0, 0, 0, 0, 0,
			     &res);

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

static const struct scmi_transport_ops scmi_smc_ops = {
	.chan_available = smc_chan_available,
	.chan_setup = smc_chan_setup,
	.chan_free = smc_chan_free,
	.send_message = smc_send_message,
	.fetch_response = smc_fetch_response,
};

const struct scmi_desc scmi_smc_desc = {
	.ops = &scmi_smc_ops,
	.max_rx_timeout_ms = 30,
	.max_msg = 20,
	.max_msg_size = 128,
	/*
	 * Setting .sync_cmds_atomic_replies to true for SMC assumes that,
	 * once the SMC instruction has completed successfully, the issued
	 * SCMI command would have been already fully processed by the SCMI
	 * platform firmware and so any possible response value expected
	 * for the issued command will be immmediately ready to be fetched
	 * from the shared memory area.
	 */
	.sync_cmds_completed_on_ret = true,
};
