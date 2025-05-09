// SPDX-License-Identifier: GPL-2.0-only

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
#include <envfs.h>
#include <fnmatch.h>

static int nv_dirty;

struct device global_device = {
	.name = "global",
	.id = DEVICE_ID_SINGLE,
};

struct device nv_device = {
	.name = "nv",
	.id = DEVICE_ID_SINGLE,
};

void nv_var_set_clean(void)
{
	nv_dirty = 0;
}

void globalvar_remove(const char *name)
{
	struct param_d *p, *tmp;

	list_for_each_entry_safe(p, tmp, &global_device.parameters, list) {
		if (fnmatch(name, p->name, 0))
			continue;

		dev_remove_param(p);
	}
}

static int __nv_save(const char *prefix, const char *name, const char *val)
{
	int fd, ret;
	char *fname;

	ret = make_directory(prefix);
	if (ret)
		return ret;

	fname = basprintf("%s/%s", prefix, name);

	fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC);

	free(fname);

	if (fd < 0)
		return fd;

	dprintf(fd, "%s", val);

	close(fd);

	return 0;
}

static int nv_save(const char *name, const char *val)
{
	int ret;
	static int once = 1;

	ret = __nv_save("/env/nv", name, val);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_ENV_HANDLING)) {
		if (once) {
			pr_info("nv variable modified, will save nv variables on shutdown\n");
			once = 0;
		}
		nv_dirty = 1;
	} else {
		if (once) {
			pr_info("nv variable modified, but won't be saved in this configuration\n");
			once = 0;
		}
	}

	return 0;
}

/**
 * dev_param_init_from_nv - initialize a device parameter from nv variable
 * @dev: The device
 * @name: The parameter name
 *
 * This function initializes a newly created device parameter from the corresponding
 * nv.dev.<devname>.<paramname> variable.
 */
void dev_param_init_from_nv(struct device *dev, const char *name)
{
	char *nvname;
	const char *val;
	int ret = 0;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return;

	if (dev == &nv_device)
		return;
	if (dev == &global_device)
		return;

	nvname = basprintf("dev.%s.%s", dev_name(dev), name);
	val = dev_get_param(&nv_device, nvname);
	if (val) {
		ret = dev_set_param(dev, name, val);
		if (ret)
			pr_err("Cannot init param from nv: %s.%s=%s: %pe\n",
				dev_name(dev), name, val, ERR_PTR(ret));
	}

	free(nvname);
}

/**
 * nvvar_device_dispatch - dispatch dev.<dev>.<param> name into device and parameter name
 * @name: The incoming name in the form dev.<dev>.<param>
 * @dev: The returned device * belonging to <dev>
 * @pname: the parameter name
 *
 * Given a dev.<dev>.<param> string this function finds the device * belonging to
 * <dev> and the parameter name from <param>.
 *
 * Return: When incoming string does not belong to the device namespace (does not begin
 * with "dev." this function returns 0. A value > 0 is returned when the incoming string
 * is in the device namespace and the string can be dispatched into a device * and a
 * parameter name. A negative error code is returned when the incoming string belongs to
 * the device namespace, but cannot be dispatched.
 */
static int nvvar_device_dispatch(const char *name, struct device **dev,
				 const char **pname)
{
	char *devname;
	const char *dot;
	int dotpos;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return -ENOSYS;

	*dev = NULL;

	if (strncmp(name, "dev.", 4))
		return 0;

	name += 4;

	dot = strchr(name, '.');
	if (!dot)
		return -EINVAL;

	dotpos = dot - name;

	devname = xstrndup(name, dotpos);
	*dev = get_device_by_name(devname);
	free(devname);

	if (*dev == &nv_device || *dev == &global_device)
		return -EINVAL;

	*pname = dot + 1;

	return 1;
}

static int nv_set(struct device *dev, struct param_d *p, const char *name,
		  const char *val)
{
	int ret;

	if (val) {
		ret = dev_set_param(&global_device, name, val);
		if (ret)
			return ret;
	}

	if (p) {
		free_const(p->value);
		p->value = xstrdup(val);
	}

	return 0;
}

