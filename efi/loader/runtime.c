// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/e9c34fab18a9a0022b36729afd8e262e062764e2/lib/efi_loader/efi_runtime.c
/*
 *  EFI application runtime services
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define pr_fmt(fmt) "efi-loader: runtime: " fmt

#include <efi/loader.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <linux/printk.h>
#include <barebox.h>
#include <efi/loader/table.h>
#include <efi/loader/trace.h>
#include <efi/loader/event.h>
#include <efi/mode.h>
#include <efi/table/rt_properties.h>
#include <poweroff.h>
#include <restart.h>
#include <linux/rtc.h>


/*
 * EFI runtime code lives in two stages. In the first stage, EFI loader and an
 * EFI payload are running concurrently at the same time. In this mode, we can
 * handle a good number of runtime callbacks
 */

#define CHECK_RT_FLAG(call) \
	(IS_ENABLED(CONFIG_EFI_RUNTIME_##call) ? EFI_RT_SUPPORTED_##call : 0)

/**
 * efi_init_runtime_supported() - create runtime properties table
 *
 * Create a configuration table specifying which services are available at
 * runtime.
 *
 * Return:	status code
 */
efi_status_t efi_init_runtime_supported(void)
{
	efi_status_t ret;
	struct efi_rt_properties_table *rt_table;

	ret = efi_allocate_pool(EFI_RUNTIME_SERVICES_DATA,
				sizeof(struct efi_rt_properties_table),
				(void **)&rt_table, "RtTable");
	if (ret != EFI_SUCCESS)
		return ret;

	rt_table->version = EFI_RT_PROPERTIES_TABLE_VERSION;
	rt_table->length = sizeof(struct efi_rt_properties_table);
	rt_table->runtime_services_supported =
		CHECK_RT_FLAG(GET_TIME) |
		CHECK_RT_FLAG(SET_TIME) |
		CHECK_RT_FLAG(GET_WAKEUP_TIME) |
		CHECK_RT_FLAG(SET_WAKEUP_TIME) |
		CHECK_RT_FLAG(GET_VARIABLE) |
		CHECK_RT_FLAG(GET_NEXT_VARIABLE_NAME) |
		CHECK_RT_FLAG(SET_VARIABLE) |
		CHECK_RT_FLAG(SET_VIRTUAL_ADDRESS_MAP) |
		CHECK_RT_FLAG(CONVERT_POINTER) |
		CHECK_RT_FLAG(GET_NEXT_HIGH_MONOTONIC_COUNT) |
		CHECK_RT_FLAG(RESET_SYSTEM) |
		CHECK_RT_FLAG(UPDATE_CAPSULE) |
		CHECK_RT_FLAG(QUERY_CAPSULE_CAPABILITIES) |
		CHECK_RT_FLAG(QUERY_VARIABLE_INFO);

	return efi_install_configuration_table(&efi_rt_properties_table_guid, rt_table);
}

/**
 * efi_reset_system_boottime() - reset system at boot time
 *
 * This function implements the ResetSystem() runtime service before
 * SetVirtualAddressMap() is called.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @reset_type:		type of reset to perform
 * @reset_status:	status code for the reset
 * @data_size:		size of reset_data
 * @reset_data:		information about the reset
 */
static void EFIAPI efi_reset_system_boottime(
			enum efi_reset_type reset_type,
			efi_status_t reset_status,
			size_t data_size, void *reset_data)
{
	struct efi_event *evt;

	EFI_ENTRY("%d %lx %zx %p", reset_type, reset_status, data_size,
		  reset_data);

	/* Notify reset */
	list_for_each_entry(evt, &efi_events, link) {
		if (evt->group &&
		    !efi_guidcmp(*evt->group,
			     efi_guid_event_group_reset_system)) {
			efi_signal_event(evt);
			break;
		}
	}

	switch (reset_type) {
	case EFI_RESET_WARM:
		restart_machine(RESTART_WARM);
		break;
	case EFI_RESET_COLD:
	case EFI_RESET_PLATFORM_SPECIFIC:
		restart_machine(0);
		break;
	case EFI_RESET_SHUTDOWN:
		poweroff_machine(0);
		break;
	}

	hang();
}

/**
 * efi_get_time_boottime() - get current time at boot time
 *
 * This function implements the GetTime runtime service before
 * SetVirtualAddressMap() is called.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * @time:		pointer to structure to receive current time
 * @capabilities:	pointer to structure to receive RTC properties
 * Returns:		status code
 */
static efi_status_t EFIAPI efi_get_time_boottime(
			struct efi_time *time,
			struct efi_time_cap *capabilities)
{
	efi_status_t ret;
	struct rtc_time tm;
	struct rtc_device *rtc;

	EFI_ENTRY("%p %p", time, capabilities);

	if (!IS_ENABLED(CONFIG_EFI_LOADER_GET_TIME)) {
		ret = EFI_UNSUPPORTED;
		goto out;
	}

	if (!time) {
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}

	rtc = rtc_lookup("rtc0");
	if (IS_ERR(rtc)) {
		ret = EFI_UNSUPPORTED;
		goto out;
	}

	if (rtc_read_time(rtc, &tm)) {
		ret = EFI_DEVICE_ERROR;
		goto out;
	}

	memset(time, 0, sizeof(*time));
	time->year = tm.tm_year;
	time->month = tm.tm_mon;
	time->day = tm.tm_mday;
	time->hour = tm.tm_hour;
	time->minute = tm.tm_min;
	time->second = tm.tm_sec;

	if (tm.tm_isdst > 0)
		time->daylight =
			EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT;
	else if (!tm.tm_isdst)
		time->daylight = EFI_TIME_ADJUST_DAYLIGHT;
	else
		time->daylight = 0;
	time->timezone = EFI_UNSPECIFIED_TIMEZONE;

	if (capabilities) {
		/* Set reasonable dummy values */
		capabilities->resolution = 1;		/* 1 Hz */
		capabilities->accuracy = 100000000;	/* 100 ppm */
		capabilities->sets_to_zero = false;
	}

	ret = EFI_SUCCESS;
out:
	return EFI_EXIT(ret);
}


/**
 * efi_validate_time() - checks if timestamp is valid
 *
 * @time:	timestamp to validate
 * Returns:	0 if timestamp is valid, 1 otherwise
 */
static int efi_validate_time(struct efi_time *time)
{
	return (!time ||
		time->year < 1900 || time->year > 9999 ||
		!time->month || time->month > 12 || !time->day ||
		time->day > rtc_month_days(time->month - 1, time->year) ||
		time->hour > 23 || time->minute > 59 || time->second > 59 ||
		time->nanosecond > 999999999 ||
		time->daylight &
		~(EFI_TIME_IN_DAYLIGHT | EFI_TIME_ADJUST_DAYLIGHT) ||
		((time->timezone < -1440 || time->timezone > 1440) &&
		time->timezone != EFI_UNSPECIFIED_TIMEZONE));
}

/**
 * efi_set_time_boottime() - set current time
 *
 * This function implements the SetTime() runtime service before
 * SetVirtualAddressMap() is called.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification
 * for details.
 *
 * @time:		pointer to structure to with current time
 * Returns:		status code
 */
static efi_status_t EFIAPI efi_set_time_boottime(struct efi_time *time)
{
	efi_status_t ret = EFI_SUCCESS;
	struct rtc_time tm;
	struct rtc_device *rtc;

	if (!IS_ENABLED(CONFIG_EFI_LOADER_SET_TIME))
		return EFI_EXIT(EFI_UNSUPPORTED);

	EFI_ENTRY("%p", time);

	if (efi_validate_time(time)) {
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}

	rtc = rtc_lookup("rtc0");
	if (IS_ERR(rtc)) {
		ret = EFI_UNSUPPORTED;
		goto out;
	}

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = time->year;
	tm.tm_mon = time->month;
	tm.tm_mday = time->day;
	tm.tm_hour = time->hour;
	tm.tm_min = time->minute;
	tm.tm_sec = time->second;

	switch (time->daylight) {
	case EFI_TIME_ADJUST_DAYLIGHT:
		tm.tm_isdst = 0;
		break;
	case EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT:
		tm.tm_isdst = 1;
		break;
	default:
		tm.tm_isdst = -1;
		break;
	}

	/* Calculate day of week */
	rtc_calc_weekday(&tm);

	if (rtc_set_time(rtc, &tm))
		ret = EFI_DEVICE_ERROR;
out:
	return EFI_EXIT(ret);
}

/*
 * The implementation of these services depends on barebox still running,
 * i.e. BS->ExitBootServices() was not yet called. When it's called,
 * the runtime services will be replaced by the detached implementation,
 * so we need not take any special consideration here.
 */
struct efi_runtime_services efi_runtime_services = {
	.hdr = {
		.signature = EFI_RUNTIME_SERVICES_SIGNATURE,
		.revision = EFI_SPECIFICATION_VERSION,
		.headersize = sizeof(struct efi_runtime_services),
	},
	.get_time = &efi_get_time_boottime,
	.set_time = &efi_set_time_boottime,
	.get_wakeup_time = (void *)efi_unimplemented,
	.set_wakeup_time = (void *)efi_unimplemented,
	.set_virtual_address_map = (void *)efi_unimplemented,
	.convert_pointer = (void *)efi_unimplemented,
	.get_variable = (void *)efi_unimplemented,
	.get_next_variable = (void *)efi_unimplemented,
	.set_variable = (void *)efi_unimplemented,
	.get_next_high_mono_count = (void *)efi_unimplemented,
	.reset_system = &efi_reset_system_boottime,
	.update_capsule = (void *)efi_unimplemented,
	.query_capsule_caps = (void *)efi_unimplemented,
	.query_variable_info = (void *)efi_unimplemented,
};
