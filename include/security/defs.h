/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BAREBOX_SECURITY_DEFS_H
#define __BAREBOX_SECURITY_DEFS_H

#include <linux/types.h>

#ifdef CONFIG_SECURITY_POLICY
# include <generated/security_autoconf.h>
#else
#define SCONFIG_NUM 0
enum security_config_option { SCONFIG__DUMMY__ };
#endif

extern const char *sconfig_names[SCONFIG_NUM];

struct security_policy {
       const char *name;
       bool chained;
       unsigned char policy[SCONFIG_NUM];
};

#endif
