/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_UIDGID_H
#define _LINUX_UIDGID_H

/*
 * A set of types for the internal kernel types representing uids and gids.
 *
 * The types defined in this header allow distinguishing which uids and gids in
 * the kernel are values used by userspace and which uid and gid values are
 * the internal kernel values.  With the addition of user namespaces the values
 * can be different.  Using the type system makes it possible for the compiler
 * to detect when we overlook these differences.
 *
 */
#include <linux/uidgid_types.h>

#define KUIDT_INIT(value) (kuid_t){ value }
#define KGIDT_INIT(value) (kgid_t){ value }

#define make_kuid(uid) KUIDT_INIT(uid)
#define make_kgid(gid) KGIDT_INIT(gid)

#define GLOBAL_ROOT_UID KUIDT_INIT(0)
#define GLOBAL_ROOT_GID KGIDT_INIT(0)

#define INVALID_UID KUIDT_INIT(-1)
#define INVALID_GID KGIDT_INIT(-1)

static inline uid_t __kuid_val(kuid_t uid)
{
	return 0;
}

static inline gid_t __kgid_val(kgid_t gid)
{
	return 0;
}

static inline uid_t from_kuid(kuid_t kuid)
{
	return __kuid_val(kuid);
}

static inline gid_t from_kgid(kgid_t kgid)
{
	return __kgid_val(kgid);
}

static inline bool uid_eq(kuid_t left, kuid_t right)
{
	return __kuid_val(left) == __kuid_val(right);
}

static inline bool gid_eq(kgid_t left, kgid_t right)
{
	return __kgid_val(left) == __kgid_val(right);
}

#endif /* _LINUX_UIDGID_H */
