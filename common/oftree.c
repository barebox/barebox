#include <common.h>
#include <environment.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>
#include <boot.h>
#include <bootsource.h>
#include <i2c/i2c.h>
#include <reset_source.h>

#define MAX_LEVEL	32		/* how deeply nested we will go */

static int is_printable_string(const void *data, int len)
{
	const char *s = data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero */
	if (s[len - 1] != '\0')
		return 0;

	/* printable or a null byte (concatenated strings) */
	while (((*s == '\0') || isprint(*s)) && (len > 0)) {
		/*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

/*
 * Print the property in the best format, a heuristic guess.  Print as
 * a string, concatenated strings, a byte, word, double word, or (if all
 * else fails) it is printed as a stream of bytes.
 */
void of_print_property(const void *data, int len)
{
	int j;

	/* no data, don't print */
	if (len == 0)
		return;

	/*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
	if (is_printable_string(data, len)) {
		puts("\"");
		j = 0;
		while (j < len) {
			if (j > 0)
				puts("\", \"");
			puts(data);
			j    += strlen(data) + 1;
			data += strlen(data) + 1;
		}
		puts("\"");
		return;
	}

	if ((len % 4) == 0) {
		const u32 *p;

		printf("<");
		for (j = 0, p = data; j < len/4; j ++)
			printf("0x%x%s", fdt32_to_cpu(p[j]), j < (len/4 - 1) ? " " : "");
		printf(">");
	} else { /* anything else... hexdump */
		const u8 *s;

		printf("[");
		for (j = 0, s = data; j < len; j++)
			printf("%02x%s", s[j], j < len - 1 ? " " : "");
		printf("]");
	}
}

void of_print_cmdline(struct device_node *root)
{
	struct device_node *node = of_find_node_by_path_from(root, "/chosen");
	const char *cmdline;

	if (!node) {
		printf("commandline: no /chosen node\n");
		return;
	}

	cmdline = of_get_property(node, "bootargs", NULL);

	printf("commandline: %s\n", cmdline);
}

static int of_fixup_bootargs_bootsource(struct device_node *root,
					struct device_node *chosen)
{
	char *alias_name = bootsource_get_alias_name();
	struct device_node *bootsource;
	int ret = 0;

	if (!alias_name)
		return 0;

	bootsource = of_find_node_by_alias(root, alias_name);
	/*
	 * If kernel DTB doesn't have the appropriate alias set up,
	 * give up and exit early. No error is reported.
	 */
	if (bootsource)
		ret = of_set_property(chosen, "bootsource", bootsource->full_name,
				      strlen(bootsource->full_name) + 1, true);

	free(alias_name);
	return ret;
}

static int of_fixup_bootargs(struct device_node *root, void *unused)
{
	struct device_node *node;
	const char *str;
	int err;
	int instance = reset_source_get_instance();
	struct device_d *dev;

	str = linux_bootargs_get();
	if (!str)
		return 0;

	str = skip_spaces(str);
	if (strlen(str) == 0)
		return 0;

	node = of_create_node(root, "/chosen");
	if (!node)
		return -ENOMEM;

	of_property_write_string(node, "barebox-version", release_string);

	err = of_property_write_string(node, "bootargs", str);
	if (err)
		return err;

	of_property_write_string(node, "reset-source", reset_source_name());
	if (instance >= 0)
		of_property_write_u32(node, "reset-source-instance", instance);


	dev = reset_source_get_device();
	if (dev && dev->device_node) {
		phandle phandle;

		phandle = of_node_create_phandle(dev->device_node);

		err = of_property_write_u32(node,
					    "reset-source-device", phandle);
		if (err)
			return err;
	}

	return of_fixup_bootargs_bootsource(root, node);
}

static int of_register_bootargs_fixup(void)
{
	return of_register_fixup(of_fixup_bootargs, NULL);
}
late_initcall(of_register_bootargs_fixup);

struct of_fixup_status_data {
	const char *path;
	bool status;
};

static int of_fixup_status(struct device_node *root, void *context)
{
	const struct of_fixup_status_data *data = context;
	struct device_node *node;

	node = of_find_node_by_path_or_alias(root, data->path);
	if (!node)
		return -ENODEV;

	if (data->status)
		return of_device_enable(node);
	else
		return of_device_disable(node);
}

/**
 * of_register_set_status_fixup - register fix up to set status of nodes
 * Register a fixup to enable or disable a node in the devicet tree by
 * passing the path or alias.
 */
int of_register_set_status_fixup(const char *path, bool status)
{
	struct of_fixup_status_data *data;

	data = xzalloc(sizeof(*data));
	data->path = path;
	data->status = status;

	return of_register_fixup(of_fixup_status, (void *)data);
}

struct of_fixup {
	int (*fixup)(struct device_node *, void *);
	void *context;
	struct list_head list;
};

static LIST_HEAD(of_fixup_list);

int of_register_fixup(int (*fixup)(struct device_node *, void *), void *context)
{
	struct of_fixup *of_fixup = xzalloc(sizeof(*of_fixup));

	of_fixup->fixup = fixup;
	of_fixup->context = context;

	list_add_tail(&of_fixup->list, &of_fixup_list);

	return 0;
}

/*
 * Remove a previously registered fixup. Only the first (if any) is removed.
 * Returns 0 if a match was found (and removed), -ENOENT otherwise.
 */
int of_unregister_fixup(int (*fixup)(struct device_node *, void *),
			void *context)
{
	struct of_fixup *of_fixup;

	list_for_each_entry(of_fixup, &of_fixup_list, list) {
		if (of_fixup->fixup == fixup && of_fixup->context == context) {
			list_del(&of_fixup->list);
			return 0;
		}
	}

	return -ENOENT;
}

/*
 * Apply registered fixups for the given fdt. The fdt must have
 * enough free space to apply the fixups.
 */
int of_fix_tree(struct device_node *node)
{
	struct of_fixup *of_fixup;
	int ret;

	list_for_each_entry(of_fixup, &of_fixup_list, list) {
		ret = of_fixup->fixup(node, of_fixup->context);
		if (ret)
			pr_warn("Failed to fixup node in %pS: %s\n",
					of_fixup->fixup, strerror(-ret));
	}

	return 0;
}

/*
 * Get the fixed fdt. This function uses the fdt input pointer
 * if provided or the barebox internal devicetree if not.
 * It increases the size of the tree and applies the registered
 * fixups.
 */
struct fdt_header *of_get_fixed_tree(struct device_node *node)
{
	int ret;
	struct fdt_header *fdt;

	if (!node) {
		node = of_get_root_node();
		if (!node)
			return NULL;
	}

	ret = of_fix_tree(node);
	if (ret)
		return NULL;

	fdt = of_flatten_dtb(node);
	if (!fdt)
		return NULL;

	return fdt;
}

/**
 * of_autoenable_device_by_path() - Autoenable a device by a device tree path
 * @param path Device tree path up from the root to the device
 * @return 0 on success, -enodev on failure. If no device found in the device
 * tree.
 *
 * This function will search for a device and will enable it in the kernel
 * device tree, if it exists and is loaded.
 */
int of_autoenable_device_by_path(char *path)
{
	struct device_node *node;
	int ret;

	node = of_find_node_by_name(NULL, path);
	if (!node)
		node = of_find_node_by_path(path);

	if (!node)
		return -ENODEV;

	if (!of_device_is_available(node))
			return -ENODEV;

	ret = of_register_set_status_fixup(path, 1);
	if (!ret)
		printf("autoenabled %s\n", node->name);
	return ret;
}

/**
 * of_autoenable_i2c_by_component - Autoenable a i2c client by a device tree path
 * @param path Device tree path up from the root to the i2c client
 * @return 0 on success, -enodev on failure. If no i2c client found in the i2c
 * device tree.
 *
 * This function will search for a i2c client, tries to write to the client and
 * will enable it in the kernel device tree, if it exists and is accessible.
 */
int of_autoenable_i2c_by_component(char *path)
{
	struct device_node *node;
	struct i2c_adapter *i2c_adapter;
	struct i2c_msg msg;
	char data[1] = {0x0};
	int ret;
	uint32_t addr;

	if (!IS_ENABLED(CONFIG_I2C))
		return -ENODEV;

	node = of_find_node_by_name(NULL, path);
	if (!node)
		node = of_find_node_by_path(path);
	if (!node)
		return -ENODEV;
	if (!node->parent)
		return -ENODEV;

	ret = of_property_read_u32(node, "reg", &addr);
	if (ret)
		return -ENODEV;

	i2c_adapter = of_find_i2c_adapter_by_node(node->parent);
	if (!i2c_adapter)
		return -ENODEV;

	msg.buf = data;
	msg.addr = addr;
	msg.len = 1;

	/* Try to communicate with the i2c client */
	ret = i2c_transfer(i2c_adapter, &msg, 1);
	if (ret == -EREMOTEIO)
		return -ENODEV;
	if (ret < 1) {
		printf("failed to autoenable i2c device on address 0x%x with %i\n",
								addr, ret);
		return ret;
	}

	ret = of_register_set_status_fixup(path, 1);
	if (!ret)
		printf("autoenabled i2c device %s\n", node->name);

	return ret;
}
