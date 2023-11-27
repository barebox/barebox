// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */

#define pr_fmt(fmt) "optee: " fmt

#include <linux/tee_drv.h>
#include "optee_private.h"

void optee_rpc_cmd(struct tee_context *ctx, struct optee *optee,
		   struct optee_msg_arg *arg)
{
	pr_notice_once("optee: No supplicant or RPC handler for command 0x%x\n", arg->cmd);
	arg->ret = TEEC_ERROR_NOT_SUPPORTED;
}
