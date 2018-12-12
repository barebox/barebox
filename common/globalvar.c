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

struct device_d global_device = {
	.name = "global",
	.id = DEVICE_ID_SINGLE,
};

struct device_d nv_device = {
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

	if (once) {
		pr_info("nv variable modified, will save nv variables on shutdown\n");
		once = 0;
	}

	nv_dirty = 1;

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
void dev_param_init_from_nv(struct device_d *dev, const char *name)
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
			pr_err("Cannot init param from nv: %s.%s=%s: %s\n",
				dev_name(dev), name, val, strerror(-ret));
	}

	free(nvname);
}

/**
 * nvvar_device_dispatch - dispatch dev.<dev>.<param> name into device and parameter name
 * @name: The incoming name in the form dev.<dev>.<param>
 * @dev: The returned device_d * belonging to <dev>
 * @pname: the parameter name
 *
 * Given a dev.<dev>.<param> string this function finds the device_d * belonging to
 * <dev> and the parameter name from <param>.
 *
 * Return: When incoming string does not belong to the device namespace (does not begin
 * with "dev." this function returns 0. A value > 0 is returned when the incoming string
 * is in the device namespace and the string can be dispatched into a device_d * and a
 * parameter name. A negative error code is returned when the incoming string belongs to
 * the device namespace, but cannot be dispatched.
 */
static int nvvar_device_dispatch(const char *name, struct device_d **dev,
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

static int nv_set(struct device_d *dev, struct param_d *p, const char *val)
{
	int ret;

	if (!val) {
		free(p->value);
		return 0;
	}

	ret = dev_set_param(&global_device, p->name, val);
	if (ret)
		return ret;

	free(p->value);
	p->value = xstrdup(val);

	return 0;
}

static const char *nv_param_get(struct device_d *dev, struct param_d *p)
{
	return p->value ? p->value : "";
}

static int nv_param_set(struct device_d *dev, struct param_d *p, const char *val)
{
	int ret;

	ret = nv_set(dev, p, val);
	if (ret)
		return ret;

	return nv_save(p->name, val);
}

static int __nvvar_add(const char *name, const char *value)
{
	struct param_d *p;
	struct device_d *dev = NULL;
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
		return nv_set(&nv_device, p, value);

	ret = nvvar_device_dispatch(name, &dev, &pname);
	if (ret > 0)
		value = dev_get_param(dev, pname);
	else
		value = dev_get_param(&global_device, name);

	if (value) {
		free(p->value);
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
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		val = read_file_line("/env/nv/%s", d->d_name);

		pr_debug("%s: Setting \"%s\" to \"%s\"\n",
				__func__, d->d_name, val);

		ret = __nvvar_add(d->d_name, val);
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

static int globalvar_simple_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct device_d *rdev;
	const char *pname = NULL;
	int ret;

	ret = nvvar_device_dispatch(p->name, &rdev, &pname);
	if (ret < 0)
		return ret;

	if (ret && rdev) {
		ret = dev_set_param(rdev, pname, val);
		if (ret)
			pr_err("Cannot init param from global: %s.%s=%s: %s\n",
				dev_name(rdev), pname, val, strerror(-ret));
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

int globalvar_add_simple_bool(const char *name, int *value)
{
	struct param_d *p;
	int ret;

	ret = globalvar_remove_unqualified(name);
	if (ret)
		return ret;

	p = dev_add_param_bool(&global_device, name, NULL, NULL,
		value, NULL);

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

	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
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
	register_device(&global_device);

	if (IS_ENABLED(CONFIG_NVVAR))
		register_device(&nv_device);

	globalvar_add_simple("version", UTS_RELEASE);

	return 0;
}
pure_initcall(globalvar_init);

BAREBOX_MAGICVAR_NAMED(global_version, global.version, "The barebox version");

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
	int ret;
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
			pr_err("Cannot save NV var: %s\n", strerror(-ret));
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

static int nv_global_param_complete(struct device_d *dev, struct string_list *sl,
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
	struct device_d *dev;
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
