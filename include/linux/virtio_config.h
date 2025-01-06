/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_VIRTIO_CONFIG_H
#define _LINUX_VIRTIO_CONFIG_H

#include <linux/err.h>
#include <linux/bug.h>
#include <linux/virtio.h>
#include <linux/virtio_byteorder.h>
#include <linux/compiler_types.h>
#include <linux/typecheck.h>
#include <uapi/linux/virtio_config.h>

#ifndef might_sleep
#define might_sleep() do { } while (0)
#endif

struct virtio_shm_region {
	u64 addr;
	u64 len;
};

struct virtqueue;

/* virtio bus operations */
struct virtio_config_ops {
	/**
	 * get_config() - read the value of a configuration field
	 *
	 * @vdev:	the real virtio device
	 * @offset:	the offset of the configuration field
	 * @buf:	the buffer to write the field value into
	 * @len:	the length of the buffer
	 * @return 0 if OK, -ve on error
	 */
	int (*get_config)(struct virtio_device *vdev, unsigned int offset,
			  void *buf, unsigned int len);
	/**
	 * set_config() - write the value of a configuration field
	 *
	 * @vdev:	the real virtio device
	 * @offset:	the offset of the configuration field
	 * @buf:	the buffer to read the field value from
	 * @len:	the length of the buffer
	 * @return 0 if OK, -ve on error
	 */
	int (*set_config)(struct virtio_device *vdev, unsigned int offset,
			  const void *buf, unsigned int len);
	/**
	 * generation() - config generation counter
	 *
	 * @vdev:	the real virtio device
	 * @return the config generation counter
	 */
	u32 (*generation)(struct virtio_device *vdev);
	/**
	 * get_status() - read the status byte
	 *
	 * @vdev:	the real virtio device
	 * @status:	the returned status byte
	 * @return 0 if OK, -ve on error
	 */
	int (*get_status)(struct virtio_device *vdev);
	/**
	 * set_status() - write the status byte
	 *
	 * @vdev:	the real virtio device
	 * @status:	the new status byte
	 * @return 0 if OK, -ve on error
	 */
	int (*set_status)(struct virtio_device *vdev, u8 status);
	/**
	 * reset() - reset the device
	 *
	 * @vdev:	the real virtio device
	 * @return 0 if OK, -ve on error
	 */
	int (*reset)(struct virtio_device *vdev);
	/**
	 * get_features() - get the array of feature bits for this device
	 *
	 * @vdev:	the real virtio device
	 * @return features
	 */
	u64 (*get_features)(struct virtio_device *vdev);
	/**
	 * find_vqs() - find virtqueues and instantiate them
	 *
	 * @vdev:	the real virtio device
	 * @nvqs:	the number of virtqueues to find
	 * @vqs:	on success, includes new virtqueues
	 * @return 0 if OK, -ve on error
	 */
	int (*find_vqs)(struct virtio_device *vdev, unsigned int nvqs,
			struct virtqueue *vqs[]);
	/**
	 * del_vqs() - free virtqueues found by find_vqs()
	 *
	 * @vdev:	the real virtio device
	 * @return 0 if OK, -ve on error
	 */
	int (*del_vqs)(struct virtio_device *vdev);
	/**
	 * notify() - notify the device to process the queue
	 *
	 * @vdev:	the real virtio device
	 * @vq:		virtqueue to process
	 * @return 0 if OK, -ve on error
	 */
	int (*notify)(struct virtio_device *vdev, struct virtqueue *vq);
	/**
	 * finalize_features() - confirm what device features we'll be using.
	 * @vdev: 	the virtio_device
	 * 		This gives the final feature bits for the device: it can change
	 * 		the dev->feature bits if it wants.
	 * @returns 0 if OK, -ve on error
	 */
	int (*finalize_features)(struct virtio_device *vdev);
};

/* If driver didn't advertise the feature, it will never appear. */
void virtio_check_driver_offered_feature(const struct virtio_device *vdev,
					 unsigned int fbit);

/**
 * __virtio_test_bit - helper to test feature bits. For use by transports.
 *                     Devices should normally use virtio_has_feature,
 *                     which includes more checks.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline bool __virtio_test_bit(const struct virtio_device *vdev,
				     unsigned int fbit)
{
	/* Did you forget to fix assumptions on max features? */
	if (__builtin_constant_p(fbit))
		BUILD_BUG_ON(fbit >= 64);
	else
		BUG_ON(fbit >= 64);

	return vdev->features & BIT_ULL(fbit);
}

