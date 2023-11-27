/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2016, Linaro Limited
 */
#ifndef TEE_PRIVATE_H
#define TEE_PRIVATE_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>

#define TEE_DEVICE_FLAG_REGISTERED	0x1
#define TEE_MAX_DEV_NAME_LEN		32

struct tee_shm;
struct tee_context;

/**
 * struct tee_device - TEE Device representation
 * @name:	name of device
 * @desc:	description of device
 * @id:		unique id of device
 * @flags:	represented by TEE_DEVICE_FLAG_REGISTERED above
 * @dev:	embedded basic device structure
 * @num_users:	number of active users of this device
 * @mutex:	mutex protecting @num_users and @idr
 */
struct tee_device {
	char name[TEE_MAX_DEV_NAME_LEN];
	const struct tee_desc *desc;
	struct list_head list;
	unsigned int flags;

	struct device dev;

	size_t num_users;
	struct mutex mutex;	/* protects num_users and idr */
};

bool tee_device_get(struct tee_device *teedev);
void tee_device_put(struct tee_device *teedev);

void teedev_ctx_get(struct tee_context *ctx);
void teedev_ctx_put(struct tee_context *ctx);

#endif /*TEE_PRIVATE_H*/
