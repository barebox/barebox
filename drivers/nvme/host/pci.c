// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <io.h>
#include <io-64-nonatomic-lo-hi.h>
#include <linux/pci.h>

#include <dma.h>

#include "nvme.h"

#define SQ_SIZE(depth)		(depth * sizeof(struct nvme_command))
#define CQ_SIZE(depth)		(depth * sizeof(struct nvme_completion))

#define NVME_MAX_KB_SZ	4096

static int io_queue_depth = 2;

struct nvme_dev;

/*
 * An NVM Express queue.  Each device has at least two (one for admin
 * commands and one for I/O commands).
 */
struct nvme_queue {
	struct nvme_dev *dev;
	struct nvme_request *req;
	struct nvme_command *sq_cmds;
	volatile struct nvme_completion *cqes;
	dma_addr_t sq_dma_addr;
	dma_addr_t cq_dma_addr;
	u32 __iomem *q_db;
	u16 q_depth;
	u16 sq_tail;
	u16 cq_head;
	u16 qid;
	u8 cq_phase;

	u16 counter;
};

/*
 * Represents an NVM Express device.  Each nvme_dev is a PCI function.
 */
struct nvme_dev {
	struct nvme_queue queues[NVME_QID_NUM];
	u32 __iomem *dbs;
	struct device *dev;
	unsigned online_queues;
	unsigned max_qid;
	int q_depth;
	u32 db_stride;
	void __iomem *bar;
	bool subsystem;
	struct nvme_ctrl ctrl;
	__le64 *prp_pool;
	unsigned int prp_pool_size;
	dma_addr_t prp_dma;
};

static inline struct nvme_dev *to_nvme_dev(struct nvme_ctrl *ctrl)
{
	return container_of(ctrl, struct nvme_dev, ctrl);
}

static int nvme_pci_setup_prps(struct nvme_dev *dev,
			       const struct nvme_request *req,
			       struct nvme_rw_command *cmnd)
{
	int length = req->buffer_len;
	const int page_size = dev->ctrl.page_size;
	dma_addr_t dma_addr = req->buffer_dma_addr;
	u32 offset = dma_addr & (page_size - 1);
	u64 prp1 = dma_addr;
	__le64 *prp_list;
	int i, nprps;
	dma_addr_t prp_dma;


	length -= (page_size - offset);
	if (length <= 0) {
		prp_dma = 0;
		goto done;
	}

	dma_addr += (page_size - offset);

	if (length <= page_size) {
		prp_dma = dma_addr;
		goto done;
	}

	nprps = DIV_ROUND_UP(length, page_size);
	if (nprps > dev->prp_pool_size) {
		dma_free_coherent(DMA_DEVICE_BROKEN,
				  dev->prp_pool, dev->prp_dma,
				  dev->prp_pool_size * sizeof(u64));
		dev->prp_pool_size = nprps;
		dev->prp_pool = dma_alloc_coherent(DMA_DEVICE_BROKEN,
						   nprps * sizeof(u64),
						   &dev->prp_dma);
	}

	prp_list = dev->prp_pool;
	prp_dma  = dev->prp_dma;

	i = 0;
	for (;;) {
		if (i == page_size >> 3) {
			__le64 *old_prp_list = prp_list;
			prp_list = &prp_list[i];
			prp_dma += page_size;
			prp_list[0] = old_prp_list[i - 1];
			old_prp_list[i - 1] = cpu_to_le64(prp_dma);
			i = 1;
		}

		prp_list[i++] = cpu_to_le64(dma_addr);
		dma_addr += page_size;
		length -= page_size;
		if (length <= 0)
			break;
	}

done:
	cmnd->dptr.prp1 = cpu_to_le64(prp1);
	cmnd->dptr.prp2 = cpu_to_le64(prp_dma);

	return 0;
}

static int nvme_map_data(struct nvme_dev *dev, struct nvme_request *req)
{
	if (!req->buffer || !req->buffer_len)
		return 0;

	req->buffer_dma_addr = dma_map_single(dev->dev, req->buffer,
					      req->buffer_len, req->dma_dir);
	if (dma_mapping_error(dev->dev, req->buffer_dma_addr))
		return -EFAULT;

	return nvme_pci_setup_prps(dev, req, &req->cmd->rw);
}

