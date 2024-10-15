/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2022 Linaro Limited
 */

#ifndef __TEE_DRV_H
#define __TEE_DRV_H

#include <linux/bits.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mod_devicetable.h>
#include <linux/tee.h>
#include <linux/types.h>
#include <linux/uuid.h>

/*
 * The file describes the API provided by the generic TEE driver to the
 * specific TEE driver.
 */

#define TEE_SHM_DYNAMIC		BIT(0)  /* Dynamic shared memory registered */
					/* in secure world */
#define TEE_SHM_USER_MAPPED	BIT(1)  /* Memory mapped in user space */
#define TEE_SHM_POOL		BIT(2)  /* Memory allocated from pool */
#define TEE_SHM_PRIV		BIT(3)  /* Memory private to TEE driver */

struct device;
struct tee_device;
struct tee_shm;

/**
 * struct tee_context - driver specific context on file pointer data
 * @teedev:	pointer to this drivers struct tee_device
 * @list_shm:	List of shared memory object owned by this context
 * @data:	driver specific context data, managed by the driver
 * @refcount:	reference counter for this structure
 * @releasing:  flag that indicates if context is being released right now.
 *		It is needed to break circular dependency on context during
 *              shared memory release.
 * @cap_memref_null: flag indicating if the TEE Client support shared
 *                   memory buffer with a NULL pointer.
 */
struct tee_context {
	struct tee_device *teedev;
	struct list_head list_shm;
	void *data;
	struct kref refcount;
	bool releasing;
	bool cap_memref_null;
};

struct tee_param_memref {
	size_t shm_offs;
	size_t size;
	struct tee_shm *shm;
};

struct tee_param_value {
	u64 a;
	u64 b;
	u64 c;
};

struct tee_param {
	u64 attr;
	union {
		struct tee_param_memref memref;
		struct tee_param_value value;
	} u;
};

/**
 * struct tee_driver_ops - driver operations vtable
 * @get_version:	returns version of driver
 * @open:		called when the device file is opened
 * @release:		release this open file
 * @open_session:	open a new session
 * @close_session:	close a session
 * @invoke_func:	invoke a trusted function
 * @shm_register:	register shared memory buffer in TEE
 * @shm_unregister:	unregister shared memory buffer in TEE
 */
struct tee_driver_ops {
	void (*get_version)(struct tee_device *teedev,
			    struct tee_ioctl_version_data *vers);
	int (*open)(struct tee_context *ctx);
	void (*release)(struct tee_context *ctx);
	int (*open_session)(struct tee_context *ctx,
			    struct tee_ioctl_open_session_arg *arg,
			    struct tee_param *param);
	int (*close_session)(struct tee_context *ctx, u32 session);
	int (*invoke_func)(struct tee_context *ctx,
			   struct tee_ioctl_invoke_arg *arg,
			   struct tee_param *param);
	int (*shm_register)(struct tee_context *ctx, struct tee_shm *shm);
	int (*shm_unregister)(struct tee_context *ctx, struct tee_shm *shm);
};

/**
 * struct tee_desc - Describes the TEE driver to the subsystem
 * @name:	name of driver
 * @ops:	driver operations vtable
 * @owner:	module providing the driver
 * @flags:	Extra properties of driver, defined by TEE_DESC_* below
 */
#define TEE_DESC_PRIVILEGED	0x1
struct tee_desc {
	const char *name;
	const struct tee_driver_ops *ops;
	struct module *owner;
	u32 flags;
};

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
				    void *driver_data);

/**
 * tee_device_release() - Releases new struct tee_device instance
 * @teedev: Device allocated with tee_device_alloc()
 *
 * Frees a struct tee_device.
 */
void tee_device_release(struct tee_device *teedev);

/**
 * tee_device_register() - Registers a TEE device
 * @teedev:	Device to register
 *
 * tee_device_unregister() need to be called to remove the @teedev if
 * this function fails.
 *
 * @returns < 0 on failure
 */
int tee_device_register(struct tee_device *teedev);

/**
 * tee_device_unregister() - Removes a TEE device
 * @teedev:	Device to unregister
 *
 * This function should be called to remove the @teedev even if
 * tee_device_register() hasn't been called yet. Does nothing if
 * @teedev is NULL.
 */
