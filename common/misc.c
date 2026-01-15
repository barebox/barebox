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
#include <linux/ctype.h>
#include <of.h>
#include <restart.h>
#include <poweroff.h>
#include <string.h>
#include <linux/stringify.h>

int errno;
EXPORT_SYMBOL(errno);


const char *strerror(int errnum)
{
	static char errno_string[sizeof("error -2147483648")];

	switch(errnum) {
#ifdef CONFIG_ERRNO_MESSAGES
	case	0		: return "No error";
	case	EPERM		: return "Operation not permitted";
	case	ENOENT		: return "No such file or directory";
	case	EIO		: return "I/O error";
	case	ENXIO		: return "No such device or address";
	case	E2BIG		: return "Arg list too long";
	case	ENOEXEC		: return "Exec format error";
	case	EBADF		: return "Bad file number";
	case	ENOMEM		: return "Out of memory";
	case	EACCES		: return "Permission denied";
	case	EFAULT		: return "Bad address";
	case	EBUSY		: return "Device or resource busy";
	case	EEXIST		: return "File exists";
	case	ENODEV		: return "No such device";
	case	ENOTDIR		: return "Not a directory";
	case	EISDIR		: return "Is a directory";
	case	EINVAL		: return "Invalid argument";
	case	ENOSPC		: return "No space left on device";
	case	ESPIPE		: return "Illegal seek";
	case	EROFS		: return "Read-only file system";
	case	ENAMETOOLONG	: return "File name too long";
	case	ENOSYS		: return "Function not implemented";
	case	ENOTEMPTY	: return "Directory not empty";
	case	EHOSTUNREACH	: return "No route to host";
	case	EINTR		: return "Interrupted system call";
	case	ENETUNREACH	: return "Network is unreachable";
	case	ENETDOWN	: return "Network is down";
	case	ETIMEDOUT	: return "Connection timed out";
	case	EPROBE_DEFER	: return "Requested probe deferral";
	case	ELOOP		: return "Too many symbolic links encountered";
	case	ENODATA		: return "No data available";
	case	EOPNOTSUPP	: return "Operation not supported";
	case	ENOIOCTLCMD	: return "Command not found";
#if 0 /* These are probably not needed */
	case	ENOTBLK		: return "Block device required";
	case	EFBIG		: return "File too large";
	case	EBADSLT		: return "Invalid slot";
	case	ETIME		: return "Timer expired";
	case	ENONET		: return "Machine is not on the network";
	case	EADV		: return "Advertise error";
	case	ECOMM		: return "Communication error on send";
	case	EPROTO		: return "Protocol error";
	case	EBADMSG		: return "Not a data message";
	case	EOVERFLOW	: return "Value too large for defined data type";
	case	EBADFD		: return "File descriptor in bad state";
	case	EREMCHG		: return "Remote address changed";
	case	EMSGSIZE	: return "Message too long";
	case	EPROTOTYPE	: return "Protocol wrong type for socket";
	case	ENOPROTOOPT	: return "Protocol not available";
	case	EPROTONOSUPPORT	: return "Protocol not supported";
	case	ESOCKTNOSUPPORT	: return "Socket type not supported";
	case	EPFNOSUPPORT	: return "Protocol family not supported";
	case	EAFNOSUPPORT	: return "Address family not supported by protocol";
	case	EADDRINUSE	: return "Address already in use";
	case	EADDRNOTAVAIL	: return "Cannot assign requested address";
	case	ENETRESET	: return "Network dropped connection because of reset";
	case	ECONNABORTED	: return "Software caused connection abort";
	case	ECONNRESET	: return "Connection reset by peer";
	case	ENOBUFS		: return "No buffer space available";
	case	ECONNREFUSED	: return "Connection refused";
	case	EHOSTDOWN	: return "Host is down";
	case	EALREADY	: return "Operation already in progress";
	case	EINPROGRESS	: return "Operation now in progress";
	case	ESTALE		: return "Stale NFS file handle";
	case	EISNAM		: return "Is a named type file";
	case	EREMOTEIO	: return "Remote I/O error";
#endif
#endif
	default:
		sprintf(errno_string, "error %d", errnum);
		return errno_string;
	};
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
static uuid_t product_uuid;

/* Note that HOST_NAME_MAX is 64 on Linux */
#define BAREBOX_HOST_NAME_MAX	64

static bool barebox_valid_ldh_char(char c)
{
	/* "LDH" -> "Letters, digits, hyphens", as per RFC 5890, Section 2.3.1 */
	return isalnum(c) || c == '-';
}

bool barebox_hostname_is_valid(const char *s)
{
	unsigned int n_dots = 0;
	const char *p;
	bool dot, hyphen;

	/*
	 * Check if s looks like a valid hostname or FQDN. This does not do full
	 * DNS validation, but only checks if the name is composed of allowed
	 * characters and the length is not above the maximum allowed by Linux.
	 * Doesn't accept empty hostnames, hostnames with leading dots, and
	 * hostnames with multiple dots in a sequence. Doesn't allow hyphens at
	 * the beginning or end of label.
	 */
	if (isempty(s))
		return false;

	for (p = s, dot = hyphen = true; *p; p++) {
		if (*p == '.') {
			if (dot || hyphen)
				return false;

			dot = true;
			hyphen = false;
			n_dots++;

		} else if (*p == '-') {
			if (dot)
				return false;

			dot = false;
			hyphen = true;

		} else {
			if (!barebox_valid_ldh_char(*p))
				return false;

			dot = false;
			hyphen = false;
		}
	}

	if (dot || hyphen)
		return false;

	if (p - s > BAREBOX_HOST_NAME_MAX)
		return false;

	return true;
}

/*
 * The hostname is supposed to be the shortname of a board. It should
 * contain only letters, numbers, hyphens. No whitespaces allowed.
 */
void barebox_set_hostname(const char *__hostname)
{
	globalvar_add_simple_string("hostname", &hostname);

	free(hostname);

	if (!barebox_hostname_is_valid(__hostname))
		pr_warn("Hostname is not valid, please fix it\n");

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

void barebox_set_product_uuid(const uuid_t *__product_uuid)
{
	globalvar_add_simple_uuid("product.uuid", &product_uuid);
	product_uuid = *__product_uuid;
}

const uuid_t *barebox_get_product_uuid(void)
{
	return &product_uuid;
}

BAREBOX_MAGICVAR(global.product.uuid, "SMBIOS-reported product UUID");

#ifdef CONFIG_OFTREE
static char *of_machine_compatible;

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

BAREBOX_MAGICVAR(global.of.kernel.add_machine_compatible, "Extra machine/board compatible to prepend to kernel DT compatible");
#endif

static void __noreturn do_panic(bool stacktrace, const char *fmt, va_list ap)
{
	if (*fmt)
		eprintf("PANIC: ");
	veprintf(fmt, ap);
	eputchar('\n');

	if (stacktrace)
		dump_stack();

	led_trigger(LED_TRIGGER_PANIC, TRIGGER_ENABLE);

	if (IS_ENABLED(CONFIG_PANIC_HANG))
		hang();

	mdelay_non_interruptible(100);	/* allow messages to go out */

	if (IS_ENABLED(CONFIG_PANIC_POWEROFF))
		poweroff_machine(0);
	else if (IS_ENABLED(CONFIG_PANIC_TRAP))
		__builtin_trap();
	else
		restart_machine(0);
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
