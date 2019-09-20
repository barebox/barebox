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

#include <config.h>
#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <param.h>
#include <console.h>
#include <driver.h>
#include <fs.h>
#include <of.h>
#include <init.h>
#include <clock.h>
#include <kfifo.h>
#include <module.h>
#include <poller.h>
#include <ratp_bb.h>
#include <magicvar.h>
#include <globalvar.h>
#include <linux/list.h>
#include <linux/stringify.h>
#include <debug_ll.h>

LIST_HEAD(console_list);
EXPORT_SYMBOL(console_list);

#define CONSOLE_UNINITIALIZED		0
#define CONSOLE_INITIALIZED_BUFFER	1
#define CONSOLE_INIT_FULL		2

#define to_console_dev(d) container_of(d, struct console_device, class_dev)

static int initialized = 0;

#define CONSOLE_BUFFER_SIZE	1024

static char console_input_buffer[CONSOLE_BUFFER_SIZE];
static char console_output_buffer[CONSOLE_BUFFER_SIZE];

static struct kfifo __console_input_fifo;
static struct kfifo __console_output_fifo;
static struct kfifo *console_input_fifo = &__console_input_fifo;
static struct kfifo *console_output_fifo = &__console_output_fifo;

int console_open(struct console_device *cdev)
{
	int ret;

	if (cdev->open && !cdev->open_count) {
		ret = cdev->open(cdev);
		if (ret)
			return ret;
	}

	cdev->open_count++;

	return 0;
}

int console_close(struct console_device *cdev)
{
	int ret;

	if (!cdev->open_count)
		return -EBADFD;

	cdev->open_count--;

	if (cdev->close && !cdev->open_count) {
		ret = cdev->close(cdev);
		if (ret)
			return ret;
	}

	return 0;
}

int console_set_active(struct console_device *cdev, unsigned flag)
{
	int ret;

	if (!cdev->getc)
		flag &= ~CONSOLE_STDIN;
	if (!cdev->putc)
		flag &= ~(CONSOLE_STDOUT | CONSOLE_STDERR);

	if (!flag && cdev->f_active && cdev->flush)
		cdev->flush(cdev);

	if (flag == cdev->f_active)
		return 0;

	if (!flag) {
		ret = console_close(cdev);
		if (ret)
			return ret;
	} else {
		ret = console_open(cdev);
		if (ret)
			return ret;
	}

	cdev->f_active = flag;

	if (initialized < CONSOLE_INIT_FULL) {
		char ch;
		initialized = CONSOLE_INIT_FULL;
		puts_ll("Switch to console [");
		puts_ll(dev_name(&cdev->class_dev));
		puts_ll("]\n");
		barebox_banner();
		while (kfifo_getc(console_output_fifo, &ch) == 0)
			console_putc(CONSOLE_STDOUT, ch);
	}

	return 0;
}

unsigned console_get_active(struct console_device *cdev)
{
	return cdev->f_active;
}

static int console_active_set(struct param_d *param, void *priv)
{
	struct console_device *cdev = priv;
	unsigned int flag = 0;
	int ret;

	if (cdev->active_string) {
		if (strchr(cdev->active_string, 'i'))
			flag |= CONSOLE_STDIN;
		if (strchr(cdev->active_string, 'o'))
			flag |= CONSOLE_STDOUT;
		if (strchr(cdev->active_string, 'e'))
			flag |= CONSOLE_STDERR;
	}

	ret = console_set_active(cdev, flag);
	if (ret)
		return ret;

	return 0;
}

static int console_active_get(struct param_d *param, void *priv)
{
	struct console_device *cdev = priv;
	unsigned int flag = cdev->f_active;

	free(cdev->active_string);
	cdev->active_string = basprintf("%s%s%s",
					flag & CONSOLE_STDIN ? "i" : "",
					flag & CONSOLE_STDOUT ? "o" : "",
					flag & CONSOLE_STDERR ? "e" : "");
	return 0;
}

int console_set_baudrate(struct console_device *cdev, unsigned baudrate)
{
	int ret;
	unsigned char c;

	if (!cdev->setbrg)
		return -ENOSYS;

	if (cdev->baudrate == baudrate)
		return 0;

	/*
	 * If the device is already active, change its baudrate.
	 * The baudrate of an inactive device will be set at activation time.
	 */
	if (cdev->f_active) {
		printf("## Switch baudrate on console %s to %d bps and press ENTER ...\n",
			dev_name(&cdev->class_dev), baudrate);
		mdelay(50);
	}

	ret = cdev->setbrg(cdev, baudrate);
	if (ret)
		return ret;

	if (cdev->f_active) {
		mdelay(50);
		do {
			c = getchar();
		} while (c != '\r' && c != '\n');
	}

	cdev->baudrate = baudrate;
	cdev->baudrate_param = baudrate;

	return 0;
}

unsigned console_get_baudrate(struct console_device *cdev)
{
	return cdev->baudrate;
}

static int console_baudrate_set(struct param_d *param, void *priv)
{
	struct console_device *cdev = priv;

	return console_set_baudrate(cdev, cdev->baudrate_param);
}

