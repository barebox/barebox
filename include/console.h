/*
 * (C) Copyright 2000
 * Paolo Scaffardi, AIRVENT SAM s.p.a - RIMINI(ITALY), arsenio@tin.it
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <param.h>
#include <linux/list.h>
#include <driver.h>
#include <serdev.h>
#include <clock.h>
#include <kfifo.h>

#define CONSOLE_STDIN           (1 << 0)
#define CONSOLE_STDOUT          (1 << 1)
#define CONSOLE_STDERR          (1 << 2)

enum console_mode {
	CONSOLE_MODE_NORMAL,
	CONSOLE_MODE_RS485,
};

struct console_device {
	struct device_d *dev;
	struct device_d class_dev;

	int (*tstc)(struct console_device *cdev);
	void (*putc)(struct console_device *cdev, char c);
	int (*puts)(struct console_device *cdev, const char *s);
	int  (*getc)(struct console_device *cdev);
	int (*setbrg)(struct console_device *cdev, int baudrate);
	void (*flush)(struct console_device *cdev);
	int (*set_mode)(struct console_device *cdev, enum console_mode mode);
	int (*open)(struct console_device *cdev);
	int (*close)(struct console_device *cdev);

	char *devname;
	int devid;

	struct list_head list;

	unsigned char f_active;
	char *active_string;

	unsigned int open_count;

	unsigned int baudrate;
	unsigned int baudrate_param;

	const char *linux_console_name;

	struct cdev devfs;
	struct cdev_operations fops;

	struct serdev_device serdev;
};

static inline struct serdev_device *to_serdev_device(struct device_d *d)
{
	struct console_device *cdev =
		container_of(d, struct console_device, class_dev);
	return &cdev->serdev;
}

static inline struct console_device *
to_console_device(struct serdev_device *serdev)
{
	return container_of(serdev, struct console_device, serdev);
}

static inline struct device_node *
console_is_serdev_node(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	if (dev && dev->device_node &&
	    of_get_child_count(dev->device_node))
		return dev->device_node;

	return NULL;
}

int console_register(struct console_device *cdev);
int console_unregister(struct console_device *cdev);

struct console_device *console_get_by_dev(struct device_d *dev);
struct console_device *console_get_by_name(const char *name);

extern struct list_head console_list;
#define for_each_console(console) list_for_each_entry(console, &console_list, list)

#define CFG_PBSIZE (CONFIG_CBSIZE+sizeof(CONFIG_PROMPT)+16)

extern int barebox_loglevel;

struct console_device *console_get_first_active(void);

int console_open(struct console_device *cdev);
int console_close(struct console_device *cdev);
int console_set_active(struct console_device *cdev, unsigned active);
unsigned console_get_active(struct console_device *cdev);
int console_set_baudrate(struct console_device *cdev, unsigned baudrate);
unsigned console_get_baudrate(struct console_device *cdev);


/**
 * console_fifo_fill - fill FIFO with as much console data as possible
 *
 * @cdev:	Console to poll for dat
 * @fifo:	FIFO to store the data in
 */
static inline int console_fifo_fill(struct console_device *cdev,
				    struct kfifo *fifo)
{
	size_t len = kfifo_len(fifo);
	while (cdev->tstc(cdev) && len < fifo->size) {
		kfifo_putc(fifo, (unsigned char)(cdev->getc(cdev)));
		len++;
	}

	return len;
}

/**
 * __console_drain - Drain console into a buffer via FIFO
 *
 * @__is_timeout	Callback used to determine timeout condition
 * @cdev		Console to drain
 * @fifo		FIFO to use as a transient buffer
 * @buf			Buffer to drain console into
 * @len			Size of the drain buffer
 * @timeout		Console polling timeout in ns
 *
 * This function is optimized to :
 * - maximize throughput (ie. read as much as is available in lower layer fifo)
 * - minimize latencies (no delay or wait timeout if data available)
 * - have a timeout
 * This is why standard getc() is not used, and input_fifo_fill() exists.
 */
static inline int __console_drain(int (*__is_timeout)(uint64_t start_ns,
						      uint64_t time_offset_ns),
				  struct console_device *cdev,
				  struct kfifo *fifo,
				  unsigned char *buf,
				  int len,
				  uint64_t timeout)
{
	int i = 0;
	uint64_t start = get_time_ns();

	if (!len)
		return -EINVAL;

	do {
		/*
		 * To minimize wait time before we start polling Rx
		 * (to potentially avoid overruning Rx FIFO) we call
		 * console_fifo_fill first
		 */
		if (console_fifo_fill(cdev, fifo))
			kfifo_getc(fifo, &buf[i++]);

	} while (i < len && !__is_timeout(start, timeout));

	return i;
}

static inline int console_drain_non_interruptible(struct console_device *cdev,
						  struct kfifo *fifo,
						  unsigned char *buf,
						  int len,
						  uint64_t timeout)
{
	return __console_drain(is_timeout_non_interruptible,
			       cdev, fifo, buf, len, timeout);
}

static inline int console_drain(struct console_device *cdev,
				struct kfifo *fifo,
				unsigned char *buf,
				int len,
				uint64_t timeout)
{
	return __console_drain(is_timeout, cdev, fifo, buf, len, timeout);
}

#ifdef CONFIG_PBL_CONSOLE
void pbl_set_putc(void (*putcf)(void *ctx, int c), void *ctx);
#else
static inline void pbl_set_putc(void (*putcf)(void *ctx, int c), void *ctx) {}
#endif

bool console_allow_color(void);

#endif
