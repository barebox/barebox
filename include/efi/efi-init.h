/* SPDX-License-Identifier: GPL-2.0 */

#ifndef EFI_INIT_H_
#define EFI_INIT_H_

#include <init.h>
#include <linux/compiler.h>

struct efi_boot_services;
extern struct efi_boot_services *BS;

#ifdef CONFIG_EFI_PAYLOAD
#define efi_payload_initcall(level, fn) level##_initcall(fn)
#else
#define efi_payload_initcall(level, fn)
#endif

/* For use by EFI payload */
#define __define_efi_initcall(level, fn) \
	static int __maybe_unused __efi_initcall_##fn(void) \
	{ \
		return BS ? fn() : 0; \
	} \
	efi_payload_initcall(level, __efi_initcall_##fn);

#define core_efi_initcall(fn)		__define_efi_initcall(core, fn)
#define postcore_efi_initcall(fn)	__define_efi_initcall(postcore, fn)
#define console_efi_initcall(fn)	__define_efi_initcall(console, fn)
#define coredevice_efi_initcall(fn)	__define_efi_initcall(coredevice, fn)
#define mem_efi_initcall(fn)		__define_efi_initcall(mem, fn)
#define device_efi_initcall(fn)		__define_efi_initcall(device, fn)
#define fs_efi_initcall(fn)		__define_efi_initcall(fs, fn)
#define late_efi_initcall(fn)		__define_efi_initcall(late, fn)

#define register_efi_driver_macro(level,bus,drv)	\
	static int __init drv##_register(void)		\
	{						\
		return bus##_driver_register(&drv);	\
	}						\
	level##_efi_initcall(drv##_register)

#define core_efi_driver(drv)	\
	register_efi_driver_macro(core, efi, drv)

#define device_efi_driver(drv)	\
	register_efi_driver_macro(device, efi, drv)

#define fs_efi_driver(drv)	\
	register_efi_driver_macro(fs, efi, drv)

#endif
