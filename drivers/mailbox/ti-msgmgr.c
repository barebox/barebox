// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments' K3 Secure proxy Driver
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 */

#include <common.h>
#include <mailbox.h>
#include <soc/ti/k3-sec-proxy.h>

/* SEC PROXY RT THREAD STATUS */
#define RT_THREAD_STATUS			0x0
#define RT_THREAD_THRESHOLD			0x4
#define RT_THREAD_STATUS_ERROR_SHIFT		31
#define RT_THREAD_STATUS_ERROR_MASK		BIT(31)
#define RT_THREAD_STATUS_CUR_CNT_SHIFT		0
#define RT_THREAD_STATUS_CUR_CNT_MASK		GENMASK(7, 0)

/* SEC PROXY SCFG THREAD CTRL */
#define SCFG_THREAD_CTRL			0x1000
#define SCFG_THREAD_CTRL_DIR_SHIFT		31
#define SCFG_THREAD_CTRL_DIR_MASK		BIT(31)

#define SEC_PROXY_THREAD(base, x)		((base) + (0x1000 * (x)))
#define THREAD_IS_RX				1
#define THREAD_IS_TX				0

/**
 * struct k3_sec_proxy_desc - Description of secure proxy integration.
 * @thread_count:	Number of Threads.
 * @max_msg_size:	Message size in bytes.
 * @data_start_offset:	Offset of the First data register of the thread
 * @data_end_offset:	Offset of the Last data register of the thread
 * @valid_threads:	List of Valid threads that the processor can access
 * @num_valid_threads:	Number of valid threads.
 */
struct k3_sec_proxy_desc {
	u16 thread_count;
	u16 max_msg_size;
	u16 data_start_offset;
	u16 data_end_offset;
	const u32 *valid_threads;
	u32 num_valid_threads;
};

/**
 * struct k3_sec_proxy_thread - Description of a secure proxy Thread
 * @id:		Thread ID
 * @data:	Thread Data path region for target
 * @scfg:	Secure Config Region for Thread
 * @rt:		RealTime Region for Thread
 * @rx_buf:	Receive buffer data, max message size.
 */
struct k3_sec_proxy_thread {
	u32 id;
	void __iomem *data;
	void __iomem *scfg;
	void __iomem *rt;
	u32 *rx_buf;
};

/**
 * struct k3_sec_proxy_mbox - Description of a Secure Proxy Instance
 * @chan:		Mailbox Channel
 * @desc:		Description of the SoC integration
 * @chans:		Array for valid thread instances
 * @target_data:	Secure Proxy region for Target Data
 * @scfg:		Secure Proxy Region for Secure configuration.
 * @rt:			Secure proxy Region for Real Time Region.
 */
struct k3_sec_proxy_mbox {
	struct device *dev;
	struct mbox_controller mbox;
	const struct k3_sec_proxy_desc *desc;
	struct k3_sec_proxy_thread *k3_chans;
	void __iomem *target_data;
	void __iomem *scfg;
	void __iomem *rt;
};

static inline u32 sp_readl(void __iomem *addr, unsigned int offset)
{
	return readl(addr + offset);
}

static inline void sp_writel(void __iomem *addr, unsigned int offset, u32 data)
{
	writel(data, addr + offset);
}

/**
 * k3_sec_proxy_request() - Request for mailbox channel
 * @chan:	Channel Pointer
 */
static int k3_sec_proxy_request(struct mbox_chan *chan)
{
	dev_dbg(chan->dev, "%s(chan=%p)\n", __func__, chan);

	return 0;
}

/**
 * k3_sec_proxy_free() - Free the mailbox channel
 * @chan:	Channel Pointer
 */
static int k3_sec_proxy_free(struct mbox_chan *chan)
{
	dev_dbg(chan->dev, "%s(chan=%p)\n", __func__, chan);

	return 0;
}

/**
 * k3_sec_proxy_verify_thread() - Verify thread status before
 *				  sending/receiving data.
 * @spt:	pointer to secure proxy thread description
 * @dir:	Direction of the thread
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static inline int k3_sec_proxy_verify_thread(struct mbox_chan *chan, u8 dir)
{
	struct k3_sec_proxy_thread *spt = chan->con_priv;

	/* Check for any errors already available */
	if (sp_readl(spt->rt, RT_THREAD_STATUS) &
	    RT_THREAD_STATUS_ERROR_MASK) {
		dev_err(chan->dev, "%s: Thread %d is corrupted, cannot send data.\n",
		       __func__, spt->id);
		return -EINVAL;
	}

	/* Make sure thread is configured for right direction */
	if ((sp_readl(spt->scfg, SCFG_THREAD_CTRL)
	    & SCFG_THREAD_CTRL_DIR_MASK) >> SCFG_THREAD_CTRL_DIR_SHIFT != dir) {
		if (dir)
			dev_err(chan->dev, "%s: Trying to receive data on tx Thread %d\n",
			       __func__, spt->id);
		else
			dev_err(chan->dev, "%s: Trying to send data on rx Thread %d\n",
			       __func__, spt->id);
		return -EINVAL;
	}

	/* Check the message queue before sending/receiving data */
	if (!(sp_readl(spt->rt, RT_THREAD_STATUS) &
	      RT_THREAD_STATUS_CUR_CNT_MASK))
		return -ENODATA;

	return 0;
}

