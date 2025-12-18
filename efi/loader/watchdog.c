// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e9c34fab18a9a0022b36729afd8e262e062764e2/lib/efi_loader/efi_watchdog.c
/*
 *  EFI watchdog
 *
 *  Copyright (c) 2017 Heinrich Schuchardt
 */

#define pr_fmt(fmt) "efi-loader: watchdog: " fmt

#include <linux/printk.h>
#include <efi/loader.h>
#include <efi/loader/event.h>
#include <efi/loader/trace.h>
#include <efi/services.h>
#include <efi/error.h>
#include <init.h>

/* Conversion factor from seconds to multiples of 100ns */
#define EFI_SECONDS_TO_100NS 10000000ULL

static struct efi_event *watchdog_timer_event;

/**
 * efi_watchdog_timer_notify() - resets system upon watchdog event
 *
 * Reset the system when the watchdog event is notified.
 *
 * @event:	the watchdog event
 * @context:	not used
 */
static void EFIAPI efi_watchdog_timer_notify(struct efi_event *event,
					     void *context)
{
	EFI_ENTRY("%p, %p", event, context);

	pr_emerg("\nEFI: Watchdog timeout\n");
	EFI_CALL_VOID(efi_runtime_services.reset_system(EFI_RESET_COLD,
							EFI_SUCCESS, 0, NULL));

	EFI_EXIT(EFI_UNSUPPORTED);
}

/**
 * efi_set_watchdog() - resets the watchdog timer
 *
 * This function is used by the SetWatchdogTimer service.
 *
 * @timeout:		seconds before reset by watchdog
 * Return:		status code
 */
efi_status_t efi_set_watchdog(unsigned long timeout)
{
	efi_status_t r;

	if (timeout)
		/* Reset watchdog */
		r = efi_set_timer(watchdog_timer_event, EFI_TIMER_RELATIVE,
				  EFI_SECONDS_TO_100NS * timeout);
	else
		/* Deactivate watchdog */
		r = efi_set_timer(watchdog_timer_event, EFI_TIMER_CANCEL, 0);
	return r;
}

/**
 * efi_watchdog_register() - initializes the EFI watchdog
 *
 * Return:	status code
 */
static efi_status_t efi_watchdog_register(void *data)
{
	efi_status_t r;

	/*
	 * Create a timer event.
	 */
	r = efi_create_event(EFI_EVT_TIMER | EFI_EVT_NOTIFY_SIGNAL, EFI_TPL_CALLBACK,
			     efi_watchdog_timer_notify, NULL, NULL,
			     &watchdog_timer_event);
	if (r) {
		pr_err("Failed to register watchdog event\n");
		return r;
	}

	return EFI_SUCCESS;
}

static int efi_watchdog_init(void)
{
	efi_register_deferred_init(efi_watchdog_register, NULL);
	return 0;
}
late_initcall(efi_watchdog_init);
