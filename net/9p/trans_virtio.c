// SPDX-License-Identifier: GPL-2.0-only
/*
 * The Virtio 9p transport driver
 *
 * This is a block based transport driver based on the lguest block driver
 * code.
 *
 *  Copyright (C) 2007, 2008 Eric Van Hensbergen, IBM Corporation
 *
 *  Based on virtio console driver
 *  Copyright (C) 2006, 2007 Rusty Russell, IBM Corporation
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/virtio_ring.h>
#include <net/9p/9p.h>
#include <linux/parser.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>
#include <linux/virtio.h>
#include <linux/virtio_9p.h>

#include <sys/stat.h>
#include <fs.h>

#define VIRTQUEUE_NUM	128

/* a single mutex to manage channel initialization and attachment */
static DEFINE_MUTEX(virtio_9p_lock);

/**
 * struct virtio_chan - per-instance transport information
 * @inuse: whether the channel is in use
 * @lock: protects multiple elements within this structure
 * @client: client instance
 * @vdev: virtio dev associated with this channel
 * @vq: virtio queue associated with this channel
 * @ring_bufs_avail: flag to indicate there is some available in the ring buf
 * @sg: scatter gather list which is used to pack a request (protected?)
 * @chan_list: linked list of channels
 *
 * We keep all per-channel information in a structure.
 * This structure is allocated within the devices dev->mem space.
 * A pointer to the structure will get put in the transport private.
 *
 */

struct virtio_chan {
	bool inuse;

	spinlock_t lock;

	struct p9_client *client;
	struct virtio_device *vdev;
	struct virtqueue *vq;
	int ring_bufs_avail;
	/* Scatterlist: can be too big for stack. */
	struct scatterlist sg[VIRTQUEUE_NUM];
	/**
	 * @tag: name to identify a mount null terminated
	 */
	char *tag;

	struct list_head chan_list;
};

static struct list_head virtio_chan_list;

/**
 * p9_virtio_close - reclaim resources of a channel
 * @client: client instance
 *
 * This reclaims a channel by freeing its resources and
 * resetting its inuse flag.
 *
 */

static void p9_virtio_close(struct p9_client *client)
{
	struct virtio_chan *chan = client->trans;

	mutex_lock(&virtio_9p_lock);
	if (chan)
		chan->inuse = false;
	mutex_unlock(&virtio_9p_lock);
}
/* We don't currently allow canceling of virtio requests */
static int p9_virtio_cancel(struct p9_client *client, struct p9_req_t *req)
{
	return 1;
}

/* Reply won't come, so drop req ref */
static int p9_virtio_cancelled(struct p9_client *client, struct p9_req_t *req)
{
	p9_req_put(client, req);
	return 0;
}

/**
 * p9_virtio_request - issue a request
 * @client: client instance issuing the request
 * @req: request to be issued
 *
 */

static int
p9_virtio_request(struct p9_client *client, struct p9_req_t *req)
{

	int err;
	int out_sgs, in_sgs;
	unsigned long flags;
	struct virtio_chan *chan = client->trans;
	struct scatterlist *sgs[2];

	p9_debug(P9_DEBUG_TRANS, "9p debug: virtio request\n");

	WRITE_ONCE(req->status, REQ_STATUS_SENT);
	spin_lock_irqsave(&chan->lock, flags);

	out_sgs = in_sgs = 0;

	/* Handle out VirtIO ring buffers */
	if (req->tc.size) {
		sg_init_one(chan->sg, req->tc.sdata, req->tc.size);
		sgs[out_sgs++] = chan->sg;
	}

	if (req->rc.capacity) {
		sg_init_one(&chan->sg[1], req->rc.sdata, req->rc.capacity);
		sgs[out_sgs + in_sgs++] = &chan->sg[1];
	}

	err = virtqueue_add_sgs(chan->vq, sgs, out_sgs, in_sgs, req);
	if (err < 0) {
		spin_unlock_irqrestore(&chan->lock, flags);
		p9_debug(P9_DEBUG_TRANS,
			 "virtio rpc add_sgs returned failure\n");
		return err == -ENOSPC ? -ENOSPC : -EIO;
	}
	virtqueue_kick(chan->vq);
	spin_unlock_irqrestore(&chan->lock, flags);

	p9_debug(P9_DEBUG_TRANS, "virtio request kicked\n");

	return 0;
}

