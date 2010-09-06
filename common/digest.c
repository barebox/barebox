/*
 * (C) Copyright 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <digest.h>
#include <malloc.h>
#include <errno.h>
#include <module.h>
#include <linux/err.h>

static LIST_HEAD(digests);

static int dummy_init(struct digest *d)
{
	return 0;
}

int digest_register(struct digest *d)
{
	if (!d || !d->name || !d->update || !d->final || d->length < 1)
		return -EINVAL;

	if (!d->init)
		d->init = dummy_init;

	if (digest_get_by_name(d->name))
		return -EEXIST;

	list_add_tail(&d->list, &digests);

	return 0;
}
EXPORT_SYMBOL(digest_register);

void digest_unregister(struct digest *d)
{
	if (!d)
		return;

	list_del(&d->list);
}
EXPORT_SYMBOL(digest_unregister);

struct digest* digest_get_by_name(char* name)
{
	struct digest* d;

	if (!name)
		return NULL;

	list_for_each_entry(d, &digests, list) {
		if(strcmp(d->name, name) == 0)
			return d;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(digest_get_by_name);
