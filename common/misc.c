// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <magicvar.h>
#include <globalvar.h>
#include <environment.h>
#include <led.h>
#include <of.h>
#include <restart.h>
#include <poweroff.h>
#include <linux/stringify.h>

int errno;
EXPORT_SYMBOL(errno);


const char *strerror(int errnum)
{
	static char errno_string[sizeof("error -2147483648")];

#ifdef CONFIG_ERRNO_MESSAGES
	char *str;
	switch(errnum) {
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
	case	EHOSTUNREACH	: str = "No route to host"; break;
	case	EINTR		: str = "Interrupted system call"; break;
	case	ENETUNREACH	: str = "Network is unreachable"; break;
	case	ENETDOWN	: str = "Network is down"; break;
	case	ETIMEDOUT	: str = "Connection timed out"; break;
	case	EPROBE_DEFER	: str = "Requested probe deferral"; break;
	case	ELOOP		: str = "Too many symbolic links encountered"; break;
	case	ENODATA		: str = "No data available"; break;
#if 0 /* These are probably not needed */
	case	ENOTBLK		: str = "Block device required"; break;
	case	EFBIG		: str = "File too large"; break;
	case	EBADSLT		: str = "Invalid slot"; break;
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
	case	ENETRESET	: str = "Network dropped connection because of reset"; break;
	case	ECONNABORTED	: str = "Software caused connection abort"; break;
	case	ECONNRESET	: str = "Connection reset by peer"; break;
	case	ENOBUFS		: str = "No buffer space available"; break;
	case	ECONNREFUSED	: str = "Connection refused"; break;
	case	EHOSTDOWN	: str = "Host is down"; break;
	case	EALREADY	: str = "Operation already in progress"; break;
	case	EINPROGRESS	: str = "Operation now in progress"; break;
	case	ESTALE		: str = "Stale NFS file handle"; break;
	case	EISNAM		: str = "Is a named type file"; break;
	case	EREMOTEIO	: str = "Remote I/O error"; break;
#endif
	default:
		sprintf(errno_string, "error %d", errnum);
		return errno_string;
	};

        return str;
#else
	sprintf(errno_string, "error %d", errnum);

	return errno_string;
#endif
}
EXPORT_SYMBOL(strerror);

void perror(const char *s)
{
#ifdef CONFIG_ERRNO_MESSAGES
	printf("%s: %m\n", s);
#else
	printf("%s returned with %d\n", s, errno);
#endif
}
EXPORT_SYMBOL(perror);

static char *model;

/*
 * The model is the verbose name of a board. It can contain
 * whitespaces, uppercase/lowcer letters, digits, ',', '.'
 * '-', '_'
 */
void barebox_set_model(const char *__model)
{
	globalvar_add_simple_string("model", &model);

	free(model);
	model = xstrdup(__model);
}
EXPORT_SYMBOL(barebox_set_model);

const char *barebox_get_model(void)
{
	if (model)
		return model;
	else
		return "none";
}
EXPORT_SYMBOL(barebox_get_model);

BAREBOX_MAGICVAR(global.model, "Product name of this hardware");

static char *hostname;
static char *serial_number;
static char *of_machine_compatible;

/*
 * The hostname is supposed to be the shortname of a board. It should
 * contain only lowercase letters, numbers, '-', '_'. No whitespaces
 * allowed.
 */
void barebox_set_hostname(const char *__hostname)
{
	globalvar_add_simple_string("hostname", &hostname);

	free(hostname);
	hostname = xstrdup(__hostname);
}

const char *barebox_get_hostname(void)
{
	return hostname;
}
EXPORT_SYMBOL(barebox_get_hostname);

void barebox_set_hostname_no_overwrite(const char *__hostname)
{
	if (!barebox_get_hostname())
		barebox_set_hostname(__hostname);
}
EXPORT_SYMBOL(barebox_set_hostname_no_overwrite);

BAREBOX_MAGICVAR(global.hostname,
		"shortname of the board. Also used as hostname for DHCP requests");

void barebox_set_serial_number(const char *__serial_number)
{
	globalvar_add_simple_string("serial_number", &serial_number);

	free(serial_number);
	serial_number = xstrdup(__serial_number);
}

const char *barebox_get_serial_number(void)
{
	if (!serial_number || *serial_number == '\0')
		return NULL;
	return serial_number;
}

BAREBOX_MAGICVAR(global.serial_number, "Board serial number");

void barebox_set_of_machine_compatible(const char *__compatible)
{
	free(of_machine_compatible);
	of_machine_compatible = xstrdup(__compatible);
}

const char *barebox_get_of_machine_compatible(void)
{
	if (!of_machine_compatible || *of_machine_compatible == '\0')
		return NULL;
	return of_machine_compatible;
}

static int of_kernel_init(void)
{
	globalvar_add_simple_string("of.kernel.add_machine_compatible",
				    &of_machine_compatible);

	return 0;
}
device_initcall(of_kernel_init);

BAREBOX_MAGICVAR(global.of.kernel.add_machine_compatible, "Additional machine/board compatible");

static void __noreturn do_panic(bool stacktrace, const char *fmt, va_list ap)
{
	vprintf(fmt, ap);
	putchar('\n');

	if (stacktrace)
		dump_stack();

	led_trigger(LED_TRIGGER_PANIC, TRIGGER_ENABLE);

	if (IS_ENABLED(CONFIG_PANIC_HANG))
		hang();

	udelay(100000);	/* allow messages to go out */

	if (IS_ENABLED(CONFIG_PANIC_POWEROFF))
		poweroff_machine();
	else
		restart_machine();
}

void __noreturn panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	do_panic(true, fmt, args);
	va_end(args);
}
EXPORT_SYMBOL(panic);

void __noreturn panic_no_stacktrace(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	do_panic(false, fmt, args);
	va_end(args);
}
EXPORT_SYMBOL(panic_no_stacktrace);
