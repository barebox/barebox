/*
 * Persistent Storage - pstore.h
 *
 * Copyright (C) 2010 Intel Corporation <tony.luck@intel.com>
 *
 * This code is the generic layer to export data records from platform
 * level persistent storage via a file system.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _LINUX_PSTORE_H
#define _LINUX_PSTORE_H

#include <linux/time.h>
#include <linux/types.h>
#include <linux/errno.h>

struct module;

/* types */
enum pstore_type_id {
	PSTORE_TYPE_DMESG	= 0,
	PSTORE_TYPE_MCE		= 1,
	PSTORE_TYPE_CONSOLE	= 2,
	PSTORE_TYPE_FTRACE	= 3,
	/* PPC64 partition types */
	PSTORE_TYPE_PPC_RTAS	= 4,
	PSTORE_TYPE_PPC_OF	= 5,
	PSTORE_TYPE_PPC_COMMON	= 6,
	PSTORE_TYPE_PMSG	= 7,
	PSTORE_TYPE_UNKNOWN	= 255
};

enum kmsg_dump_reason {
	KMSG_DUMP_UNDEF,
};

struct pstore_info;

struct pstore_record {
	struct pstore_info	*psi;
	enum pstore_type_id	type;
	u64			id;
	char			*buf;
	ssize_t			size;
	ssize_t			ecc_notice_size;

	int			count;
	enum kmsg_dump_reason	reason;
	unsigned int		part;
	bool			compressed;
};

struct pstore_info {
	struct module	*owner;
	char		*name;
	int		flags;
	int		(*open)(struct pstore_info *psi);
	int		(*close)(struct pstore_info *psi);
	ssize_t		(*read)(struct pstore_record *record);
	int		(*write)(struct pstore_record *record);
	int		(*write_buf)(enum pstore_type_id type,
			enum kmsg_dump_reason reason, u64 *id,
			unsigned int part, const char *buf, bool compressed,
			size_t size, struct pstore_info *psi);
	int		(*erase)(enum pstore_type_id type, u64 id,
			int count, struct pstore_info *psi);
	void		*data;
};

#define	PSTORE_FLAGS_FRAGILE	1

#ifdef CONFIG_FS_PSTORE
extern int pstore_register(struct pstore_info *);
extern bool pstore_cannot_block_path(enum kmsg_dump_reason reason);
extern void pstore_log(const char *msg);
#else
static inline int
pstore_register(struct pstore_info *psi)
{
	return -ENODEV;
}
static inline bool
pstore_cannot_block_path(enum kmsg_dump_reason reason)
{
	return false;
}
static inline void pstore_log(const char *msg)
{
}
#endif

#endif /*_LINUX_PSTORE_H*/
