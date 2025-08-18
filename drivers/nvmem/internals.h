/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_NVMEM_INTERNALS_H
#define _LINUX_NVMEM_INTERNALS_H

#include <device.h>
#include <linux/nvmem-consumer.h>
#include <linux/nvmem-provider.h>

struct nvmem_device {
	const char		*name;
	struct device		dev;
	struct list_head	node;
	int			stride;
	int			word_size;
	int			ncells;
	int			users;
	size_t			size;
	bool			read_only;
	struct cdev		cdev;
	void			*priv;
	struct list_head	cells;
	nvmem_cell_post_process_t cell_post_process;
	int			(*reg_write)(void *ctx, unsigned int reg,
					     const void *val, size_t val_size);
	int			(*reg_read)(void *ctx, unsigned int reg,
					    void *val, size_t val_size);
	int			(*reg_protect)(void *ctx, unsigned int reg,
					       size_t bytes, int prot);
};

#endif  /* ifndef _LINUX_NVMEM_INTERNALS_H */
