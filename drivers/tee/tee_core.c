// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2016, Linaro Limited
 */

#define pr_fmt(fmt) "tee_core: " fmt

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include "tee_private.h"

#define TEE_NUM_DEVICES	32

#define TEE_IOCTL_PARAM_SIZE(x) (sizeof(struct tee_param) * (x))

static LIST_HEAD(tee_clients);

struct tee_context *teedev_open(struct tee_device *teedev)
{
	struct tee_context *ctx;
	int rc;

	if (!tee_device_get(teedev))
		return ERR_PTR(-EINVAL);

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	kref_init(&ctx->refcount);
	ctx->teedev = teedev;
	rc = teedev->desc->ops->open(ctx);
	if (rc)
		goto err;

	INIT_LIST_HEAD(&ctx->list_shm);

	pr_debug("%s ctx=%p teedev=%p\n", __func__, ctx, teedev);

	return ctx;
err:
	kfree(ctx);
	tee_device_put(teedev);
	return ERR_PTR(rc);
}
EXPORT_SYMBOL_GPL(teedev_open);

void teedev_ctx_get(struct tee_context *ctx)
{
	if (ctx->releasing)
		return;

	kref_get(&ctx->refcount);
}

static void teedev_ctx_release(struct kref *ref)
{
	struct tee_context *ctx = container_of(ref, struct tee_context,
					       refcount);
	ctx->releasing = true;
	ctx->teedev->desc->ops->release(ctx);
	kfree(ctx);
}

void teedev_ctx_put(struct tee_context *ctx)
{
	if (ctx->releasing)
		return;

	kref_put(&ctx->refcount, teedev_ctx_release);
}

void teedev_close_context(struct tee_context *ctx)
{
	struct tee_device *teedev = ctx->teedev;

	teedev_ctx_put(ctx);
	tee_device_put(teedev);
}
EXPORT_SYMBOL_GPL(teedev_close_context);

static int tee_open(struct cdev *cdev, unsigned long flags)
{
	struct tee_context *ctx;

	if (cdev->priv)
		return -EBUSY;

	ctx = teedev_open(container_of(cdev, struct tee_device, cdev));
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	cdev->priv = ctx;

	return 0;
}

static int tee_release(struct cdev *cdev)
{
	struct tee_context *ctx = cdev->priv;

	teedev_close_context(ctx);
	cdev->priv = NULL;

	return 0;
}

int tee_session_calc_client_uuid(uuid_t *uuid, u32 connection_method,
				 const u8 connection_data[TEE_IOCTL_UUID_LEN])
{
	/* Linux could generate a UUIDv5 here out of UID or GID, but in barebox,
	 * we just mimic what it would do for LOGIN_PUBLIC and LOGIN_REE_KERNEL,
	 * namely pass the nil UUID into the TEE environment
	 */

	uuid_copy(uuid, &uuid_null);
	return 0;

}
EXPORT_SYMBOL_GPL(tee_session_calc_client_uuid);

static int tee_ioctl_version(struct tee_context *ctx,
			     struct tee_ioctl_version_data __user *uvers)
{
	struct tee_ioctl_version_data vers;

	ctx->teedev->desc->ops->get_version(ctx->teedev, &vers);

	if (ctx->teedev->desc->flags & TEE_DESC_PRIVILEGED)
		vers.gen_caps |= TEE_GEN_CAP_PRIVILEGED;

	if (copy_to_user(uvers, &vers, sizeof(vers)))
		return -EFAULT;

	return 0;
}

static int tee_ioctl_shm_alloc(struct tee_context *ctx,
			       struct tee_ioctl_shm_alloc_data *data)
{
	struct tee_shm *shm;

	/* Currently no input flags are supported */
	if (data->flags)
		return -EINVAL;

	shm = tee_shm_alloc_user_buf(ctx, data->size);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	data->id = shm->dev.id;
	data->flags = shm->flags;
	data->size = shm->size;