/**
 * __virtio_set_bit - helper to set feature bits. For use by transports.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline void __virtio_set_bit(struct virtio_device *vdev,
				    unsigned int fbit)
{
	/* Did you forget to fix assumptions on max features? */
	if (__builtin_constant_p(fbit))
		BUILD_BUG_ON(fbit >= 64);
	else
		BUG_ON(fbit >= 64);

	vdev->features |= BIT_ULL(fbit);
}

/**
 * __virtio_clear_bit - helper to clear feature bits. For use by transports.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline void __virtio_clear_bit(struct virtio_device *vdev,
				      unsigned int fbit)
{
	/* Did you forget to fix assumptions on max features? */
	if (__builtin_constant_p(fbit))
		BUILD_BUG_ON(fbit >= 64);
	else
		BUG_ON(fbit >= 64);

	vdev->features &= ~BIT_ULL(fbit);
}

/**
 * virtio_has_feature - helper to determine if this device has this feature.
 * @vdev: the device
 * @fbit: the feature bit
 */
static inline bool virtio_has_feature(const struct virtio_device *vdev,
				      unsigned int fbit)
{
	if (fbit < VIRTIO_TRANSPORT_F_START)
		virtio_check_driver_offered_feature(vdev, fbit);

	return __virtio_test_bit(vdev, fbit);
}

/**
 * virtio_has_dma_quirk - determine whether this device has the DMA quirk
 * @vdev: the device
 */
static inline bool virtio_has_dma_quirk(const struct virtio_device *vdev)
{
	/*
	 * Note the reverse polarity of the quirk feature (compared to most
	 * other features), this is for compatibility with legacy systems.
	 */
	return !virtio_has_feature(vdev, VIRTIO_F_ACCESS_PLATFORM);
}

static inline bool virtio_is_little_endian(struct virtio_device *vdev)
{
	return virtio_has_feature(vdev, VIRTIO_F_VERSION_1) ||
		virtio_legacy_is_little_endian();
}


/**
 * virtio_get_config() - read the value of a configuration field
 *
 * @vdev:	the real virtio device
 * @offset:	the offset of the configuration field
 * @buf:	the buffer to write the field value into
 * @len:	the length of the buffer
 * @return 0 if OK, -ve on error
 */
static inline int virtio_get_config(struct virtio_device *vdev, unsigned int offset,
				    void *buf, unsigned int len)
{
	return vdev->config->get_config(vdev, offset, buf, len);
}

/**
 * virtio_set_config() - write the value of a configuration field
 *
 * @vdev:	the real virtio device
 * @offset:	the offset of the configuration field
 * @buf:	the buffer to read the field value from
 * @len:	the length of the buffer
 * @return 0 if OK, -ve on error
 */
static inline int virtio_set_config(struct virtio_device *vdev, unsigned int offset,
		      void *buf, unsigned int len)
{
	return vdev->config->set_config(vdev, offset, buf, len);
}

/**
 * virtio_find_vqs() - find virtqueues and instantiate them
 *
 * @vdev:	the real virtio device
 * @nvqs:	the number of virtqueues to find
 * @vqs:	on success, includes new virtqueues
 * @return 0 if OK, -ve on error
 */
static inline
int virtio_find_vqs(struct virtio_device *vdev, unsigned int nvqs,
		    struct virtqueue *vqs[])
{
	return vdev->config->find_vqs(vdev, nvqs, vqs);
}

/**
 * virtio_device_ready - enable vq use in probe function
 * @vdev: the device
 *
 * Driver must call this to use vqs in the probe function.
 *
 * Note: vqs are enabled automatically after probe returns.
 */
static inline
void virtio_device_ready(struct virtio_device *dev)
{
	unsigned status = dev->config->get_status(dev);

	BUG_ON(status & VIRTIO_CONFIG_S_DRIVER_OK);
	dev->config->set_status(dev, status | VIRTIO_CONFIG_S_DRIVER_OK);
}


/* Memory accessors */
static inline u16 virtio16_to_cpu(struct virtio_device *vdev, __virtio16 val)
{
	return __virtio16_to_cpu(virtio_is_little_endian(vdev), val);
}

