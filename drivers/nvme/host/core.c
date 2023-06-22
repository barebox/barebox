// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>

#include "nvme.h"

int __nvme_submit_sync_cmd(struct nvme_ctrl *ctrl,
			   struct nvme_command *cmd,
			   union nvme_result *result,
			   void *buffer, unsigned bufflen,
			   unsigned timeout, int qid)
{
	return ctrl->ops->submit_sync_cmd(ctrl, cmd, result, buffer, bufflen,
					  timeout, qid);
}
EXPORT_SYMBOL_GPL(__nvme_submit_sync_cmd);

int nvme_submit_sync_cmd(struct nvme_ctrl *ctrl,
			 struct nvme_command *cmd,
			 void *buffer, unsigned bufflen)
{
	return __nvme_submit_sync_cmd(ctrl, cmd, NULL, buffer, bufflen, 0,
				      NVME_QID_ADMIN);
}
EXPORT_SYMBOL_GPL(nvme_submit_sync_cmd);

static int nvme_identify_ctrl(struct nvme_ctrl *dev, struct nvme_id_ctrl **id)
{
	struct nvme_command c = { };
	int error;

	/* gcc-4.4.4 (at least) has issues with initializers and anon unions */
	c.identify.opcode = nvme_admin_identify;
	c.identify.cns = NVME_ID_CNS_CTRL;

	*id = kmalloc(sizeof(struct nvme_id_ctrl), GFP_KERNEL);
	if (!*id)
		return -ENOMEM;

	error = nvme_submit_sync_cmd(dev, &c, *id,
				     sizeof(struct nvme_id_ctrl));
	if (error)
		kfree(*id);

	return error;
}

static int
nvme_set_features(struct nvme_ctrl *dev, unsigned fid, unsigned dword11,
		  void *buffer, size_t buflen, u32 *result)
{
	struct nvme_command c;
	union nvme_result res;
	int ret;

	memset(&c, 0, sizeof(c));
	c.features.opcode = nvme_admin_set_features;
	c.features.fid = cpu_to_le32(fid);
	c.features.dword11 = cpu_to_le32(dword11);

	ret = __nvme_submit_sync_cmd(dev, &c, &res, buffer, buflen, 0,
				     NVME_QID_ADMIN);
	if (ret >= 0 && result)
		*result = le32_to_cpu(res.u32);
	return ret;
}