	return tee_shm_get_fd(shm);
}

static int
tee_ioctl_shm_register(struct tee_context *ctx,
		       struct tee_ioctl_shm_register_data *data)
{
	struct tee_shm *shm;

	/* Currently no input flags are supported */
	if (data->flags)
		return -EINVAL;

	shm = tee_shm_register_user_buf(ctx, data->addr, data->length);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	data->id = shm->dev.id;
	data->flags = shm->flags;
	data->length = shm->size;

	return tee_shm_get_fd(shm);
}

static int params_from_user(struct tee_context *ctx, struct tee_param *params,
			    size_t num_params,
			    struct tee_ioctl_param __user *uparams)
{
	size_t n;

	for (n = 0; n < num_params; n++) {
		struct tee_shm *shm;
		struct tee_ioctl_param ip;

		if (copy_from_user(&ip, uparams + n, sizeof(ip)))
			return -EFAULT;

		/* All unused attribute bits has to be zero */
		if (ip.attr & ~TEE_IOCTL_PARAM_ATTR_MASK)
			return -EINVAL;

		params[n].attr = ip.attr;
		switch (ip.attr & TEE_IOCTL_PARAM_ATTR_TYPE_MASK) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_NONE:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			params[n].u.value.a = ip.a;
			params[n].u.value.b = ip.b;
			params[n].u.value.c = ip.c;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			/*
			 * If a NULL pointer is passed to a TA in the TEE,
			 * the ip.c IOCTL parameters is set to TEE_MEMREF_NULL
			 * indicating a NULL memory reference.
			 */
			if (ip.c != TEE_MEMREF_NULL) {
				/*
				 * If we fail to get a pointer to a shared
				 * memory object (and increase the ref count)
				 * from an identifier we return an error. All
				 * pointers that has been added in params have
				 * an increased ref count. It's the callers
				 * responibility to do tee_shm_put() on all
				 * resolved pointers.
				 */
				shm = tee_shm_get_from_id(ctx, ip.c);
				if (IS_ERR(shm))
					return PTR_ERR(shm);

				/*
				 * Ensure offset + size does not overflow
				 * offset and does not overflow the size of
				 * the referred shared memory object.
				 */
				if ((ip.a + ip.b) < ip.a ||
				    (ip.a + ip.b) > shm->size) {
					tee_shm_put(shm);
					return -EINVAL;
				}
			} else if (ctx->cap_memref_null) {
				/* Pass NULL pointer to OP-TEE */
				shm = NULL;
			} else {
				return -EINVAL;
			}

			params[n].u.memref.shm_offs = ip.a;
			params[n].u.memref.size = ip.b;
			params[n].u.memref.shm = shm;
			break;
		default:
			/* Unknown attribute */
			return -EINVAL;
		}
	}
	return 0;
}

static int params_to_user(struct tee_ioctl_param __user *uparams,
			  size_t num_params, struct tee_param *params)
{
	size_t n;

	for (n = 0; n < num_params; n++) {
		struct tee_ioctl_param __user *up = uparams + n;
		struct tee_param *p = params + n;

		switch (p->attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			if (put_user(p->u.value.a, &up->a) ||
			    put_user(p->u.value.b, &up->b) ||
			    put_user(p->u.value.c, &up->c))
				return -EFAULT;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			if (put_user((u64)p->u.memref.size, &up->b))
				return -EFAULT;
			break;
		default:
			break;
		}
	}
	return 0;
}

static int tee_ioctl_open_session(struct tee_context *ctx,
				  struct tee_ioctl_buf_data __user *ubuf)
{
	int rc;
	size_t n;
	struct tee_ioctl_buf_data buf;
	struct tee_ioctl_open_session_arg __user *uarg;
	struct tee_ioctl_open_session_arg arg;
	struct tee_ioctl_param __user *uparams = NULL;
	struct tee_param *params = NULL;
	bool have_session = false;

