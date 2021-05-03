// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#include <file-list.h>
#include <param.h>
#include <globalvar.h>
#include <init.h>
#include <magicvar.h>
#include <system-partitions.h>

static struct file_list *system_partitions;

bool system_partitions_empty(void)
{
	return file_list_empty(system_partitions);
}

struct file_list *system_partitions_get(void)
{
	return file_list_dup(system_partitions);
}

void system_partitions_set(struct file_list *files)
{
	file_list_free(system_partitions);
	system_partitions = files;
}

static int system_partitions_var_init(void)
{
	struct param_d *param;

	system_partitions = file_list_parse("");
	param = dev_add_param_file_list(&global_device, "system.partitions",
					NULL, NULL, &system_partitions, NULL);

	return PTR_ERR_OR_ZERO(param);
}
postcore_initcall(system_partitions_var_init);

BAREBOX_MAGICVAR(global.system.partitions,
		 "board-specific list of updatable partitions");
