/* SPDX-License-Identifier: GPL-2.0 */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/fs/pstore/internal.h?id=24b8f8dcb9a139a36cf48bfbe935e8dc1f33ed79 */
#ifndef __PSTORE_INTERNAL_H__
#define __PSTORE_INTERNAL_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/pstore.h>

struct dentry;

#ifdef CONFIG_PSTORE_FTRACE
extern unsigned long decode_ip(unsigned long ip);
extern void pstore_register_ftrace(void);
extern void pstore_unregister_ftrace(void);
ssize_t pstore_ftrace_combine_log(char **dest_log, size_t *dest_log_size,
				  const char *src_log, size_t src_log_size);
#else
static inline void pstore_register_ftrace(void) {}
static inline void pstore_unregister_ftrace(void) {}
static inline unsigned long decode_ip(unsigned long ip) { return ip; }
static inline ssize_t
pstore_ftrace_combine_log(char **dest_log, size_t *dest_log_size,
			  const char *src_log, size_t src_log_size)
{
	*dest_log_size = 0;
	return 0;
}
#endif

#ifdef CONFIG_PSTORE_PMSG
extern void pstore_register_pmsg(void);
extern void pstore_unregister_pmsg(void);
#else
static inline void pstore_register_pmsg(void) {}
static inline void pstore_unregister_pmsg(void) {}
#endif

extern struct pstore_info *psinfo;

extern void	pstore_get_records(int);
extern void	pstore_get_backend_records(struct pstore_info *psi,
					   struct dentry *root, int quiet);
extern int	pstore_mkfile(struct dentry *root,
			      struct pstore_record *record);
extern void	pstore_record_init(struct pstore_record *record,
				   struct pstore_info *psi);

#endif