	if (!ctx->teedev->desc->ops->open_session)
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, sizeof(buf)))
		return -EFAULT;

	if (buf.buf_len > TEE_MAX_ARG_SIZE ||
	    buf.buf_len < sizeof(struct tee_ioctl_open_session_arg))
		return -EINVAL;

	uarg = u64_to_user_ptr(buf.buf_ptr);
	if (copy_from_user(&arg, uarg, sizeof(arg)))
		return -EFAULT;

	if (sizeof(arg) + TEE_IOCTL_PARAM_SIZE(arg.num_params) != buf.buf_len)
		return -EINVAL;

	if (arg.num_params) {
		params = kcalloc(arg.num_params, sizeof(struct tee_param),
				 GFP_KERNEL);
		if (!params)
			return -ENOMEM;
		uparams = uarg->params;
		rc = params_from_user(ctx, params, arg.num_params, uparams);
		if (rc)
			goto out;
	}

	if (arg.clnt_login >= TEE_IOCTL_LOGIN_REE_KERNEL_MIN &&
	    arg.clnt_login <= TEE_IOCTL_LOGIN_REE_KERNEL_MAX) {
		pr_debug("login method not allowed for user-space client\n");
		rc = -EPERM;
		goto out;
	}

	rc = ctx->teedev->desc->ops->open_session(ctx, &arg, params);
	if (rc)
		goto out;
	have_session = true;

	if (put_user(arg.session, &uarg->session) ||
	    put_user(arg.ret, &uarg->ret) ||
	    put_user(arg.ret_origin, &uarg->ret_origin)) {
		rc = -EFAULT;
		goto out;
	}
	rc = params_to_user(uparams, arg.num_params, params);
out:
	/*
	 * If we've succeeded to open the session but failed to communicate
	 * it back to user space, close the session again to avoid leakage.
	 */
	if (rc && have_session && ctx->teedev->desc->ops->close_session)
		ctx->teedev->desc->ops->close_session(ctx, arg.session);

	if (params) {
		/* Decrease ref count for all valid shared memory pointers */
		for (n = 0; n < arg.num_params; n++)
			if (tee_param_is_memref(params + n) &&
			    params[n].u.memref.shm)
				tee_shm_put(params[n].u.memref.shm);
		kfree(params);
	}

	return rc;
}

static int tee_ioctl_invoke(struct tee_context *ctx,
			    struct tee_ioctl_buf_data __user *ubuf)
{
	int rc;
	size_t n;
	struct tee_ioctl_buf_data buf;
	struct tee_ioctl_invoke_arg __user *uarg;
	struct tee_ioctl_invoke_arg arg;
	struct tee_ioctl_param __user *uparams = NULL;
	struct tee_param *params = NULL;

	if (!ctx->teedev->desc->ops->invoke_func)
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, sizeof(buf)))
		return -EFAULT;

	if (buf.buf_len > TEE_MAX_ARG_SIZE ||
	    buf.buf_len < sizeof(struct tee_ioctl_invoke_arg))
		return -EINVAL;

	uarg = u64_to_user_ptr(buf.buf_ptr);
	if (copy_from_user(&arg, uarg, sizeof(arg)))
		return -EFAULT;

	if (sizeof(arg) + TEE_IOCTL_PARAM_SIZE(arg.num_params) != buf.buf_len)
		return -EINVAL;

	if (arg.num_params) {
		params = kcalloc(arg.num_params, sizeof(struct tee_param),
				 GFP_KERNEL);
		if (!params)
			return -ENOMEM;
		uparams = uarg->params;
		rc = params_from_user(ctx, params, arg.num_params, uparams);
		if (rc)
			goto out;
	}

	rc = ctx->teedev->desc->ops->invoke_func(ctx, &arg, params);
	if (rc)
		goto out;

	if (put_user(arg.ret, &uarg->ret) ||
	    put_user(arg.ret_origin, &uarg->ret_origin)) {
		rc = -EFAULT;
		goto out;
	}
	rc = params_to_user(uparams, arg.num_params, params);
