// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */

#include <linux/tee_drv.h>
#include <linux/uuid.h>
#include <linux/array_size.h>
#include <uapi/linux/tee.h>

#include <rk_secure_boot.h>

#include "optee_private.h"
#include "pta_rk_secure_boot.h"

const uuid_t ta_uuid = PTA_RK_SECURE_BOOT_UUID;

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return ver->impl_id == TEE_IMPL_ID_OPTEE ? 1 : 0;
}

static int rk_secure_open_session(struct tee_context *ctx, __u32 *session)
{
	struct tee_ioctl_open_session_arg sess_arg = {};
	int res = 0;

	export_uuid(sess_arg.uuid, &ta_uuid);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	res = tee_client_open_session(ctx, &sess_arg, NULL);
	if (res < 0)
		return res;
	switch (sess_arg.ret) {
	case TEEC_SUCCESS:
		res = 0;
		break;
	case TEEC_ERROR_ITEM_NOT_FOUND:
		pr_err("cannot find TA %pU\n", &sess_arg.uuid);
		return -ENOENT;
	default:
		pr_err("TA %pU: error 0x%x\n", &sess_arg.uuid, sess_arg.ret);
		return -EINVAL;
	}

	*session = sess_arg.session;
	return res;
}

int rk_secure_boot_get_info(struct rk_secure_boot_info *out)
{
	struct tee_context *ctx = NULL;
	struct tee_ioctl_invoke_arg inv_arg;
	struct tee_param param[1];
	struct tee_shm *info_shm;
	struct pta_rk_secure_boot_info *info;
	__u32 session = 0;
	int res = 0;

	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx))
		return -ENODEV;

	res = rk_secure_open_session(ctx, &session);
	if (res < 0)
		goto out_ctx;

	info_shm = tee_shm_alloc_kernel_buf(ctx, sizeof(*info));
	if (IS_ERR(info_shm)) {
		res = -ENOMEM;
		goto close_session;
	}
	info = info_shm->kaddr;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[0].u.memref.shm = info_shm;
	param[0].u.memref.size = sizeof(*info);

	inv_arg.func = PTA_RK_SECURE_BOOT_GET_INFO;
	inv_arg.session = session;
	inv_arg.num_params = ARRAY_SIZE(param);

	res = tee_client_invoke_func(ctx, &inv_arg, param);
	if (res)
		goto out;
	if (inv_arg.ret) {
		pr_err("invoke func failed with 0x%08x\n", inv_arg.ret);
		res = -EIO;
	}

	memcpy(out->hash, info->hash.value, sizeof(out->hash));
	out->lockdown = info->enabled;
	out->simulation = info->simulation;

out:
	tee_shm_free(info_shm);
close_session:
	tee_client_close_session(ctx, session);
out_ctx:
	tee_client_close_context(ctx);

	return res;
}

int rk_secure_boot_burn_hash(const u8 *hash, u32 key_size_bits)
{
	struct tee_context *ctx = NULL;
	struct tee_ioctl_invoke_arg inv_arg;
	struct tee_param param[2];
	struct tee_shm *shm_hash;
	struct pta_rk_secure_boot_hash *pta_hash;
	__u32 session = 0;
	int res = 0;

	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx))
		return -ENODEV;

	res = rk_secure_open_session(ctx, &session);
	if (res < 0)
		goto out_ctx;

	shm_hash = tee_shm_alloc_kernel_buf(ctx, sizeof(*pta_hash));
	if (IS_ERR(shm_hash)) {
		res = -ENOMEM;
		goto close_session;
	}
	pta_hash = shm_hash->kaddr;

	memcpy(pta_hash->value, hash, sizeof(pta_hash->value));

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = shm_hash;
	param[0].u.memref.size = sizeof(*pta_hash);

	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = key_size_bits;

	inv_arg.func = PTA_RK_SECURE_BOOT_BURN_HASH;
	inv_arg.session = session;
	inv_arg.num_params = ARRAY_SIZE(param);

	res = tee_client_invoke_func(ctx, &inv_arg, param);
	if (res)
		goto out;
	if (inv_arg.ret) {
		pr_err("invoke func failed with 0x%08x\n", inv_arg.ret);
		res = -EIO;
	}

out:
	tee_shm_free(shm_hash);
close_session:
	tee_client_close_session(ctx, session);
out_ctx:
	tee_client_close_context(ctx);

	return res;
}

int rk_secure_boot_lockdown_device(void)
{
	struct tee_context *ctx = NULL;
	struct tee_ioctl_invoke_arg inv_arg;
	__u32 session = 0;
	int res = 0;

	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx))
		return -ENODEV;

	res = rk_secure_open_session(ctx, &session);
	if (res < 0)
		goto out_ctx;

	inv_arg.func = PTA_RK_SECURE_BOOT_LOCKDOWN_DEVICE;
	inv_arg.session = session;
	inv_arg.num_params = 0;

	res = tee_client_invoke_func(ctx, &inv_arg, NULL);
	if (res)
		goto close_session;
	if (inv_arg.ret) {
		pr_err("invoke func failed with 0x%08x\n", inv_arg.ret);
		res = -EIO;
	}

close_session:
	tee_client_close_session(ctx, session);
out_ctx:
	tee_client_close_context(ctx);

	return res;
}
