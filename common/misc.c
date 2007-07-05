#include <common.h>
#include <mem_malloc.h>
#include <errno.h>

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start = 0;
static ulong mem_malloc_end = 0;
static ulong mem_malloc_brk = 0;

void mem_malloc_init (void *start, void *end)
{
	mem_malloc_start = (ulong)start;
	mem_malloc_end = (ulong)end;
	mem_malloc_brk = mem_malloc_start;
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

        if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
 		return (NULL);
	}
	mem_malloc_brk = new;

	return ((void *) old);
}

int errno;

#ifndef CONFIG_ERRNO_MESSAGES
static char errno_str[5];
#endif

const char *errno_str(void)
{
#ifdef CONFIG_ERRNO_MESSAGES
	char *str;
	switch(-errno) {
	case	0		: str = "No error"; break;
	case	EPERM		: str = "Operation not permitted"; break;
	case	ENOENT		: str = "No such file or directory"; break;
	case	EIO		: str = "I/O error"; break;
	case	ENXIO		: str = "No such device or address"; break;
	case	E2BIG		: str = "Arg list too long"; break;
	case	ENOEXEC		: str = "Exec format error"; break;
	case	EBADF		: str = "Bad file number"; break;
	case	ENOMEM		: str = "Out of memory"; break;
	case	EACCES		: str = "Permission denied"; break;
	case	EFAULT		: str = "Bad address"; break;
	case	EBUSY		: str = "Device or resource busy"; break;
	case	EEXIST		: str = "File exists"; break;
	case	ENODEV		: str = "No such device"; break;
	case	ENOTDIR		: str = "Not a directory"; break;
	case	EISDIR		: str = "Is a directory"; break;
	case	EINVAL		: str = "Invalid argument"; break;
	case	ENOSPC		: str = "No space left on device"; break;
	case	ESPIPE		: str = "Illegal seek"; break;
	case	EROFS		: str = "Read-only file system"; break;
	case	ENAMETOOLONG	: str = "File name too long"; break;
	case	ENOSYS		: str = "Function not implemented"; break;
	case	ENOTEMPTY	: str = "Directory not empty"; break;
#if 0 /* These are probably not needed */
	case	ENOTBLK		: str = "Block device required"; break;
	case	EFBIG		: str = "File too large"; break;
	case	EBADSLT		: str = "Invalid slot"; break;
	case	ENODATA		: str = "No data available"; break;
	case	ETIME		: str = "Timer expired"; break;
	case	ENONET		: str = "Machine is not on the network"; break;
	case	EADV		: str = "Advertise error"; break;
	case	ECOMM		: str = "Communication error on send"; break;
	case	EPROTO		: str = "Protocol error"; break;
	case	EBADMSG		: str = "Not a data message"; break;
	case	EOVERFLOW	: str = "Value too large for defined data type"; break;
	case	EBADFD		: str = "File descriptor in bad state"; break;
	case	EREMCHG		: str = "Remote address changed"; break;
	case	EMSGSIZE	: str = "Message too long"; break;
	case	EPROTOTYPE	: str = "Protocol wrong type for socket"; break;
	case	ENOPROTOOPT	: str = "Protocol not available"; break;
	case	EPROTONOSUPPORT	: str = "Protocol not supported"; break;
	case	ESOCKTNOSUPPORT	: str = "Socket type not supported"; break;
	case	EPFNOSUPPORT	: str = "Protocol family not supported"; break;
	case	EAFNOSUPPORT	: str = "Address family not supported by protocol"; break;
	case	EADDRINUSE	: str = "Address already in use"; break;
	case	EADDRNOTAVAIL	: str = "Cannot assign requested address"; break;
	case	ENETDOWN	: str = "Network is down"; break;
	case	ENETUNREACH	: str = "Network is unreachable"; break;
	case	ENETRESET	: str = "Network dropped connection because of reset"; break;
	case	ECONNABORTED	: str = "Software caused connection abort"; break;
	case	ECONNRESET	: str = "Connection reset by peer"; break;
	case	ENOBUFS		: str = "No buffer space available"; break;
	case	ETIMEDOUT	: str = "Connection timed out"; break;
	case	ECONNREFUSED	: str = "Connection refused"; break;
	case	EHOSTDOWN	: str = "Host is down"; break;
	case	EHOSTUNREACH	: str = "No route to host"; break;
	case	EALREADY	: str = "Operation already in progress"; break;
	case	EINPROGRESS	: str = "Operation now in progress"; break;
	case	ESTALE		: str = "Stale NFS file handle"; break;
	case	EISNAM		: str = "Is a named type file"; break;
	case	EREMOTEIO	: str = "Remote I/O error"; break;
#endif
	default			: str = "unknown error"; break;
	};

        return str;
#else
	sprintf(errno_str, "%d", errno);
	return errno_str;
#endif
}

void perror(const char *s)
{
#ifdef CONFIG_ERRNO_MESSAGES
	printf("%s: %s\n", s, errno_str());
#else
	printf("%s returned with %d\n", s, errno);
#endif
}
