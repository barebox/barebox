/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION */

/*
 * set.h - Definitions for the cbootimage state setting code.
 */

#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

#include "cbootimage.h"
#include "parse.h"
#include "string.h"
#include "sys/stat.h"

int
set_bootloader(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point);

int
context_set_value(build_image_context	*context,
		parse_token	token,
		u_int32_t	value);

int
read_from_image(char *filename,
		u_int8_t	**Image,
		u_int32_t	*actual_size,
		file_type	f_type);

#endif /* #ifndef INCLUDED_SET_H */
