// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <bthread.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ring.h>
#include <input/input.h>
#include <dt-bindings/input/linux-event-codes.h>

#include <uapi/linux/virtio_ids.h>
#include <uapi/linux/virtio_input.h>

struct virtio_input {
	struct input_device        idev;
	struct virtio_device       *vdev;
	struct virtqueue           *evt;
	struct virtio_input_event  evts[64];
	struct bthread             *bthread;
};

static void virtinput_queue_evtbuf(struct virtio_input *vi,
				   struct virtio_input_event *evtbuf)
{
	struct virtio_sg sg[1];
	virtio_sg_init_one(sg, evtbuf, sizeof(*evtbuf));
	virtqueue_add_inbuf(vi->evt, sg, 1);
}

static int virtinput_recv_events(struct virtio_input *vi)
{
	struct device_d *dev = &vi->vdev->dev;
	struct virtio_input_event *event;
	unsigned int len;
	int i = 0;

	while ((event = virtqueue_get_buf(vi->evt, &len)) != NULL) {
		if (le16_to_cpu(event->type) == EV_KEY)
			input_report_key_event(&vi->idev, le16_to_cpu(event->code),
					       le32_to_cpu(event->value));

		pr_debug("\n%s: input event #%td received (type=%u, code=%u, value=%u)\n",
			 dev_name(dev),
			 event - &vi->evts[0],
			 le16_to_cpu(event->type), le16_to_cpu(event->code),
			 le32_to_cpu(event->value));

		virtinput_queue_evtbuf(vi, event);
		i++;
	}

	return i;
}

static int virtinput_poll_vqs(void *_vi)
{
	struct virtio_input *vi = _vi;

	while (!bthread_should_stop()) {
		int bufs = 0;

		bufs += virtinput_recv_events(vi);

		if (bufs)
			virtqueue_kick(vi->evt);
	}

	return 0;
}

static u8 virtinput_cfg_select(struct virtio_input *vi,
			       u8 select, u8 subsel)
{
	u8 size;

	virtio_cwrite_le(vi->vdev, struct virtio_input_config, select, &select);
	virtio_cwrite_le(vi->vdev, struct virtio_input_config, subsel, &subsel);
	virtio_cread_le(vi->vdev, struct virtio_input_config, size, &size);
	return size;
}

static void virtinput_fill_evt(struct virtio_input *vi)
{
	int i, size;

	size = virtqueue_get_vring_size(vi->evt);
	if (size > ARRAY_SIZE(vi->evts))
		size = ARRAY_SIZE(vi->evts);
	for (i = 0; i < size; i++)
		virtinput_queue_evtbuf(vi, &vi->evts[i]);
	virtqueue_kick(vi->evt);
}

static int virtinput_init_vqs(struct virtio_input *vi)
{
	struct virtqueue *vqs[1];
	int err;


	err = virtio_find_vqs(vi->vdev, 1, vqs);
	if (err)
		return err;

	vi->evt = vqs[0];

	return 0;
}

static int virtinput_probe(struct virtio_device *vdev)
{
	struct virtio_input *vi;
	char name[64];
	size_t size;
	int err;

	if (!virtio_has_feature(vdev, VIRTIO_F_VERSION_1))
		return -ENODEV;

	vi = kzalloc(sizeof(*vi), GFP_KERNEL);
	if (!vi)
		return -ENOMEM;

	vdev->priv = vi;
	vi->vdev = vdev;

	err = virtinput_init_vqs(vi);
	if (err)
		goto err_init_vq;

	size = virtinput_cfg_select(vi, VIRTIO_INPUT_CFG_ID_NAME, 0);
	virtio_cread_bytes(vi->vdev, offsetof(struct virtio_input_config, u.string),
			   name, min(size, sizeof(name)));
	name[size] = '\0';

	virtio_device_ready(vdev);

	err = input_device_register(&vi->idev);
	if (err)
		goto err_input_register;

	virtinput_fill_evt(vi);

	vi->bthread = bthread_run(virtinput_poll_vqs, vi,
				  "%s/input0", dev_name(&vdev->dev));
	if (!vi->bthread) {
		err = -ENOMEM;
		goto err_bthread_run;
	}

	dev_info(&vdev->dev, "'%s' probed\n", name);

	return 0;

err_bthread_run:
	bthread_free(vi->bthread);
err_input_register:
	vdev->config->del_vqs(vdev);
err_init_vq:
	kfree(vi);
	return err;
}

static void virtinput_remove(struct virtio_device *vdev)
{
	struct virtio_input *vi = vdev->priv;

	bthread_stop(vi->bthread);
	bthread_free(vi->bthread);

	vdev->config->reset(vdev);
	vdev->config->del_vqs(vdev);
	kfree(vi);
}

static const struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_INPUT, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_input_driver = {
	.driver.name         = "virtio_input",
	.id_table            = id_table,
	.probe               = virtinput_probe,
	.remove              = virtinput_remove,
};
device_virtio_driver(virtio_input_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtio input device driver");
MODULE_AUTHOR("Gerd Hoffmann <kraxel@redhat.com>");
MODULE_AUTHOR("Ahmad Fatoum <a.fatoum@pengutronix.de>");