static const char *nv_param_get(struct device *dev, struct param_d *p)
{
	return p->value ? p->value : "";
}

static int nv_param_set(struct device *dev, struct param_d *p,
			const char *val)
{
	int ret;

	ret = nv_set(dev, p, p->name, val);
	if (ret)
		return ret;

	return nv_save(p->name, val);
}

static int __nvvar_add(const char *name, const char *value)
{
	struct param_d *p;
	struct device *dev = NULL;
	const char *pname;
	int ret;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return -ENOSYS;

	/* Get param. If it doesn't exist yet, create it */
	p = get_param_by_name(&nv_device, name);
	if (!p) {
		p = dev_add_param(&nv_device, name, nv_param_set, nv_param_get, 0);
		if (IS_ERR(p))
			return PTR_ERR(p);
	}

	/* Create corresponding globalvar if it doesn't exist yet */
	ret = globalvar_add_simple(name, value);
	if (ret && ret != -EEXIST)
		return ret;

	if (value)
		return nv_set(&nv_device, p, name, value);

	ret = nvvar_device_dispatch(name, &dev, &pname);
	if (ret > 0)
		value = dev_get_param(dev, pname);
	else
		value = dev_get_param(&global_device, name);

	if (value && p) {
		free_const(p->value);
		p->value = xstrdup(value);
	}

	return 0;
}

int nvvar_add(const char *name, const char *value)
{
	int ret;

	if (!strncmp(name, "nv.", 3))
		name += 3;

	ret = __nvvar_add(name, value);
	if (ret)
		return ret;

	return nv_save(name, value);
}

int nvvar_remove(const char *name)
{
	struct param_d *p, *tmp;
	char *fname;
	int ret = -ENOENT;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return -ENOSYS;

	list_for_each_entry_safe(p, tmp, &nv_device.parameters, list) {
		if (fnmatch(name, p->name, 0))
			continue;

		fname = basprintf("/env/nv/%s", p->name);

		dev_remove_param(p);

		unlink(fname);
		free(fname);

		ret = 0;
	}

	return ret;
}

struct globalvar_deprecated {
	char *newname;
	char *oldname;
	struct list_head list;
};

static LIST_HEAD(globalvar_deprecated_list);

/*
 * globalvar_alias_deprecated - add an alias
 *
 * @oldname: The old name for the variable
 * @newname: The new name for the variable
 *
 * This function is a helper for globalvars that are renamed from one
 * release to another. when a variable @oldname is found in the persistent
 * environment, a warning is issued and its value is written to @newname.
 *
 * Note that when both @oldname and @newname contain values, then the values
 * existing in @newname are overwritten.
 */
void globalvar_alias_deprecated(const char *oldname, const char *newname)
{
	struct globalvar_deprecated *gd;

	gd = xzalloc(sizeof(*gd));
	gd->newname = xstrdup(newname);
	gd->oldname = xstrdup(oldname);
	list_add_tail(&gd->list, &globalvar_deprecated_list);
}

static const char *globalvar_new_name(const char *oldname)
{
	struct globalvar_deprecated *gd;

	list_for_each_entry(gd, &globalvar_deprecated_list, list) {
		if (!strcmp(oldname, gd->oldname)) {
			pr_warn("nv.%s is deprecated, converting to nv.%s\n", oldname,
				gd->newname);
			nv_dirty = 1;
			return gd->newname;
		}
	}

	return oldname;
}

int nvvar_load(void)
{
	char *val;
	int ret;
	DIR *dir;
	struct dirent *d;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return -ENOSYS;

	dir = opendir("/env/nv");
	if (!dir)
		return -ENOENT;

	while ((d = readdir(dir))) {
		const char *n;

		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		val = read_file_line("/env/nv/%s", d->d_name);

		pr_debug("%s: Setting \"%s\" to \"%s\"\n",
				__func__, d->d_name, val);

		n = globalvar_new_name(d->d_name);
		ret = __nvvar_add(n, val);
		if (ret)
			pr_err("failed to create nv variable %s: %pe\n",
					n, ERR_PTR(ret));
		free(val);
	}

	closedir(dir);

	return 0;
}

