#include <common.h>
#include <malloc.h>
#include <globalvar.h>
#include <init.h>

static struct device_d global_device = {
	.name = "global",
	.id = DEVICE_ID_SINGLE,
};

int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	return dev_add_param(&global_device, name, set, get, flags);
}

/*
 * globalvar_get_match
 *
 * get a concatenated string of all globalvars beginning with 'match'.
 * This adds whitespaces between the different globalvars
 */
char *globalvar_get_match(const char *match, const char *seperator)
{
	char *val = NULL;
	struct param_d *param;

	list_for_each_entry(param, &global_device.parameters, list) {
		if (!strncmp(match, param->name, strlen(match))) {
			const char *p = dev_get_param(&global_device, param->name);
			if (val) {
				char *new = asprintf("%s%s%s", val, seperator, p);
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
int globalvar_add_simple(const char *name)
{
	return globalvar_add(name, NULL, NULL, 0);
}

static int globalvar_init(void)
{
	register_device(&global_device);

	return 0;
}
postconsole_initcall(globalvar_init);