static void nvme_unmap_data(struct nvme_dev *dev, struct nvme_request *req)
{
	if (!req->buffer || !req->buffer_len)
		return;

	dma_unmap_single(dev->dev, req->buffer_dma_addr, req->buffer_len,
			 req->dma_dir);
}

static int nvme_alloc_queue(struct nvme_dev *dev, int qid, int depth)
{
	struct nvme_queue *nvmeq = &dev->queues[qid];

	if (dev->ctrl.queue_count > qid)
		return 0;

	nvmeq->cqes = dma_alloc_coherent(DMA_DEVICE_BROKEN,
					 CQ_SIZE(depth),
					 &nvmeq->cq_dma_addr);
	if (!nvmeq->cqes)
		goto free_nvmeq;

	nvmeq->sq_cmds = dma_alloc_coherent(DMA_DEVICE_BROKEN,
					    SQ_SIZE(depth),
					    &nvmeq->sq_dma_addr);
	if (!nvmeq->sq_cmds)
		goto free_cqdma;

	nvmeq->dev = dev;
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
	nvmeq->q_depth = depth;
	nvmeq->qid = qid;
	dev->ctrl.queue_count++;

	return 0;

 free_cqdma:
	dma_free_coherent(DMA_DEVICE_BROKEN,
			  (void *)nvmeq->cqes, nvmeq->cq_dma_addr,
			  CQ_SIZE(depth));
 free_nvmeq:
	return -ENOMEM;
}

static int adapter_delete_queue(struct nvme_dev *dev, u8 opcode, u16 id)
{
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.delete_queue.opcode = opcode;
	c.delete_queue.qid = cpu_to_le16(id);

	return nvme_submit_sync_cmd(&dev->ctrl, &c, NULL, 0);
}

static int adapter_alloc_cq(struct nvme_dev *dev, u16 qid,
		struct nvme_queue *nvmeq, s16 vector)
{
	struct nvme_command c;
	int flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

	/*
	 * Note: we (ab)use the fact that the prp fields survive if no data
	 * is attached to the request.
	 */
	memset(&c, 0, sizeof(c));
	c.create_cq.opcode = nvme_admin_create_cq;
	c.create_cq.prp1 = cpu_to_le64(nvmeq->cq_dma_addr);
	c.create_cq.cqid = cpu_to_le16(qid);
	c.create_cq.qsize = cpu_to_le16(nvmeq->q_depth - 1);
	c.create_cq.cq_flags = cpu_to_le16(flags);
	c.create_cq.irq_vector = cpu_to_le16(vector);

	return nvme_submit_sync_cmd(&dev->ctrl, &c, NULL, 0);
}

static int adapter_alloc_sq(struct nvme_dev *dev, u16 qid,
			    struct nvme_queue *nvmeq)
{
	struct nvme_command c;
	int flags = NVME_QUEUE_PHYS_CONTIG;

	/*
	 * Note: we (ab)use the fact that the prp fields survive if no data
	 * is attached to the request.
	 */
	memset(&c, 0, sizeof(c));
	c.create_sq.opcode = nvme_admin_create_sq;
	c.create_sq.prp1 = cpu_to_le64(nvmeq->sq_dma_addr);
	c.create_sq.sqid = cpu_to_le16(qid);
	c.create_sq.qsize = cpu_to_le16(nvmeq->q_depth - 1);
	c.create_sq.sq_flags = cpu_to_le16(flags);
	c.create_sq.cqid = cpu_to_le16(qid);

	return nvme_submit_sync_cmd(&dev->ctrl, &c, NULL, 0);
}

static int adapter_delete_cq(struct nvme_dev *dev, u16 cqid)
{
	return adapter_delete_queue(dev, nvme_admin_delete_cq, cqid);
}

static void nvme_init_queue(struct nvme_queue *nvmeq, u16 qid)
{
	struct nvme_dev *dev = nvmeq->dev;

	nvmeq->sq_tail = 0;
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
	dev->online_queues++;
}

