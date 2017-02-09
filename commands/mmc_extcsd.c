/*
 *
 * (C) Copyright 2015 Phytec Messtechnik GmbH
 * Author: Daniel Schultz <d.schultz@phytec.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <driver.h>
#include <stdio.h>
#include <mci.h>
#include <getopt.h>

#define EXT_CSD_BLOCKSIZE	512

/* Access types */
#define R		"R"
#define RW		"R/W"
#define RWaR		"R/W & R"
#define RWaRWE		"R/W & R/W/E"
#define RWaRWC_P	"R/W & R/W/C_P"
#define RWaRWC_PaRWE_P	"R/W, R/W/C_P & R/W/E_P"
#define WE		"W/E"
#define RWE		"R/W/E"
#define RWEaR		"R/W/E & R"
#define RWEaRWE_P	"R/W/E & R/W/E_P"
#define RWC_P		"R/W/C_P"
#define RWE_P		"R/W/E_P"
#define WE_P		"W/E_P"

#define print_field_caption(reg_name, access_mode)			       \
	do {								       \
		printf(#reg_name"[%u]:\n", EXT_CSD_##reg_name);		       \
		printf("\tValue: %#02x\n", reg[index]);			       \
		printf("\tAccess: "access_mode"\n");			       \
	} while (false);

#define print_field_caption_with_offset(reg_name, offset, access_mode)	       \
	do {								       \
		printf(#reg_name"[%u]:\n", EXT_CSD_##reg_name + offset);       \
		printf("\tValue: %#02x\n", reg[index]);			       \
		printf("\tAccess: "access_mode"\n");			       \
	} while (false);

#define get_field_val(reg_name, offset, mask)				       \
	((reg[EXT_CSD_##reg_name] >> offset) & mask)

#define get_field_val_with_index(index, offset, mask)			       \
	((reg[index] >> offset) & mask)

static void print_access_type_key(void)
{
	printf("\nR:\tRead only\n"
		"W:\tOne time programmable\n"
		"E:\tMultiple programmable\n"
		"C_P:\tValue cleared by power failure\n"
		"E_P:\tValue cleared by power failure and CMD0\n");
}

static int print_field_ge_v7(u8 *reg, int index)
{
	int rev;
	u32 val;
	u32 tmp;
	u64 tmp64;
	char *str = NULL;

	rev = reg[EXT_CSD_REV];

	switch (index) {
	case EXT_CSD_CMDQ_MODE_EN:
		print_field_caption(CMDQ_MODE_EN, RWE_P);
		val = get_field_val(CMDQ_MODE_EN, 0, 0x1);
		printf("\tCommand queuing is %sabled\n", val ? "en" : "dis");
		return 1;

	case EXT_CSD_SECURE_REMOVAL_TYPE:
		print_field_caption(SECURE_REMOVAL_TYPE, RWaR);
		val = get_field_val(SECURE_REMOVAL_TYPE, 0, 0xF);
		switch (val) {
		case 0x0:
			str = "erase";
			break;
		case 0x1:
			str = "overwrite, then erase";
			break;
		case 0x2:
			str = "overwrite, complement, then random";
			break;
		case 0x3:
			str = "vendor defined";
			break;
		}
		printf("\t[3-0] Supported Secure Removal Type: %s\n", str);
		val = get_field_val(SECURE_REMOVAL_TYPE, 4, 0xF);
		switch (val) {
		case 0x0:
			str = "erase";
			break;
		case 0x1:
			str = "overwrite, then erase";
			break;
		case 0x2:
			str = "overwrite, complement, then random";
			break;
		case 0x3:
			str = "vendor defined";
			break;
		}
		printf("\t[7-4] Configure Secure Removal Type: %s\n", str);
		return 1;

	case EXT_CSD_PRODUCT_ST8_AWARENSS_ENABLEMENT:
		print_field_caption(PRODUCT_ST8_AWARENSS_ENABLEMENT, RWEaR);
		val = get_field_val(PRODUCT_ST8_AWARENSS_ENABLEMENT, 0, 0x1);
		printf("\t[0] Manual mode is %ssupported\n",
			val ? "" : "not ");
		val = get_field_val(PRODUCT_ST8_AWARENSS_ENABLEMENT, 1, 0x1);
		printf("\t[1] Auto mode is %ssupported\n", (val ? "" : "not "));
		val = get_field_val(PRODUCT_ST8_AWARENSS_ENABLEMENT, 4, 0x1);
		printf("\t[4] Production State Awareness is %sabled\n",
			val ? "en" : "dis");
		val = get_field_val(PRODUCT_ST8_AWARENSS_ENABLEMENT, 5, 0x1);
		printf("\t[5] Auto mode is %sabled\n", (val ? "en" : "dis"));
		return 1;

	/*   EXT_CSD_MAX_PRE_LOADING_DATA_SIZE */
	case 25:
	case 24:
	case 23:
	case 22:
		print_field_caption_with_offset(MAX_PRE_LOADING_DATA_SIZE,
			index - EXT_CSD_MAX_PRE_LOADING_DATA_SIZE, R);
		tmp64 = get_field_val(MAX_PRE_LOADING_DATA_SIZE, 0, 0xFF);
		tmp64 = tmp64 | get_field_val(
			MAX_PRE_LOADING_DATA_SIZE + 1, 0, 0xFF) << 8;
		tmp64 = tmp64 | get_field_val(
				MAX_PRE_LOADING_DATA_SIZE + 2, 0, 0xFF) << 16;
		tmp64 = tmp64 | get_field_val(
				MAX_PRE_LOADING_DATA_SIZE + 3, 0, 0xFF) << 24;
		tmp = get_field_val(DATA_SECTOR_SIZE, 0, 0x1);
		if (tmp64 == 0xFFFFFFFF)
			if (tmp)
				str = strdup("16 TB");
			else
				str = strdup("2 TB");
		else
			if (tmp)
				str = basprintf("%llu B", tmp64 * 4096);
			else
				str = basprintf("%llu B", tmp64 * 512);
		printf("\tMax_Pre_Loading_Data_Size: %s\n", str);
		free(str);
		return 1;

	/*   EXT_CSD_PRE_LOADING_DATA_SIZE */
	case 21:
	case 20:
	case 19:
	case 18:
		print_field_caption_with_offset(PRE_LOADING_DATA_SIZE,
			index - EXT_CSD_PRE_LOADING_DATA_SIZE, RWE_P);
		tmp64 = get_field_val(PRE_LOADING_DATA_SIZE, 0, 0xFF);
		tmp64 = tmp64 | get_field_val(
			PRE_LOADING_DATA_SIZE + 1, 0, 0xFF) << 8;
		tmp64 = tmp64 | get_field_val(
			PRE_LOADING_DATA_SIZE + 2, 0, 0xFF) << 16;
		tmp64 = tmp64 | get_field_val(
			PRE_LOADING_DATA_SIZE + 3, 0, 0xFF) << 24;
		tmp = get_field_val(DATA_SECTOR_SIZE, 0, 0x1);
		if (tmp64 == 0xFFFFFFFF)
			if (tmp)
				str = strdup("16 TB");
			else
				str = strdup("2 TB");
		else
			if (tmp)
				str = basprintf("%llu B", tmp64 * 4096);
			else
				str = basprintf("%llu B", tmp64 * 512);
		printf("\tPre_Loading_Data_Size: %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_FFU_STATUS:
		print_field_caption(FFU_STATUS, R);
		val = get_field_val(FFU_STATUS, 0, 0x13);
		switch (val) {
		case 0x0:
			str = "success";
			break;
		case 0x10:
			str = "general error";
			break;
		case 0x11:
			str = "firmware install error";
			break;
		case 0x12:
			str = "firmware download error";
			break;
		}
		printf("\t[5-0] Code: %s\n", str);
		return 1;

	case EXT_CSD_MODE_CONFIG:
		print_field_caption(MODE_CONFIG, RWE_P);
		val = get_field_val(MODE_CONFIG, 0, 0xFF);
		switch (val) {
		case 0x0:
			str = "normal";
			break;
		case 0x1:
			str = "FFU";
			break;
		case 0x10:
			str = "vendor";
			break;
		}
		printf("\t[7-0] Value: %s\n", str);
		return 1;

	case EXT_CSD_BARRIER_CTRL:
		print_field_caption(BARRIER_CTRL, RW);
		val = get_field_val(BARRIER_CTRL, 0, 0x1);
		printf("\t[0] BARRIER_EN: %s\n", val ? "ON" : "OFF");
		return 1;

	case EXT_CSD_OUT_OF_INTERRUPT_TIME:
		print_field_caption(OUT_OF_INTERRUPT_TIME, R);
		val = get_field_val(OUT_OF_INTERRUPT_TIME, 0, 0xFF);
		val = val * 10;
		if (val)
			printf("\tOut-of-interrupt timeout definition: %u ms\n",
				val);
		else
			printf("\tNot Defined\n");
		return 1;

	case EXT_CSD_PARTITION_SWITCH_TIME:
		print_field_caption(PARTITION_SWITCH_TIME, R);
		val = get_field_val(PARTITION_SWITCH_TIME, 0, 0xFF);
		val = val * 10;
		if (val)
			printf("\tPartition switch timeout definition: %u ms\n",
				val);
		else
			printf("\tNot Defined\n");
		return 1;

	case EXT_CSD_DRIVER_STRENGTH:
		print_field_caption(DRIVER_STRENGTH, R);
		val = get_field_val(DRIVER_STRENGTH, 0, 0x1);
		printf("\t[0] Type 0: %ssupported\n", val ? "" : "not ");
		val = get_field_val(DRIVER_STRENGTH, 1, 0x1);
		printf("\t[1] Type 1: %ssupported\n", val ? "" : "not ");
		val = get_field_val(DRIVER_STRENGTH, 2, 0x1);
		printf("\t[2] Type 2: %ssupported\n", val ? "" : "not ");
		val = get_field_val(DRIVER_STRENGTH, 3, 0x1);
		printf("\t[3] Type 3: %ssupported\n", val ? "" : "not ");
		val = get_field_val(DRIVER_STRENGTH, 4, 0x1);
		printf("\t[4] Type 4: %ssupported\n", val ? "" : "not ");
		return 1;

	case EXT_CSD_CACHE_FLUSH_POLICY:
		print_field_caption(CACHE_FLUSH_POLICY, R);
		val = get_field_val(CACHE_FLUSH_POLICY, 0, 0x1);
		if (val)
			str = "FIFO policy for cache";
		else
			str = "not provided";
		printf("\t[0] Device flushing: %s", str);
		return 1;

	case EXT_CSD_OPTIMAL_READ_SIZE:
		print_field_caption(OPTIMAL_READ_SIZE, R);
		val = get_field_val(OPTIMAL_READ_SIZE, 0, 0xFF);
		val = val * 4048;
		printf("\t[7-0] Minimum optimal read unit size: %u\n", val);
		return 1;

	case EXT_CSD_OPTIMAL_WRITE_SIZE:
		print_field_caption(OPTIMAL_WRITE_SIZE, R);
		val = get_field_val(OPTIMAL_WRITE_SIZE, 0, 0xFF);
		val = val * 4048;
		printf("\t[7-0] Minimum optimal write unit size: %u\n", val);
		return 1;

	case EXT_CSD_PRE_EOL_INFO:
		print_field_caption(PRE_EOL_INFO, R);
		val = get_field_val(PRE_EOL_INFO, 0, 0x3);
		switch (val) {
		case 1:
			str = "normal";
			break;
		case 2:
			str = "warning";
			break;
		case 3:
			str = "urgent";
			break;
		default:
			str = "Not defined";
		}
		printf("\t[1-0] Device life time: %s\n", str);
		return 1;

	case EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A:
		print_field_caption(DEVICE_LIFE_TIME_EST_TYP_A, R);
		val = get_field_val(DEVICE_LIFE_TIME_EST_TYP_A, 0, 0xFF);
		val = val * 10;
		if (val == 0)
			str = strdup("not defined");
		else if (val == 0xB)
			str = strdup("maximum");
		else
			str = basprintf("%u%% - %u%%", (val - 10), val);
		printf("\tDevice life time, type A (estimation): %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B:
		print_field_caption(DEVICE_LIFE_TIME_EST_TYP_B, R);
		val = get_field_val(DEVICE_LIFE_TIME_EST_TYP_B, 0, 0xFF);
		val = val * 10;
		if (val == 0)
			str = strdup("not defined");
		else if (val == 0xB)
			str = strdup("maximum");
		else
			str = basprintf("%u%% - %u%%", (val - 10), val);
		printf("\tDevice life time, type B (estimation): %s\n", str);
		free(str);
		return 1;

	/*   EXT_CSD_NMBR_OF_FW_SCTRS_CRRCTLY_PRGRMD */
	case 305:
	case 304:
	case 303:
	case 302:
		print_field_caption_with_offset(NMBR_OF_FW_SCTRS_CRRCTLY_PRGRMD,
			index - EXT_CSD_NMBR_OF_FW_SCTRS_CRRCTLY_PRGRMD, R);
		return 1;

	case EXT_CSD_CMDQ_DEPTH:
		print_field_caption(CMDQ_DEPTH, R);
		val = get_field_val(CMDQ_DEPTH, 0, 0xF);
		++val;
		printf("\t[3-0] Queue Depth: %u", val);
		return 1;

	case EXT_CSD_CMDQ_SUPPORT:
		print_field_caption(CMDQ_SUPPORT, R);
		val = get_field_val(CMDQ_SUPPORT, 0, 0x1);
		printf("\t[0] Command queuing: %ssupported\n",
			val ? "" : "not ");
		return 1;

	case EXT_CSD_BARRIER_SUPPORT:
		print_field_caption(BARRIER_SUPPORT, R);
		val = get_field_val(BARRIER_SUPPORT, 0, 0x1);
		printf("\t[0] Barrier command: %ssupported\n",
			val ? "" : "not ");
		return 1;

	/*   EXT_CSD_FFU_ARG */
	case 490:
	case 489:
	case 488:
	case 487:
		print_field_caption_with_offset(FFU_ARG,
			index - EXT_CSD_FFU_ARG, R);
		return 1;

	case EXT_CSD_OPERATION_CODES_TIMEOUT:
		print_field_caption(OPERATION_CODES_TIMEOUT, R);
		val = get_field_val(OPERATION_CODES_TIMEOUT, 0, 0xFF);
		printf("\t[7-0] Timeout Values: %#02x\n", val);
		return 1;

	case EXT_CSD_FFU_FEATURES:
		print_field_caption(FFU_FEATURES, R);
		val = get_field_val(FFU_FEATURES, 0, 0x1);
		printf("\t[0] NUMBER_OF_FW_SECTORS_CORRECTLY_PROGRAMMED:\n"
			"%ssupported\n", val ? "" : " not");
		return 1;

	case EXT_CSD_SUPPORTED_MODES:
		print_field_caption(SUPPORTED_MODES, R);
		val = get_field_val(SUPPORTED_MODES, 0, 0x1);
		printf("\t[0] FFU: %ssupported\n", val ? "" : "not ");
		val = get_field_val(SUPPORTED_MODES, 1, 0x1);
		printf("\t[1] VSM: %ssupported\n", val ? "" : "not ");
		return 1;
	}
	return 0;
}
static int print_field_ge_v6(u8 *reg, int index)
{
	int rev;
	u32 val;
	u32 tmp;
	char *str = NULL;

	rev = reg[EXT_CSD_REV];

	switch (index) {
	case EXT_CSD_CACHE_CTRL:
		print_field_caption(CACHE_CTRL, RWE_P);
		val = get_field_val(CACHE_CTRL, 0, 0x1);
		printf("\t[0] CACHE_EN: %s\n", val ? "ON" : "OFF");
		return 1;

	case EXT_CSD_POWER_OFF_NOTIFICATION:
		print_field_caption(POWER_OFF_NOTIFICATION, RWE_P);
		val = get_field_val(POWER_OFF_NOTIFICATION, 0, 0x7);
		switch (val) {
		case 0x0:
			printf("\t[2-0] NO_POWER_NOTIFICATION\n");
			break;
		case 0x1:
			printf("\t[2-0] POWERED_ON\n");
			break;
		case 0x2:
			printf("\t[2-0] POWER_OFF_SHORT\n");
			break;
		case 0x3:
			printf("\t[2-0] POWER_OFF_LONG\n");
			break;
		case 0x4:
			printf("\t[2-0] SLEEP_NOTIFICATION\n");
			break;
		}
		return 1;

	case EXT_CSD_PACKED_FAILURE_INDEX:
		print_field_caption(PACKED_FAILURE_INDEX, R);
		val = get_field_val(PACKED_FAILURE_INDEX, 0, 0xFF);
		printf("\t[7-0] PACKED_FAILURE_INDEX: %u\n", val);
		return 1;

	case EXT_CSD_PACKED_COMMAND_STATUS:
		print_field_caption(PACKED_COMMAND_STATUS, R);
		val = get_field_val(PACKED_COMMAND_STATUS, 0, 0x1);
		printf("\t[0] Error: %u\n", val);
		val = get_field_val(PACKED_COMMAND_STATUS, 1, 0x1);
		printf("\t[1] Indexed Error: %u\n", val);
		return 1;

	/* EXT_CSD_CONTEXT_CONF */
	case 51:
	case 50:
	case 49:
	case 48:
	case 47:
	case 46:
	case 45:
	case 44:
	case 43:
	case 42:
	case 41:
	case 40:
	case 39:
	case 38:
	case 37:
		print_field_caption_with_offset(CONTEXT_CONF,
			index - EXT_CSD_CONTEXT_CONF, RWE_P);
		val = get_field_val_with_index(index, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "closed, not active";
			break;
		case 0x1:
			str = "configured and activated as write-only";
			break;
		case 0x2:
			str = "configured and activated as read-only";
			break;
		case 0x3:
			str = "configured and activated as read/write";
			break;
		}
		printf("\t[1-0] Activation and direction: context is %s\n",
			str);
		val = get_field_val_with_index(index, 2, 0x1);
		if (val)
			str = "follows rules";
		else
			str = "doesn't follow rules";
		printf("\t[2]   Large Unit context: %s\n", str);
		val = get_field_val_with_index(index, 3, 0x3);
		printf("\t[5-3] Large Unit multiplier: %u\n", val);
		val = get_field_val_with_index(index, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "MODE0";
			break;
		case 0x1:
			str = "MODE1";
			break;
		case 0x2:
			str = "MODE2";
			break;
		}
		printf("\t[7-6] Reliability mode: %s\n", str);
		return 1;

	/*   EXT_CSD_EXT_PARTITIONS_ATTRIBUTE */
	case 52:
		print_field_caption_with_offset(
			EXT_PARTITIONS_ATTRIBUTE,
			index - EXT_CSD_EXT_PARTITIONS_ATTRIBUTE, RW);
		printf("\t[3-0] EXT_1\n");
		printf("\t[7-4] EXT_2\n");
		return 1;
	case 53:
		print_field_caption_with_offset(
			EXT_PARTITIONS_ATTRIBUTE,
			index - EXT_CSD_EXT_PARTITIONS_ATTRIBUTE, RW);
		printf("\t[11-8]  EXT_3\n");
		printf("\t[15-12] EXT_4\n");
		return 1;

	/*   EXT_CSD_EXCEPTION_EVENTS_STATUS */
	case 54:
		print_field_caption_with_offset(
			EXCEPTION_EVENTS_STATUS,
			index - EXT_CSD_EXCEPTION_EVENTS_STATUS, R);
		val = get_field_val(EXCEPTION_EVENTS_STATUS, 0, 0x1);
		printf("\t[0] URGENT_BKOPS: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_STATUS, 1, 0x1);
		printf("\t[1] DYNCAP_NEEDED: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_STATUS, 2, 0x1);
		printf("\t[2] SYSPOOL_EXHAUSTED: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_STATUS, 3, 0x1);
		printf("\t[3] PACKED_FAILURE: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_STATUS, 4, 0x1);
		printf("\t[4] EXTENDED_SECURITY_FAILURE: %i\n", val);
		return 1;
	case 55:
		print_field_caption_with_offset(
			EXCEPTION_EVENTS_STATUS,
			index - EXT_CSD_EXCEPTION_EVENTS_STATUS, R);
		return 1;

	/*   EXT_CSD_EXCEPTION_EVENTS_CTRL */
	case 56:
		print_field_caption_with_offset(EXCEPTION_EVENTS_CTRL,
			index - EXT_CSD_EXCEPTION_EVENTS_CTRL, RWE_P);
		val = get_field_val(EXCEPTION_EVENTS_CTRL, 1, 0x1);
		printf("\t[1] DYNCAP_EVENT_EN: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_CTRL, 2, 0x1);
		printf("\t[2] SYSPOOL_EVENT_EN: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_CTRL, 3, 0x1);
		printf("\t[3] PACKED_EVENT_EN: %i\n", val);
		val = get_field_val(EXCEPTION_EVENTS_CTRL, 4, 0x1);
		printf("\t[4] EXTENDED_SECURITY_EN: %i\n", val);
		return 1;
	case 57:
		print_field_caption(EXCEPTION_EVENTS_CTRL, RWE_P);
		printf("\tR/W/E_P\n");
		printf("\tValue: %#02x\n", reg[index]);
		return 1;

	case EXT_CSD_CLASS_6_CTRL:
		print_field_caption(CLASS_6_CTRL, RWE_P);
		val = get_field_val(CLASS_6_CTRL, 0, 0x1);
		if (val)
			printf("\t[0] Dynamic Capacity\n");
		else
			printf("\t[0] Write Protect\n");
		return 1;

	case EXT_CSD_INI_TIMEOUT_EMU:
		print_field_caption(INI_TIMEOUT_EMU, R);
		val = get_field_val(INI_TIMEOUT_EMU, 0, 0xFF);
		val = val * 100;
		printf("\tInitialization Time out value: %u ms\n", val);
		return 1;

	case EXT_CSD_DATA_SECTOR_SIZE:
		print_field_caption(DATA_SECTOR_SIZE, R);
		val = get_field_val(DATA_SECTOR_SIZE, 0, 0x1);
		if (val)
			str = "4 KB";
		else
			str = "512 B";
		printf("\t[0] Data sector size is %s\n", str);
		return 1;

	case EXT_CSD_USE_NATIVE_SECTOR:
		print_field_caption(USE_NATIVE_SECTOR, RW);
		val = get_field_val(USE_NATIVE_SECTOR, 0, 0x1);
		if (val)
			printf("\t[0] Device sector size is larger than 512 B\n");
		else
			printf("\t[0] Device sector size is 512 B\n"
			"(emulated or native)\n");
		return 1;

	case EXT_CSD_NATIVE_SECTOR_SIZE:
		print_field_caption(NATIVE_SECTOR_SIZE, R);
		val = get_field_val(NATIVE_SECTOR_SIZE, 0, 0x1);
		if (val)
			str = "4 KB";
		else
			str = "512 B";
		printf("\t[0] Native sector size is %s\n", str);
		return 1;

	case EXT_CSD_PROGRAM_CID_CSD_DDR_SUPPORT:
		print_field_caption(PROGRAM_CID_CSD_DDR_SUPPORT, R);
		val = get_field_val(PROGRAM_CID_CSD_DDR_SUPPORT, 0, 0x1);
		if (val)
			str = "single data rate and dual data rate";
		else
			str = "single data rate";
		printf("\t[0] PROGRAM_CID_CSD_DDR_SUPPORT: CMD26 and CMD27\n"
			"%s mode\n", str);
		return 1;

	case EXT_CSD_PERIODIC_WAKEUP:
		print_field_caption(PERIODIC_WAKEUP, RWE);
		val = get_field_val(PERIODIC_WAKEUP, 0, 0x1F);
		printf("\t[5-0] WAKEUP_PERIOD: %u\n", val);
		val = get_field_val(PERIODIC_WAKEUP, 5, 0x7);
		switch (val) {
		case 0x0:
			str = "infinity";
			break;
		case 0x1:
			str = "months";
			break;
		case 0x2:
			str = "weeks";
			break;
		case 0x3:
			str = "days";
			break;
		case 0x4:
			str = "hours";
			break;
		case 0x5:
			str = "minutes";
			break;
		}
		printf("\t[7-5] WAKEUP_UNIT: %s\n", str);
		return 1;

	case EXT_CSD_PWR_CL_200_195:
		print_field_caption(PWR_CL_200_195, R);
		val = get_field_val(PWR_CL_200_195, 0, 0xFF);
		printf("\tPower class for 200MHz, at 1.95V %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_200_360:
		print_field_caption(PWR_CL_200_360, R);
		val = get_field_val(PWR_CL_200_360, 0, 0xFF);
		printf("\tPower class for 200MHz, at 3.6V %#02x\n", val);
		return 1;

	case EXT_CSD_POWER_OFF_LONG_TIME:
		print_field_caption(POWER_OFF_LONG_TIME, R);
		val = get_field_val(POWER_OFF_LONG_TIME, 0, 0xFF);
		val = val * 10;
		printf("\tGeneric Switch Timeout Definition: %u ms\n", val);
		return 1;

	case EXT_CSD_GENERIC_CMD6_TIME:
		print_field_caption(GENERIC_CMD6_TIME, R);
		val = get_field_val(GENERIC_CMD6_TIME, 0, 0xFF);
		val = val * 10;
		printf("\tGeneric Switch Timeout Definition: %u ms\n", val);
		return 1;

	/*   EXT_CSD_CACHE_SIZE */
	case 252:
	case 251:
	case 250:
	case 249:
		print_field_caption_with_offset(CACHE_SIZE,
			index - EXT_CSD_CACHE_SIZE, R);
		val = get_field_val(CACHE_SIZE, 0, 0xFF);
		val = val | get_field_val(CACHE_SIZE + 1, 0, 0xFF) << 8;
		val = val | get_field_val(CACHE_SIZE + 2, 0, 0xFF) << 16;
		val = val | get_field_val(CACHE_SIZE + 3, 0, 0xFF) << 24;
		printf("\tCache Size: %u KiB\n", val);
		return 1;

	case EXT_CSD_EXT_SUPPORT:
		print_field_caption(EXT_SUPPORT, R);
		val = get_field_val(EXT_SUPPORT, 0, 0x1);
		printf("\t[0] System code: %ssupported\n", val ? "" : "not ");
		val = get_field_val(EXT_SUPPORT, 1, 0x1);
		printf("\t[1] Non-persistent: %ssupported\n",
			val ? "" : "not ");
		return 1;

	case EXT_CSD_LARGE_UNIT_SIZE_M1:
		print_field_caption(LARGE_UNIT_SIZE_M1, R);
		val = get_field_val(LARGE_UNIT_SIZE_M1, 0, 0xFF);
		printf("\tLarge Unit size: %#02x\n", val);
		return 1;

	case EXT_CSD_CONTEXT_CAPABILITIES:
		print_field_caption(CONTEXT_CAPABILITIES, R);
		val = get_field_val(CONTEXT_CAPABILITIES, 0, 0xF);
		printf("\t[3-0] MAX_CONTEXT_ID: %#02x\n", val);
		val = get_field_val(CONTEXT_CAPABILITIES, 4, 0x7);
		printf("\t[6-4] LARGE_UNIT_MAX_MULTIPLIER_M1: %#02x\n", val);
		return 1;

	case EXT_CSD_TAG_RES_SIZE:
		print_field_caption(TAG_RES_SIZE, R);
		val = get_field_val(TAG_RES_SIZE, 0, 0xFF);
		printf("\tSystem Data Tag Resources Size: %#02x\n", val);
		return 1;

	case EXT_CSD_TAG_UNIT_SIZE:
		print_field_caption(TAG_UNIT_SIZE, R);
		tmp = get_field_val(NATIVE_SECTOR_SIZE, 0, 0x1);
		tmp = (tmp == 0) ? 512 : 4048;
		val = 2 << (1 - get_field_val(TAG_UNIT_SIZE, 0, 0xFF));
		val = val * tmp;
		printf("\tTag Unit Size: %u\n", val);
		return 1;

	case EXT_CSD_DATA_TAG_SUPPORT:
		print_field_caption(DATA_TAG_SUPPORT, R);
		val = get_field_val(DATA_TAG_SUPPORT, 0, 0x1);
		printf("\t[0] SYSTEM_DATA_TAG_SUPPORT: %u\n", val);
		return 1;

	case EXT_CSD_MAX_PACKED_WRITES:
		print_field_caption(MAX_PACKED_WRITES, R);
		val = get_field_val(MAX_PACKED_WRITES, 0, 0xFF);
		printf("\tmaximum number of commands in write command: %u\n",
			val);
		return 1;

	case EXT_CSD_MAX_PACKED_READS:
		print_field_caption(MAX_PACKED_READS, R);
		val = get_field_val(MAX_PACKED_READS, 0, 0xFF);
		printf("\tmaximum number of commands in read command: %u\n",
			val);
		return 1;
	}
	return 0;
}
static int print_field_ge_v5(u8 *reg, int index)
{
	int rev;
	u32 val;
	u32 tmp;
	u64 tmp64;
	char *str = NULL;

	rev = reg[EXT_CSD_REV];

	switch (index) {
	case EXT_CSD_TCASE_SUPPORT:
		print_field_caption(TCASE_SUPPORT, WE_P);
		val = get_field_val(TCASE_SUPPORT, 0, 0xFF);
		printf("\t[7-0] TCASE_SUPPORT: %#02x\n", val);
		return 1;

	case EXT_CSD_PRODUCTION_STATE_AWARENESS:
		print_field_caption(PRODUCTION_STATE_AWARENESS, RWE);
		val = get_field_val(PRODUCTION_STATE_AWARENESS, 0, 0xFF);
		switch (val) {
		case 0x0:
			str = "NORMAL";
			break;
		case 0x1:
			str = "PRE_SOLDERING_WRITES";
			break;
		case 0x2:
			str = "PRE_SOLDERING_POST_WRITES";
			break;
		case 0x3:
			str = "AUTO_PRE_SOLDERING";
			break;
		}
		printf("\t[7-0] State: %s\n", str);
		return 1;

	case EXT_CSD_SEC_BAD_BLK_MGMNT:
		print_field_caption(SEC_BAD_BLK_MGMNT, RW);
		val = get_field_val(SEC_BAD_BLK_MGMNT, 0, 0x1);
		printf("\t[0] SEC_BAD_BLK: %sabled\n", val ? "en" : "dis");
		return 1;

	/*   EXT_CSD_ENH_START_ADDR */
	case 139:
	case 138:
	case 137:
	case 136:
		print_field_caption_with_offset(ENH_START_ADDR,
			index - EXT_CSD_ENH_START_ADDR, RW);
		val = get_field_val(ENH_START_ADDR, 0, 0xFF);
		val = val | get_field_val(ENH_START_ADDR + 1, 0, 0xFF) << 8;
		val = val | get_field_val(ENH_START_ADDR + 2, 0, 0xFF) << 16;
		val = val | get_field_val(ENH_START_ADDR + 3, 0, 0xFF) << 24;
		printf("\tEnhanced User Data Start Address: 0x%x\n", val);
		return 1;

	/*   EXT_CSD_ENH_SIZE_MULT */
	case 142:
	case 141:
	case 140:
		print_field_caption_with_offset(ENH_SIZE_MULT,
			index - EXT_CSD_ENH_SIZE_MULT, RW);
		val = get_field_val(ENH_SIZE_MULT, 0, 0xFF);
		val = val | get_field_val(ENH_SIZE_MULT + 1, 0, 0xFF) << 8;
		val = val | get_field_val(ENH_SIZE_MULT + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tEnhanced User Data Area %i Size: %llu B\n",
				index - EXT_CSD_ENH_SIZE_MULT, tmp64);
		return 1;

	/*   EXT_CSD_GP_SIZE_MULT_GPX */
	case 154:
	case 153:
	case 152:
		tmp = index - EXT_CSD_GP_SIZE_MULT;
		print_field_caption_with_offset(GP_SIZE_MULT, tmp, RW);
		val = get_field_val_with_index(tmp, 0, 0xFF);
		val = val | get_field_val_with_index(tmp + 1, 0, 0xFF) << 8;
		val = val | get_field_val_with_index(tmp + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tGeneral_Purpose_Partition_%i Size: %llu B\n",
				index - EXT_CSD_GP_SIZE_MULT, tmp64);
		return 1;
	case 151:
	case 150:
	case 149:
		tmp = index - EXT_CSD_GP_SIZE_MULT;
		print_field_caption_with_offset(GP_SIZE_MULT, tmp, RW);
		val = get_field_val_with_index(tmp, 0, 0xFF);
		val = val | get_field_val_with_index(tmp + 1, 0, 0xFF) << 8;
		val = val | get_field_val_with_index(tmp + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tGeneral_Purpose_Partition_%i Size: %llu B\n",
				index - EXT_CSD_GP_SIZE_MULT, tmp64);
		return 1;
	case 148:
	case 147:
	case 146:
		tmp = index - EXT_CSD_GP_SIZE_MULT;
		print_field_caption_with_offset(GP_SIZE_MULT, tmp, RW);
		val = get_field_val_with_index(tmp, 0, 0xFF);
		val = val | get_field_val_with_index(tmp + 1, 0, 0xFF) << 8;
		val = val | get_field_val_with_index(tmp + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tGeneral_Purpose_Partition_%i Size: %llu B\n",
				index - EXT_CSD_GP_SIZE_MULT, tmp64);
		return 1;
	case 145:
	case 144:
	case 143:
		tmp = index - EXT_CSD_GP_SIZE_MULT;
		print_field_caption_with_offset(GP_SIZE_MULT, tmp, RW);
		val = get_field_val_with_index(tmp, 0, 0xFF);
		val = val | get_field_val_with_index(tmp + 1, 0, 0xFF) << 8;
		val = val | get_field_val_with_index(tmp + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tGeneral_Purpose_Partition_%i Size: %llu B\n",
				index - EXT_CSD_GP_SIZE_MULT, tmp64);
		return 1;

	case EXT_CSD_PARTITION_SETTING_COMPLETED:
		print_field_caption(PARTITION_SETTING_COMPLETED, RW);
		val = get_field_val(PARTITION_SETTING_COMPLETED, 0, 0x1);
		printf("\t[0] PARTITION_SETTING_COMPLETED: %u\n", val);
		return 1;

	case EXT_CSD_PARTITIONS_ATTRIBUTE:
		print_field_caption(PARTITIONS_ATTRIBUTE, RW);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 0, 0x1);
		if (val)
			str = "enhanced attribute in user data area";
		else
			str = "default";
		printf("\t[0] ENH_USR: %s\n", str);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 1, 0x1);
		if (val)
			str = "enhanced attribute in general purpose part 1";
		else
			str = "default";
		printf("\t[1] ENH_1: %s\n", str);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 2, 0x1);
		if (val)
			str = "enhanced attribute in general purpose part 2";
		else
			str = "default";
		printf("\t[2] ENH_2: %s\n", str);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 3, 0x1);
		if (val)
			str = "enhanced attribute in general purpose part 3";
		else
			str = "default";
		printf("\t[3] ENH_3: %s\n", str);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 4, 0x1);
		if (val)
			str = "enhanced attribute in general purpose part 4";
		else
			str = "default";
		printf("\t[4] ENH_4: %s\n", str);
		val = get_field_val(PARTITIONS_ATTRIBUTE, 5, 0x1);
		if (val)
			str = "enhanced attribute in general purpose part 5";
		else
			str = "default";
		printf("\t[5] ENH_5: %s\n", str);
		return 1;

	/*   EXT_CSD_MAX_ENH_SIZE_MULT */
	case 159:
	case 158:
	case 157:
		print_field_caption_with_offset(MAX_ENH_SIZE_MULT,
			index - EXT_CSD_MAX_ENH_SIZE_MULT, R);
		val = get_field_val(MAX_ENH_SIZE_MULT, 0, 0xFF);
		val = val | get_field_val(MAX_ENH_SIZE_MULT + 1, 0, 0xFF) << 8;
		val = val | get_field_val(MAX_ENH_SIZE_MULT + 2, 0, 0xFF) << 16;
		tmp = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		tmp = tmp + get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		tmp64 = val * tmp * 524288;
		printf("\tMax Enhanced Area: %llu B\n", tmp64);
		return 1;

	case EXT_CSD_PARTITIONING_SUPPORT:
		print_field_caption(PARTITIONING_SUPPORT, R);
		val = get_field_val(PARTITIONING_SUPPORT, 0, 0x1);
		printf("\t[0] PARTITIONING_EN: %ssupported\n",
			val ? "" : "not ");
		val = get_field_val(PARTITIONING_SUPPORT, 1, 0x1);
		printf("\t[1] ENH_ATTRIBUTE_EN: %ssupported\n",
			val ? "" : "not ");
		val = get_field_val(PARTITIONING_SUPPORT, 2, 0x1);
		printf("\t[2] EXT_ATTRIBUTE_EN: %ssupported\n",
			val ? "" : "not ");
		return 1;

	case EXT_CSD_HPI_MGMT:
		print_field_caption(HPI_MGMT, R);
		val = get_field_val(HPI_MGMT, 0, 0xFF);
		printf("\t[7-0] HPI_EN: %sactivated\n", val ? "" : "not ");
		return 1;

	case EXT_CSD_RST_N_FUNCTION:
		print_field_caption(RST_N_FUNCTION, R);
		val = get_field_val(RST_N_FUNCTION, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "temporarily disabled";
			break;
		case 0x1:
			str = "permanently enabled";
			break;
		case 0x2:
			str = "permanently disabled";
			break;
		}
		printf("\t[1-0] RST_N_ENABLE: RST_n signal is %s\n", str);
		return 1;

	case EXT_CSD_BKOPS_EN:
		print_field_caption(BKOPS_EN, RWaRWE);
		val = get_field_val(BKOPS_EN, 0, 0x1);
		if (val)
			str = "Host is indicated";
		else
			str = "not supported by Host";
		printf("\t[0] MANUAL_EN: %s\n", str);
		val = get_field_val(BKOPS_EN, 1, 0x1);
		if (val)
			str = "may";
		else
			str = "shall not";
		printf("\t[1] AUTO_EN: Device %s perform background ops while\n"
			"not servicing the host\n", str);
		return 1;

	case EXT_CSD_BKOPS_START:
		print_field_caption(BKOPS_START, WE_P);
		printf("\t[7-0] Writing shall start a background operations.\n");
		return 1;

	case EXT_CSD_SANITIZE_START:
		print_field_caption(SANITIZE_START, WE_P);
		printf("\t[7-0] Writing shall start a sanitize operation.\n");
		return 1;

	case EXT_CSD_WR_REL_PARAM:
		print_field_caption(WR_REL_PARAM, R);
		val = get_field_val(WR_REL_PARAM, 0, 0x1);
		if (val)
			str = "all the WR_DATA_REL parameters in the WR_REL_SET registers are R/W";
		else
			str = "obsolete";
		printf("\t[0] HS_CTRL_REL: %s\n", str);
		val = get_field_val(WR_REL_PARAM, 2, 0x1);
		if (val)
			str = "the device supports the enhanced definition of reliable write";
		else
			str = "obsolete";
		printf("\t[2] EN_REL_WR: %s\n", str);
		val = get_field_val(WR_REL_PARAM, 4, 0x1);
		printf("\t[4] EN_RPMB_REL_WR: RPMB transfer size is either\n"
			"256B (1 512B frame), 512B (2 512B frame)%s\n",
			val ? ", 8KB (32 512B frames)" : "");
		return 1;

	case EXT_CSD_WR_REL_SET:
		print_field_caption(WR_REL_SET, RW);
		val = get_field_val(WR_REL_SET, 0, 0x1);
		if (val)
			str = "the device protects previously written data if power failure occurs";
		else
			str = "optimized for performance,data could be at risk if on power failure";
		printf("\t[0] WR_DATA_REL_USR: %s\n", str);
		val = get_field_val(WR_REL_SET, 1, 0x1);
		if (val)
			str = "the device protects previously written data if power failure occurs";
		else
			str = "optimized for performance,data could be at risk if on power failure";
		printf("\t[1] WR_DATA_REL_1: %s\n", str);
		val = get_field_val(WR_REL_SET, 2, 0x1);
		if (val)
			str = "the device protects previously written data if power failure occurs";
		else
			str = "optimized for performance,data could be at risk if on power failure";
		printf("\t[2] WR_DATA_REL_2: %s\n", str);
		val = get_field_val(WR_REL_SET, 3, 0x1);
		if (val)
			str = "the device protects previously written data if power failure occurs";
		else
			str = "optimized for performance,data could be at risk if on power failure";
		printf("\t[3] WR_DATA_REL_3: %s\n", str);
		val = get_field_val(WR_REL_SET, 4, 0x1);
		if (val)
			str = "the device protects previously written data if power failure occurs";
		else
			str = "optimized for performance,data could be at risk if on power failure";
		printf("\t[4] WR_DATA_REL_4: %s\n", str);
		return 1;

	case EXT_CSD_RPMB_SIZE_MULT:
		print_field_caption(RPMB_SIZE_MULT, R);
		val = get_field_val(RPMB_SIZE_MULT, 0, 0xFF);
		val = val * 131072;
		printf("\t[7-0] RPMB Partition Size: %u KB\n", val);
		return 1;

	case EXT_CSD_FW_CONFIG:
		print_field_caption(FW_CONFIG, RW);
		val = get_field_val(FW_CONFIG, 0, 0x1);
		printf("\t[0] Update_Disable: FW update %sabled\n",
			val ? "dis" : "en");
		return 1;

	case EXT_CSD_USER_WP:
		print_field_caption(USER_WP, RWaRWC_PaRWE_P);
		val = get_field_val(USER_WP, 0, 0x1);
		if (val)
			str = "apply Power-On Period protection to the protection group indicated by CMD28";
		else
			str = "power-on write protection is not applied when CMD28 is issued";
		printf("\t[0] US_PWR_WP_EN: %s\n", str);
		val = get_field_val(USER_WP, 2, 0x1);
		if (val)
			str = "apply permanent write protection to the protection group indicated by CMD28";
		else
			str = "permanent write protection is not applied when CMD28 is issued";
		printf("\t[2] US_PERM_WP_EN: %s\n", str);
		val = get_field_val(USER_WP, 3, 0x1);
		if (val)
			str = "disable the use of power-on period write protection";
		else
			str = "power-on write protection can be applied to write protection groups";
		printf("\t[3] US_PWR_WP_DIS: %s\n", str);
		val = get_field_val(USER_WP, 4, 0x1);
		if (val)
			str = "permanently disable the use of permanent write protection";
		else
			str = "permanent write protection can be applied to write protection groups";
		printf("\t[4] US_PERM_WP_DIS: %s\n", str);
		val = get_field_val(USER_WP, 6, 0x1);
		if (val)
			str = "disable the use of PERM_WRITE_PROTECT (CSD[13])";
		else
			str = "host is permitted to set PERM_WRITE_PROTECT (CSD[13])";
		printf("\t[6] CD_PERM_WP_DIS: %s\n", str);

		val = get_field_val(USER_WP, 7, 0x1);
		printf("\t[7] PERM_PSWD_DS: Password protection features\n"
			"are %sabled\n", val ? "dis" : "en");
		return 1;

	case EXT_CSD_BOOT_WP:
		print_field_caption(BOOT_WP, RWaRWC_P);
		val = get_field_val(BOOT_WP, 0, 0x1);
		if (val)
			str = "enable Power-On Period write protection to the boot area";
		else
			str = "boot region is not power-on write protected";
		printf("\t[0] B_PWR_WP_EN: %s\n", str);
		val = get_field_val(BOOT_WP, 1, 0x1);
		printf("\t[1] B_PWR_WP_SEC_SEL: B_PWR_WP_EN(Bit 0) applies\n"
			"to boot Area%s only, if B_SEC_WP_SEL (bit 7 is set)\n",
			val ? "2" : "1");
		val = get_field_val(BOOT_WP, 2, 0x1);
		printf("\t[2] B_PERM_WP_EN: Boot region is %spermanently\n"
			"write protected\n", val ? "" : "not ");
		val = get_field_val(BOOT_WP, 3, 0x1);
		printf("\t[3] B_PERM_WP_SEC_SEL: B_PERM_WP_EN(Bit 2) applies\n"
			"to boot Area%s only, if B_SEC_WP_SEL (bit 7 is set)\n",
			val ? "2" : "1");
		val = get_field_val(BOOT_WP, 4, 0x1);
		if (val)
			str = "permanently disable the use of";
		else
			str = "master is permitted to use";
		printf("\t[4] B_PERM_WP_DIS: %s B_PERM_WP_EN(bit 2)\n", str);
		val = get_field_val(BOOT_WP, 6, 0x1);
		if (val)
			str = "disable the use of";
		else
			str = "master is permitted to set";
		printf("\t[5] B_PWR_WP_DIS: %s B_PWR_WP_EN(bit 0)\n", str);
		val = get_field_val(BOOT_WP, 7, 0x1);
		if (val)
			str = "boot partition selected by B_PERM_WP_SEC_SEL (bit 3) and B_PWR_WP_SEC_SEL (bit 1)";
		else
			str = "boot partitions and B_PERM_WP_SEC_SEL (bit 3) and B_PWR_WP_SEC_SEL (bit 1) have no impact";
		printf("\t[6] B_SEC_WP_SEL: B_PERM_WP_EN(bit2) and\n"
			"B_PWR_WP_EN (bit 0) apply to %s\n", str);
		return 1;

	case EXT_CSD_BOOT_WP_STATUS:
		print_field_caption(BOOT_WP_STATUS, R);
		val = get_field_val(BOOT_WP_STATUS, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "not protected";
			break;
		case 0x1:
			str = "Power on protected";
			break;
		case 0x2:
			str = "Permanently Protected";
			break;
		}
		printf("\t[1-0] B_AREA_1_WP: Boot Area 1 is %s\n", str);
		val = get_field_val(BOOT_WP_STATUS, 2, 0x3);
		switch (val) {
		case 0x0:
			str = "not protected";
			break;
		case 0x1:
			str = "Power on protected";
			break;
		case 0x2:
			str = "Permanently Protected";
			break;
		}
		printf("\t[3-2] B_AREA_2_WP: Boot Area 2 is %s\n", str);
		return 1;

	case EXT_CSD_SEC_FEATURE_SUPPORT:
		print_field_caption(SEC_FEATURE_SUPPORT, R);
		val = get_field_val(SEC_FEATURE_SUPPORT, 0, 0x1);
		printf("\t[0] SECURE_ER_EN: %ssupported\n", val ? "" : "not ");
		val = get_field_val(SEC_FEATURE_SUPPORT, 2, 0x1);
		printf("\t[1] SEC_BD_BLK_EN: %ssupported\n", val ? "" : "not ");
		val = get_field_val(SEC_FEATURE_SUPPORT, 4, 0x1);
		printf("\t[4] SEC_GB_CL_EN: %ssupported\n", val ? "" : "not ");
		val = get_field_val(SEC_FEATURE_SUPPORT, 6, 0x1);
		printf("\t[6] SEC_SANITIZE: %ssupported\n", val ? "" : "not ");
		return 1;

	case EXT_CSD_TRIM_MULT:
		print_field_caption(TRIM_MULT, R);
		val = get_field_val(TRIM_MULT, 0, 0xFF);
		val = val * 300;
		printf("\t[7-0] TRIM/DISCARD Time out value: %u\n", val);
		return 1;

	case EXT_CSD_MIN_PERF_DDR_R_8_52:
		print_field_caption(MIN_PERF_DDR_R_8_52, R);
		val = get_field_val(MIN_PERF_DDR_R_8_52, 0, 0xFF);
		printf("\t[7-0] Minimum Read Performance for 8bit at 52MHz in\n"
			"DDR mode %#02x\n", val);
		return 1;

	case EXT_CSD_MIN_PERF_DDR_W_8_52:
		print_field_caption(MIN_PERF_DDR_W_8_52, R);
		val = get_field_val(MIN_PERF_DDR_W_8_52, 0, 0xFF);
		printf("\tMinimum Write Performance for 8bit at 52MHz in DDR\n"
			"mode %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_DDR_52_195:
		print_field_caption(PWR_CL_DDR_52_195, R);
		val = get_field_val(PWR_CL_DDR_52_195, 0, 0xFF);
		printf("\tPower class for 52MHz, DDR at 1.95V %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_DDR_52_360:
		print_field_caption(PWR_CL_DDR_52_360, R);
		val = get_field_val(PWR_CL_DDR_52_360, 0, 0xFF);
		printf("\tPower class for 52MHz, DDR at 3.6V %#02x\n", val);
		return 1;

	case EXT_CSD_INI_TIMEOUT_AP:
		print_field_caption(INI_TIMEOUT_AP, R);
		val = get_field_val(INI_TIMEOUT_AP, 0, 0xFF);
		val = val * 100;
		printf("\tInitialization Time out value: %u ms\n", val);
		return 1;

	/*   EXT_CSD_CORRECTLY_PRG_SECTORS_NUM */
	case 245:
	case 244:
	case 243:
	case 242:
		print_field_caption_with_offset(CORRECTLY_PRG_SECTORS_NUM,
			index - EXT_CSD_CORRECTLY_PRG_SECTORS_NUM, R);
		val = get_field_val(CORRECTLY_PRG_SECTORS_NUM, 0, 0xFF);
		val = val | get_field_val(CORRECTLY_PRG_SECTORS_NUM + 1, 0,
			0xFF) << 8;
		val = val | get_field_val(CORRECTLY_PRG_SECTORS_NUM + 2, 0,
			0xFF) << 16;
		val = val | get_field_val(CORRECTLY_PRG_SECTORS_NUM + 3, 0,
			0xFF) << 24;
		printf("\tNumber of correctly programmed sectors: %u\n",
			val);
		return 1;

	case EXT_CSD_BKOPS_STATUS:
		print_field_caption(BKOPS_STATUS, R);
		val = get_field_val(BKOPS_STATUS, 0, 0x3);
		switch (val) {
		case 0:
			str = "not required";
			break;
		case 1:
			str = "outstanding (non critical)";
			break;
		case 2:
			str = "outstanding (performance being impacted)";
			break;
		case 3:
			str = "outstanding (critical)";
			break;
		}
		printf("\t[1-0] Operations %s\n", str);
		return 1;

	}

	return 0;
}
static int print_field_eq_v5(u8 *reg, int index)
{
	int rev;
	u32 val;

	rev = reg[EXT_CSD_REV];

	switch (index) {
	case EXT_CSD_SEC_TRIM_MULT:
		print_field_caption(SEC_TRIM_MULT, R);
		val = get_field_val(SEC_TRIM_MULT, 0, 0xFF);
		val = val * 300 * get_field_val(ERASE_TIMEOUT_MULT, 0,
			0xFF);
		printf("\tSecure Trim time-out value: %u\n", val);
		return 1;

	case EXT_CSD_SEC_ERASE_MULT:
		print_field_caption(SEC_ERASE_MULT, R);
		val = get_field_val(SEC_ERASE_MULT, 0, 0xFF);
		val = val * 300 * get_field_val(ERASE_TIMEOUT_MULT, 0,
			0xFF);
		printf("\tSecure Erase time-out value: %u\n", val);
		return 1;

	}

	return 0;
}

static int print_field(u8 *reg, int index)
{
	int rev;
	u32 val;
	u64 tmp64;
	char *str = NULL;

	rev = reg[EXT_CSD_REV];

	if (rev >= 7)
		return print_field_ge_v7(reg, index);
	if (rev >= 6)
		return print_field_ge_v6(reg, index);
	if (rev >= 5)
		return print_field_ge_v5(reg, index);
	if (rev == 5)
		return print_field_eq_v5(reg, index);

	switch (index) {
	case EXT_CSD_ERASE_GROUP_DEF:
		print_field_caption(ERASE_GROUP_DEF, RWE_P);
		val = get_field_val(ERASE_GROUP_DEF, 0, 0x1);
		printf("\t[0] ENABLE: Use %s size definition\n",
			val ? "high-capacity" : "old");
		return 1;

	case EXT_CSD_BOOT_BUS_CONDITIONS:
		print_field_caption(BOOT_BUS_CONDITIONS, RWE);
		val = get_field_val(BOOT_BUS_CONDITIONS, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "x1 (sdr) or x4 (ddr)";
			break;
		case 0x1:
			str = "x4 (sdr/ddr)";
			break;
		case 0x2:
			str = "x8 (sdr/ddr)";
			break;
		}
		printf("\t[1-0] BOOT_BUS_WIDTH: %s bus width in boot operation mode\n",
			str);
		val = get_field_val(BOOT_BUS_CONDITIONS, 2, 0x1);
		if (val)
			str = "Reset bus width to x1, SDR and backward compatible timings after boot operation";
		else
			str = "Retain BOOT_BUS_WIDTH and BOOT_MODE values after boot operation";
		printf("\t[2] RESET_BOOT_BUS_CONDITIONS: %s", str);
		val = get_field_val(BOOT_BUS_CONDITIONS, 3, 0x3);
		switch (val) {
		case 0x0:
			str = "Use SDR + backward compatible timings in boot operation";
			break;
		case 0x1:
			str = "Use SDR + HS timings in boot operation mode";
			break;
		case 0x2:
			str = "Use DDR in boot operation";
			break;
		}
		printf("\t[3] BOOT_MODE: %s\n", str);
		return 1;

	case EXT_CSD_BOOT_CONFIG_PROT:
		print_field_caption(BOOT_CONFIG_PROT, RWaRWC_P);
		val = get_field_val(BOOT_CONFIG_PROT, 0, 0x1);
		printf("\t[0] PWR_BOOT_CONFIG_PROT: %u\n", val);
		val = get_field_val(BOOT_CONFIG_PROT, 4, 0x1);
		printf("\t[4] PERM_BOOT_CONFIG_PROT: %u\n", val);
		return 1;

	case EXT_CSD_PARTITION_CONFIG:
		print_field_caption(PARTITION_CONFIG, RWEaRWE_P);
		val = get_field_val(PARTITION_CONFIG, 0, 0x7);
		switch (val) {
		case 0x0:
			str = "No access to boot partition";
			break;
		case 0x1:
			str = "R/W boot partition 1";
			break;
		case 0x2:
			str = "R/W boot partition 2";
			break;
		case 0x3:
			str = "R/W Replay Protected Memory Block (RPMB)";
			break;
		case 0x4:
			str = "Access to General Purpose partition 1";
			break;
		case 0x5:
			str = "Access to General Purpose partition 2";
			break;
		case 0x6:
			str = "Access to General Purpose partition 3";
			break;
		case 0x7:
			str = "Access to General Purpose partition 4";
			break;
		}
		printf("\t[2-0] PARTITION_ACCESS: %s\n", str);
		val = get_field_val(PARTITION_CONFIG, 3, 0x7);
		switch (val) {
		case 0x0:
			str = "Device not boot enabled";
			break;
		case 0x1:
			str = "Boot partition 1 enabled for boot";
			break;
		case 0x2:
			str = "Boot partition 2 enabled for boot";
			break;
		case 0x7:
			str = "User area enabled for boot";
			break;
		}
		printf("\t[5-3] BOOT_PARTITION_ENABLE: %s\n", str);
		val = get_field_val(PARTITION_CONFIG, 6, 0x1);
		if (val)
			str = "Boot acknowledge sent during boot operation Bit";
		else
			str = "No boot acknowledge sent";
		printf("\t[6] BOOT_ACK: %s\n", str);
		return 1;

	case EXT_CSD_ERASED_MEM_CONT:
		print_field_caption(ERASED_MEM_CONT, R);
		val = get_field_val(ERASED_MEM_CONT, 0, 0x1);
		printf("\t[0] Erased Memory Content: %u\n", val);
		return 1;

	case EXT_CSD_BUS_WIDTH:
		print_field_caption(BUS_WIDTH, WE_P);
		val = get_field_val(BUS_WIDTH, 0, 0xF);
		switch (val) {
		case 0:
			str = "1 bit data bus";
			break;
		case 1:
			str = "4 bit data bus";
			break;
		case 2:
			str = "8 bit data bus";
			break;
		case 5:
			str = "4 bit data bus (dual data rate)";
			break;
		case 6:
			str = "8 bit data bus (DDR)";
			break;
		}
		printf("\t[3-0] Bus Mode: %s\n", str);
		val = get_field_val(BUS_WIDTH, 7, 0x1);
		printf("\t[7] Strobe is provided during Data Out, CRC response%s\n",
			val ? ", CMD Response" : "");
		return 1;

	case EXT_CSD_STROBE_SUPPORT:
		print_field_caption(STROBE_SUPPORT, R);
		val = get_field_val(STROBE_SUPPORT, 0, 0x1);
		printf("\t[0] Enhanced Strobe mode: %ssupported\n",
			val ? "" : "not ");
		return 1;

	case EXT_CSD_HS_TIMING:
		print_field_caption(HS_TIMING, RWE_P);
		val = get_field_val(HS_TIMING, 0, 0xF);
		switch (val) {
		case 0x0:
			str = "Selecting backwards compatibility interface timing";
			break;
		case 0x1:
			str = "High Speed";
			break;
		case 0x2:
			str = "HS200";
			break;
		case 0x3:
			str = "HS400";
			break;
		}
		printf("\t[3-0] Timing Interface: %s\n", str);
		return 1;

	case EXT_CSD_POWER_CLASS:
		print_field_caption(POWER_CLASS, RWE_P);
		val = get_field_val(POWER_CLASS, 0, 0xFF);
		printf("\t[7-0] Device power class code: %#02x\n", val);
		return 1;

	case EXT_CSD_CMD_SET_REV:
		print_field_caption(CMD_SET_REV, R);
		val = get_field_val(CMD_SET_REV, 0, 0xFF);
		printf("\t[7-0] Command set revisions: %#02x\n", val);
		return 1;

	case EXT_CSD_CMD_SET:
		print_field_caption(CMD_SET, RWE_P);
		val = get_field_val(CMD_SET, 0, 0xFF);
		printf("\t[7-0] Command set that is currently active in the Device: %#02x\n",
			val);
		return 1;

	case EXT_CSD_REV:
		print_field_caption(REV, R);
		val = get_field_val(REV, 0, 0x1F);
		switch (val) {
		case 0:
			str = "1.0 (for MMC v4.0)";
			break;
		case 1:
			str = "1.1 (for MMC v4.1)";
			break;
		case 2:
			str = "1.2 (for MMC v4.2)";
			break;
		case 3:
			str = "1.3 (for MMC v4.3)";
			break;
		case 4:
			str = "1.4 (Obsolete)";
			break;
		case 5:
			str = "1.5 (for MMC v4.41)";
			break;
		case 6:
			str = "1.6 (for MMC v4.5, v4.51)";
			break;
		case 7:
			str = "1.7 (for MMC v5.0, v5.01)";
			break;
		case 8:
			str = "1.8 (for MMC v5.1)";
			break;
		}
		printf("\t[4-0] Extended CSD Revision: Revision %s\n", str);
		return 1;

	case EXT_CSD_CSD_STRUCTURE:
		print_field_caption(CSD_STRUCTURE, R);
		val = get_field_val(CSD_STRUCTURE, 0, 0x3);
		switch (val) {
		case 0x0:
			str = "0";
			break;
		case 0x1:
			str = "1";
			break;
		case 0x2:
			str = "2";
			break;
		}
		printf("\t[1-0] CSD structure version: CSD version No. 1.%s\n",
			str);
		return 1;

	case EXT_CSD_DEVICE_TYPE:
		print_field_caption(DEVICE_TYPE, R);
		val = get_field_val(DEVICE_TYPE, 0, 0xFF);
		switch (val) {
		case 0x1:
			str = "HS eMMC @26MHz - at rated device voltage(s)";
			break;
		case 0x2:
			str = "HS eMMC @52MHz - at rated device voltage(s)";
			break;
		case 0x4:
			str = "HS Dual Data Rate eMMC @52MHz 1.8V or 3VI/O";
			break;
		case 0x8:
			str = "HS Dual Data Rate eMMC @52MHz 1.2VI/O";
			break;
		case 0x10:
			str = "HS200 Single Data Rate eMMC @200MHz 1.8VI/O";
			break;
		case 0x20:
			str = "HS200 Single Data Rate eMMC @200MHz 1.2VI/O";
			break;
		}
		printf("\t%s\n", str);
		return 1;

	/* TODO: missing JEDEC documention */
	case EXT_CSD_PWR_CL_52_195:
		print_field_caption(PWR_CL_52_195, R)
		val = get_field_val(PWR_CL_52_195, 0, 0xFF);
		printf("\tPower class for 52 MHz at 1.95 V 1 R: %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_26_195:
		print_field_caption(PWR_CL_26_195, R)
		val = get_field_val(PWR_CL_26_195, 0, 0xFF);
		printf("\tPower class for 26 MHz at 1.95 V 1 R: %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_52_360:
		print_field_caption(PWR_CL_52_360, R)
		val = get_field_val(PWR_CL_52_360, 0, 0xFF);
		printf("\tPower class for 52 MHz at 3.6 V 1 R: %#02x\n", val);
		return 1;

	case EXT_CSD_PWR_CL_26_360:
		print_field_caption(PWR_CL_26_360, R)
		val = get_field_val(PWR_CL_26_360, 0, 0xFF);
		printf("\tPower class for 26 MHz at 3.6 V 1 R: %#02x\n", val);
		return 1;

	case EXT_CSD_MIN_PERF_R_4_26:
		print_field_caption(MIN_PERF_R_4_26, R)
		val = get_field_val(MIN_PERF_R_4_26, 0, 0xFF);
		printf("\tMinimum Read Performance for 4bit at 26 MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_MIN_PERF_W_4_26:
		print_field_caption(MIN_PERF_W_4_26, R)
		val = get_field_val(MIN_PERF_W_4_26, 0, 0xFF);
		printf("\tMinimum Write Performance for 4bit at 26 MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_MIN_PERF_R_8_26_4_52:
		print_field_caption(MIN_PERF_R_8_26_4_52, R)
		val = get_field_val(MIN_PERF_R_8_26_4_52, 0, 0xFF);
		printf("\tMinimum Read Performance for 8bit at 26 MHz, for 4bit at 52MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_MIN_PERF_W_8_26_4_52:
		print_field_caption(MIN_PERF_W_8_26_4_52, R)
		val = get_field_val(MIN_PERF_W_8_26_4_52, 0, 0xFF);
		printf("\tMinimum Write Performance for 8bit at 26 MHz, for 4bit at 52MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_MIN_PERF_R_8_52:
		print_field_caption(MIN_PERF_R_8_52, R)
		val = get_field_val(MIN_PERF_R_8_52, 0, 0xFF);
		printf("\tMinimum Read Performance for 8bit at 52 MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_MIN_PERF_W_8_52:
		print_field_caption(MIN_PERF_W_8_52, R)
		val = get_field_val(MIN_PERF_W_8_52, 0, 0xFF);
		printf("\tMinimum Write Performance for 8bit at52 MHz: %#02x\n",
			val);
		return 1;

	case EXT_CSD_SECURE_WP_INFO:
		print_field_caption(SECURE_WP_INFO, R);
		val = get_field_val(SECURE_WP_INFO, 0, 0x1);
		printf("\t[0] SECURE_WP_SUPPORT: %ssupported\n",
			val ? "" : "not ");
		val = get_field_val(SECURE_WP_INFO, 1, 0x1);
		printf("\t[1] SECURE_WP_EN_STATUS: %s Write Protection mode\n",
			val ? "Secure" : "Legacy");
		return 1;

	/*   EXT_CSD_SEC_COUNT */
	case 215:
	case 214:
	case 213:
	case 212:
		print_field_caption_with_offset(SEC_COUNT,
			index - EXT_CSD_SEC_COUNT, R);
		val = get_field_val(SEC_COUNT, 0, 0xFF);
		val = val | get_field_val(SEC_COUNT + 1, 0, 0xFF) << 8;
		val = val | get_field_val(SEC_COUNT + 2, 0, 0xFF) << 16;
		val = val | get_field_val(SEC_COUNT + 3, 0, 0xFF) << 24;
		tmp64 = val * 512;
		printf("\tDevice density: %llu B\n", tmp64);
		return 1;

	case EXT_CSD_SLEEP_NOTIFICATION_TIME:
		print_field_caption(SLEEP_NOTIFICATION_TIME, R);
		val = get_field_val(SLEEP_NOTIFICATION_TIME, 0, 0xFF);
		val = 100 << val;
		if (val)
			str = basprintf("Sleep Notification timeout values: %u us",
				       val);
		else
			str = strdup("Not defined");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_S_A_TIMEOUT:
		print_field_caption(S_A_TIMEOUT, R);
		val = get_field_val(S_A_TIMEOUT, 0, 0xFF);
		val = 100 << val;
		if (val)
			str = basprintf("Sleep/awake timeout values: %u ns", val);
		else
			str = strdup("Not defined");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_PRODUCTION_ST8_AWARENSS_TIMEOUT:
		print_field_caption(PRODUCTION_ST8_AWARENSS_TIMEOUT, R);
		val = get_field_val(PRODUCTION_ST8_AWARENSS_TIMEOUT, 0, 0xFF);
		val = 100 << val;
		if (val)
			str = basprintf(
			"Production State Awareness timeout definition: %u us",
				val);
		else
			str = strdup("Not defined");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_S_C_VCCQ:
		print_field_caption(S_C_VCCQ, R);
		val = get_field_val(S_C_VCCQ, 0, 0xF);
		val = 1 << val;
		if (val)
			str = basprintf("S_C_VCCQ Sleep Current: %u uA", val);
		else
			str = strdup("Not defined");
		printf("\t[3-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_S_C_VCC:
		print_field_caption(S_C_VCC, R);
		val = get_field_val(S_C_VCC, 0, 0xFF);
		val = 1 << val;
		if (val)
			str = basprintf("S_C_VCC Sleep Current: %u uA", val);
		else
			str = strdup("Not defined");
		printf("\t[3-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_HC_WP_GRP_SIZE:
		print_field_caption(HC_WP_GRP_SIZE, R);
		val = get_field_val(HC_WP_GRP_SIZE, 0, 0xFF);
		if (val)
			str = basprintf("Write protect group size: %u", val);
		else
			str = strdup("No support");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_REL_WR_SEC_C:
		print_field_caption(REL_WR_SEC_C, R);
		val = get_field_val(REL_WR_SEC_C, 0, 0xFF);
		printf("\t[7-0] Reliable Write Sector Count: %u\n", val);
		return 1;

	case EXT_CSD_ERASE_TIMEOUT_MULT:
		print_field_caption(ERASE_TIMEOUT_MULT, R);
		val = get_field_val(ERASE_TIMEOUT_MULT, 0, 0xFF);
		val = val * 300;
		if (val)
			str = basprintf("Erase timeout values: %u", val);
		else
			str = strdup("No support");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_HC_ERASE_GRP_SIZE:
		print_field_caption(HC_ERASE_GRP_SIZE, R);
		val = get_field_val(HC_ERASE_GRP_SIZE, 0, 0xFF);
		val = val * 524288;
		if (val)
			str = basprintf("Erase-unit size: %u", val);
		else
			str = strdup("No support");
		printf("\t[7-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_ACC_SIZE:
		print_field_caption(ACC_SIZE, R);
		val = get_field_val(ACC_SIZE, 0, 0xF);
		val = val * 512;
		if (val)
			str = basprintf("Superpage size: %u", val);
		else
			str = strdup("Not defined");
		printf("\t[3-0] %s\n", str);
		free(str);
		return 1;

	case EXT_CSD_BOOT_SIZE_MULT:
		print_field_caption(BOOT_SIZE_MULT, R);
		val = get_field_val(BOOT_SIZE_MULT, 0, 0xFF);
		val = val * 131072;
		printf("\tBoot partition size: %u\n", val);
		return 1;

	case EXT_CSD_BOOT_INFO:
		print_field_caption(BOOT_INFO, R);
		val = get_field_val(BOOT_INFO, 0, 0x1);
		printf("\t[0] ALT_BOOT_MODE: %ssupported\n", val ? "" : "not ");
		val = get_field_val(BOOT_INFO, 1, 0x1);
		printf("\t[1] DDR_BOOT_MODE: %ssupported\n", val ? "" : "not ");
		val = get_field_val(BOOT_INFO, 2, 0x1);
		printf("\t[2] HS_BOOT_MODE: %ssupported\n", val ? "" : "not ");
		return 1;

	case EXT_CSD_BKOPS_SUPPORT:
		print_field_caption(BKOPS_SUPPORT, R);
		val = get_field_val(BKOPS_SUPPORT, 0, 0x1);
		printf("\t[0] SUPPORTED: %u\n", val);
		return 1;

	case EXT_CSD_HPI_FEATURES:
		print_field_caption(HPI_FEATURES, R);
		val = get_field_val(HPI_FEATURES, 0, 0x1);
		printf("\t[0] HPI_SUPPORTED: %u\n", val);
		val = get_field_val(HPI_FEATURES, 1, 0x1);
		printf("\t[1] HPI_FEATURES: implementation based on CMD1%s\n",
			val ? "2" : "3");
		return 1;

	case EXT_CSD_S_CMD_SET:
		print_field_caption(S_CMD_SET, R);
		val = get_field_val(S_CMD_SET, 0, 0xFF);
		printf("\t[7-0] Command Set: %#02x\n", val);
		return 1;

	case EXT_CSD_EXT_SECURITY_ERR:
		print_field_caption(EXT_SECURITY_ERR, R);
		val = get_field_val(EXT_SECURITY_ERR, 0, 0x1);
		printf("\t[0] SEC_INVALID_COMMAND_PARAMETERS: %u\n", val);
		val = get_field_val(EXT_SECURITY_ERR, 1, 0x1);
		printf("\t[1] ACCESS_DENIED: %u\n", val);
		return 1;

	}

	return 0;
}

static void print_register_raw(u8 *reg, int index)
{
	if (index == 0)
		memory_display(reg, 0, EXT_CSD_BLOCKSIZE, 1, 0);
	else
		printf("%u: %#02x\n", index, reg[index]);
}

static void print_register_readable(u8 *reg, int index)
{
	int i;

	if (index == 0)
		for (i = 0; i < EXT_CSD_BLOCKSIZE; i++)
			print_field(reg, i);
	else
		if (!print_field(reg, index))
			printf("No field with this index found\n");

	print_access_type_key();
}

static int request_write_operation(void)
{
	int c;

	printf("This is a one time programmable field!\nDo you want to write? [y/N] ");
	c = getchar();
	/* default is N */
	if (c == 0xD) {
		printf("\n");
		return 0;
	}
	printf("%c", c);
	getchar(); /* wait for carriage return */
	printf("\n");
	if (c == 'y' || c == 'Y')
		return 1;
	return 0;
}

static void write_field(struct mci *mci, u8 *reg, u16 index, u8 value,
				int always_write)
{

	switch (index) {
	case EXT_CSD_BOOT_CONFIG_PROT:
	case EXT_CSD_BOOT_WP:
	case EXT_CSD_USER_WP:
	case EXT_CSD_FW_CONFIG:
	case EXT_CSD_WR_REL_SET:
	case EXT_CSD_BKOPS_EN:
	case EXT_CSD_RST_N_FUNCTION:
	case EXT_CSD_PARTITIONS_ATTRIBUTE:
	case EXT_CSD_PARTITION_SETTING_COMPLETED:
	/*   EXT_CSD_GP_SIZE_MULT */
	case 154:
	case 153:
	case 152:
	case 151:
	case 150:
	case 149:
	case 148:
	case 147:
	case 146:
	case 145:
	case 144:
	case 143:
	/*   EXT_CSD_ENH_SIZE_MULT */
	case 142:
	case 141:
	case 140:
	/*   EXT_CSD_ENH_START_ADDR */
	case 139:
	case 138:
	case 137:
	case 136:
	case EXT_CSD_SEC_BAD_BLK_MGMNT:
	case EXT_CSD_USE_NATIVE_SECTOR:
	/*   EXT_CSD_EXT_PARTITIONS_ATTRIBUTE */
	case 53:
	case 52:
	case EXT_CSD_BARRIER_CTRL:
	case EXT_CSD_SECURE_REMOVAL_TYPE:
		if (!always_write)
			if (request_write_operation() == 0) {
				printf("Abort write operation!\n");
				goto out;
			}
		break;
	}

	mci_switch(mci, 0, index, value);

out:
	return;
}

static int do_mmc_extcsd(int argc, char *argv[])
{
	struct mci		*mci;
	struct mci_host		*host;
	u8			*dst;
	int			retval = 0;
	int			opt;
	char			*devname;
	int			index = 0;
	int			value = 0;
	int			write_operation = 0;
	int			always_write = 0;
	int			print_as_raw = 0;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "i:v:yr")) > 0)
		switch (opt) {
		case 'i':
			index = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			value = simple_strtoul(optarg, NULL, 0);
			write_operation = 1;
			break;
		case 'y':
			always_write = 1;
			break;
		case 'r':
			print_as_raw = 1;
			break;
		}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	devname = argv[optind];
	if (!strncmp(devname, "/dev/", 5))
		devname += 5;

	mci = mci_get_device_by_name(devname);
	if (mci == NULL) {
		retval = -ENOENT;
		goto error;
	}
	host = mci->host;
	if (host == NULL) {
		retval = -ENOENT;
		goto error;
	}
	dst = xmalloc(EXT_CSD_BLOCKSIZE);

	retval = mci_send_ext_csd(mci, dst);
	if (retval != 0)
		goto error_with_mem;

	if (dst[EXT_CSD_REV] < 3) {
		printf("MMC version 4.2 or lower are not supported");
		retval = 1;
		goto error_with_mem;
	}

	if (write_operation)
		if (!print_field(dst, index)) {
			printf("No field with this index found. Abort write operation!\n");
		} else {
			write_field(mci, dst, index, value, always_write);
			printf("\nValue written!\n\n");
			retval = mci_send_ext_csd(mci, dst);
			if (retval != 0)
				goto error_with_mem;
			print_field(dst, index);
		}
	else
		if (print_as_raw)
			print_register_raw(dst, index);
		else
			print_register_readable(dst, index);

error_with_mem:
	free(dst);
error:
	return retval;
}

BAREBOX_CMD_HELP_START(mmc_extcsd)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-i", "field index of the register")
BAREBOX_CMD_HELP_OPT("-r", "print the register as raw data")
BAREBOX_CMD_HELP_OPT("-v", "value which will be written")
BAREBOX_CMD_HELP_OPT("-y", "don't request when writing to one time programmable fields")
BAREBOX_CMD_HELP_OPT("",   "__CAUTION__: this could damage the device!")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mmc_extcsd)
	.cmd		= do_mmc_extcsd,
	BAREBOX_CMD_DESC("Read/write the extended CSD register.")
	BAREBOX_CMD_OPTS("dev [-r | -i index [-r | -v value [-y]]]")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_mmc_extcsd_help)
BAREBOX_CMD_END