out:
	if (params) {
		/* Decrease ref count for all valid shared memory pointers */
		for (n = 0; n < arg.num_params; n++)
			if (tee_param_is_memref(params + n) &&
			    params[n].u.memref.shm)
				tee_shm_put(params[n].u.memref.shm);
		kfree(params);
	}
	return rc;
}

static int tee_ioctl_cancel(struct tee_context *ctx,
			    struct tee_ioctl_cancel_arg __user *uarg)
{
	return -EINVAL;
}

static int
tee_ioctl_close_session(struct tee_context *ctx,
			struct tee_ioctl_close_session_arg __user *uarg)
{
	struct tee_ioctl_close_session_arg arg;

	if (!ctx->teedev->desc->ops->close_session)
		return -EINVAL;

	if (copy_from_user(&arg, uarg, sizeof(arg)))
		return -EFAULT;

	return ctx->teedev->desc->ops->close_session(ctx, arg.session);
}

static int tee_ioctl(struct cdev *cdev, int cmd, void *arg)
{
	struct tee_context *ctx = cdev->priv;
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case TEE_IOC_VERSION:
		return tee_ioctl_version(ctx, uarg);
	case TEE_IOC_SHM_ALLOC:
		return tee_ioctl_shm_alloc(ctx, uarg);
	case TEE_IOC_SHM_REGISTER:
		return tee_ioctl_shm_register(ctx, uarg);
	case TEE_IOC_OPEN_SESSION:
		return tee_ioctl_open_session(ctx, uarg);
	case TEE_IOC_INVOKE:
		return tee_ioctl_invoke(ctx, uarg);
	case TEE_IOC_CANCEL:
		return tee_ioctl_cancel(ctx, uarg);
	case TEE_IOC_CLOSE_SESSION:
		return tee_ioctl_close_session(ctx, uarg);
	case TEE_IOC_SUPPL_RECV:
		return -ENOSYS;
	case TEE_IOC_SUPPL_SEND:
		return -ENOSYS;
	default:
		return -EINVAL;
	}
}

static const struct cdev_operations tee_cdev_ops = {
	.open = tee_open,
	.close = tee_release,
	.ioctl = tee_ioctl,
};

static void tee_devinfo(struct device *dev)
{
	struct tee_device *teedev = dev->priv;
	struct tee_ioctl_version_data vers;

	teedev->desc->ops->get_version(teedev, &vers);
	printf("Implementation ID: %d\n", vers.impl_id);
}

/**
 * tee_device_alloc() - Allocate a new struct tee_device instance
 * @teedesc:	Descriptor for this driver
 * @dev:	Parent device for this device
 * @driver_data: Private driver data for this device
 *
 * Allocates a new struct tee_device instance. The device is
 * removed by tee_device_unregister().
 *
 * @returns a pointer to a 'struct tee_device' or an ERR_PTR on failure
 */
struct tee_device *tee_device_alloc(const struct tee_desc *teedesc,
				    struct device *dev,
				    void *driver_data)
{
	struct tee_device *teedev;
	void *ret;
	int rc;

	if (!teedesc || !teedesc->name || !teedesc->ops ||
	    !teedesc->ops->get_version || !teedesc->ops->open ||
	    !teedesc->ops->release)
		return ERR_PTR(-EINVAL);

	teedev = kzalloc(sizeof(*teedev), GFP_KERNEL);
	if (!teedev) {
		ret = ERR_PTR(-ENOMEM);
		goto err;
	}

	teedev->dev.id = DEVICE_ID_DYNAMIC;
	teedev->dev.parent = dev;
	teedev->dev.type_data = driver_data;
	teedev->dev.priv = teedev;
	teedev->dev.info = tee_devinfo;

	rc = dev_set_name(&teedev->dev, "tee%s",
			  teedesc->flags & TEE_DESC_PRIVILEGED ? "priv" : "");
	if (rc) {
		ret = ERR_PTR(rc);
		goto err;
	}

	if (IS_ENABLED(CONFIG_OPTEE_DEVFS)) {
		teedev->cdev.dev = &teedev->dev;
		teedev->cdev.ops = &tee_cdev_ops;
	}

	/* 1 as tee_device_unregister() does one final tee_device_put() */
	teedev->num_users = 1;
	mutex_init(&teedev->mutex);

	teedev->desc = teedesc;

	return teedev;
err:
	pr_err("could not register %s driver\n",
	       teedesc->flags & TEE_DESC_PRIVILEGED ? "privileged" : "client");
	kfree(teedev);
	return ret;
}
EXPORT_SYMBOL_GPL(tee_device_alloc);