/**
 * p9_virtio_poll - poll for request completion
 * @client: client instance issuing the request
 */

static int
p9_virtio_poll(struct p9_client *client)
{
	struct virtio_chan *chan = client->trans;
	unsigned int len;
	struct p9_req_t *req;

	while ((req = virtqueue_get_buf(chan->vq, &len)) != NULL) {
		p9_debug(P9_DEBUG_TRANS, ": request done\n");

		if (len) {
			req->rc.size = len;
			p9_client_cb(chan->client, req, REQ_STATUS_RCVD);
		}
	}

	return 0;
}

static void p9_mount_tag_show(struct device *dev)
{
	struct virtio_chan *chan;
	struct virtio_device *vdev;
	int tag_len;

	vdev = dev_to_virtio(dev);
	chan = vdev->priv;
	tag_len = strlen(chan->tag);

	printf("%.*s\n", tag_len + 1, chan->tag);
}

/**
 * p9_virtio_probe - probe for existence of 9P virtio channels
 * @vdev: virtio device to probe
 *
 * This probes for existing virtio channels.
 *
 */

static int p9_virtio_probe(struct virtio_device *vdev)
{
	__u16 tag_len;
	char *tag;
	int err;
	struct virtio_chan *chan;
	char *path, *cmd;

	if (!vdev->config->get_config) {
		dev_err(&vdev->dev, "%s failure: config access disabled\n",
			__func__);
		return -EINVAL;
	}

	chan = kmalloc(sizeof(struct virtio_chan), GFP_KERNEL);
	if (!chan) {
		pr_err("Failed to allocate virtio 9P channel\n");
		err = -ENOMEM;
		goto fail;
	}

	chan->vdev = vdev;

	/* We expect one virtqueue, for requests. */
	err = virtio_find_vqs(vdev, 1, &chan->vq);
	if (err)
		goto out_free_chan;

	chan->vq->vdev->priv = chan;
	spin_lock_init(&chan->lock);

	sg_init_table(chan->sg, VIRTQUEUE_NUM);

	chan->inuse = false;
	if (virtio_has_feature(vdev, VIRTIO_9P_MOUNT_TAG)) {
		virtio_cread(vdev, struct virtio_9p_config, tag_len, &tag_len);
	} else {
		err = -EINVAL;
		goto out_free_vq;
	}
	tag = kzalloc(tag_len + 1, GFP_KERNEL);
	if (!tag) {
		err = -ENOMEM;
		goto out_free_vq;
	}

	virtio_cread_bytes(vdev, offsetof(struct virtio_9p_config, tag),
			   tag, tag_len);
	chan->tag = tag;

	chan->ring_bufs_avail = 1;

	virtio_device_ready(vdev);

	mutex_lock(&virtio_9p_lock);
	list_add_tail(&chan->chan_list, &virtio_chan_list);
	mutex_unlock(&virtio_9p_lock);

	devinfo_add(&vdev->dev, p9_mount_tag_show);

	path = xasprintf("/mnt/9p/%s", chan->tag);
	cmd = xasprintf("mount -t 9p -o trans=virtio %s %s", chan->tag, path);

	if (mkdir(path, 0755) == 0) {
		err = automount_add(path, cmd);
		if (err)
			dev_warn(&vdev->dev,
				 "adding automountpoint at %s failed: %pe\n",
				 path, ERR_PTR(err));
	}

	free(cmd);
	free(path);

	return 0;

	kfree(tag);
out_free_vq:
	vdev->config->del_vqs(vdev);
out_free_chan:
	kfree(chan);
fail:
	return err;
}