static int nvme_create_queue(struct nvme_queue *nvmeq, int qid)
{
	struct nvme_dev *dev = nvmeq->dev;
	int result;
	s16 vector;

	vector = 0;
	result = adapter_alloc_cq(dev, qid, nvmeq, vector);
	if (result)
		return result;

	result = adapter_alloc_sq(dev, qid, nvmeq);
	if (result < 0)
		return result;
	else if (result)
		goto release_cq;

	nvme_init_queue(nvmeq, qid);

	return result;

release_cq:
	adapter_delete_cq(dev, qid);
	return result;
}

/**
 * nvme_submit_cmd() - Copy a command into a queue and ring the doorbell
 * @nvmeq: The queue to use
 * @cmd: The command to send
 */
static void nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
	memcpy(&nvmeq->sq_cmds[nvmeq->sq_tail], cmd, sizeof(*cmd));

	if (++nvmeq->sq_tail == nvmeq->q_depth)
		nvmeq->sq_tail = 0;
	writel(nvmeq->sq_tail, nvmeq->q_db);
}

/* We read the CQE phase first to check if the rest of the entry is valid */
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
	return (le16_to_cpu(nvmeq->cqes[nvmeq->cq_head].status) & 1) ==
			nvmeq->cq_phase;
}

static inline void nvme_ring_cq_doorbell(struct nvme_queue *nvmeq)
{
	u16 head = nvmeq->cq_head;

	writel(head, nvmeq->q_db + nvmeq->dev->db_stride);
}

static inline void nvme_handle_cqe(struct nvme_queue *nvmeq, u16 idx)
{
	volatile struct nvme_completion *cqe = &nvmeq->cqes[idx];
	struct nvme_request *req = nvmeq->req;

	if (unlikely(cqe->command_id >= nvmeq->q_depth)) {
		dev_warn(nvmeq->dev->ctrl.dev,
			"invalid id %d completed on queue %d\n",
			cqe->command_id, le16_to_cpu(cqe->sq_id));
		return;
	}

	if (WARN_ON(cqe->command_id != req->cmd->common.command_id))
		return;

	nvme_end_request(req, cqe->status, cqe->result);
}

static void nvme_complete_cqes(struct nvme_queue *nvmeq, u16 start, u16 end)
{
	while (start != end) {
		nvme_handle_cqe(nvmeq, start);
		if (++start == nvmeq->q_depth)
			start = 0;
	}
}

static inline void nvme_update_cq_head(struct nvme_queue *nvmeq)
{
	if (++nvmeq->cq_head == nvmeq->q_depth) {
		nvmeq->cq_head = 0;
		nvmeq->cq_phase = !nvmeq->cq_phase;
	}
}

static inline bool nvme_process_cq(struct nvme_queue *nvmeq, u16 *start,
		u16 *end, int tag)
{
	bool found = false;

	*start = nvmeq->cq_head;
	while (!found && nvme_cqe_pending(nvmeq)) {
		if (nvmeq->cqes[nvmeq->cq_head].command_id == tag)
			found = true;
		nvme_update_cq_head(nvmeq);
	}
	*end = nvmeq->cq_head;

	if (*start != *end)
		nvme_ring_cq_doorbell(nvmeq);
	return found;
}

static bool nvme_poll(struct nvme_queue *nvmeq, unsigned int tag)
{
	u16 start, end;
	bool found;

	if (!nvme_cqe_pending(nvmeq))
		return false;

	found = nvme_process_cq(nvmeq, &start, &end, tag);

	nvme_complete_cqes(nvmeq, start, end);
	return found;
}