static void device_param_print(struct device *dev)
{
	struct param_d *param;

	list_for_each_entry(param, &dev->parameters, list) {
		const char *p = dev_get_param(dev, param->name);
		const char *nv = NULL;

		if (IS_ENABLED(CONFIG_NVVAR) && dev != &nv_device)
			nv = dev_get_param(&nv_device, param->name);

		printf("%s%s: %s", nv ? "* " : "  ", param->name, p);
		if (param->info)
			param->info(param);
		printf("\n");
	}
}

void nvvar_print(void)
{
	if (!IS_ENABLED(CONFIG_NVVAR))
		return;

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
				char *new = basprintf("%s%s%s", val,
							separator, p);
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

void globalvar_set(const char *name, const char *val)
{
	dev_set_param(&global_device, name, val);
}

static int globalvar_simple_set(struct device *dev, struct param_d *p,
				const char *val)
{
	struct device *rdev;
	const char *pname = NULL;
	int ret;

	ret = nvvar_device_dispatch(p->name, &rdev, &pname);
	if (ret < 0)
		return ret;

	if (ret && rdev) {
		ret = dev_set_param(rdev, pname, val);
		if (ret)
			pr_err("Cannot init param from global: %s.%s=%s: %pe\n",
				dev_name(rdev), pname, val, ERR_PTR(ret));
	}

	/* Pass to the generic function we have overwritten */
	return dev_param_set_generic(dev, p, val);
}

/*
 * globalvar_add_simple
 *
 * add a new globalvar named 'name'
 */
int globalvar_add_simple(const char *name, const char *value)
{
	struct param_d *param;

	param = dev_add_param(&global_device, name, globalvar_simple_set, NULL,
			      PARAM_GLOBALVAR_UNQUALIFIED);
	if (IS_ERR(param)) {
		if (PTR_ERR(param) != -EEXIST)
			return PTR_ERR(param);
	}

	if (!value)
		return 0;

	return dev_set_param(&global_device, name, value);
}

static int globalvar_remove_unqualified(const char *name)
{
	struct param_d *p;

	p = get_param_by_name(&global_device, name);
	if (!p)
		return 0;

	if (!(p->flags & PARAM_GLOBALVAR_UNQUALIFIED))
		return -EEXIST;

	dev_remove_param(p);

	return 0;
}

static void globalvar_nv_sync(const char *name)
{
	const char *val;

	if (!IS_ENABLED(CONFIG_NVVAR))
		return;

	val = dev_get_param(&nv_device, name);
	if (val)
		dev_set_param(&global_device, name, val);
}

int globalvar_add_simple_string(const char *name, char **value)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_string(&global_device, name, NULL, NULL,
		value, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	if (!*value)
		*value = xstrdup("");

	return 0;
}

int globalvar_add_simple_int(const char *name, int *value,
			     const char *format)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_int(&global_device, name, NULL, NULL,
		value, format, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	return 0;
}

int globalvar_add_simple_uint64(const char *name, u64 *value,
				const char *format)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_uint64(&global_device, name, NULL, NULL,
		value, format, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	return 0;
}

int globalvar_add_bool(const char *name,
		       int (*set)(struct param_d *, void *),
		       int *value, void *priv)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_bool(&global_device, name, set, NULL,
		value, priv);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	return 0;
}

int globalvar_add_simple_enum(const char *name,	int *value,
			      const char * const *names, int max)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_enum(&global_device, name, NULL, NULL,
		value, names, max, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	return 0;
}

int globalvar_add_simple_bitmask(const char *name, unsigned long *value,
				 const char * const *names, int max)
{
	struct param_d *p;

	p = dev_add_param_bitmask(&global_device, name, NULL, NULL,
		value, names, max, NULL);

	return PTR_ERR_OR_ZERO(p);
}

