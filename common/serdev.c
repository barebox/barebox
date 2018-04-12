
#include <common.h>
#include <serdev.h>

static void serdev_device_poller(void *context)
{
	struct serdev_device *serdev = context;
	struct console_device *cdev = to_console_device(serdev);
	unsigned char *buf = serdev->buf;
	int ret, len;

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
	}
}

int serdev_device_open(struct serdev_device *serdev)
{
	struct console_device *cdev = to_console_device(serdev);
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

	return console_open(cdev);
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