/**
 * p9_virtio_create - allocate a new virtio channel
 * @client: client instance invoking this transport
 * @devname: string identifying the channel to connect to (unused)
 * @args: args passed from sys_mount() for per-transport options (unused)
 *
 * This sets up a transport channel for 9p communication.  Right now
 * we only match the first available channel, but eventually we could look up
 * alternate channels by matching devname versus a virtio_config entry.
 * We use a simple reference count mechanism to ensure that only a single
 * mount has a channel open at a time.
 *
 */

static int
p9_virtio_create(struct p9_client *client, const char *devname, char *args)
{
	struct virtio_chan *chan;
	int ret = -ENOENT;
	int found = 0;

	if (devname == NULL)
		return -EINVAL;

	mutex_lock(&virtio_9p_lock);
	list_for_each_entry(chan, &virtio_chan_list, chan_list) {
		if (!strcmp(devname, chan->tag)) {
			if (!chan->inuse) {
				chan->inuse = true;
				found = 1;
				break;
			}
			ret = -EBUSY;
		}
	}
	mutex_unlock(&virtio_9p_lock);

	if (!found) {
		pr_err("no channels available for device %s\n", devname);
		return ret;
	}

	client->trans = (void *)chan;
	client->status = Connected;
	client->trans_tag = chan->tag;
	chan->client = client;

	return 0;
}

/**
 * p9_virtio_remove - clean up resources associated with a virtio device
 * @vdev: virtio device to remove
 *
 */

static void p9_virtio_remove(struct virtio_device *vdev)
{
	struct virtio_chan *chan = vdev->priv;

	mutex_lock(&virtio_9p_lock);

	/* Remove self from list so we don't get new users. */
	list_del(&chan->chan_list);

	/* Wait for existing users to close. */
	if (chan->inuse)
		dev_emerg(&vdev->dev,
			  "p9_virtio_remove: device in use.\n");

	vdev->config->reset(vdev);
	vdev->config->del_vqs(vdev);

	kfree(chan->tag);
	kfree(chan);
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_9P, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static unsigned int features[] = {
	VIRTIO_9P_MOUNT_TAG,
};

/* The standard "struct lguest_driver": */
static struct virtio_driver p9_virtio_drv = {
	.feature_table  = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name    = KBUILD_MODNAME,
	.id_table	= id_table,
	.probe		= p9_virtio_probe,
	.remove		= p9_virtio_remove,
};

static struct p9_trans_module p9_virtio_trans = {
	.name = "virtio",
	.create = p9_virtio_create,
	.close = p9_virtio_close,
	.request = p9_virtio_request,
	.poll = p9_virtio_poll,
	.cancel = p9_virtio_cancel,
	.cancelled = p9_virtio_cancelled,
	/*
	 * We leave one entry for input and one entry for response
	 * headers. We also skip one more entry to accommodate, address
	 * that are not at page boundary, that can result in an extra
	 * page in zero copy.
	 */
	.maxsize = PAGE_SIZE * (VIRTQUEUE_NUM - 3),
	.pooled_rbuffers = false,
	.def = 1,
};

/* The standard init function */
static int __init p9_virtio_init(void)
{
	int rc;

	INIT_LIST_HEAD(&virtio_chan_list);

	v9fs_register_trans(&p9_virtio_trans);
	rc = virtio_driver_register(&p9_virtio_drv);
	if (rc)
		v9fs_unregister_trans(&p9_virtio_trans);

	return rc;
}
device_initcall(p9_virtio_init);
MODULE_ALIAS_9P("virtio");

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_AUTHOR("Eric Van Hensbergen <ericvh@gmail.com>");
MODULE_DESCRIPTION("Virtio 9p Transport");
MODULE_LICENSE("GPL");
