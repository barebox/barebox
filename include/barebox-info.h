/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __BAREBOX_INFO_H__
#define __BAREBOX_INFO_H__

#include <linux/types.h>
#include <linux/uuid.h>

extern const char version_string[];
extern const char release_string[];
extern const char buildsystem_version_string[];

#ifdef CONFIG_BANNER
void barebox_banner(void);
#else
static inline void barebox_banner(void) {}
#endif

const char *barebox_get_model(void);
void barebox_set_model(const char *);
const char *barebox_get_hostname(void);
void barebox_set_hostname(const char *);
void barebox_set_hostname_no_overwrite(const char *);
bool barebox_hostname_is_valid(const char *s);

const char *barebox_get_serial_number(void);
void barebox_set_serial_number(const char *);

void barebox_set_product_uuid(const uuid_t *uuid);
const uuid_t *barebox_get_product_uuid(void);

#ifdef CONFIG_OFTREE
void barebox_set_of_machine_compatible(const char *);
const char *barebox_get_of_machine_compatible(void);
#else
static inline void barebox_set_of_machine_compatible(const char *compat) {}
static inline const char *barebox_get_of_machine_compatible(void) { return NULL; }
#endif

#endif
