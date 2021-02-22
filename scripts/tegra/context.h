/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION */

#ifndef INCLUDED_CONTEXT_H
#define INCLUDED_CONTEXT_H

#include "cbootimage.h"

int init_context(build_image_context *context);
void cleanup_context(build_image_context *context);

#endif /* #ifndef INCLUDED_CONTEXT_H */
