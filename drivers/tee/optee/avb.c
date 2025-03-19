#include <linux/types.h>
#include <tee/avb.h>
#include <linux/tee_drv.h>

#include "optee_private.h"

#define TA_AVB_UUID UUID_INIT(0x023f8f1a, 0x292a, 0x432b, \
                      0x8f, 0xc4, 0xde, 0x84, 0x71, 0x35, 0x80, 0x67)
#define TEE_PARAM_ATTR_TYPE_MEMREF_INOUT       7       /* input and output */
#define TA_AVB_CMD_READ_PERSIST_VALUE     4
#define TEE_PARAM_ATTR_TYPE_MEMREF_INPUT       5
#define TA_AVB_CMD_WRITE_PERSIST_VALUE    5

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	if (ver->impl_id == TEE_IMPL_ID_OPTEE)
		return 1;
	else
		return 0;
}

int avb_read_persistent_value(const char *name, size_t buffer_size,
			      u8 *out_buffer, size_t *out_num_bytes_read)
{
	const uuid_t avb_uuid = TA_AVB_UUID;
	int rc = 0;
	struct tee_shm *shm_name;
	struct tee_shm *shm_buf;
	struct tee_param param[2];
	size_t name_size = strlen(name) + 1;
	struct tee_ioctl_open_session_arg sess_arg = {};
	struct tee_context *ctx = NULL;
	struct tee_ioctl_invoke_arg arg;

	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx))
		return -ENODEV;

	export_uuid(sess_arg.uuid, &avb_uuid);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	rc = tee_client_open_session(ctx, &sess_arg, NULL);
	if ((rc < 0) || (sess_arg.ret != TEEC_SUCCESS)) {
		pr_debug("%s device enumeration pseudo TA not found\n", __func__);
		rc = 0;
		goto out_ctx;
	}

	shm_name = tee_shm_alloc_kernel_buf(ctx, name_size);
	if (IS_ERR(shm_name)) {
		rc = -ENOMEM;
		goto close_session;
	}

	shm_buf = tee_shm_alloc_kernel_buf(ctx, buffer_size);
	if (IS_ERR(shm_buf)) {
		rc = -ENOMEM;
		goto free_name;
	}

	memcpy(shm_name->kaddr, name, name_size);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = shm_name;
	param[0].u.memref.size = name_size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = buffer_size;

	arg.func = TA_AVB_CMD_READ_PERSIST_VALUE;
	arg.session = sess_arg.session;
	arg.num_params = 2;

	rc = tee_client_invoke_func(ctx, &arg, param);
	if (rc)
		goto out;
	switch (arg.ret) {
	case TEEC_SUCCESS:
		rc = 0;
		break;
	case TEEC_ERROR_ITEM_NOT_FOUND:
		rc = -ENOENT;
		break;
	default:
		rc = -EINVAL;
		break;
	}
	if (rc)
		goto out;

	if (param[1].u.memref.size > buffer_size) {
		rc = -EINVAL;
		goto out;
	}

	*out_num_bytes_read = param[1].u.memref.size;

	memcpy(out_buffer, shm_buf->kaddr, *out_num_bytes_read);

out:
	tee_shm_free(shm_buf);
free_name:
	tee_shm_free(shm_name);
close_session:
	tee_client_close_session(ctx, sess_arg.session);
out_ctx:
	tee_client_close_context(ctx);

	return rc;
}

int avb_write_persistent_value(const char *name, size_t value_size,
			       const u8 *value)
{
	const uuid_t avb_uuid = TA_AVB_UUID;
	int rc = 0;
	struct tee_shm *shm_name;
	struct tee_shm *shm_buf;
	struct tee_param param[2];
	struct tee_ioctl_open_session_arg sess_arg = {};
	struct tee_context *ctx = NULL;
	size_t name_size = strlen(name) + 1;
	struct tee_ioctl_invoke_arg inv_arg;

	if (!value_size)
		return -EINVAL;

	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx))
		return -ENODEV;

	export_uuid(sess_arg.uuid, &avb_uuid);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	rc = tee_client_open_session(ctx, &sess_arg, NULL);
	if ((rc < 0) || (sess_arg.ret != TEEC_SUCCESS)) {
		pr_err("%s AVB TA not found\n", __func__);
		goto out_ctx;
	}

	shm_name = tee_shm_alloc_kernel_buf(ctx, name_size);
	if (IS_ERR(shm_name)) {
		rc = -ENOMEM;
		goto close_session;
	}

	shm_buf = tee_shm_alloc_kernel_buf(ctx, value_size);
	if (IS_ERR(shm_buf)) {
		rc = -ENOMEM;
		goto free_name;
	}

	memcpy(shm_name->kaddr, name, name_size);
	memcpy(shm_buf->kaddr, value, value_size);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = shm_name;
	param[0].u.memref.size = name_size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = value_size;

	inv_arg.func = TA_AVB_CMD_WRITE_PERSIST_VALUE;
	inv_arg.session = sess_arg.session;
	inv_arg.num_params = 2;

	rc = tee_client_invoke_func(ctx, &inv_arg, param);
	if (rc)
		goto out;
	if (inv_arg.ret) {
		pr_err("invoke func failed with 0x%08x\n", inv_arg.ret);
		rc = -EIO;
	}

out:
	tee_shm_free(shm_buf);
free_name:
	tee_shm_free(shm_name);
close_session:
	tee_client_close_session(ctx, sess_arg.session);
out_ctx:
	tee_client_close_context(ctx);

	return rc;
}
