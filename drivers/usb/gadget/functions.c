#include <common.h>
#include <linux/err.h>

#include <usb/composite.h>

static LIST_HEAD(func_list);

static struct usb_function_instance *try_get_usb_function_instance(const char *name)
{
	struct usb_function_driver *fd;
	struct usb_function_instance *fi;

	fi = ERR_PTR(-ENOENT);

	list_for_each_entry(fd, &func_list, list) {

		if (strcmp(name, fd->name))
			continue;

		fi = fd->alloc_inst();
		if (!IS_ERR(fi))
			fi->fd = fd;
		break;
	}

	return fi;
}

struct usb_function_instance *usb_get_function_instance(const char *name)
{
	struct usb_function_instance *fi;
	int ret;

	fi = try_get_usb_function_instance(name);
	if (!IS_ERR(fi))
		return fi;
	ret = PTR_ERR(fi);
	if (ret != -ENOENT)
		return fi;
	return try_get_usb_function_instance(name);
}
EXPORT_SYMBOL_GPL(usb_get_function_instance);

struct usb_function *usb_get_function(struct usb_function_instance *fi)
{
	struct usb_function *f;

	f = fi->fd->alloc_func(fi);
	if (IS_ERR(f))
		return f;
	f->fi = fi;
	return f;
}
EXPORT_SYMBOL_GPL(usb_get_function);

void usb_put_function_instance(struct usb_function_instance *fi)
{
	struct module *mod;

	if (!fi)
		return;

	mod = fi->fd->mod;
	fi->free_func_inst(fi);
}
EXPORT_SYMBOL_GPL(usb_put_function_instance);

void usb_put_function(struct usb_function *f)
{
	if (!f)
		return;

	f->free_func(f);
}
EXPORT_SYMBOL_GPL(usb_put_function);

int usb_function_register(struct usb_function_driver *newf)
{
	struct usb_function_driver *fd;
	int ret;

	ret = -EEXIST;

	list_for_each_entry(fd, &func_list, list) {
		if (!strcmp(fd->name, newf->name))
			goto out;
	}
	ret = 0;
	list_add_tail(&newf->list, &func_list);
out:
	return ret;
}
EXPORT_SYMBOL_GPL(usb_function_register);

void usb_function_unregister(struct usb_function_driver *fd)
{
	list_del(&fd->list);
}
EXPORT_SYMBOL_GPL(usb_function_unregister);
