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

	list_add_tail(&teedev->list, &tee_clients);

	teedev->flags |= TEE_DEVICE_FLAG_REGISTERED;
	return 0;
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