static int nvme_pci_submit_sync_cmd(struct nvme_ctrl *ctrl,
				    struct nvme_command *cmd,
				    union nvme_result *result,
				    void *buffer,
				    unsigned int buffer_len,
				    unsigned timeout, int qid)
{
	struct nvme_dev *dev = to_nvme_dev(ctrl);
	struct nvme_queue *nvmeq = &dev->queues[qid];
	struct nvme_request req = { };
	const u16 tag = nvmeq->counter++ & (nvmeq->q_depth - 1);
	enum dma_data_direction dma_dir;
	int ret;

	switch (qid) {
	case NVME_QID_ADMIN:
		switch (cmd->common.opcode) {
		case nvme_admin_create_sq:
		case nvme_admin_create_cq:
		case nvme_admin_delete_sq:
		case nvme_admin_delete_cq:
		case nvme_admin_set_features:
			dma_dir = DMA_TO_DEVICE;
			break;
		case nvme_admin_identify:
			dma_dir = DMA_FROM_DEVICE;
			break;
		default:
			return -EINVAL;
		}
		break;
	case NVME_QID_IO:
		switch (cmd->rw.opcode) {
		case nvme_cmd_write:
			dma_dir = DMA_TO_DEVICE;
			break;
		case nvme_cmd_read:
			dma_dir = DMA_FROM_DEVICE;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	cmd->common.command_id = tag;

	timeout = timeout ?: ADMIN_TIMEOUT;

	req.cmd        = cmd;
	req.buffer     = buffer;
	req.buffer_len = buffer_len;
	req.dma_dir    = dma_dir;

	ret = nvme_map_data(dev, &req);
	if (ret) {
		dev_err(dev->dev, "Failed to map request data\n");
		return ret;
	}

	nvme_submit_cmd(nvmeq, cmd);

	nvmeq->req = &req;
	ret = wait_on_timeout(timeout, nvme_poll(nvmeq, tag));
	nvmeq->req = NULL;

	nvme_unmap_data(dev, &req);

	if (result)
		*result = req.result;

	return ret ?: req.status;
}

static int nvme_pci_configure_admin_queue(struct nvme_dev *dev)
{
	int result;
	u32 aqa;
	struct nvme_queue *nvmeq;

	dev->subsystem = readl(dev->bar + NVME_REG_VS) >= NVME_VS(1, 1, 0) ?
				NVME_CAP_NSSRC(dev->ctrl.cap) : 0;

	if (dev->subsystem &&
	    (readl(dev->bar + NVME_REG_CSTS) & NVME_CSTS_NSSRO))
		writel(NVME_CSTS_NSSRO, dev->bar + NVME_REG_CSTS);

	result = nvme_disable_ctrl(&dev->ctrl, dev->ctrl.cap);
	if (result < 0)
		return result;

	result = nvme_alloc_queue(dev, NVME_QID_ADMIN, NVME_AQ_DEPTH);
	if (result)
		return result;

	nvmeq = &dev->queues[NVME_QID_ADMIN];
	aqa = nvmeq->q_depth - 1;
	aqa |= aqa << 16;

	writel(aqa, dev->bar + NVME_REG_AQA);
	writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ);
	writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ);

	result = nvme_enable_ctrl(&dev->ctrl, dev->ctrl.cap);
	if (result)
		return result;

	nvme_init_queue(nvmeq, NVME_QID_ADMIN);

	return result;
}

static int nvme_create_io_queues(struct nvme_dev *dev)
{
	unsigned i, max;
	int ret = 0;

	for (i = dev->ctrl.queue_count; i <= dev->max_qid; i++) {
		if (nvme_alloc_queue(dev, i, dev->q_depth)) {
			ret = -ENOMEM;
			break;
		}
	}

	max = min(dev->max_qid, dev->ctrl.queue_count - 1);
	for (i = dev->online_queues; i <= max; i++) {
		ret = nvme_create_queue(&dev->queues[i], i);
		if (ret)
			break;
	}

	/*
	 * Ignore failing Create SQ/CQ commands, we can continue with less
	 * than the desired amount of queues, and even a controller without
	 * I/O queues can still be used to issue admin commands.  This might
	 * be useful to upgrade a buggy firmware for example.
	 */
	return ret >= 0 ? 0 : ret;
}

static int nvme_setup_io_queues(struct nvme_dev *dev)
{
	int result, nr_io_queues;

	nr_io_queues = NVME_QID_NUM - 1;
	result = nvme_set_queue_count(&dev->ctrl, &nr_io_queues);
	if (result < 0)
		return result;

	dev->max_qid = nr_io_queues;

	return nvme_create_io_queues(dev);
}

static int nvme_pci_enable(struct nvme_dev *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	if (pci_enable_device(pdev))
		return -ENOMEM;

	pci_set_master(pdev);

	if (readl(dev->bar + NVME_REG_CSTS) == -1)
		return -ENODEV;

	dev->ctrl.cap = readq(dev->bar + NVME_REG_CAP);

	dev->q_depth = min_t(int, NVME_CAP_MQES(dev->ctrl.cap) + 1,
			     io_queue_depth);
	dev->db_stride = 1 << NVME_CAP_STRIDE(dev->ctrl.cap);
	dev->dbs = dev->bar + 4096;

	return 0;
}

