
#include <common.h>
#include <serdev.h>

static void serdev_device_poller(void *context)
{
	struct serdev_device *serdev = context;
	struct console_device *cdev = to_console_device(serdev);
	unsigned char *buf = serdev->buf;
	int ret, len;

	if (serdev->locked)
		return;

	serdev->locked = true;
	/*
	 * Since this callback is a part of poller infrastructure we
	 * want to use _non_interruptible version of the function
	 * below to prevent recursion from happening (regular
	 * console_drain will call is_timeout, which might end up
	 * calling this function again).
	 */
	len = console_drain_non_interruptible(cdev, serdev->fifo, buf,
					      PAGE_SIZE,
					      serdev->polling_window);
	while (len > 0) {
		ret = serdev->receive_buf(serdev, buf, len);
		len -= ret;
		buf += ret;
	}

	if (serdev->polling_interval) {
		/*
		 * Re-schedule ourselves in 'serdev->polling_interval'
		 * nanoseconds
		 */
		poller_call_async(&serdev->poller,
				  serdev->polling_interval,
				  serdev_device_poller,
				  serdev);
	} else {
		poller_async_cancel(&serdev->poller);
	}

	serdev->locked = false;
}

static int serdev_device_set_polling_interval(struct param_d *param, void *serdev)
{
	/*
	 * We execute poller ever time polling_interval changes to get
	 * any unprocessed immediate Rx data as well as to propagate
	 * polling_interval chagnes to outstanging async pollers.
	 */
	serdev_device_poller(serdev);
	return 0;
}

int serdev_device_open(struct serdev_device *serdev)
{
	struct console_device *cdev = to_console_device(serdev);
	struct param_d *p;
	int ret;

	if (!cdev->putc || !cdev->getc)
		return -EINVAL;

	if (!serdev->polling_window)
		return -EINVAL;

	serdev->buf = xzalloc(PAGE_SIZE);
	serdev->fifo = kfifo_alloc(PAGE_SIZE);
	if (!serdev->fifo)
		return -ENOMEM;

	ret = poller_async_register(&serdev->poller);
	if (ret)
		return ret;

	ret = console_open(cdev);
	if (ret)
		return ret;

	p = dev_add_param_uint64(serdev->dev, "polling_interval",
				 serdev_device_set_polling_interval, NULL,
				 &serdev->polling_interval, "%llu", serdev);
	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}

unsigned int serdev_device_set_baudrate(struct serdev_device *serdev,
					unsigned int speed)
{
	struct console_device *cdev = to_console_device(serdev);

	if (console_set_baudrate(cdev, speed) < 0)
		return 0;

	return console_get_baudrate(cdev);
}

int serdev_device_write(struct serdev_device *serdev, const unsigned char *buf,
			size_t count, unsigned long timeout)
{
	struct console_device *cdev = to_console_device(serdev);

	while (count--)
		cdev->putc(cdev, *buf++);
	/*
	 * Poll Rx once right after we just send some data in case our
	 * serdev device implements command/response type of a
	 * protocol and we need to start draining input as soon as
	 * possible.
	 */
	serdev_device_poller(serdev);
	return 0;
}

/*
 * NOTE: Code below is given primarily as an example of serdev API
 * usage. It may or may not be as useful or work as well as the
 * functions above.
 */

struct serdev_device_reader {
	unsigned char *buf;
	size_t len;
	size_t capacity;
};

static int serdev_device_reader_receive_buf(struct serdev_device *serdev,
					    const unsigned char *buf,
					    size_t size)
{
	struct device_d *dev = serdev->dev;
	struct serdev_device_reader *r = dev->priv;
	const size_t room = min(r->capacity - r->len, size);

	memcpy(r->buf + r->len, buf, room);
	r->len += room;
	/*
	 * It's important we return 'size' even if we didn't truly
	 * consume all of the data, since otherwise, serdev will keep
	 * re-executing us until we do. Given the logic above that
	 * would mean infinite loop.
	 */
	return size;
}

/**
 * serdev_device_reader_open - Open a reader serdev device
 *
 * @serdev:	Underlying serdev device
 * @capacity:	Storage capacity of the reader
 *
 * This function is intended for creating of reader serdev devices that
 * can be used in conjunction with serdev_device_read() to perform
 * trivial fixed length reads from a serdev device.
 */
int serdev_device_reader_open(struct serdev_device *serdev, size_t capacity)
{
	struct serdev_device_reader *r;

	if (serdev->receive_buf)
		return -EINVAL;

	r = xzalloc(sizeof(*r));
	r->capacity = capacity;
	r->buf = xzalloc(capacity);

	serdev->receive_buf = serdev_device_reader_receive_buf;
	serdev->dev->priv   = r;

	return 0;
}

/**
 * serdev_device_read - Read data from serdev device
 *
 * @serdev:	Serdev device to read from (must be a serdev reader)
 * @buf:	Buffer to read data into
 * @count:	Number of bytes to read
 * @timeout:	Read operation timeout
 *
 */
int serdev_device_read(struct serdev_device *serdev, unsigned char *buf,
		       size_t count, unsigned long timeout)
{
	struct device_d *dev = serdev->dev;
	struct serdev_device_reader *r = dev->priv;
	int ret;

	uint64_t start = get_time_ns();

	if (serdev->receive_buf != serdev_device_reader_receive_buf)
		return -EINVAL;

	/*
	 * is_timeout will implicitly poll serdev via poller
	 * infrastructure
	 */
	while (r->len < count) {
		if (is_timeout(start, timeout))
			return -ETIMEDOUT;
	}

	memcpy(buf, r->buf, count);
	ret = (r->len == count) ? 0 : -EMSGSIZE;
	r->len = 0;

	return ret;
}
