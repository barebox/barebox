/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_RESET_H_
#define _LINUX_RESET_H_

struct device;
struct reset_control;

/**
 * struct reset_control_bulk_data - Data used for bulk reset control operations.
 *
 * @id: reset control consumer ID
 * @rstc: struct reset_control * to store the associated reset control
 *
 * The reset APIs provide a series of reset_control_bulk_*() API calls as
 * a convenience to consumers which require multiple reset controls.
 * This structure is used to manage data for these calls.
 */
struct reset_control_bulk_data {
	const char			*id;
	struct reset_control		*rstc;
};

#ifdef CONFIG_RESET_CONTROLLER

int reset_control_status(struct reset_control *rstc);
int reset_control_reset(struct reset_control *rstc);
int reset_control_assert(struct reset_control *rstc);
int reset_control_deassert(struct reset_control *rstc);

struct reset_control *__reset_control_get(struct device *dev, const char *id, bool optional);
struct reset_control *of_reset_control_get(struct device_node *node,
					   const char *id);
void reset_control_put(struct reset_control *rstc);

int __must_check device_reset(struct device *dev);

int __must_check device_reset_us(struct device *dev, int us);

int __must_check device_reset_all(struct device *dev);

int reset_control_get_count(struct device *dev);

struct reset_control *reset_control_array_get(struct device *dev);

int __reset_control_bulk_get(struct device *dev, int num_rstcs,
			     struct reset_control_bulk_data *rstcs,
			     bool optional);

#else

static inline int reset_control_status(struct reset_control *rstc)
{
	return 0;
}

static inline int reset_control_reset(struct reset_control *rstc)
{
	return 0;
}

static inline int reset_control_assert(struct reset_control *rstc)
{
	return 0;
}

static inline int reset_control_deassert(struct reset_control *rstc)
{
	return 0;
}

static inline struct reset_control *
of_reset_control_get(struct device_node *node, const char *id)
{
	return NULL;
}

static inline struct reset_control *
__reset_control_get(struct device *dev, const char *id, bool optional)
{
	return NULL;
}

static inline void reset_control_put(struct reset_control *rstc)
{
}

static inline int device_reset_us(struct device *dev, int us)
{
	return 0;
}

static inline int device_reset(struct device *dev)
{
	return 0;
}

static inline int device_reset_all(struct device *dev)
{
	return 0;
}

static inline int reset_control_get_count(struct device *dev)
{
	return 0;
}

static inline struct reset_control *reset_control_array_get(struct device *dev)
{
	return NULL;
}

static inline int
__reset_control_bulk_get(struct device *dev, int num_rstcs,
			 struct reset_control_bulk_data *rstcs,
			 bool optional)
{
	return optional ? 0 : -EOPNOTSUPP;
}

#endif /* CONFIG_RESET_CONTROLLER */

static inline struct reset_control *reset_control_get_optional(struct device *dev,
							       const char *id)
{
	return __reset_control_get(dev, id, true);
}

static inline struct reset_control *reset_control_get(struct device *dev,
						      const char *id)
{
	return __reset_control_get(dev, id, false);
}

/**
 * reset_control_bulk_get - Lookup and obtain references to
 *                                    multiple reset controllers.
 * @dev: device to be reset by the controller
 * @num_rstcs: number of entries in rstcs array
 * @rstcs: array of struct reset_control_bulk_data with reset line names set
 *
 * Fills the rstcs array with pointers to reset controls and
 * returns 0, or an IS_ERR() condition containing errno.
 */
static inline int __must_check
reset_control_bulk_get(struct device *dev, int num_rstcs,
		       struct reset_control_bulk_data *rstcs)
{
	return __reset_control_bulk_get(dev, num_rstcs, rstcs, false);
}

#endif