void tee_device_unregister(struct tee_device *teedev);

/**
 * tee_session_calc_client_uuid() - Calculates client UUID for session
 * @uuid:		Resulting UUID
 * @connection_method:	Connection method for session (TEE_IOCTL_LOGIN_*)
 * @connectuon_data:	Connection data for opening session
 *
 * Based on connection method calculates UUIDv5 based client UUID.
 *
 * For group based logins verifies that calling process has specified
 * credentials.
 *
 * @return < 0 on failure
 */
int tee_session_calc_client_uuid(uuid_t *uuid, u32 connection_method,
				 const u8 connection_data[TEE_IOCTL_UUID_LEN]);

/**
 * struct tee_shm - shared memory object
 * @ctx:	context using the object
 * @paddr:	physical address of the shared memory
 * @kaddr:	virtual address of the shared memory
 * @size:	size of shared memory
 * @refcount:	reference counter
 * @flags:	defined by TEE_SHM_* in tee_drv.h
 * @link:	list head for registering object globally
 * @fd:		file descriptor for use in userspace
 * @dev:	device for registering shared memory
 * @res:	resource to be associated with device
 *
 * This pool is only supposed to be accessed directly from the TEE
 * subsystem and from drivers that implements their own shm pool manager.
 */
struct tee_shm {
	struct tee_context *ctx;
	phys_addr_t paddr;
	void *kaddr;
	size_t size;
	refcount_t refcount;
	u32 flags;
	struct list_head link;

	int fd;
	struct device_d dev;
	struct cdev cdev;
	struct resource res;
};

/**
 * tee_get_drvdata() - Return driver_data pointer
 * @returns the driver_data pointer supplied to tee_register().
 */
void *tee_get_drvdata(struct tee_device *teedev);

struct tee_shm *tee_shm_alloc_priv_buf(struct tee_context *ctx, size_t size);
struct tee_shm *tee_shm_alloc_kernel_buf(struct tee_context *ctx, size_t size);

struct tee_shm *tee_shm_alloc_user_buf(struct tee_context *ctx, size_t size);
struct tee_shm *tee_shm_register_user_buf(struct tee_context *ctx,
					  unsigned long addr, size_t length);

/**
 * tee_shm_is_dynamic() - Check if shared memory object is of the dynamic kind
 * @shm:	Shared memory handle
 * @returns true if object is dynamic shared memory
 */
static inline bool tee_shm_is_dynamic(struct tee_shm *shm)
{
	return shm && (shm->flags & TEE_SHM_DYNAMIC);
}

/**
 * tee_shm_free() - Free shared memory
 * @shm:	Handle to shared memory to free
 */
void tee_shm_free(struct tee_shm *shm);

/**
 * tee_shm_put() - Decrease reference count on a shared memory handle
 * @shm:	Shared memory handle
 */
void tee_shm_put(struct tee_shm *shm);

/**
 * tee_shm_get_va() - Get virtual address of a shared memory plus an offset
 * @shm:	Shared memory handle
 * @offs:	Offset from start of this shared memory
 * @returns virtual address of the shared memory + offs if offs is within
 *	the bounds of this shared memory, else an ERR_PTR
 */
void *tee_shm_get_va(struct tee_shm *shm, size_t offs);

/**
 * tee_shm_get_pa() - Get physical address of a shared memory plus an offset
 * @shm:	Shared memory handle
 * @offs:	Offset from start of this shared memory
 * @pa:		Physical address to return
 * @returns 0 if offs is within the bounds of this shared memory, else an
 *	error code.
 */
int tee_shm_get_pa(struct tee_shm *shm, size_t offs, phys_addr_t *pa);

/**
 * tee_shm_get_size() - Get size of shared memory buffer
 * @shm:	Shared memory handle
 * @returns size of shared memory
 */
static inline size_t tee_shm_get_size(struct tee_shm *shm)
{
	return shm->size;
}

/**
 * tee_shm_get_id() - Get id of a shared memory object
 * @shm:	Shared memory handle
 * @returns id
 */
static inline int tee_shm_get_id(struct tee_shm *shm)
{
	/* Only call on non-private SHMs */
	BUG_ON(shm->dev.id < 0);
	return shm->dev.id;
}

