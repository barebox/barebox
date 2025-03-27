// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */

#define pr_fmt(fmt) "optee: " fmt

#include <linux/tee_drv.h>
#include "optee_private.h"
#include "optee_rpc_cmd.h"

static void cmd_shm_alloc(struct tee_context *ctx, struct optee_msg_arg *arg)
{
	struct tee_shm *shm;

	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (arg->num_params != 1 ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	shm = tee_shm_alloc_kernel_buf(ctx, arg->params[0].u.value.b);
	if (IS_ERR(shm)) {
		if (PTR_ERR(shm) == -ENOMEM)
			arg->ret = TEEC_ERROR_OUT_OF_MEMORY;
		else
			arg->ret = TEEC_ERROR_GENERIC;
		return;
	}

	arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT |
			      OPTEE_MSG_ATTR_NONCONTIG;
	arg->params[0].u.tmem.buf_ptr = virt_to_phys(shm->pages_list);
	arg->params[0].u.tmem.size = shm->size;
	arg->params[0].u.tmem.shm_ref = (ulong)shm;
	arg->ret = TEEC_SUCCESS;
}

static void cmd_shm_free(struct optee_msg_arg *arg)
{
	arg->ret_origin = TEEC_ORIGIN_COMMS;

	if (arg->num_params != 1 ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEEC_ERROR_BAD_PARAMETERS;
		return;
	}

	tee_shm_free((struct tee_shm *)(ulong)arg->params[0].u.value.b);
	arg->ret = TEEC_SUCCESS;
}

void optee_rpc_cmd(struct tee_context *ctx, struct optee *optee,
		   struct optee_msg_arg *arg)
{
	pr_debug("%s: receive RPC CMD: %d\n", __func__, arg->cmd);

	switch (arg->cmd) {
	case OPTEE_RPC_CMD_SHM_ALLOC:
		cmd_shm_alloc(ctx, arg);
		break;
	case OPTEE_RPC_CMD_SHM_FREE:
		cmd_shm_free(arg);
		break;
	case OPTEE_RPC_CMD_RPMB:
		optee_suppl_cmd_rpmb(ctx, arg);
		break;
	default:
		pr_notice_once("optee: No supplicant or RPC handler for command 0x%x\n", arg->cmd);
		arg->ret = TEEC_ERROR_NOT_IMPLEMENTED;
	}

	arg->ret_origin = TEEC_ORIGIN_COMMS;
}
