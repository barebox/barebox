// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include "optee_private.h"

#define MAX_ARG_PARAM_COUNT	6

static struct optee_session *find_session(struct optee_context_data *ctxdata,
					  u32 session_id)
{
	struct optee_session *sess;

	list_for_each_entry(sess, &ctxdata->sess_list, list_node)
		if (sess->session_id == session_id)
			return sess;

	return NULL;
}

size_t optee_msg_arg_size(void)
{
	return OPTEE_MSG_GET_ARG_SIZE(MAX_ARG_PARAM_COUNT);
}

/**
 * optee_get_msg_arg() - Provide shared memory for argument struct
 * @ctx:	Caller TEE context
 * @num_params:	Number of parameter to store
 * @shm_ret:	Shared memory buffer
 *
 * @returns a pointer to the argument struct in memory, else an ERR_PTR
 */
struct optee_msg_arg *optee_get_msg_arg(struct tee_context *ctx,
					size_t num_params,
					struct tee_shm **shm_ret)
{

	size_t sz = OPTEE_MSG_GET_ARG_SIZE(num_params);
	struct optee_msg_arg *ma;
	struct tee_shm *shm;

	if (num_params > MAX_ARG_PARAM_COUNT)
		return ERR_PTR(-EINVAL);

	shm = tee_shm_alloc_priv_buf(ctx, sz);
	if (IS_ERR(shm))
		return ERR_CAST(shm);

	ma = tee_shm_get_va(shm, 0);
	if (IS_ERR(ma)) {
		tee_shm_free(shm);
		return ERR_CAST(ma);
	}

	memset(ma, 0, OPTEE_MSG_GET_ARG_SIZE(num_params));
	ma->num_params = num_params;

	*shm_ret = shm;
	return ma;
}

/**
 * optee_free_msg_arg() - Free previsouly obtained shared memory
 * @ctx:	Caller TEE context
 * @shm:	Pointer returned when the shared memory was obtained
 *
 * This function frees the shared memory obtained with optee_get_msg_arg().
 */
void optee_free_msg_arg(struct tee_context *ctx,
			struct tee_shm *shm)
{
	tee_shm_free(shm);
}

int optee_open_session(struct tee_context *ctx,
		       struct tee_ioctl_open_session_arg *arg,
		       struct tee_param *param)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_context_data *ctxdata = ctx->data;
	struct tee_shm *shm;
	struct optee_msg_arg *msg_arg;
	struct optee_session *sess = NULL;
	uuid_t client_uuid;
	int rc;

	/* +2 for the meta parameters added below */
	msg_arg = optee_get_msg_arg(ctx, arg->num_params + 2, &shm);
	if (IS_ERR(msg_arg))
		return PTR_ERR(msg_arg);

	msg_arg->cmd = OPTEE_MSG_CMD_OPEN_SESSION;

	/*
	 * Initialize and add the meta parameters needed when opening a
	 * session.
	 */
	msg_arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT |
				  OPTEE_MSG_ATTR_META;
	msg_arg->params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT |
				  OPTEE_MSG_ATTR_META;
	memcpy(&msg_arg->params[0].u.value, arg->uuid, sizeof(arg->uuid));
	msg_arg->params[1].u.value.c = arg->clnt_login;

	rc = tee_session_calc_client_uuid(&client_uuid, arg->clnt_login,
					  arg->clnt_uuid);
	if (rc)
		goto out;
	export_uuid(msg_arg->params[1].u.octets, &client_uuid);

	rc = optee->ops->to_msg_param(optee, msg_arg->params + 2,
				      arg->num_params, param);
	if (rc)
		goto out;

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess) {
		rc = -ENOMEM;
		goto out;
	}

	if (optee->ops->do_call_with_arg(ctx, msg_arg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	if (msg_arg->ret == TEEC_SUCCESS) {
		/* A new session has been created, add it to the list. */
		sess->session_id = msg_arg->session;
		list_add(&sess->list_node, &ctxdata->sess_list);
	} else {
		kfree(sess);
	}

	if (optee->ops->from_msg_param(optee, param, arg->num_params,
				       msg_arg->params + 2)) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		/* Close session again to avoid leakage */
		optee_close_session(ctx, msg_arg->session);
	} else {
		arg->session = msg_arg->session;
		arg->ret = msg_arg->ret;
		arg->ret_origin = msg_arg->ret_origin;
	}

out:
	optee_free_msg_arg(ctx, shm);

	return rc;
}

int optee_close_session_helper(struct tee_context *ctx, u32 session)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_msg_arg *msg_arg;
	struct tee_shm *shm;

	msg_arg = optee_get_msg_arg(ctx, 0, &shm);
	if (IS_ERR(msg_arg))
		return PTR_ERR(msg_arg);

	msg_arg->cmd = OPTEE_MSG_CMD_CLOSE_SESSION;
	msg_arg->session = session;
	optee->ops->do_call_with_arg(ctx, msg_arg);

	optee_free_msg_arg(ctx, shm);

	return 0;
}

int optee_close_session(struct tee_context *ctx, u32 session)
{
	struct optee_context_data *ctxdata = ctx->data;
	struct optee_session *sess;

	/* Check that the session is valid and remove it from the list */
	sess = find_session(ctxdata, session);
	if (sess)
		list_del(&sess->list_node);
	if (!sess)
		return -EINVAL;
	kfree(sess);

	return optee_close_session_helper(ctx, session);
}

int optee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg,
		      struct tee_param *param)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_context_data *ctxdata = ctx->data;
	struct optee_msg_arg *msg_arg;
	struct optee_session *sess;
	struct tee_shm *shm;
	int rc;

	/* Check that the session is valid */
	sess = find_session(ctxdata, arg->session);
	if (!sess)
		return -EINVAL;

	msg_arg = optee_get_msg_arg(ctx, arg->num_params,
				    &shm);
	if (IS_ERR(msg_arg))
		return PTR_ERR(msg_arg);
	msg_arg->cmd = OPTEE_MSG_CMD_INVOKE_COMMAND;
	msg_arg->func = arg->func;
	msg_arg->session = arg->session;

	rc = optee->ops->to_msg_param(optee, msg_arg->params, arg->num_params,
				      param);
	if (rc)
		goto out;

	if (optee->ops->do_call_with_arg(ctx, msg_arg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	if (optee->ops->from_msg_param(optee, param, arg->num_params,
				       msg_arg->params)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	arg->ret = msg_arg->ret;
	arg->ret_origin = msg_arg->ret_origin;
out:
	optee_free_msg_arg(ctx, shm);
	return rc;
}