static inline __virtio16 cpu_to_virtio16(struct virtio_device *vdev, u16 val)
{
	return __cpu_to_virtio16(virtio_is_little_endian(vdev), val);
}

static inline u32 virtio32_to_cpu(struct virtio_device *vdev, __virtio32 val)
{
	return __virtio32_to_cpu(virtio_is_little_endian(vdev), val);
}

static inline __virtio32 cpu_to_virtio32(struct virtio_device *vdev, u32 val)
{
	return __cpu_to_virtio32(virtio_is_little_endian(vdev), val);
}

static inline u64 virtio64_to_cpu(struct virtio_device *vdev, __virtio64 val)
{
	return __virtio64_to_cpu(virtio_is_little_endian(vdev), val);
}

static inline __virtio64 cpu_to_virtio64(struct virtio_device *vdev, u64 val)
{
	return __cpu_to_virtio64(virtio_is_little_endian(vdev), val);
}

/* Read @count fields, @bytes each */
static inline void __virtio_cread_many(struct virtio_device *vdev,
				       unsigned int offset,
				       void *buf, size_t count, size_t bytes)
{
	u32 old, gen;
	int i;

	/* no need to check return value as generation can be optional */
	gen = vdev->config->generation(vdev);
	do {
		old = gen;

		for (i = 0; i < count; i++)
			virtio_get_config(vdev, offset + bytes * i,
					  buf + i * bytes, bytes);

		gen = vdev->config->generation(vdev);
	} while (gen != old);
}

static inline void virtio_cread_bytes(struct virtio_device *vdev,
				      unsigned int offset,
				      void *buf, size_t len)
{
	__virtio_cread_many(vdev, offset, buf, len, 1);
}

static inline u8 virtio_cread8(struct virtio_device *vdev, unsigned int offset)
{
	u8 ret;

	virtio_get_config(vdev, offset, &ret, sizeof(ret));
	return ret;
}

static inline void virtio_cwrite8(struct virtio_device *vdev,
				  unsigned int offset, u8 val)
{
	virtio_set_config(vdev, offset, &val, sizeof(val));
}

static inline u16 virtio_cread16(struct virtio_device *vdev,
				 unsigned int offset)
{
	u16 ret;

	virtio_get_config(vdev, offset, &ret, sizeof(ret));
	return virtio16_to_cpu(vdev, (__force __virtio16)ret);
}

static inline void virtio_cwrite16(struct virtio_device *vdev,
				   unsigned int offset, u16 val)
{
	val = (__force u16)cpu_to_virtio16(vdev, val);
	virtio_set_config(vdev, offset, &val, sizeof(val));
}

static inline u32 virtio_cread32(struct virtio_device *vdev,
				 unsigned int offset)
{
	u32 ret;

	virtio_get_config(vdev, offset, &ret, sizeof(ret));
	return virtio32_to_cpu(vdev, (__force __virtio32)ret);
}

static inline void virtio_cwrite32(struct virtio_device *vdev,
				   unsigned int offset, u32 val)
{
	val = (__force u32)cpu_to_virtio32(vdev, val);
	virtio_set_config(vdev, offset, &val, sizeof(val));
}

static inline u64 virtio_cread64(struct virtio_device *vdev,
				 unsigned int offset)
{
	u64 ret;

	__virtio_cread_many(vdev, offset, &ret, 1, sizeof(ret));
	return virtio64_to_cpu(vdev, (__force __virtio64)ret);
}

static inline void virtio_cwrite64(struct virtio_device *vdev,
				   unsigned int offset, u64 val)
{
	val = (__force u64)cpu_to_virtio64(vdev, val);
	virtio_set_config(vdev, offset, &val, sizeof(val));
}

/* Config space read accessor */
#define virtio_cread(vdev, structname, member, ptr)			\
	do {								\
		/* Must match the member's type, and be integer */	\
		if (!typecheck(typeof((((structname *)0)->member)), *(ptr))) \
			(*ptr) = 1;					\
									\
		switch (sizeof(*ptr)) {					\
		case 1:							\
			*(ptr) = virtio_cread8(vdev,			\
					       offsetof(structname, member)); \
			break;						\
		case 2:							\
			*(ptr) = virtio_cread16(vdev,			\
						offsetof(structname, member)); \
			break;						\
		case 4:							\
			*(ptr) = virtio_cread32(vdev,			\
						offsetof(structname, member)); \
			break;						\
		case 8:							\
			*(ptr) = virtio_cread64(vdev,			\
						offsetof(structname, member)); \
			break;						\
		default:						\
			WARN_ON(true);					\
		}							\
	} while (0)