/**
 * k3_sec_proxy_send() - Send data via mailbox channel
 * @chan:	Channel Pointer
 * @data:	Pointer to k3_sec_proxy_msg
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_sec_proxy_send(struct mbox_chan *chan, const void *data)
{
	const struct k3_sec_proxy_msg *msg = (struct k3_sec_proxy_msg *)data;
	struct mbox_controller *mbox = chan->mbox;
	struct k3_sec_proxy_mbox *spm = container_of(mbox, struct k3_sec_proxy_mbox, mbox);
	struct k3_sec_proxy_thread *spt = chan->con_priv;
	int num_words, trail_bytes, ret;
	void __iomem *data_reg;
	u32 *word_data;

	dev_dbg(chan->dev, "%s(chan=%p, data=%p)\n", __func__, chan, data);

	ret = k3_sec_proxy_verify_thread(chan, THREAD_IS_TX);
	if (ret) {
		dev_err(chan->dev,
			"%s: Thread%d verification failed. ret = %d\n",
			__func__, spt->id, ret);
		return ret;
	}

	/* Check the message size. */
	if (msg->len > spm->desc->max_msg_size)
		return -EINVAL;

	/* Send the message */
	data_reg = spt->data + spm->desc->data_start_offset;
	for (num_words = msg->len / sizeof(u32), word_data = (u32 *)msg->buf;
	     num_words;
	     num_words--, data_reg += sizeof(u32), word_data++)
		writel(*word_data, data_reg);

	trail_bytes = msg->len % sizeof(u32);
	if (trail_bytes) {
		u32 data_trail = *word_data;

		/* Ensure all unused data is 0 */
		data_trail &= 0xFFFFFFFF >> (8 * (sizeof(u32) - trail_bytes));
		writel(data_trail, data_reg);
		data_reg++;
	}

	/*
	 * 'data_reg' indicates next register to write. If we did not already
	 * write on tx complete reg(last reg), we must do so for transmit
	 */
	if (data_reg <= (spt->data + spm->desc->data_end_offset))
		sp_writel(spt->data, spm->desc->data_end_offset, 0);

	return 0;
}

/**
 * k3_sec_proxy_recv() - Receive data via mailbox channel
 * @chan:	Channel Pointer
 * @data:	Pointer to k3_sec_proxy_msg
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int k3_sec_proxy_recv(struct mbox_chan *chan, void *data)
{
	struct mbox_controller *mbox = chan->mbox;
	struct k3_sec_proxy_mbox *spm = container_of(mbox, struct k3_sec_proxy_mbox, mbox);
	struct k3_sec_proxy_thread *spt = chan->con_priv;
	struct k3_sec_proxy_msg *msg = data;
	void __iomem *data_reg;
	int num_words, ret;
	u32 *word_data;

	dev_dbg(chan->dev, "%s(chan=%p, data=%p)\n", __func__, chan, data);

	ret = k3_sec_proxy_verify_thread(chan, THREAD_IS_RX);
	if (ret)
		return ret;

	msg->len = spm->desc->max_msg_size;
	msg->buf = spt->rx_buf;
	data_reg = spt->data + spm->desc->data_start_offset;
	word_data = spt->rx_buf;
	for (num_words = spm->desc->max_msg_size / sizeof(u32);
	     num_words;
	     num_words--, data_reg += sizeof(u32), word_data++)
		*word_data = readl(data_reg);

	return 0;
}

static struct mbox_chan *k3_sec_proxy_of_xlate(struct mbox_controller *mbox,
					       const struct of_phandle_args *p)
{
	struct k3_sec_proxy_mbox *spm = container_of(mbox, struct k3_sec_proxy_mbox, mbox);
	int ncells = 1;
	int req_pid;
	int i;

	if (p->args_count != ncells) {
		dev_err(mbox->dev, "Invalid arguments in dt[%d]. Must be %d\n",
			p->args_count, ncells);
		return ERR_PTR(-EINVAL);
	}

	req_pid = p->args[0];

	for (i = 0; i < spm->desc->num_valid_threads; i++) {
		struct mbox_chan *chan;

		if (spm->k3_chans[i].id == req_pid) {
			chan = &spm->mbox.chans[i];
			return chan;
		}
	}

	return NULL;
}

static struct mbox_chan_ops k3_sec_proxy_mbox_ops = {
	.request = k3_sec_proxy_request,
	.rfree = k3_sec_proxy_free,
	.send = k3_sec_proxy_send,
	.recv = k3_sec_proxy_recv,
};

/**
 * k3_sec_proxy_thread_setup - Initialize the parameters for all valid threads
 * @spm:	Mailbox instance for which threads needs to be initialized
 *
 * Return: 0 if all went ok, else corresponding error message
 */