static void nvme_reset_work(struct nvme_dev *dev)
{
	int result = -ENODEV;

	result = nvme_pci_enable(dev);
	if (result)
		goto out;

	result = nvme_pci_configure_admin_queue(dev);
	if (result)
		goto out;

	/*
	 * Limit the max command size to prevent iod->sg allocations going
	 * over a single page.
	 */
	dev->ctrl.max_hw_sectors = NVME_MAX_KB_SZ << 1;

	result = nvme_init_identify(&dev->ctrl);
	if (result)
		goto out;

	result = nvme_setup_io_queues(dev);
	if (result) {
		dev_err(dev->ctrl.dev, "IO queues not created\n");
		goto out;
	}

	nvme_start_ctrl(&dev->ctrl);
out:
	return;
}

static int nvme_pci_reg_read32(struct nvme_ctrl *ctrl, u32 off, u32 *val)
{
	*val = readl(to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static int nvme_pci_reg_write32(struct nvme_ctrl *ctrl, u32 off, u32 val)
{
	writel(val, to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static int nvme_pci_reg_read64(struct nvme_ctrl *ctrl, u32 off, u64 *val)
{
	*val = readq(to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static const struct nvme_ctrl_ops nvme_pci_ctrl_ops = {
	.reg_read32		= nvme_pci_reg_read32,
	.reg_write32		= nvme_pci_reg_write32,
	.reg_read64		= nvme_pci_reg_read64,
	.submit_sync_cmd	= nvme_pci_submit_sync_cmd,
};

static void nvme_dev_map(struct nvme_dev *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	dev->bar = pci_iomap(pdev, 0);
}

static void nvme_delete_queue(struct nvme_queue *nvmeq, u8 opcode)
{
	int ret;
	ret = adapter_delete_queue(nvmeq->dev, opcode, nvmeq->qid);
	if (ret < 0)
		dev_err(nvmeq->dev->dev, "%s: %s\n", __func__,
			strerror(-ret));
	else if (ret)
		dev_err(nvmeq->dev->dev,
			"%s: status code type: %xh, status code %02xh\n",
			__func__, (ret >> 8) & 0xf, ret & 0xff);
}

static void nvme_disable_io_queues(struct nvme_dev *dev)
{
	int i, queues = dev->online_queues - 1;

	for (i = queues; i > 0; i--) {
		nvme_delete_queue(&dev->queues[i], nvme_admin_delete_sq);
		nvme_delete_queue(&dev->queues[i], nvme_admin_delete_cq);
	}
}

static void nvme_disable_admin_queue(struct nvme_dev *dev)
{
	struct nvme_queue *nvmeq = &dev->queues[0];
	u16 start, end;

	nvme_shutdown_ctrl(&dev->ctrl);
	nvme_process_cq(nvmeq, &start, &end, -1);
	nvme_complete_cqes(nvmeq, start, end);
}

static int nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct nvme_dev *dev;
	int result;

	dev = xzalloc(sizeof(*dev));
	dev->dev = &pdev->dev;
	pdev->dev.priv = dev;

	nvme_dev_map(dev);
	result = nvme_init_ctrl(&dev->ctrl, &pdev->dev, &nvme_pci_ctrl_ops);
	if (result)
		return result;

	nvme_reset_work(dev);

	return 0;
}

static void nvme_remove(struct pci_dev *pdev)
{
	struct nvme_dev *dev = pdev->dev.priv;
	bool dead = true;

	u32 csts = readl(dev->bar + NVME_REG_CSTS);

	dead = !!((csts & NVME_CSTS_CFS) || !(csts & NVME_CSTS_RDY));

	if (!dead && dev->ctrl.queue_count > 0) {
		nvme_disable_io_queues(dev);
		nvme_disable_admin_queue(dev);
	}
}

static const struct pci_device_id nvme_id_table[] = {
	{ PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, PCI_ANY_ID) },
	{ 0, },
};

static struct pci_driver nvme_driver = {
	.name		= "nvme",
	.id_table	= nvme_id_table,
	.probe		= nvme_probe,
	.remove		= nvme_remove,
};
device_pci_driver(nvme_driver);
