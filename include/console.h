/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2000
 * Paolo Scaffardi, AIRVENT SAM s.p.a - RIMINI(ITALY), arsenio@tin.it
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <param.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <driver.h>
#include <serdev.h>
#include <clock.h>
#include <kfifo.h>

#define CONSOLE_STDIN           (1 << 0)
#define CONSOLE_STDOUT          (1 << 1)
#define CONSOLE_STDERR          (1 << 2)

#define CONSOLE_STDIOE          (CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR)

enum console_mode {
	CONSOLE_MODE_NORMAL,
	CONSOLE_MODE_RS485,
};

struct console_device {
	struct device *dev;
	struct device class_dev;

	int (*tstc)(struct console_device *cdev);
	void (*putc)(struct console_device *cdev, char c);
	int (*puts)(struct console_device *cdev, const char *s, size_t nbytes);
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
	const char *linux_earlycon_name;
	void __iomem *phys_base;

	struct cdev devfs;
	struct cdev_operations fops;

	struct serdev_device serdev;
};

static inline struct serdev_device *to_serdev_device(struct device *d)
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
	struct device *dev = cdev->dev;
	if (dev && dev->of_node &&
	    of_get_child_count(dev->of_node))
		return dev->of_node;

	return NULL;
}

int console_register(struct console_device *cdev);
int console_unregister(struct console_device *cdev);

struct console_device *console_get_by_dev(struct device *dev);
struct console_device *console_get_by_name(const char *name);
struct console_device *of_console_get_by_alias(const char *alias);

#define CFG_PBSIZE (CONFIG_CBSIZE+sizeof(CONFIG_PROMPT)+16)

int console_open(struct console_device *cdev);
int console_close(struct console_device *cdev);
int console_set_active(struct console_device *cdev, unsigned active);
unsigned console_get_active(struct console_device *cdev);
int console_set_baudrate(struct console_device *cdev, unsigned baudrate);
unsigned console_get_baudrate(struct console_device *cdev);

struct console_device *of_console_by_stdout_path(void);

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

#ifndef CONFIG_CONSOLE_NONE
extern struct list_head console_list;
#define for_each_console(console) list_for_each_entry(console, &console_list, list)

struct console_device *console_get_first_active(void);

extern int barebox_loglevel;
static inline int barebox_set_loglevel(int loglevel)
{
	int old_loglevel = barebox_loglevel;
	barebox_loglevel = loglevel;
	return old_loglevel;
}
#else
#define for_each_console(console) while (((void)console, 0))

static inline struct console_device *console_get_first_active(void)
{
	return NULL;
}

static inline int barebox_set_loglevel(int loglevel)
{
	return loglevel;
}
#endif

#ifdef CONFIG_CONSOLE_FULL
void console_ctrlc_allow(void);
void console_ctrlc_forbid(void);
#else
static inline void console_ctrlc_allow(void) { }
static inline void console_ctrlc_forbid(void) { }
#endif

/**
 * clk_get_for_console - get clock, ignoring known unavailable clock controller
 * @dev: device for clock "consumer"
 * @id: clock consumer ID
 *
 * Return: a struct clk corresponding to the clock producer, a
 * valid IS_ERR() condition containing errno or NULL if it could
 * be determined that the clock producer will never be probed in
 * absence of modules. The NULL return allows serial drivers to
 * skip clock handling for the stdout console and rely on CONFIG_DEBUG_LL.
 */
static inline struct clk *clk_get_for_console(struct device *dev, const char *id)
{
	__always_unused unsigned baudrate;
	struct clk *clk;

	if (!IS_ENABLED(CONFIG_DEBUG_LL) || !of_device_is_stdout_path(dev, &baudrate))
		return clk_get(dev, id);

	clk = clk_get_if_available(dev, id);
	if (clk == NULL)
		dev_warn(dev, "couldn't get clock (ignoring)\n");

	return clk;
}

#endif
