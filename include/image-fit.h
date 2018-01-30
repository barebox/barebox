/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) Jan Lübbe, 2014
 */

#ifndef __IMAGE_FIT_H__
#define __IMAGE_FIT_H__

#include <linux/types.h>
#include <bootm.h>

struct fit_handle {
	const void *fit;
	void *fit_alloc;
	size_t size;

	bool verbose;
	enum bootm_verify verify;

	struct device_node *root;
	struct device_node *images;
	struct device_node *configurations;
};

struct fit_handle *fit_open(const char *filename, bool verbose,
			    enum bootm_verify verify);
struct fit_handle *fit_open_buf(const void *buf, size_t len, bool verbose,
				enum bootm_verify verify);
void *fit_open_configuration(struct fit_handle *handle, const char *name);
int fit_has_image(struct fit_handle *handle, void *configuration,
		  const char *name);
int fit_open_image(struct fit_handle *handle, void *configuration,
		   const char *name, const void **outdata,
		   unsigned long *outsize);

void fit_close(struct fit_handle *handle);

#endif	/* __IMAGE_FIT_H__ */
