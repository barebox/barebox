// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <poller.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ring.h>
#include <input/input.h>
#include <sound.h>
#include <dt-bindings/input/linux-event-codes.h>

#include <uapi/linux/virtio_ids.h>
#include <uapi/linux/virtio_input.h>

struct virtio_input {
	struct input_device        idev;
	struct virtio_device       *vdev;
	struct virtqueue           *evt, *sts;
	struct virtio_input_event  evts[64];
	struct poller_struct       poller;
	struct sound_card          beeper;
	unsigned long              sndbit[BITS_TO_LONGS(SND_CNT)];
};

static void virtinput_queue_evtbuf(struct virtio_input *vi,
				   struct virtio_input_event *evtbuf)
{
	struct scatterlist sg;
	sg_init_one(&sg, evtbuf, sizeof(*evtbuf));
	virtqueue_add_inbuf(vi->evt, &sg, 1, evtbuf);
}

static int virtinput_recv_events(struct virtio_input *vi)
{
	struct device *dev = &vi->vdev->dev;
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

/*
 * On error we are losing the status update, which isn't critical as
 * this is used for the bell.
 */
static int virtinput_send_status(struct sound_card *beeper, unsigned freq, unsigned duration)
{
	struct virtio_input *vi = container_of(beeper, struct virtio_input, beeper);
	struct virtio_input_event *stsbuf;
	struct scatterlist sg;
	u16 code;
	int rc;

	stsbuf = kzalloc(sizeof(*stsbuf), 0);
	if (!stsbuf)
		return -ENOMEM;

	code = vi->sndbit[0] & BIT_MASK(SND_TONE) ? SND_TONE : SND_BELL;

	stsbuf->type  = cpu_to_le16(EV_SND);
	stsbuf->code  = cpu_to_le16(code);
	stsbuf->value = cpu_to_le32(freq);
	sg_init_one(&sg, stsbuf, sizeof(*stsbuf));

	rc = virtqueue_add_outbuf(vi->sts, &sg, 1, stsbuf);
	virtqueue_kick(vi->sts);

	if (rc != 0)
		kfree(stsbuf);
	return rc;
}

static int virtinput_recv_status(struct virtio_input *vi)
{
	struct virtio_input_event *stsbuf;
	unsigned int len;
	int i = 0;

	while ((stsbuf = virtqueue_get_buf(vi->sts, &len)) != NULL) {
		kfree(stsbuf);
		i++;
	}

	return i;
}

static void virtinput_poll_vqs(struct poller_struct *poller)
{
	struct virtio_input *vi = container_of(poller, struct virtio_input, poller);

	int bufs = 0;

	bufs += virtinput_recv_events(vi);
	bufs += virtinput_recv_status(vi);

	if (bufs)
		virtqueue_kick(vi->evt);
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

static void virtinput_cfg_bits(struct virtio_input *vi, int select, int subsel,
			       unsigned long *bits, unsigned int bitcount)
{
	unsigned int bit;
	u8 *virtio_bits;
	u8 bytes;

	bytes = virtinput_cfg_select(vi, select, subsel);
	if (!bytes)
		return;
	if (bitcount > bytes * 8)
		bitcount = bytes * 8;

	/*
	 * Bitmap in virtio config space is a simple stream of bytes,
	 * with the first byte carrying bits 0-7, second bits 8-15 and
	 * so on.
	 */
	virtio_bits = kzalloc(bytes, GFP_KERNEL);
	if (!virtio_bits)
		return;
	virtio_cread_bytes(vi->vdev, offsetof(struct virtio_input_config,
					      u.bitmap),
			   virtio_bits, bytes);
	for (bit = 0; bit < bitcount; bit++) {
		if (virtio_bits[bit / 8] & (1 << (bit % 8)))
			__set_bit(bit, bits);
	}
	kfree(virtio_bits);
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
	struct virtqueue *vqs[2];
	int err;


	err = virtio_find_vqs(vi->vdev, 2, vqs);
	if (err)
		return err;

	vi->evt = vqs[0];
	vi->sts = vqs[1];

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

	dev_info(&vdev->dev, "'%s' detected\n", name);

	virtinput_cfg_bits(vi, VIRTIO_INPUT_CFG_EV_BITS, EV_SND,
			   vi->sndbit, SND_CNT);

	virtio_device_ready(vdev);

	vi->idev.parent = &vdev->dev;
	err = input_device_register(&vi->idev);
	if (err)
		goto err_input_register;

	virtinput_fill_evt(vi);

	vi->poller.func = virtinput_poll_vqs;
	snprintf(name, sizeof(name), "%s/input0", dev_name(&vdev->dev));

	err = poller_register(&vi->poller, name);
	if (err)
		goto err_poller_register;

	if (IS_ENABLED(CONFIG_SOUND) &&
	    (vi->sndbit[0] & (BIT_MASK(SND_BELL) | BIT_MASK(SND_TONE)))) {
		struct sound_card *beeper;

		beeper = &vi->beeper;
		beeper->name = basprintf("%s/beeper0", dev_name(&vdev->dev));
		beeper->beep = virtinput_send_status;

		err = sound_card_register(&vi->beeper);
		if (err)
			dev_warn(&vdev->dev, "bell registration failed: %pe\n", ERR_PTR(err));
		else
			dev_info(&vdev->dev, "bell registered\n");
	}

	return 0;

err_poller_register:
	input_device_unregister(&vi->idev);
err_input_register:
	vdev->config->del_vqs(vdev);
err_init_vq:
	kfree(vi);
	return err;
}

static void virtinput_remove(struct virtio_device *vdev)
{
	struct virtio_input *vi = vdev->priv;

	vdev->config->reset(vdev);
	poller_unregister(&vi->poller);
	input_device_unregister(&vi->idev);
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