static int k3_sec_proxy_thread_setup(struct k3_sec_proxy_mbox *spm)
{
	struct k3_sec_proxy_thread *spt;
	int i, ind;

	for (i = 0; i < spm->desc->num_valid_threads; i++) {
		spt = &spm->k3_chans[i];
		ind = spm->desc->valid_threads[i];
		spt->id = ind;
		spt->data = (void *)SEC_PROXY_THREAD(spm->target_data, ind);
		spt->scfg = (void *)SEC_PROXY_THREAD(spm->scfg, ind);
		spt->rt = (void *)SEC_PROXY_THREAD(spm->rt, ind);
		spt->rx_buf = xzalloc(spm->desc->max_msg_size);
	}

	return 0;
}

/**
 * k3_sec_proxy_probe() - Basic probe
 * @dev:	corresponding mailbox device
 *
 * Return: 0 if all went ok, else corresponding error message
 */
static int k3_sec_proxy_probe(struct device *dev)
{
	struct k3_sec_proxy_mbox *spm;
	struct mbox_controller *mbox;
	struct resource *res;
	const void *data;
	int i, ret;

	spm = xzalloc(sizeof(*spm));

	spm->dev = dev;

	res = dev_request_mem_resource_by_name(spm->dev, "target_data");
	if (IS_ERR(res))
		return PTR_ERR(res);

	spm->target_data = IOMEM(res->start);

	res = dev_request_mem_resource_by_name(spm->dev, "scfg");
	if (IS_ERR(res))
		return PTR_ERR(res);

	spm->scfg = IOMEM(res->start);

	res = dev_request_mem_resource_by_name(spm->dev, "rt");
	if (IS_ERR(res))
		return PTR_ERR(res);

	spm->rt = IOMEM(res->start);

	ret = dev_get_drvdata(dev, &data);
        if (ret)
                return ret;

	spm->desc = data;
	spm->k3_chans = xzalloc(spm->desc->num_valid_threads * sizeof(*spm->k3_chans));

	ret = k3_sec_proxy_thread_setup(spm);
	if (ret) {
		dev_err(dev, "secure proxy thread setup failed\n");
		return ret;
	}

	mbox = &spm->mbox;
	mbox->dev = dev;
	mbox->ops = &k3_sec_proxy_mbox_ops;
	mbox->of_xlate = k3_sec_proxy_of_xlate;
	mbox->chans = xzalloc(spm->desc->num_valid_threads * sizeof(*mbox->chans));
	mbox->num_chans = spm->desc->num_valid_threads;

	for (i = 0; i < spm->desc->num_valid_threads; i++) {
		mbox->chans[i].dev = dev;
		mbox->chans[i].mbox = mbox;
		mbox->chans[i].con_priv = &spm->k3_chans[i];
	}

	ret = mbox_controller_register(mbox);
	if (ret)
		return ret;

	return 0;
}

static const u32 am6x_valid_threads[] = { 0, 1, 4, 5, 6, 7, 8, 9, 11, 12, 13, 20, 21, 22, 23 };

static const struct k3_sec_proxy_desc am654_desc = {
	.thread_count = 90,
	.max_msg_size = 60,
	.data_start_offset = 0x4,
	.data_end_offset = 0x3C,
	.valid_threads = am6x_valid_threads,
	.num_valid_threads = ARRAY_SIZE(am6x_valid_threads),
};

static const struct of_device_id k3_sec_proxy_ids[] = {
	{
		.compatible = "ti,am654-secure-proxy",
		.data = &am654_desc
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, k3_sec_proxy_ids);

static struct driver k3_sec_proxy_driver = {
        .name   = "k3-secure-proxy",
        .probe  = k3_sec_proxy_probe,
        .of_compatible = DRV_OF_COMPAT(k3_sec_proxy_ids),
};
core_platform_driver(k3_sec_proxy_driver);