void tee_device_release(struct tee_device *teedev)
{
	kfree(teedev);
}

/**
 * tee_device_register() - Registers a TEE device
 * @teedev:	Device to register
 *
 * tee_device_unregister() need to be called to remove the @teedev if
 * this function fails.
 *
 * @returns < 0 on failure
 */
int tee_device_register(struct tee_device *teedev)
{
	int rc;

	if (teedev->flags & TEE_DEVICE_FLAG_REGISTERED) {
		dev_err(&teedev->dev, "attempt to register twice\n");
		return -EINVAL;
	}

	rc = register_device(&teedev->dev);
	if (rc)
		return rc;

	if (IS_ENABLED(CONFIG_OPTEE_DEVFS)) {
		teedev->cdev.name = teedev->dev.unique_name;

		rc = devfs_create(&teedev->cdev);
		if (rc)
			goto out;
	}

	list_add_tail(&teedev->list, &tee_clients);

	teedev->flags |= TEE_DEVICE_FLAG_REGISTERED;
	return 0;

out:
	unregister_device(&teedev->dev);
	return rc;
}
EXPORT_SYMBOL_GPL(tee_device_register);

void tee_device_put(struct tee_device *teedev)
{
	mutex_lock(&teedev->mutex);
	/* Shouldn't put in this state */
	if (!WARN_ON(!teedev->desc)) {
		teedev->num_users--;
		if (!teedev->num_users) {
			teedev->desc = NULL;
		}
	}
	mutex_unlock(&teedev->mutex);
}

bool tee_device_get(struct tee_device *teedev)
{
	mutex_lock(&teedev->mutex);
	if (!teedev->desc) {
		mutex_unlock(&teedev->mutex);
		return false;
	}
	teedev->num_users++;
	mutex_unlock(&teedev->mutex);
	return true;
}

/**
 * tee_device_unregister() - Removes a TEE device
 * @teedev:	Device to unregister
 *
 * This function should be called to remove the @teedev even if
 * tee_device_register() hasn't been called yet. Does nothing if
 * @teedev is NULL.
 */
void tee_device_unregister(struct tee_device *teedev)
{
	if (!teedev)
		return;

	list_del(&teedev->list);
	if (IS_ENABLED(CONFIG_OPTEE_DEVFS))
		devfs_remove(&teedev->cdev);
	unregister_device(&teedev->dev);
}
EXPORT_SYMBOL_GPL(tee_device_unregister);

/**
 * tee_get_drvdata() - Return driver_data pointer
 * @teedev:	Device containing the driver_data pointer
 * @returns the driver_data pointer supplied to tee_device_alloc().
 */
void *tee_get_drvdata(struct tee_device *teedev)
{
	return teedev->dev.type_data;
}
EXPORT_SYMBOL_GPL(tee_get_drvdata);

struct match_dev_data {
	struct tee_ioctl_version_data *vers;
	const void *data;
	int (*match)(struct tee_ioctl_version_data *, const void *);
};

