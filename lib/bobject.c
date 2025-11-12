// SPDX-License-Identifier: GPL-2.0-only

#include <bobject.h>
#include <stdio.h>

/**
 * bobject_set_name - set a barebox object's name
 * @bobj: barebox object or device
 * @fmt: format string for the object's name
 *
 * NOTE: This function expects bobj->name to be free()-able, so extra
 * precautions needs to be taken when mixing its usage with manual
 * assignement of bobject.name.
 */
int bobject_set_name(bobject_t bobj, const char *fmt, ...)
{
	va_list vargs;
	int err;
	/*
	 * Save old pointer in case we are overriding already set name
	 */
	char *oldname = bobj.bobj->name;

	va_start(vargs, fmt);
	err = vasprintf(&bobj.bobj->name, fmt, vargs);
	va_end(vargs);

	/*
	 * Free old pointer, we do this after vasprintf call in case
	 * old device name was in one of vargs
	 */
	free(oldname);

	return WARN_ON(err < 0) ? err : 0;
}
EXPORT_SYMBOL_GPL(bobject_set_name);

struct bobject *bobject_alloc(const char *name)
{
	struct bobject *bobj = xzalloc(sizeof(*bobj));

	bobject_init(bobj);
	bobject_set_name(bobj, "%s", name);

	return bobj;
}
EXPORT_SYMBOL_GPL(bobject_alloc);

void bobject_free(struct bobject *bobj)
{
	if (!bobj)
		return;

	bobject_del(bobj);
	free(bobj);
}
EXPORT_SYMBOL_GPL(bobject_free);

/**
 * bobject_del - remove all parameters from a bobject and free their
 * memory
 * @param bobject	The barebox object
 */
void bobject_del(struct bobject *bobj)
{
	struct param_d *p, *n;

	list_for_each_entry_safe(p, n, &bobj->parameters, list)
		param_remove(p);

	free(bobj->name);
}
EXPORT_SYMBOL(bobject_del);