static void console_init_early(void)
{
	kfifo_init(console_input_fifo, console_input_buffer,
			CONSOLE_BUFFER_SIZE);
	kfifo_init(console_output_fifo, console_output_buffer,
			CONSOLE_BUFFER_SIZE);

	initialized = CONSOLE_INITIALIZED_BUFFER;
}

static void console_set_stdoutpath(struct console_device *cdev)
{
	int id;
	char *str;

	if (!cdev->linux_console_name)
		return;

	id = of_alias_get_id(cdev->dev->device_node, "serial");
	if (id < 0)
		return;

	str = basprintf("console=%s%d,%dn8", cdev->linux_console_name, id,
			  cdev->baudrate);

	globalvar_add_simple("linux.bootargs.console", str);

	free(str);
}

static int __console_puts(struct console_device *cdev, const char *s,
			  size_t nbytes)
{
	size_t i;

	for (i = 0; i < nbytes; i++) {
		if (*s == '\n')
			cdev->putc(cdev, '\r');

		cdev->putc(cdev, *s);
		s++;
	}
	return i;
}

static int fops_open(struct cdev *cdev, unsigned long flags)
{
	struct console_device *priv = cdev->priv;

	if ((flags & (O_WRONLY | O_RDWR)) && !priv->puts )
		return -EPERM;

	return console_open(priv);
}

static int fops_close(struct cdev *dev)
{
	struct console_device *priv = dev->priv;

	return console_close(priv);
}

static int fops_flush(struct cdev *dev)
{
	struct console_device *priv = dev->priv;

	if (priv->flush)
		priv->flush(priv);

	return 0;
}

static ssize_t fops_write(struct cdev* dev, const void* buf, size_t count,
		      loff_t offset, ulong flags)
{
	struct console_device *priv = dev->priv;

	priv->puts(priv, buf, count);

	return count;
}