/* Config space write accessor */
#define virtio_cwrite(vdev, structname, member, ptr)			\
	do {								\
		/* Must match the member's type, and be integer */	\
		if (!typecheck(typeof((((structname *)0)->member)), *(ptr))) \
			WARN_ON((*ptr) == 1);				\
									\
		switch (sizeof(*ptr)) {					\
		case 1:							\
			virtio_cwrite8(vdev,				\
				       offsetof(structname, member),	\
				       *(ptr));				\
			break;						\
		case 2:							\
			virtio_cwrite16(vdev,				\
					offsetof(structname, member),	\
					*(ptr));			\
			break;						\
		case 4:							\
			virtio_cwrite32(vdev,				\
					offsetof(structname, member),	\
					*(ptr));			\
			break;						\
		case 8:							\
			virtio_cwrite64(vdev,				\
					offsetof(structname, member),	\
					*(ptr));			\
			break;						\
		default:						\
			WARN_ON(true);					\
		}							\
	} while (0)

/* Conditional config space accessors */
#define virtio_cread_feature(vdev, fbit, structname, member, ptr)	\
	({								\
		int _r = 0;						\
		if (!virtio_has_feature(vdev, fbit))			\
			_r = -ENOENT;					\
		else							\
			virtio_cread(vdev, structname, member, ptr);	\
		_r;							\
	})

/*
 * Nothing virtio-specific about these, but let's worry about generalizing
 * these later.
 */
#define virtio_le_to_cpu(x) \
	_Generic((x), \
		__u8: (u8)(x), \
		 __le16: (u16)le16_to_cpu(x), \
		 __le32: (u32)le32_to_cpu(x), \
		 __le64: (u64)le64_to_cpu(x) \
		)

#define virtio_cpu_to_le(x, m) \
	_Generic((m), \
		 __u8: (x), \
		 __le16: cpu_to_le16(x), \
		 __le32: cpu_to_le32(x), \
		 __le64: cpu_to_le64(x) \
		)

/* LE (e.g. modern) Config space accessors. */
#define virtio_cread_le(vdev, structname, member, ptr)			\
	do {								\
		typeof(((structname*)0)->member) virtio_cread_v;	\
									\
		/* Sanity check: must match the member's type */	\
		typecheck(typeof(virtio_le_to_cpu(virtio_cread_v)), *(ptr)); \
									\
		switch (sizeof(virtio_cread_v)) {			\
		case 1:							\
		case 2:							\
		case 4:							\
			vdev->config->get_config((vdev),		\
					  offsetof(structname, member), \
					  &virtio_cread_v,		\
					  sizeof(virtio_cread_v));	\
			break;						\
		default:						\
			__virtio_cread_many((vdev), 			\
					  offsetof(structname, member), \
					  &virtio_cread_v,		\
					  1,				\
					  sizeof(virtio_cread_v));	\
			break;						\
		}							\
		*(ptr) = virtio_le_to_cpu(virtio_cread_v);		\
	} while(0)

#define virtio_cwrite_le(vdev, structname, member, ptr)			\
	do {								\
		typeof(((structname*)0)->member) virtio_cwrite_v =	\
			virtio_cpu_to_le(*(ptr), ((structname*)0)->member); \
									\
		/* Sanity check: must match the member's type */	\
		typecheck(typeof(virtio_le_to_cpu(virtio_cwrite_v)), *(ptr)); \
									\
		vdev->config->set_config((vdev), offsetof(structname, member),	\
				  &virtio_cwrite_v,			\
				  sizeof(virtio_cwrite_v));		\
	} while(0)


#ifdef CONFIG_ARCH_HAS_RESTRICTED_VIRTIO_MEMORY_ACCESS
int arch_has_restricted_virtio_memory_access(void);
#else
static inline int arch_has_restricted_virtio_memory_access(void)
{
	return 0;
}
#endif /* CONFIG_ARCH_HAS_RESTRICTED_VIRTIO_MEMORY_ACCESS */


#undef might_sleep

#endif /* _LINUX_VIRTIO_CONFIG_H */