int nvme_set_queue_count(struct nvme_ctrl *ctrl, int *count)
{
	u32 q_count = (*count - 1) | ((*count - 1) << 16);
	u32 result;
	int status, nr_io_queues;

	status = nvme_set_features(ctrl, NVME_FEAT_NUM_QUEUES, q_count, NULL, 0,
			&result);
	if (status < 0)
		return status;

	/*
	 * Degraded controllers might return an error when setting the queue
	 * count.  We still want to be able to bring them online and offer
	 * access to the admin queue, as that might be only way to fix them up.
	 */
	if (status > 0) {
		dev_err(ctrl->dev, "Could not set queue count (%d)\n", status);
		*count = 0;
	} else {
		nr_io_queues = min(result & 0xffff, result >> 16) + 1;
		*count = min(*count, nr_io_queues);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nvme_set_queue_count);

static int nvme_wait_ready(struct nvme_ctrl *ctrl, u64 cap, bool enabled)
{
	uint64_t start = get_time_ns();
	uint64_t timeout =
		((NVME_CAP_TIMEOUT(cap) + 1) * HZ / 2);
	u32 csts, bit = enabled ? NVME_CSTS_RDY : 0;
	int ret;

	while ((ret = ctrl->ops->reg_read32(ctrl, NVME_REG_CSTS, &csts)) == 0) {
		if (csts == ~0)
			return -ENODEV;
		if ((csts & NVME_CSTS_RDY) == bit)
			break;

		mdelay(100);

		if (is_timeout(start, timeout)) {
			dev_err(ctrl->dev,
				"Device not ready; aborting %s\n", enabled ?
						"initialisation" : "reset");
			return -ENODEV;
		}
	}

	return ret;
}

static int nvme_identify_ns_list(struct nvme_ctrl *dev, unsigned nsid, __le32 *ns_list)
{
	struct nvme_command c = { };

	c.identify.opcode = nvme_admin_identify;
	c.identify.cns = NVME_ID_CNS_NS_ACTIVE_LIST;
	c.identify.nsid = cpu_to_le32(nsid);
	return nvme_submit_sync_cmd(dev, &c, ns_list, NVME_IDENTIFY_DATA_SIZE);
}

static struct nvme_id_ns *nvme_identify_ns(struct nvme_ctrl *ctrl,
		unsigned nsid)
{
	struct nvme_id_ns *id;
	struct nvme_command c = { };
	int error;

	/* gcc-4.4.4 (at least) has issues with initializers and anon unions */
	c.identify.opcode = nvme_admin_identify;
	c.identify.nsid = cpu_to_le32(nsid);
	c.identify.cns = NVME_ID_CNS_NS;

	id = kmalloc(sizeof(*id), GFP_KERNEL);
	if (!id)
		return NULL;

	error = nvme_submit_sync_cmd(ctrl, &c, id, sizeof(*id));
	if (error) {
		dev_warn(ctrl->dev, "Identify namespace failed\n");
		kfree(id);
		return NULL;
	}

	return id;
}

static struct nvme_ns_head *nvme_alloc_ns_head(struct nvme_ctrl *ctrl,
		unsigned nsid, struct nvme_id_ns *id)
{
	static int instance = 1;
	struct nvme_ns_head *head;
	int ret = -ENOMEM;

	head = kzalloc(sizeof(*head), GFP_KERNEL);
	if (!head)
		goto out;

	head->instance = instance++;
	head->ns_id = nsid;

	return head;
out:
	return ERR_PTR(ret);
}

static int nvme_init_ns_head(struct nvme_ns *ns, unsigned nsid,
		struct nvme_id_ns *id)
{
	struct nvme_ctrl *ctrl = ns->ctrl;
	const bool is_shared = id->nmic & (1 << 0);
	struct nvme_ns_head *head = NULL;

	if (is_shared) {
		dev_info(ctrl->dev, "Skipping shared namespace %u\n", nsid);
		return -ENOTSUPP;
	}

	head = nvme_alloc_ns_head(ctrl, nsid, id);
	if (IS_ERR(head))
		return PTR_ERR(head);

	ns->head = head;

	return 0;
}

#define DISK_NAME_LEN			32

static void nvme_update_disk_info(struct block_device *blk, struct nvme_ns *ns,
				  struct nvme_id_ns *id)
{
	blk->blockbits = ns->lba_shift;
	blk->num_blocks = le64_to_cpup(&id->nsze);

	ns->readonly = id->nsattr & (1 << 0);
}

static void __nvme_revalidate_disk(struct block_device *blk,
				   struct nvme_id_ns *id)
{
	struct nvme_ns *ns = to_nvme_ns(blk);

	/*
	 * If identify namespace failed, use default 512 byte block size so
	 * block layer can use before failing read/write for 0 capacity.
	 */
	ns->lba_shift = id->lbaf[id->flbas & NVME_NS_FLBAS_LBA_MASK].ds;
	if (ns->lba_shift == 0)
		ns->lba_shift = 9;

	nvme_update_disk_info(blk, ns, id);
}

static void nvme_setup_rw(struct nvme_ns *ns, struct nvme_command *cmnd,
			  sector_t block, blkcnt_t num_block)
{
	cmnd->rw.nsid = cpu_to_le32(ns->head->ns_id);
	cmnd->rw.slba = cpu_to_le64(nvme_block_nr(ns, block));
	cmnd->rw.length = cpu_to_le16(num_block - 1);
	cmnd->rw.control = 0;
	cmnd->rw.dsmgmt = 0;
}

static void nvme_setup_flush(struct nvme_ns *ns, struct nvme_command *cmnd)
{
	memset(cmnd, 0, sizeof(*cmnd));
	cmnd->common.opcode = nvme_cmd_flush;
	cmnd->common.nsid = cpu_to_le32(ns->head->ns_id);
}

static int nvme_submit_sync_rw(struct nvme_ns *ns, struct nvme_command *cmnd,
			       void *buffer, sector_t block, blkcnt_t num_blocks)
{
	/*
	 * ns->ctrl->max_hw_sectors is in units of 512 bytes, so we
	 * need to make sure we adjust it to discovered lba_shift
	 */
	const u32 max_hw_sectors =
		ns->ctrl->max_hw_sectors >> (ns->lba_shift - 9);
	int ret;

	if (num_blocks > max_hw_sectors) {
		while (num_blocks) {
			const u32 chunk = min_t(blkcnt_t, num_blocks,
						max_hw_sectors);

			ret = nvme_submit_sync_rw(ns, cmnd, buffer, block,
						  chunk);
			if (ret)
				break;

			num_blocks -= chunk;
			buffer += chunk;
			block += chunk;
		}

		return ret;
	}

	nvme_setup_rw(ns, cmnd, block, num_blocks);

	ret = __nvme_submit_sync_cmd(ns->ctrl, cmnd, NULL, buffer,
				     num_blocks << ns->lba_shift,
				     0, NVME_QID_IO);

	if (ret) {
		dev_err(ns->ctrl->dev,
			"I/O failed: block: %llu, num blocks: %llu, status code type: %xh, status code %02xh\n",
			block, num_blocks, (ret >> 8) & 0xf,
			ret & 0xff);
		return -EIO;
	}

	return 0;
}


static int nvme_block_device_read(struct block_device *blk, void *buffer,
				  sector_t block, blkcnt_t num_blocks)
{
	struct nvme_ns *ns = to_nvme_ns(blk);
	struct nvme_command cmnd = { };

	cmnd.rw.opcode = nvme_cmd_read;

	return nvme_submit_sync_rw(ns, &cmnd, buffer, block, num_blocks);
}

static int __maybe_unused
nvme_block_device_write(struct block_device *blk, const void *buffer,
			sector_t block, blkcnt_t num_blocks)
{
	struct nvme_ns *ns = to_nvme_ns(blk);
	struct nvme_command cmnd = { };

	if (ns->readonly)
		return -EINVAL;

	cmnd.rw.opcode = nvme_cmd_write;

	return nvme_submit_sync_rw(ns, &cmnd, (void *)buffer, block,
				   num_blocks);
}

static int __maybe_unused nvme_block_device_flush(struct block_device *blk)
{
	struct nvme_ns *ns = to_nvme_ns(blk);
	struct nvme_command cmnd = { };

	nvme_setup_flush(ns, &cmnd);

	return __nvme_submit_sync_cmd(ns->ctrl, &cmnd, NULL, NULL,
				      0, 0, NVME_QID_IO);
}

static struct block_device_ops nvme_block_device_ops = {
	.read = nvme_block_device_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = nvme_block_device_write,
	.flush = nvme_block_device_flush,
#endif
};

static void nvme_alloc_ns(struct nvme_ctrl *ctrl, unsigned nsid)
{
	struct nvme_ns *ns;
	struct nvme_id_ns *id;
	char disk_name[DISK_NAME_LEN];
	int ret, flags;

	ns = kzalloc(sizeof(*ns), GFP_KERNEL);
	if (!ns)
		return;

	ns->ctrl = ctrl;
	ns->lba_shift = 9; /* set to a default value for 512 until
			    * disk is validated */

	id = nvme_identify_ns(ctrl, nsid);
	if (!id)
		goto out_free_ns;

	if (id->ncap == 0)
		goto out_free_id;

	if (nvme_init_ns_head(ns, nsid, id))
		goto out_free_id;

	nvme_set_disk_name(disk_name, ns, ctrl, &flags);

	ns->blk.dev = ctrl->dev;
	ns->blk.ops = &nvme_block_device_ops;
	ns->blk.cdev.name = strdup(disk_name);

	__nvme_revalidate_disk(&ns->blk, id);
	kfree(id);

	ret = blockdevice_register(&ns->blk);
	if (ret) {
		dev_err(ctrl->dev, "Cannot register block device (%d)\n", ret);
		goto out_free_id;
	}

	return;
out_free_id:
	kfree(id);
out_free_ns:
	kfree(ns);
}

static int nvme_scan_ns_list(struct nvme_ctrl *ctrl, unsigned nn)
{
	__le32 *ns_list;
	unsigned i, j, nsid, prev = 0, num_lists = DIV_ROUND_UP(nn, 1024);
	int ret = 0;

	ns_list = kzalloc(NVME_IDENTIFY_DATA_SIZE, GFP_KERNEL);
	if (!ns_list)
		return -ENOMEM;

	for (i = 0; i < num_lists; i++) {
		ret = nvme_identify_ns_list(ctrl, prev, ns_list);
		if (ret)
			goto out;

		for (j = 0; j < min(nn, 1024U); j++) {
			nsid = le32_to_cpu(ns_list[j]);
			if (!nsid)
				goto out;

			nvme_alloc_ns(ctrl, nsid);
		}
		nn -= j;
	}
 out:
	kfree(ns_list);
	return ret;
}

static void nvme_scan_ns_sequential(struct nvme_ctrl *ctrl, unsigned nn)
{
	unsigned i;

	for (i = 1; i <= nn; i++)
		nvme_alloc_ns(ctrl, i);
}

static void nvme_scan_work(struct nvme_ctrl *ctrl)
{
	struct nvme_id_ctrl *id;
	unsigned nn;

	if (nvme_identify_ctrl(ctrl, &id))
		return;

	nn = le32_to_cpu(id->nn);
	if (ctrl->vs >= NVME_VS(1, 1, 0)) {
		if (!nvme_scan_ns_list(ctrl, nn))
			goto out_free_id;
	}
	nvme_scan_ns_sequential(ctrl, nn);
out_free_id:
	kfree(id);
}

void nvme_start_ctrl(struct nvme_ctrl *ctrl)
{
	if (ctrl->queue_count > 1)
		nvme_scan_work(ctrl);
}
EXPORT_SYMBOL_GPL(nvme_start_ctrl);

/*
 * If the device has been passed off to us in an enabled state, just clear
 * the enabled bit.  The spec says we should set the 'shutdown notification
 * bits', but doing so may cause the device to complete commands to the
 * admin queue ... and we don't know what memory that might be pointing at!
 */
int nvme_disable_ctrl(struct nvme_ctrl *ctrl, u64 cap)
{
	int ret;

	ctrl->ctrl_config &= ~NVME_CC_SHN_MASK;
	ctrl->ctrl_config &= ~NVME_CC_ENABLE;

	ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
	if (ret)
		return ret;

	return nvme_wait_ready(ctrl, cap, false);
}
EXPORT_SYMBOL_GPL(nvme_disable_ctrl);

int nvme_enable_ctrl(struct nvme_ctrl *ctrl, u64 cap)
{
	/*
	 * Default to a 4K page size, with the intention to update this
	 * path in the future to accomodate architectures with differing
	 * kernel and IO page sizes.
	 */
	unsigned dev_page_min = NVME_CAP_MPSMIN(cap) + 12, page_shift = 12;
	int ret;

	if (page_shift < dev_page_min) {
		dev_err(ctrl->dev,
			"Minimum device page size %u too large for host (%u)\n",
			1 << dev_page_min, 1 << page_shift);
		return -ENODEV;
	}

	ctrl->page_size = 1 << page_shift;

	ctrl->ctrl_config = NVME_CC_CSS_NVM;
	ctrl->ctrl_config |= (page_shift - 12) << NVME_CC_MPS_SHIFT;
	ctrl->ctrl_config |= NVME_CC_AMS_RR | NVME_CC_SHN_NONE;
	ctrl->ctrl_config |= NVME_CC_IOSQES | NVME_CC_IOCQES;
	ctrl->ctrl_config |= NVME_CC_ENABLE;

	ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
	if (ret)
		return ret;
	return nvme_wait_ready(ctrl, cap, true);
}
EXPORT_SYMBOL_GPL(nvme_enable_ctrl);

int nvme_shutdown_ctrl(struct nvme_ctrl *ctrl)
{
	uint64_t start = get_time_ns();
	uint64_t timeout = SHUTDOWN_TIMEOUT;
	u32 csts;
	int ret;

	ctrl->ctrl_config &= ~NVME_CC_SHN_MASK;
	ctrl->ctrl_config |= NVME_CC_SHN_NORMAL;

	ret = ctrl->ops->reg_write32(ctrl, NVME_REG_CC, ctrl->ctrl_config);
	if (ret)
		return ret;

	while ((ret = ctrl->ops->reg_read32(ctrl, NVME_REG_CSTS, &csts)) == 0) {
		if ((csts & NVME_CSTS_SHST_MASK) == NVME_CSTS_SHST_CMPLT)
			break;

		mdelay(100);

		if (is_timeout(start, timeout)) {
			dev_err(ctrl->dev,
				"Device shutdown incomplete; abort shutdown\n");
			return -ENODEV;
		}
	}

	return ret;
}
EXPORT_SYMBOL_GPL(nvme_shutdown_ctrl);

#define NVME_ID_MAX_LEN		41

static void nvme_print(struct nvme_ctrl *ctrl, const char *prefix,
		       const char *_string, size_t _length)
{
	char string[NVME_ID_MAX_LEN];
	const size_t length = min(_length, sizeof(string) - 1);

	memcpy(string, _string, length);
	string[length - 1] = '\0';

	dev_info(ctrl->dev, "%s: %s\n", prefix, string);
}

static int nvme_init_subsystem(struct nvme_ctrl *ctrl, struct nvme_id_ctrl *id)
{
	nvme_print(ctrl, "serial",   id->sn, sizeof(id->sn));
	nvme_print(ctrl, "model",    id->mn, sizeof(id->mn));
	nvme_print(ctrl, "firmware", id->fr, sizeof(id->fr));

	return 0;
}

/*
 * Initialize the cached copies of the Identify data and various controller
 * register in our nvme_ctrl structure.  This should be called as soon as
 * the admin queue is fully up and running.
 */
int nvme_init_identify(struct nvme_ctrl *ctrl)
{
	struct nvme_id_ctrl *id;
	u64 cap;
	int ret, page_shift;
	u32 max_hw_sectors;

	ret = ctrl->ops->reg_read32(ctrl, NVME_REG_VS, &ctrl->vs);
	if (ret) {
		dev_err(ctrl->dev, "Reading VS failed (%d)\n", ret);
		return ret;
	}

	ret = ctrl->ops->reg_read64(ctrl, NVME_REG_CAP, &cap);
	if (ret) {
		dev_err(ctrl->dev, "Reading CAP failed (%d)\n", ret);
		return ret;
	}
	page_shift = NVME_CAP_MPSMIN(cap) + 12;

	ret = nvme_identify_ctrl(ctrl, &id);
	if (ret) {
		dev_err(ctrl->dev, "Identify Controller failed (%d)\n", ret);
		return -EIO;
	}

	ret = nvme_init_subsystem(ctrl, id);
	if (ret)
		return ret;

	if (id->mdts)
		max_hw_sectors = 1 << (id->mdts + page_shift - 9);
	else
		max_hw_sectors = UINT_MAX;
	ctrl->max_hw_sectors =
		min_not_zero(ctrl->max_hw_sectors, max_hw_sectors);

	kfree(id);
	return 0;
}
EXPORT_SYMBOL_GPL(nvme_init_identify);


/*
 * Initialize a NVMe controller structures.  This needs to be called during
 * earliest initialization so that we have the initialized structured around
 * during probing.
 */
int nvme_init_ctrl(struct nvme_ctrl *ctrl, struct device *dev,
		   const struct nvme_ctrl_ops *ops)
{
	static int instance = 0;

	ctrl->dev = dev;
	ctrl->ops = ops;
	ctrl->instance = instance++;

	return 0;
}
EXPORT_SYMBOL_GPL(nvme_init_ctrl);
