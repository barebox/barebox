/*
 * Copyright (c) 2011-2014, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef _NVME_H
#define _NVME_H

#include <linux/nvme.h>
#include <dma.h>
#include <block.h>

#define ADMIN_TIMEOUT		(60 * HZ)
#define SHUTDOWN_TIMEOUT	( 5 * HZ)

/*
 * Common request structure for NVMe passthrough.  All drivers must have
 * this structure as the first member of their request-private data.
 */
struct nvme_request {
	struct nvme_command	*cmd;
	union nvme_result	result;
	u16			status;

	void *buffer;
	unsigned int buffer_len;
	dma_addr_t buffer_dma_addr;
	enum dma_data_direction dma_dir;
};

struct nvme_ctrl {
	const struct nvme_ctrl_ops *ops;
	struct device_d *dev;
	int instance;

	u32 ctrl_config;
	u32 queue_count;
	u64 cap;
	u32 page_size;
	u32 max_hw_sectors;
	u32 vs;
};

/*
 * Anchor structure for namespaces.  There is one for each namespace in a
 * NVMe subsystem that any of our controllers can see, and the namespace
 * structure for each controller is chained of it.  For private namespaces
 * there is a 1:1 relation to our namespace structures, that is ->list
 * only ever has a single entry for private namespaces.
 */
struct nvme_ns_head {
	unsigned		ns_id;
	int			instance;
};

struct nvme_ns {
	struct nvme_ctrl *ctrl;
	struct nvme_ns_head *head;
	struct block_device blk;

	int lba_shift;
	bool readonly;
};

static inline struct nvme_ns *to_nvme_ns(struct block_device *blk)
{
	return container_of(blk, struct nvme_ns, blk);
}

struct nvme_ctrl_ops {
	int (*reg_read32)(struct nvme_ctrl *ctrl, u32 off, u32 *val);
	int (*reg_write32)(struct nvme_ctrl *ctrl, u32 off, u32 val);
	int (*reg_read64)(struct nvme_ctrl *ctrl, u32 off, u64 *val);

	int (*submit_sync_cmd)(struct nvme_ctrl *ctrl,
			       struct nvme_command *cmd,
			       union nvme_result *result,
			       void *buffer,
			       unsigned bufflen,
			       unsigned timeout, int qid);
};

static inline bool nvme_ctrl_ready(struct nvme_ctrl *ctrl)
{
	u32 val = 0;

	if (ctrl->ops->reg_read32(ctrl, NVME_REG_CSTS, &val))
		return false;
	return val & NVME_CSTS_RDY;
}

static inline u64 nvme_block_nr(struct nvme_ns *ns, sector_t sector)
{
	return (sector >> (ns->lba_shift - 9));
}

static inline void nvme_end_request(struct nvme_request *rq, __le16 status,
		union nvme_result result)
{
	rq->status = le16_to_cpu(status) >> 1;
	rq->result = result;
}

int nvme_disable_ctrl(struct nvme_ctrl *ctrl, u64 cap);
int nvme_enable_ctrl(struct nvme_ctrl *ctrl, u64 cap);
int nvme_shutdown_ctrl(struct nvme_ctrl *ctrl);
int nvme_init_ctrl(struct nvme_ctrl *ctrl, struct device_d *dev,
		   const struct nvme_ctrl_ops *ops);
void nvme_start_ctrl(struct nvme_ctrl *ctrl);
int nvme_init_identify(struct nvme_ctrl *ctrl);

enum nvme_queue_id {
	NVME_QID_ADMIN,
	NVME_QID_IO,
	NVME_QID_NUM,
	NVME_QID_ANY = -1,
};

int __nvme_submit_sync_cmd(struct nvme_ctrl *ctrl,
			   struct nvme_command *cmd,
			   union nvme_result *result,
			   void *buffer, unsigned bufflen,
			   unsigned timeout, int qid);
int nvme_submit_sync_cmd(struct nvme_ctrl *ctrl,
			 struct nvme_command *cmd,
			 void *buffer, unsigned bufflen);


int nvme_set_queue_count(struct nvme_ctrl *ctrl, int *count);
/*
 * Without the multipath code enabled, multiple controller per subsystems are
 * visible as devices and thus we cannot use the subsystem instance.
 */
static inline void nvme_set_disk_name(char *disk_name, struct nvme_ns *ns,
				      struct nvme_ctrl *ctrl, int *flags)
{
	sprintf(disk_name, "nvme%dn%d", ctrl->instance, ns->head->instance);
}

#endif /* _NVME_H */
