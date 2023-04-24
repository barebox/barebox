/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SANDBOX_RESET_SOURCE_H
#define __SANDBOX_RESET_SOURCE_H

#include <reset_source.h>
#include <linux/nvmem-consumer.h>

static inline void sandbox_save_reset_source(struct nvmem_cell *reset_source_cell,
					     enum reset_src_type src)
{
	if (reset_source_cell)
		WARN_ON(nvmem_cell_write(reset_source_cell, &(u8) { src }, 1) <= 0);
}

#endif
