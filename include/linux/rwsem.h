/* rwsem.h: R/W semaphores, public interface
 *
 * Written by David Howells (dhowells@redhat.com).
 * Derived from asm-i386/semaphore.h
 */

#ifndef _LINUX_RWSEM_H
#define _LINUX_RWSEM_H

struct rw_semaphore { int i; };

#define init_rwsem(...)			do { } while (0)
#define down_read(...)			do { } while (0)
#define down_write(...)			do { } while (0)
#define down_write_trylock(...)		1
#define up_read(...)			do { } while (0)
#define up_write(...)			do { } while (0)

#endif /* _LINUX_RWSEM_H */
