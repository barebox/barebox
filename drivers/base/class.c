// SPDX-License-Identifier: GPL-2.0-only
#include <device.h>
#include <driver.h>
#include <linux/list.h>

LIST_HEAD(class_list);

void class_register(struct class *class)
{
	list_add_tail(&class->list, &class_list);
}

int class_add_device(struct class *class, struct device *dev)
{
	list_add_tail(&dev->class_list, &class->devices);

	return 0;
}

static int __class_register_device(struct device *class_dev, const char *name, int id)
{
	class_dev->id = id;
	dev_set_name(class_dev, name);

	return register_device(class_dev);
}

int class_register_device(struct class *class,
			  struct device *class_dev,
			  const char *name)
{
	struct device *parent = class_dev->parent;
	const char *alias = NULL;
	int ret = 0;

	if (dev_of_node(parent))
		alias = of_alias_get(parent->of_node);

	if (alias)
		ret = __class_register_device(class_dev, alias, DEVICE_ID_SINGLE);

	if (!alias || ret)
		ret = __class_register_device(class_dev, name, DEVICE_ID_DYNAMIC);

	if (ret)
		return ret;

	class_add_device(class, class_dev);
	return 0;
}

void class_unregister_device(struct device *class_dev)
{
	list_del(&class_dev->class_list);
	unregister_device(class_dev);
}

void class_remove_device(struct class *class, struct device *dev)
{
	list_del(&dev->class_list);
}
