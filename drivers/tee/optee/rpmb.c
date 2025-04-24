// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2018 Linaro Limited
 */

#define pr_fmt(fmt)     "optee-rpmb: " fmt

#include <linux/array_size.h>
#include <mci.h>

#include "optee_msg.h"
#include "optee_private.h"

/*
 * Request and response definitions must be in sync with the secure side of
 * OP-TEE.
 */

/* Request */
struct rpmb_req {
	u16 cmd;
#define RPMB_CMD_DATA_REQ      0x00
#define RPMB_CMD_GET_DEV_INFO  0x01
	u16 dev_id;
	u16 block_count;
	/* Optional data frames (rpmb_data_frame) follow */
};

#define RPMB_REQ_DATA(req) ((void *)((struct rpmb_req *)(req) + 1))

/* Response to device info request */
struct rpmb_dev_info {
	u8 cid[16];
	u8 rpmb_size_mult;	/* EXT CSD-slice 168: RPMB Size */
	u8 rel_wr_sec_c;	/* EXT CSD-slice 222: Reliable Write Sector */
				/*                    Count */
	u8 ret_code;
#define RPMB_CMD_GET_DEV_INFO_RET_OK     0x00
#define RPMB_CMD_GET_DEV_INFO_RET_ERROR  0x01
};

static u32 rpmb_get_dev_info(struct mci *mci, struct rpmb_dev_info *info)
{
	int i;

	if (!mci->ext_csd)
		return TEEC_ERROR_GENERIC;

	for (i = 0; i < ARRAY_SIZE(mci->raw_cid); i++)
		((u32 *) info->cid)[i] = cpu_to_be32(mci->raw_cid[i]);

	info->rel_wr_sec_c = mci->ext_csd[222];
	info->rpmb_size_mult = mci->ext_csd[168];
	info->ret_code = RPMB_CMD_GET_DEV_INFO_RET_OK;

	return TEEC_SUCCESS;
}

static u32 rpmb_process_request(struct tee_context *ctx, void *req,
				ulong req_size, void *rsp, ulong rsp_size)
{
	struct rpmb_req *sreq = req;
	struct mci *mci;

	if (req_size < sizeof(*sreq))
		return TEEC_ERROR_BAD_PARAMETERS;

	mci = mci_get_rpmb_dev(sreq->dev_id);
	if (!mci) {
		pr_err_once("Cannot find RPMB MMC with id %u\n", sreq->dev_id);
		return TEEC_ERROR_ITEM_NOT_FOUND;
	}

	switch (sreq->cmd) {
	case RPMB_CMD_DATA_REQ:
		if (mci_rpmb_route_frames(mci, RPMB_REQ_DATA(req),
					  req_size - sizeof(struct rpmb_req),
					  rsp, rsp_size))
			return TEEC_ERROR_BAD_PARAMETERS;
		return TEEC_SUCCESS;

	case RPMB_CMD_GET_DEV_INFO:
		if (rsp_size != sizeof(struct rpmb_dev_info)) {
			pr_debug("Invalid req/rsp size\n");
			return TEEC_ERROR_BAD_PARAMETERS;
		}

		return rpmb_get_dev_info(mci, rsp);

	default:
		pr_debug("Unsupported RPMB command: %d\n", sreq->cmd);
		return TEEC_ERROR_BAD_PARAMETERS;
	}
}

void optee_suppl_cmd_rpmb(struct tee_context *ctx, struct optee_msg_arg *arg)
{
	struct tee_shm *req_shm;
	struct tee_shm *rsp_shm;
	void *req_buf;
	void *rsp_buf;
	ulong req_size;
	ulong rsp_size;

	if (arg->num_params != 2 ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_RMEM_INPUT ||
	    arg->params[1].attr != OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	req_shm = (struct tee_shm *)(ulong)arg->params[0].u.rmem.shm_ref;
	req_buf = (u8 *)req_shm->kaddr + arg->params[0].u.rmem.offs;
	req_size = arg->params[0].u.rmem.size;

	rsp_shm = (struct tee_shm *)(ulong)arg->params[1].u.rmem.shm_ref;
	rsp_buf = (u8 *)rsp_shm->kaddr + arg->params[1].u.rmem.offs;
	rsp_size = arg->params[1].u.rmem.size;

	arg->ret = rpmb_process_request(ctx, req_buf, req_size,
					rsp_buf, rsp_size);
}