static int match_dev(struct device *dev, const void *data)
{
	const struct match_dev_data *match_data = data;
	struct tee_device *teedev = container_of(dev, struct tee_device, dev);

	teedev->desc->ops->get_version(teedev, match_data->vers);
	return match_data->match(match_data->vers, match_data->data);
}

struct tee_context *
tee_client_open_context(struct tee_context *start,
			int (*match)(struct tee_ioctl_version_data *,
				     const void *),
			const void *data, struct tee_ioctl_version_data *vers)
{
	struct device *startdev = NULL;
	struct tee_device *teedev;
	struct tee_ioctl_version_data v;
	struct match_dev_data match_data = { vers ? vers : &v, data, match };

	if (start)
		startdev = &start->teedev->dev;

	list_for_each_entry(teedev, &tee_clients, list) {
		struct device *dev = &teedev->dev;
		struct tee_context *ctx ;

		if (startdev) {
			if (dev == startdev)
				startdev = NULL;
			continue;
		}

		if (!match_dev(dev, &match_data))
			continue;

		ctx = teedev_open(teedev);
		if (IS_ERR(ctx) && PTR_ERR(ctx) != -ENOMEM)
			continue;

		/* On success or -ENOMEM, early exit the iteration */
		return ctx;
	}

	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL_GPL(tee_client_open_context);

void tee_client_close_context(struct tee_context *ctx)
{
	teedev_close_context(ctx);
}
EXPORT_SYMBOL_GPL(tee_client_close_context);

void tee_client_get_version(struct tee_context *ctx,
			    struct tee_ioctl_version_data *vers)
{
	ctx->teedev->desc->ops->get_version(ctx->teedev, vers);
}
EXPORT_SYMBOL_GPL(tee_client_get_version);

int tee_client_open_session(struct tee_context *ctx,
			    struct tee_ioctl_open_session_arg *arg,
			    struct tee_param *param)
{
	if (!ctx->teedev->desc->ops->open_session)
		return -EINVAL;
	return ctx->teedev->desc->ops->open_session(ctx, arg, param);
}
EXPORT_SYMBOL_GPL(tee_client_open_session);

int tee_client_close_session(struct tee_context *ctx, u32 session)
{
	if (!ctx->teedev->desc->ops->close_session)
		return -EINVAL;
	return ctx->teedev->desc->ops->close_session(ctx, session);
}
EXPORT_SYMBOL_GPL(tee_client_close_session);

int tee_client_invoke_func(struct tee_context *ctx,
			   struct tee_ioctl_invoke_arg *arg,
			   struct tee_param *param)
{
	if (!ctx->teedev->desc->ops->invoke_func)
		return -EINVAL;
	return ctx->teedev->desc->ops->invoke_func(ctx, arg, param);
}
EXPORT_SYMBOL_GPL(tee_client_invoke_func);

static int tee_client_device_match(struct device *dev,
				   struct device_driver *drv)
{
	const struct tee_client_device_id *id_table;
	struct tee_client_device *tee_device;

	id_table = to_tee_client_driver(drv)->id_table;
	tee_device = to_tee_client_device(dev);

	while (!uuid_is_null(&id_table->uuid)) {
		if (uuid_equal(&tee_device->id.uuid, &id_table->uuid))
			return 0;
		id_table++;
	}

	return -1;
}

static int tee_bus_probe(struct device *dev)
{
	return dev->driver->probe(dev);
}

static void tee_bus_remove(struct device *dev)
{
	if (dev->driver->remove)
		dev->driver->remove(dev);
}

struct bus_type tee_bus_type = {
	.name		= "tee",
	.match		= tee_client_device_match,
	.probe		= tee_bus_probe,
	.remove		= tee_bus_remove,
};
EXPORT_SYMBOL_GPL(tee_bus_type);

static int __init tee_init(void)
{
	return bus_register(&tee_bus_type);
}
pure_initcall(tee_init);

MODULE_AUTHOR("Linaro");
MODULE_DESCRIPTION("TEE Driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