int console_register(struct console_device *newcdev)
{
	struct device_node *serdev_node = console_is_serdev_node(newcdev);
	struct device_d *dev = &newcdev->class_dev;
	int activate = 0, ret;

	if (!serdev_node && initialized == CONSOLE_UNINITIALIZED)
		console_init_early();

	if (newcdev->devname) {
		dev->id = newcdev->devid;
		dev_set_name(dev, newcdev->devname);
	} else {
		dev->id = DEVICE_ID_DYNAMIC;
		dev_set_name(dev, "cs");
	}

	if (newcdev->dev)
		dev->parent = newcdev->dev;
	platform_device_register(dev);

	if (!newcdev->devname)
		newcdev->devname = xstrdup(dev_name(dev));

	newcdev->open_count = 0;

	/*
	 * If our console device is a serdev, we skip the creation of
	 * corresponding entry in /dev as well as registration in
	 * console_list and just go straight to populating child
	 * devices.
	 */
	if (serdev_node)
		return of_platform_populate(serdev_node, NULL, dev);

	if (newcdev->setbrg) {
		ret = newcdev->setbrg(newcdev, CONFIG_BAUDRATE);
		if (ret)
			return ret;
		newcdev->baudrate_param = newcdev->baudrate = CONFIG_BAUDRATE;
		dev_add_param_uint32(dev, "baudrate", console_baudrate_set,
			NULL, &newcdev->baudrate_param, "%u", newcdev);
	}

	if (newcdev->putc && !newcdev->puts)
		newcdev->puts = __console_puts;

	dev_add_param_string(dev, "active", console_active_set, console_active_get,
			     &newcdev->active_string, newcdev);

	if (IS_ENABLED(CONFIG_CONSOLE_ACTIVATE_FIRST)) {
		if (list_empty(&console_list))
			activate = 1;
	} else if (IS_ENABLED(CONFIG_CONSOLE_ACTIVATE_ALL)) {
		activate = 1;
	}

	if (newcdev->dev && of_device_is_stdout_path(newcdev->dev)) {
		activate = 1;
		console_set_stdoutpath(newcdev);
	}

	list_add_tail(&newcdev->list, &console_list);

	if (activate)
		console_set_active(newcdev, CONSOLE_STDIN |
				CONSOLE_STDOUT | CONSOLE_STDERR);

	/* expose console as device in fs */
	newcdev->devfs.name = basprintf("%s%d", newcdev->class_dev.name,
					newcdev->class_dev.id);
	newcdev->devfs.priv = newcdev;
	newcdev->devfs.dev = dev;
	newcdev->devfs.ops = &newcdev->fops;
	newcdev->devfs.flags = DEVFS_IS_CHARACTER_DEV;
	newcdev->fops.open = fops_open;
	newcdev->fops.close = fops_close;
	newcdev->fops.flush = fops_flush;
	newcdev->fops.write = fops_write;

	ret = devfs_create(&newcdev->devfs);

	if (ret) {
		pr_err("devfs entry creation failed: %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(console_register);

int console_unregister(struct console_device *cdev)
{
	struct device_d *dev = &cdev->class_dev;
	int status;

	/*
	 * We don't do any sophisticated serdev device de-population
	 * and instead claim this console busy, preventing its
	 * de-initialization, 'till the very end of our execution.
	 */
	if (console_is_serdev_node(cdev))
		return -EBUSY;

	devfs_remove(&cdev->devfs);

	list_del(&cdev->list);
	if (list_empty(&console_list))
		initialized = CONSOLE_UNINITIALIZED;

	status = unregister_device(dev);
	if (!status)
		memset(cdev, 0, sizeof(*cdev));
	return status;
}
EXPORT_SYMBOL(console_unregister);

static int getc_raw(void)
{
	struct console_device *cdev;
	int active = 0;

	while (1) {
		for_each_console(cdev) {
			if (!(cdev->f_active & CONSOLE_STDIN))
				continue;
			active = 1;
			if (cdev->tstc(cdev)) {
				int ch = cdev->getc(cdev);

				if (IS_ENABLED(CONFIG_RATP) && ch == 0x01) {
					barebox_ratp(cdev);
					return -1;
				}

				return ch;
			}
		}
		if (!active)
			/* no active console found. bail out */
			return -1;
	}
}

static int tstc_raw(void)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (!(cdev->f_active & CONSOLE_STDIN))
			continue;
		if (cdev->tstc(cdev))
			return 1;
	}

	return 0;
}

int getchar(void)
{
	unsigned char ch;
	uint64_t start;

	/*
	 * For 100us we read the characters from the serial driver
	 * into a kfifo. This helps us not to lose characters
	 * in small hardware fifos.
	 */
	start = get_time_ns();
	while (1) {
		if (tstc_raw()) {
			int c = getc_raw();

			if (c < 0)
				break;

			kfifo_putc(console_input_fifo, c);

			start = get_time_ns();
		}

		if (is_timeout(start, 100 * USECOND) &&
				kfifo_len(console_input_fifo))
			break;
	}

	if (!kfifo_len(console_input_fifo))
		return -1;

	kfifo_getc(console_input_fifo, &ch);

	return ch;
}
EXPORT_SYMBOL(getchar);

int tstc(void)
{
	return kfifo_len(console_input_fifo) || tstc_raw();
}
EXPORT_SYMBOL(tstc);

void console_putc(unsigned int ch, char c)
{
	struct console_device *cdev;
	int init = initialized;

	switch (init) {
	case CONSOLE_UNINITIALIZED:
		console_init_early();
		/* fall through */

	case CONSOLE_INITIALIZED_BUFFER:
		kfifo_putc(console_output_fifo, c);
		if (c == '\n')
			putc_ll('\r');
		putc_ll(c);
		return;

	case CONSOLE_INIT_FULL:
		for_each_console(cdev) {
			if (cdev->f_active & ch) {
				if (c == '\n')
					cdev->putc(cdev, '\r');
				cdev->putc(cdev, c);
			}
		}
		return;
	default:
		/* If we have problems inititalizing our data
		 * get them early
		 */
		hang();
	}
}
EXPORT_SYMBOL(console_putc);

int console_puts(unsigned int ch, const char *str)
{
	struct console_device *cdev;
	const char *s = str;
	int n = 0;

	if (initialized == CONSOLE_INIT_FULL) {
		for_each_console(cdev) {
			if (cdev->f_active & ch) {
				n = cdev->puts(cdev, str, strlen(str));
			}
		}
		return n;
	}

	while (*s) {
		if (*s == '\n')
			n++;

		console_putc(ch, *s);
		n++;
		s++;
	}
	return n;
}
EXPORT_SYMBOL(console_puts);

void console_flush(void)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (cdev->flush)
			cdev->flush(cdev);
	}
}
EXPORT_SYMBOL(console_flush);

static int ctrlc_abort;
static int ctrlc_allowed;

void ctrlc_handled(void)
{
	ctrlc_abort = 0;
}

/* test if ctrl-c was pressed */
int ctrlc(void)
{
	int ret = 0;

	if (!ctrlc_allowed)
		return 0;

	if (ctrlc_abort)
		return 1;

	poller_call();

#ifdef ARCH_HAS_CTRLC
	ret = arch_ctrlc();
#else
	if (tstc() && getchar() == 3)
		ret = 1;
#endif

	if (ret)
		ctrlc_abort = 1;

	return ret;
}
EXPORT_SYMBOL(ctrlc);

static int console_ctrlc_init(void)
{
	globalvar_add_simple_bool("console.ctrlc_allowed", &ctrlc_allowed);
	return 0;
}
device_initcall(console_ctrlc_init);

void console_ctrlc_allow(void)
{
	ctrlc_allowed = 1;
}

void console_ctrlc_forbid(void)
{
	ctrlc_allowed = 0;
}

BAREBOX_MAGICVAR_NAMED(global_console_ctrlc_allowed, global.console.ctrlc_allowed,
		"If true, scripts can be aborted with ctrl-c");

BAREBOX_MAGICVAR_NAMED(global_linux_bootargs_console, global.linux.bootargs.console,
		"console= argument for Linux from the stdout-path property in /chosen node");
