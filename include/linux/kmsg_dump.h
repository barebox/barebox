/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_KMSG_DUMP_H
#define _LINUX_KMSG_DUMP_H

/* SPDX-SnippetBegin */
/* SPDX-Snippet-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/kmsg_dump.h?id=e1a261ba599eec97e1c5c7760d5c3698fc24e6a6 */

/*
 * Keep this list arranged in rough order of priority. Anything listed after
 * KMSG_DUMP_OOPS will not be logged by default unless printk.always_kmsg_dump
 * is passed to the kernel.
 */
enum kmsg_dump_reason {
	KMSG_DUMP_UNDEF,
	KMSG_DUMP_PANIC,
	KMSG_DUMP_OOPS,
	KMSG_DUMP_EMERG,
	KMSG_DUMP_SHUTDOWN,
	KMSG_DUMP_MAX
};

/* SPDX-SnippetEnd */

#endif /* _LINUX_KMSG_DUMP_H */
