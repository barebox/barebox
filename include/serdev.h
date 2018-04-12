#ifndef _SERDEV_H_
#define _SERDEV_H_

#include <driver.h>
#include <poller.h>
#include <kfifo.h>

/**
 * struct serdev_device - Basic representation of an serdev device
 *
 * @dev:		Corresponding device
 * @fifo:		Circular buffer used for console draining
 * @buf:		Buffer used to pass Rx data to consumers
 * @poller		Async poller used to poll this serdev
 * @polling_interval:	Async poller periodicity
 * @polling_window:	Duration of a single busy loop poll
 * @receive_buf:	Function called with data received from device;
 *			returns number of bytes accepted;
 */
struct serdev_device {
	struct device_d *dev;
	struct kfifo *fifo;
	unsigned char *buf;
	struct poller_async poller;
	uint64_t polling_interval;
	uint64_t polling_window;

	int (*receive_buf)(struct serdev_device *, const unsigned char *,
			   size_t);
};

int serdev_device_open(struct serdev_device *);
unsigned int serdev_device_set_baudrate(struct serdev_device *, unsigned int);
int serdev_device_write(struct serdev_device *, const unsigned char *,
			size_t, unsigned long);

/*
 * The following two functions are not a part of original Linux API
 */
int serdev_device_reader_open(struct serdev_device *, size_t);
int serdev_device_read(struct serdev_device *, unsigned char *,
		       size_t, unsigned long);

#endif
