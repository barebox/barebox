/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __FASTBOOT_NET__
#define __FASTBOOT_NET__

#include <fastboot.h>

struct fastboot_net;

struct fastboot_net *fastboot_net_init(struct fastboot_opts *opts);
void fastboot_net_free(struct fastboot_net *fbn);

#endif
