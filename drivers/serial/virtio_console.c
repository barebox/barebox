// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2006, 2007, 2009 Rusty Russell, IBM Corporation
 * Copyright (C) 2009, 2010, 2011 Red Hat, Inc.
 * Copyright (C) 2009, 2010, 2011 Amit Shah <amit.shah@redhat.com>
 * Copyright (C) 2021 Ahmad Fatoum
 *
 * This ridiculously simple implementation does a DMA transfer for
 * every single character. On the plus side, we neither need to
 * buffer RX or to wade through TX to turn LFs to CRLFs.
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/list.h>
#include <malloc.h>
#include <console.h>
#include <xfuncs.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/virtio_console.h>

struct virtio_console {
	struct console_device cdev;
	struct virtqueue *in_vq, *out_vq;
	char inbuf[1];
};

static bool have_one;

/*
 * The put_chars() callback is pretty straightforward.
 *
 * We turn the characters into a scatter-gather list, add it to the
 * output queue and then kick the Host.  Then we sit here waiting for
 * it to finish: inefficient in theory, but in practice
 * implementations will do it immediately (lguest's Launcher does).
 */
static void put_chars(struct virtio_console *virtcons, const char *buf, int count)
{
	struct virtqueue *out_vq = virtcons->out_vq;
	unsigned int len;
	struct scatterlist sg;

	sg_init_one(&sg, buf, count);

	/*
	 * add_buf wants a token to identify this buffer: we hand it
	 * any non-NULL pointer, since there's only ever one buffer.
	 */
	if (virtqueue_add_outbuf(out_vq, &sg, 1, (void *)buf) >= 0) {
		/* Tell Host to go! */
		virtqueue_kick(out_vq);
		/* Chill out until it's done with the buffer. */
		if (!virtqueue_get_buf_timeout(out_vq, &len, NSEC_PER_SEC))
			dev_warn(virtcons->cdev.dev, "Timeout waiting for TX ack\n");
	}
}

static void virtcons_putc(struct console_device *cdev, char c)
{
	struct virtio_console *virtcons = container_of(cdev, struct virtio_console, cdev);

	return put_chars(virtcons, &c, 1);
}

/*
 * Create a scatter-gather list representing our input buffer and put
 * it in the queue.
 */
static void add_inbuf(struct virtio_console *virtcons)
{
	struct scatterlist sg;

	sg_init_one(&sg, virtcons->inbuf, sizeof(virtcons->inbuf));

	/* We should always be able to add one buffer to an empty queue. */
	if (virtqueue_add_inbuf(virtcons->in_vq, &sg, 1, virtcons->inbuf) < 0)
		BUG();
	virtqueue_kick(virtcons->in_vq);
}

static int virtcons_tstc(struct console_device *cdev)
{
	struct virtio_console *virtcons = container_of(cdev, struct virtio_console, cdev);

	return virtqueue_poll(virtcons->in_vq, virtcons->in_vq->last_used_idx);
}

static int virtcons_getc(struct console_device *cdev)
{
	struct virtio_console *virtcons = container_of(cdev, struct virtio_console, cdev);
	char *in;
	int ch;

	in = virtqueue_get_buf(virtcons->in_vq, NULL);
	if (!in)
		BUG();

	ch = *in;

	add_inbuf(virtcons);

	return ch;
}

static int virtcons_probe(struct virtio_device *vdev)
{
	struct virtqueue *vqs[2];
	struct virtio_console *virtcons;
	int err;

	if (have_one) {
		/* Neither multiport consoles (one virtio_device for multiple consoles)
		 * nor multiple consoles (one virtio_device per each console
		 * is supported. I would've expected:
		 *   -chardev socket,path=/tmp/bar,server,nowait,id=bar \
		 *   -device virtconsole,chardev=bar,name=console.bar \
		 *   -device virtio-serial-device \
		 *   -chardev socket,path=/tmp/baz,server,nowait,id=baz \
		 *   -device virtconsole,chardev=baz,name=console.baz \
		 * to just work, but it doesn't
		 */
		dev_warn(&vdev->dev,
			 "Multiple virtio-console devices not supported yet\n");
		return -EEXIST;
	}

	/* Find the queues. */
	err = virtio_find_vqs(vdev, 2, vqs);
	if (err)
		return err;

	virtcons = xzalloc(sizeof(*virtcons));

	vdev->priv = virtcons;

	virtcons->in_vq = vqs[0];
	virtcons->out_vq = vqs[1];

	/* Register the input buffer the first time. */
	add_inbuf(virtcons);

	virtcons->cdev.dev = &vdev->dev;
	virtcons->cdev.tstc = virtcons_tstc;
	virtcons->cdev.getc = virtcons_getc;
	virtcons->cdev.putc = virtcons_putc;

	have_one = true;

	return console_register(&virtcons->cdev);
}

static void virtcons_remove(struct virtio_device *vdev)
{
	struct virtio_console *virtcons = vdev->priv;

	vdev->config->reset(vdev);
	console_unregister(&virtcons->cdev);
	vdev->config->del_vqs(vdev);

	free(virtcons);
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_CONSOLE, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_console = {
	.driver.name =	"virtio_console",
	.id_table =	id_table,
	.probe =	virtcons_probe,
	.remove =	virtcons_remove,
};
device_virtio_driver(virtio_console);

MODULE_DESCRIPTION("Virtio console driver");
MODULE_LICENSE("GPL");
