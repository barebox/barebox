#include <common.h>
#include <malloc.h>
#include <globalvar.h>
#include <errno.h>
#include <init.h>
#include <environment.h>
#include <magicvar.h>
#include <fs.h>
#include <fcntl.h>
#include <libfile.h>
#include <generated/utsrelease.h>

struct device_d global_device = {
	.name = "global",
	.id = DEVICE_ID_SINGLE,
};

struct device_d nv_device = {
	.name = "nv",
	.id = DEVICE_ID_SINGLE,
};

int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	struct param_d *param;

	param = dev_add_param(&global_device, name, set, get, flags);
	if (IS_ERR(param))
		return PTR_ERR(param);
	return 0;
}

static int nv_save(const char *name, const char *val)
{
	int fd, ret;
	char *fname;

	ret = make_directory("/env/nv");
	if (ret)
		return ret;

	fname = asprintf("/env/nv/%s", name);

	fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC);

	free(fname);

	if (fd < 0)
		return fd;

	fprintf(fd, "%s", val);

	close(fd);

	return 0;
}

static int nv_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct param_d *gp;
	int ret;

	if (!val)
		val = "";

	gp = get_param_by_name(&global_device, p->name);
	if (!gp)
		return -EINVAL;

	ret = gp->set(&global_device, gp, val);
	if (ret)
		return ret;

	free(p->value);
	p->value = xstrdup(val);

	return nv_save(p->name, val);
}

static const char *nv_get(struct device_d *dev, struct param_d *p)
{
	return p->value ? p->value : "";
}

int nvvar_add(const char *name, const char *value)
{
	struct param_d *p, *gp;
	int ret;

	gp = get_param_by_name(&nv_device, name);
	if (gp) {
		ret = dev_set_param(&global_device, name, value);
		if (ret)
			return ret;

		ret = dev_set_param(&nv_device, name, value);
		if (ret)
			return ret;

		return 0;
	}

	ret = globalvar_add_simple(name, value);
	if (ret && ret != -EEXIST)
		return ret;

	p = dev_add_param(&nv_device, name, nv_set, nv_get, 0);
	if (IS_ERR(p))
		return PTR_ERR(p);

	if (value) {
		ret = dev_set_param(&global_device, name, value);
		if (ret)
			return ret;
	} else {
		value = dev_get_param(&global_device, name);
		if (!value)
			value = "";
	}

	p->value = xstrdup(value);

	return nv_save(p->name, value);
}

int nvvar_remove(const char *name)
{
	struct param_d *p;
	char *fname;

	p = get_param_by_name(&nv_device, name);
	if (!p)
		return -ENOENT;

	fname = asprintf("/env/nv/%s", p->name);
	unlink(fname);
	free(fname);

	list_del(&p->list);
	free(p->name);
	free(p);

	return 0;
}

int nvvar_load(void)
{
	char *val;
	int ret;
	DIR *dir;
	struct dirent *d;

	dir = opendir("/env/nv");
	if (!dir)
		return -ENOENT;

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		val = read_file_line("/env/nv/%s", d->d_name);

		pr_debug("%s: Setting \"%s\" to \"%s\"\n",
				__func__, d->d_name, val);

		ret = nvvar_add(d->d_name, val);
		if (ret)
			pr_err("failed to create nv variable %s: %s\n",
					d->d_name, strerror(-ret));
	}

	closedir(dir);

	return 0;
}

static void device_param_print(struct device_d *dev)
{
	struct param_d *param;

	list_for_each_entry(param, &dev->parameters, list) {
		const char *p = dev_get_param(dev, param->name);
		const char *nv = NULL;

		if (dev != &nv_device)
			nv = dev_get_param(&nv_device, param->name);

		printf("%s%s: %s\n", nv ? "* " : "  ", param->name, p);
	}
}

void nvvar_print(void)
{
	device_param_print(&nv_device);
}

void globalvar_print(void)
{
	device_param_print(&global_device);
}

/*
 * globalvar_get_match
 *
 * get a concatenated string of all globalvars beginning with 'match'.
 * This adds whitespaces between the different globalvars
 */
char *globalvar_get_match(const char *match, const char *separator)
{
	char *val = NULL;
	struct param_d *param;

	list_for_each_entry(param, &global_device.parameters, list) {
		if (!strncmp(match, param->name, strlen(match))) {
			const char *p = dev_get_param(&global_device, param->name);
			if (val) {
				char *new = asprintf("%s%s%s", val, separator, p);
				free(val);
				val = new;
			} else {
				val = xstrdup(p);
			}
		}
	}

	if (!val)
		val = xstrdup("");

	return val;
}

void globalvar_set_match(const char *match, const char *val)
{
	struct param_d *param;

	list_for_each_entry(param, &global_device.parameters, list) {
		if (!strncmp(match, param->name, strlen(match)))
			dev_set_param(&global_device, param->name, val);
	}
}

/*
 * globalvar_add_simple
 *
 * add a new globalvar named 'name'
 */
int globalvar_add_simple(const char *name, const char *value)
{
	int ret;

	ret = globalvar_add(name, NULL, NULL, 0);
	if (ret && ret != -EEXIST)
		return ret;

	if (!value)
		return 0;

	return dev_set_param(&global_device, name, value);
}

static int globalvar_init(void)
{
	register_device(&global_device);
	register_device(&nv_device);

	globalvar_add_simple("version", UTS_RELEASE);

	return 0;
}
pure_initcall(globalvar_init);

BAREBOX_MAGICVAR_NAMED(global_version, global.version, "The barebox version");