int globalvar_add_simple_ip(const char *name, IPaddr_t *ip)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_ip(&global_device, name, NULL, NULL,
		ip, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	globalvar_nv_sync(name);

	return 0;
}

static int globalvar_init(void)
{
	const char *endianness;

	register_device(&global_device);

	if (IS_ENABLED(CONFIG_NVVAR))
		register_device(&nv_device);

	globalvar_add_simple("version", UTS_RELEASE);

	if (strlen(buildsystem_version_string) > 0)
		globalvar_add_simple("buildsystem.version", buildsystem_version_string);

	globalvar_add_simple("arch", CONFIG_ARCH_LINUX_NAME);

#ifdef __BIG_ENDIAN
	endianness = "big";
#elif defined(__LITTLE_ENDIAN)
	endianness = "little";
#else
#error "could not determine byte order"
#endif

	globalvar_add_simple("endianness", endianness);

	return 0;
}
pure_initcall(globalvar_init);

BAREBOX_MAGICVAR(global.version, "The barebox version");
BAREBOX_MAGICVAR(global.buildsystem.version,
		 "version of buildsystem barebox was built with");
BAREBOX_MAGICVAR(global.endianness, "The barebox endianness");
BAREBOX_MAGICVAR(global.arch, "Name of architecture as used by Linux");

/**
 * nvvar_save - save NV variables to persistent environment
 *
 * This saves the NV variables to the persisitent environment without saving
 * the other files in the environment that might be changed.
 */
int nvvar_save(void)
{
	struct param_d *param;
	const char *env = default_environment_path_get();
	int ret = 0;
#define TMPDIR "/.env.tmp"
	if (!nv_dirty || !env)
		return 0;

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT))
		defaultenv_load(TMPDIR, 0);

	envfs_load(env, TMPDIR, 0);
	unlink_recursive(TMPDIR "/nv", NULL);

	list_for_each_entry(param, &nv_device.parameters, list) {
		ret = __nv_save(TMPDIR "/nv", param->name,
				dev_get_param(&nv_device, param->name));
		if (ret) {
			pr_err("Cannot save NV var: %pe\n", ERR_PTR(ret));
			goto out;
		}
	}

	envfs_save(env, TMPDIR, 0);
out:
	unlink_recursive(TMPDIR, NULL);

	if (!ret)
		nv_dirty = 0;

	return ret;
}

static void nv_exit(void)
{
	if (nv_dirty)
		pr_info("nv variables modified, saving them\n");

	nvvar_save();
}
predevshutdown_exitcall(nv_exit);

static int nv_global_param_complete(struct device *dev,
				    struct string_list *sl,
				    char *instr, int eval)
{
	struct param_d *param;
	int len;

	len = strlen(instr);

	list_for_each_entry(param, &dev->parameters, list) {
		if (strncmp(instr, param->name, len))
			continue;

		string_list_add_asprintf(sl, "%s%c",
			param->name,
			eval ? ' ' : '=');
	}

	return 0;
}

int nv_complete(struct string_list *sl, char *instr)
{
	struct device *dev;
	struct param_d *param;
	char *str;
	int len;

	nv_global_param_complete(&global_device, sl, instr, 0);

	len = strlen(instr);

	if (strncmp(instr, "dev.", min_t(int, len, 4)))
		return 0;

	for_each_device(dev) {
		if (dev == &global_device || dev == &nv_device)
			continue;

		list_for_each_entry(param, &dev->parameters, list) {
			str = basprintf("dev.%s.%s=", dev_name(dev), param->name);
			if (strncmp(instr, str, len))
				free(str);
			else
				string_list_add(sl, str);
		}
	}

	return 0;
}

int global_complete(struct string_list *sl, char *instr)
{
	nv_global_param_complete(&global_device, sl, instr, 0);

	return 0;
}