/**
 * tee_shm_get_from_id() - Find shared memory object and increase reference
 * count
 * @ctx:	Context owning the shared memory
 * @id:		Id of shared memory object
 * @returns a pointer to 'struct tee_shm' on success or an ERR_PTR on failure
 */
struct tee_shm *tee_shm_get_from_id(struct tee_context *ctx, int id);

/**
 * tee_client_open_context() - Open a TEE context
 * @start:	if not NULL, continue search after this context
 * @match:	function to check TEE device
 * @data:	data for match function
 * @vers:	if not NULL, version data of TEE device of the context returned
 *
 * This function does an operation similar to open("/dev/teeX") in user space.
 * A returned context must be released with tee_client_close_context().
 *
 * Returns a TEE context of the first TEE device matched by the match()
 * callback or an ERR_PTR.
 */
struct tee_context *
tee_client_open_context(struct tee_context *start,
			int (*match)(struct tee_ioctl_version_data *,
				     const void *),
			const void *data, struct tee_ioctl_version_data *vers);

/**
 * tee_client_close_context() - Close a TEE context
 * @ctx:	TEE context to close
 *
 * Note that all sessions previously opened with this context will be
 * closed when this function is called.
 */
void tee_client_close_context(struct tee_context *ctx);

/**
 * tee_client_get_version() - Query version of TEE
 * @ctx:	TEE context to TEE to query
 * @vers:	Pointer to version data
 */
void tee_client_get_version(struct tee_context *ctx,
			    struct tee_ioctl_version_data *vers);

/**
 * tee_client_open_session() - Open a session to a Trusted Application
 * @ctx:	TEE context
 * @arg:	Open session arguments, see description of
 *		struct tee_ioctl_open_session_arg
 * @param:	Parameters passed to the Trusted Application
 *
 * Returns < 0 on error else see @arg->ret for result. If @arg->ret
 * is TEEC_SUCCESS the session identifier is available in @arg->session.
 */
int tee_client_open_session(struct tee_context *ctx,
			    struct tee_ioctl_open_session_arg *arg,
			    struct tee_param *param);

/**
 * tee_client_close_session() - Close a session to a Trusted Application
 * @ctx:	TEE Context
 * @session:	Session id
 *
 * Return < 0 on error else 0, regardless the session will not be
 * valid after this function has returned.
 */
int tee_client_close_session(struct tee_context *ctx, u32 session);

/**
 * tee_client_invoke_func() - Invoke a function in a Trusted Application
 * @ctx:	TEE Context
 * @arg:	Invoke arguments, see description of
 *		struct tee_ioctl_invoke_arg
 * @param:	Parameters passed to the Trusted Application
 *
 * Returns < 0 on error else see @arg->ret for result.
 */
int tee_client_invoke_func(struct tee_context *ctx,
			   struct tee_ioctl_invoke_arg *arg,
			   struct tee_param *param);

static inline bool tee_param_is_memref(struct tee_param *param)
{
	switch (param->attr & TEE_IOCTL_PARAM_ATTR_TYPE_MASK) {
	case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
	case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
	case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
		return true;
	default:
		return false;
	}
}

extern struct bus_type tee_bus_type;

/**
 * struct tee_client_device - tee based device
 * @id:			device identifier
 * @dev:		device structure
 */
struct tee_client_device {
	struct tee_client_device_id id;
	struct device dev;
};

#define to_tee_client_device(d) container_of(d, struct tee_client_device, dev)

/**
 * struct tee_client_driver - tee client driver
 * @id_table:		device id table supported by this driver
 * @driver:		driver structure
 */
struct tee_client_driver {
	const struct tee_client_device_id *id_table;
	struct device_driver driver;
};

#define to_tee_client_driver(d) \
		container_of(d, struct tee_client_driver, driver)

/**
 * teedev_open() - Open a struct tee_device
 * @teedev:	Device to open
 *
 * @return a pointer to struct tee_context on success or an ERR_PTR on failure.
 */
struct tee_context *teedev_open(struct tee_device *teedev);

/**
 * teedev_close_context() - closes a struct tee_context
 * @ctx:	The struct tee_context to close
 */
void teedev_close_context(struct tee_context *ctx);

#endif /*__TEE_DRV_H*/
