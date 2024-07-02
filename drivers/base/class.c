// SPDX-License-Identifier: GPL-2.0-only
#include <device.h>
#include <driver.h>
#include <linux/list.h>

LIST_HEAD(class_list);

static int class_register(struct class *class)
{
	list_add_tail(&class->list, &class_list);

	return 0;
}

int class_add_device(struct class *class, struct device *dev)
{
	list_add_tail(&dev->class_list, &class->devices);

	return 0;
}

void class_remove_device(struct class *class, struct device *dev)
{
	list_del(&dev->class_list);
}

extern struct class __barebox_class_start[];
extern struct class __barebox_class_end[];

static int register_classes(void)
{
	struct class *c;

	for (c = __barebox_class_start;
	     c != __barebox_class_end;
	     c++)
		class_register(c);

	return 0;
}
device_initcall(register_classes);
