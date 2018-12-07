/*
 * Copyright (C) 2014, 2015 Marc Kleine-Budde <mkl@pengutronix.de>
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)  "HABv4: " fmt

#include <common.h>
#include <hab.h>
#include <init.h>
#include <types.h>

#include <mach/generic.h>

#define HABV4_RVT_IMX28 0xffff8af8
#define HABV4_RVT_IMX6_OLD 0x00000094
#define HABV4_RVT_IMX6_NEW 0x00000098
#define HABV4_RVT_IMX6UL 0x00000100

struct __packed hab_hdr {
	uint8_t tag;			/* Tag field */
	__be16 len;			/* Length field in bytes (big-endian) */
	uint8_t par;			/* Parameters field */
};

struct __packed hab_event_record {
	struct hab_hdr hdr;
	uint8_t status;			/* Status -> enum hab_status*/
	uint8_t reason;			/* Reason -> enum hab_reason */
	uint8_t context;		/* Context -> enum hab_context */
	uint8_t engine;			/* Engine -> enum hab_engine */
	uint8_t data[0];		/* Record Data */
};

enum hab_tag {
	HAB_TAG_IVT = 0xd1,		/* Image Vector Table */
	HAB_TAG_DCD = 0xd2,		/* Device Configuration Data */
	HAB_TAG_CSF = 0xd4,		/* Command Sequence File */
	HAB_TAG_CRT = 0xd7,		/* Certificate */
	HAB_TAG_SIG = 0xd8,		/* Signature */
	HAB_TAG_EVT = 0xdb,		/* Event */
	HAB_TAG_RVT = 0xdd,		/* ROM Vector Table */
	HAB_TAG_WRP = 0x81,		/* Wrapped Key */
	HAB_TAG_MAC = 0xac,		/* Message Authentication Code */
};

/* Status definitions */
enum hab_status {
	HAB_STATUS_ANY = 0x00,		/* Match any status in report_event */
	HAB_STATUS_FAILURE = 0x33,	/* Operation failed */
	HAB_STATUS_WARNING = 0x69,	/* Operation completed with warning */
	HAB_STATUS_SUCCESS = 0xf0,	/* Operation completed successfully */
};

/* Security Configuration definitions */
enum hab_config {
	HAB_CONFIG_FAB = 0x00,		/* Un-programmed IC */
	HAB_CONFIG_RETURN = 0x33,	/* Field Return IC */
	HAB_CONFIG_OPEN = 0xf0,		/* Non-secure IC */
	HAB_CONFIG_CLOSED = 0xcc,	/* Secure IC */
};

/* State definitions */
enum hab_state {
	HAB_STATE_INITIAL = 0x33,	/* Initialising state (transitory) */
	HAB_STATE_CHECK = 0x55,		/* Check state (non-secure) */
	HAB_STATE_NONSECURE = 0x66,	/* Non-secure state */
	HAB_STATE_TRUSTED = 0x99,	/* Trusted state */
	HAB_STATE_SECURE = 0xaa,	/* Secure state */
	HAB_STATE_FAIL_SOFT = 0xcc,	/* Soft fail state */
	HAB_STATE_FAIL_HARD = 0xff,	/* Hard fail state (terminal) */
	HAB_STATE_NONE = 0xf0,		/* No security state machine */
};

enum hab_reason {
	HAB_REASON_RSN_ANY = 0x00,		/* Match any reason */
	HAB_REASON_UNS_COMMAND = 0x03,		/* Unsupported command */
	HAB_REASON_INV_IVT = 0x05,		/* Invalid ivt */
	HAB_REASON_INV_COMMAND = 0x06,		/* Invalid command: command malformed */
	HAB_REASON_UNS_STATE = 0x09,		/* Unsuitable state */
	HAB_REASON_UNS_ENGINE = 0x0a,		/* Unsupported engine */
	HAB_REASON_INV_ASSERTION = 0x0c,	/* Invalid assertion */
	HAB_REASON_INV_INDEX = 0x0f,		/* Invalid index: access denied */
	HAB_REASON_INV_CSF = 0x11,		/* Invalid csf */
	HAB_REASON_UNS_ALGORITHM = 0x12,	/* Unsupported algorithm */
	HAB_REASON_UNS_PROTOCOL = 0x14,		/* Unsupported protocol */
	HAB_REASON_INV_SIZE = 0x17,		/* Invalid data size */
	HAB_REASON_INV_SIGNATURE = 0x18,	/* Invalid signature */
	HAB_REASON_UNS_KEY = 0x1b,		/* Unsupported key type/parameters */
	HAB_REASON_INV_KEY = 0x1d,		/* Invalid key */
	HAB_REASON_INV_RETURN = 0x1e,		/* Failed callback function */
	HAB_REASON_INV_CERTIFICATE = 0x21,	/* Invalid certificate */
	HAB_REASON_INV_ADDRESS = 0x22,		/* Invalid address: access denied */
	HAB_REASON_UNS_ITEM = 0x24,		/* Unsupported configuration item */
	HAB_REASON_INV_DCD = 0x27,		/* Invalid dcd */
	HAB_REASON_INV_CALL = 0x28,		/* Function called out of sequence */
	HAB_REASON_OVR_COUNT = 0x2b,		/* Expired poll count */
	HAB_REASON_OVR_STORAGE = 0x2d,		/* Exhausted storage region */
	HAB_REASON_MEM_FAIL = 0x2e,		/* Memory failure */
	HAB_REASON_ENG_FAIL = 0x30,		/* Engine failure */
};

enum hab_context {
	HAB_CONTEXT_ANY = 0x00,			/* Match any context */
	HAB_CONTEXT_AUTHENTICATE = 0x0a,	/* Logged in hab_rvt.authenticate_image() */
	HAB_CONTEXT_TARGET = 0x33,		/* Event logged in hab_rvt.check_target() */
	HAB_CONTEXT_ASSERT = 0xa0,		/* Event logged in hab_rvt.assert() */
	HAB_CONTEXT_COMMAND = 0xc0,		/* Event logged executing csf/dcd command */
	HAB_CONTEXT_CSF = 0xcf,			/* Event logged in hab_rvt.run_csf() */
	HAB_CONTEXT_AUT_DAT = 0xdb,		/* Authenticated data block */
	HAB_CONTEXT_DCD = 0xdd,			/* Event logged in hab_rvt.run_dcd() */
	HAB_CONTEXT_ENTRY = 0xe1,		/* Event logged in hab_rvt.entry() */
	HAB_CONTEXT_EXIT = 0xee,		/* Event logged in hab_rvt.exit() */
	HAB_CONTEXT_FAB = 0xff,			/* Event logged in hab_fab_test() */
};

enum hab_engine {
	HAB_ENGINE_ANY = 0x00,		/* Select first compatible engine */
	HAB_ENGINE_SCC = 0x03,		/* Security controller */
	HAB_ENGINE_RTIC = 0x05,		/* Run-time integrity checker */
	HAB_ENGINE_SAHARA = 0x06,	/* Crypto accelerator */
	HAB_ENGINE_CSU = 0x0a,		/* Central Security Unit */
	HAB_ENGINE_SRTC = 0x0c,		/* Secure clock */
	HAB_ENGINE_DCP = 0x1b,		/* Data Co-Processor */
	HAB_ENGINE_CAAM = 0x1d,		/* CAAM */
	HAB_ENGINE_SNVS = 0x1e,		/* Secure Non-Volatile Storage */
	HAB_ENGINE_OCOTP = 0x21,	/* Fuse controller */
	HAB_ENGINE_DTCP = 0x22,		/* DTCP co-processor */
	HAB_ENGINE_HDCP = 0x24,		/* HDCP co-processor */
	HAB_ENGINE_ROM = 0x36,		/* Protected ROM area */
	HAB_ENGINE_RTL = 0x77,		/* RTL simulation engine */
	HAB_ENGINE_SW = 0xff,		/* Software engine */
};

enum hab_target {
	HAB_TARGET_MEMORY = 0x0f,	/* Check memory white list */
	HAB_TARGET_PERIPHERAL = 0xf0,	/* Check peripheral white list*/
	HAB_TARGET_ANY = 0x55,		/* Check memory & peripheral white list */
};

enum hab_assertion {
	HAB_ASSERTION_BLOCK = 0x0,	/* Check if memory is authenticated after CSF */
};

struct hab_header {
	uint8_t tag;
	uint16_t len;			/* len including the header */
	uint8_t par;
} __packed;

typedef enum hab_status hab_loader_callback_fn(void **start, uint32_t *bytes, const void *boot_data);

struct habv4_rvt {
	struct hab_header header;
	enum hab_status (*entry)(void);
	enum hab_status (*exit)(void);
	enum hab_status (*check_target)(enum hab_target target, const void *start, uint32_t bytes);
	void *(*authenticate_image)(uint8_t cid, uint32_t ivt_offset, void **start, uint32_t *bytes, hab_loader_callback_fn *loader);
	enum hab_status (*run_dcd)(const void *dcd);
	enum hab_status (*run_csf)(const void *csf, uint8_t cid);
	enum hab_status (*assert)(enum hab_assertion assertion, const void *data, uint32_t count);
	enum hab_status (*report_event)(enum hab_status status, uint32_t index, void *event, uint32_t *bytes);
	enum hab_status (*report_status)(enum hab_config *config, enum hab_state *state);
	void (*failsafe)(void);
} __packed;

static const char *habv4_get_status_str(enum hab_status status)
{
	switch (status) {
	case HAB_STATUS_ANY:
		return "Match any status in report_event"; break;
	case HAB_STATUS_FAILURE:
		return "Operation failed"; break;
	case HAB_STATUS_WARNING:
		return "Operation completed with warning"; break;
	case HAB_STATUS_SUCCESS:
		return "Operation completed successfully"; break;
	}

	return "<unknown>";
}

static const char *habv4_get_config_str(enum hab_config config)
{
	switch (config) {
	case HAB_CONFIG_FAB:
		return "Un-programmed IC"; break;
	case HAB_CONFIG_RETURN:
		return "Field Return IC"; break;
	case HAB_CONFIG_OPEN:
		return "Non-secure IC"; break;
	case HAB_CONFIG_CLOSED:
		return "Secure IC"; break;
	}

	return "<unknown>";
}

static const char *habv4_get_state_str(enum hab_state state)
{
	switch (state) {
	case HAB_STATE_INITIAL:
		return "Initialising state (transitory)"; break;
	case HAB_STATE_CHECK:
		return "Check state (non-secure)"; break;
	case HAB_STATE_NONSECURE:
		return "Non-secure state"; break;
	case HAB_STATE_TRUSTED:
		return "Trusted state"; break;
	case HAB_STATE_SECURE:
		return "Secure state"; break;
	case HAB_STATE_FAIL_SOFT:
		return "Soft fail state"; break;
	case HAB_STATE_FAIL_HARD:
		return "Hard fail state (terminal)"; break;
	case HAB_STATE_NONE:
		return "No security state machine"; break;
	}

	return "<unknown>";
}

static const char *habv4_get_reason_str(enum hab_reason reason)
{
	switch (reason) {
	case HAB_REASON_RSN_ANY:
		return "Match any reason"; break;
	case HAB_REASON_UNS_COMMAND:
		return "Unsupported command"; break;
	case HAB_REASON_INV_IVT:
		return "Invalid ivt"; break;
	case HAB_REASON_INV_COMMAND:
		return "Invalid command: command malformed"; break;
	case HAB_REASON_UNS_STATE:
		return "Unsuitable state"; break;
	case HAB_REASON_UNS_ENGINE:
		return "Unsupported engine"; break;
	case HAB_REASON_INV_ASSERTION:
		return "Invalid assertion"; break;
	case HAB_REASON_INV_INDEX:
		return "Invalid index: access denied"; break;
	case HAB_REASON_INV_CSF:
		return "Invalid csf"; break;
	case HAB_REASON_UNS_ALGORITHM:
		return "Unsupported algorithm"; break;
	case HAB_REASON_UNS_PROTOCOL:
		return "Unsupported protocol"; break;
	case HAB_REASON_INV_SIZE:
		return "Invalid data size"; break;
	case HAB_REASON_INV_SIGNATURE:
		return "Invalid signature"; break;
	case HAB_REASON_UNS_KEY:
		return "Unsupported key type/parameters"; break;
	case HAB_REASON_INV_KEY:
		return "Invalid key"; break;
	case HAB_REASON_INV_RETURN:
		return "Failed callback function"; break;
	case HAB_REASON_INV_CERTIFICATE:
		return "Invalid certificate"; break;
	case HAB_REASON_INV_ADDRESS:
		return "Invalid address: access denied"; break;
	case HAB_REASON_UNS_ITEM:
		return "Unsupported configuration item"; break;
	case HAB_REASON_INV_DCD:
		return "Invalid dcd"; break;
	case HAB_REASON_INV_CALL:
		return "Function called out of sequence"; break;
	case HAB_REASON_OVR_COUNT:
		return "Expired poll count"; break;
	case HAB_REASON_OVR_STORAGE:
		return "Exhausted storage region"; break;
	case HAB_REASON_MEM_FAIL:
		return "Memory failure"; break;
	case HAB_REASON_ENG_FAIL:
		return "Engine failure"; break;
	}

	return "<unknown>";
}

static const char *habv4_get_context_str(enum hab_context context)
{
	switch (context){
	case HAB_CONTEXT_ANY:
		return "Match any context"; break;
	case HAB_CONTEXT_AUTHENTICATE:
		return "Logged in hab_rvt.authenticate_image()"; break;
	case HAB_CONTEXT_TARGET:
		return "Event logged in hab_rvt.check_target()"; break;
	case HAB_CONTEXT_ASSERT:
		return "Event logged in hab_rvt.assert()"; break;
	case HAB_CONTEXT_COMMAND:
		return "Event logged executing csf/dcd command"; break;
	case HAB_CONTEXT_CSF:
		return "Event logged in hab_rvt.run_csf()"; break;
	case HAB_CONTEXT_AUT_DAT:
		return "Authenticated data block"; break;
	case HAB_CONTEXT_DCD:
		return "Event logged in hab_rvt.run_dcd()"; break;
	case HAB_CONTEXT_ENTRY:
		return "Event logged in hab_rvt.entry()"; break;
	case HAB_CONTEXT_EXIT:
		return "Event logged in hab_rvt.exit()"; break;
	case HAB_CONTEXT_FAB:
		return "Event logged in hab_fab_test()"; break;
	}

	return "<unknown>";
}

static const char *habv4_get_engine_str(enum hab_engine engine)
{
	switch (engine){
	case HAB_ENGINE_ANY:
		return "Select first compatible engine"; break;
	case HAB_ENGINE_SCC:
		return "Security controller"; break;
	case HAB_ENGINE_RTIC:
		return "Run-time integrity checker"; break;
	case HAB_ENGINE_SAHARA:
		return "Crypto accelerator"; break;
	case HAB_ENGINE_CSU:
		return "Central Security Unit"; break;
	case HAB_ENGINE_SRTC:
		return "Secure clock"; break;
	case HAB_ENGINE_DCP:
		return "Data Co-Processor"; break;
	case HAB_ENGINE_CAAM:
		return "CAAM"; break;
	case HAB_ENGINE_SNVS:
		return "Secure Non-Volatile Storage"; break;
	case HAB_ENGINE_OCOTP:
		return "Fuse controller"; break;
	case HAB_ENGINE_DTCP:
		return "DTCP co-processor"; break;
	case HAB_ENGINE_HDCP:
		return "HDCP co-processor"; break;
	case HAB_ENGINE_ROM:
		return "Protected ROM area"; break;
	case HAB_ENGINE_RTL:
		return "RTL simulation engine"; break;
	case HAB_ENGINE_SW:
		return "Software engine"; break;
	}

	return "<unknown>";
}

static void habv4_display_event_record(struct hab_event_record *record)
{
	pr_err("Status: %s (0x%02x)\n", habv4_get_status_str(record->status), record->status);
	pr_err("Reason: %s (0x%02x)\n", habv4_get_reason_str(record->reason), record->reason);
	pr_err("Context: %s (0x%02x)\n", habv4_get_context_str(record->context), record->context);
	pr_err("Engine: %s (0x%02x)\n", habv4_get_engine_str(record->engine), record->engine);
}

static void habv4_display_event(uint8_t *data, uint32_t len)
{
	unsigned int i;

	if (data && len) {
		for (i = 0; i < len; i++) {
			if ((i % 8) == 0)
				pr_err(" %02x", data[i]);
			else if ((i % 4) == 0)
				pr_cont("  %02x", data[i]);
			else if ((i % 8) == 7)
				pr_cont(" %02x\n", data[i]);
			else
				pr_cont(" %02x", data[i]);
		}
		pr_cont("\n");
	}

	habv4_display_event_record((struct hab_event_record *)data);
}

/* Some chips with HAB >= 4.2.3 have an incorrect implementation of the RNG
 * self-test in ROM code. In this case, an HAB event is generated, and a
 * software self-test should be run. This variable is set to @c true by
 * habv4_get_status() when this occurs. */
bool habv4_need_rng_software_self_test = false;
EXPORT_SYMBOL(habv4_need_rng_software_self_test);

#define RNG_FAIL_EVENT_SIZE 36
static uint8_t habv4_known_rng_fail_events[][RNG_FAIL_EVENT_SIZE] = {
	{ 0xdb, 0x00, 0x24, 0x42,  0x69, 0x30, 0xe1, 0x1d,
	  0x00, 0x80, 0x00, 0x02,  0x40, 0x00, 0x36, 0x06,
	  0x55, 0x55, 0x00, 0x03,  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x01 },
	{ 0xdb, 0x00, 0x24, 0x42,  0x69, 0x30, 0xe1, 0x1d,
	  0x00, 0x04, 0x00, 0x02,  0x40, 0x00, 0x36, 0x06,
	  0x55, 0x55, 0x00, 0x03,  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x01 },
};

static bool is_known_rng_fail_event(const uint8_t *data, size_t len)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(habv4_known_rng_fail_events); i++) {
		if (memcmp(data, habv4_known_rng_fail_events[i],
			   min(len, (uint32_t)RNG_FAIL_EVENT_SIZE)) == 0) {
			return true;
		}
	}
	return false;
}

static int habv4_get_status(const struct habv4_rvt *rvt)
{
	uint8_t data[256];
	uint32_t len;
	uint32_t index = 0;
	enum hab_status status;
	enum hab_config config = 0x0;
	enum hab_state state = 0x0;

	if (rvt->header.tag != HAB_TAG_RVT) {
		pr_err("ERROR - RVT not found!\n");
		return -EINVAL;
	}

	status = rvt->report_status(&config, &state);
	pr_info("Status: %s (0x%02x)\n", habv4_get_status_str(status), status);
	pr_info("Config: %s (0x%02x)\n", habv4_get_config_str(config), config);
	pr_info("State: %s (0x%02x)\n",	habv4_get_state_str(state), state);

	if (status == HAB_STATUS_SUCCESS) {
		pr_info("No HAB Failure Events Found!\n\n");
		return 0;
	}

	len = sizeof(data);
	while (rvt->report_event(HAB_STATUS_WARNING, index, data, &len) == HAB_STATUS_SUCCESS) {

		/* suppress RNG self-test fail events if they can be handled in software */
		if (IS_ENABLED(CONFIG_CRYPTO_DEV_FSL_CAAM_RNG_SELF_TEST) &&
		    is_known_rng_fail_event(data, len)) {
			pr_debug("RNG self-test failure detected, will run software self-test\n");
			habv4_need_rng_software_self_test = true;
		} else {
			pr_err("-------- HAB warning Event %d --------\n", index);
			pr_err("event data:\n");
			habv4_display_event(data, len);
		}

		len = sizeof(data);
		index++;
	}

	len = sizeof(data);
	while (rvt->report_event(HAB_STATUS_FAILURE, index, data, &len) == HAB_STATUS_SUCCESS) {
		pr_err("-------- HAB failure Event %d --------\n", index);
		pr_err("event data:\n");

		habv4_display_event(data, len);
		len = sizeof(data);
		index++;
	}

	/* Check reason for stopping */
	len = sizeof(data);
	if (rvt->report_event(HAB_STATUS_ANY, index, NULL, &len) == HAB_STATUS_SUCCESS)
		pr_err("ERROR: Recompile with larger event data buffer (at least %d bytes)\n\n", len);

	return -EPERM;
}

int imx6_hab_get_status(void)
{
	const struct habv4_rvt *rvt;

	rvt = (void *)HABV4_RVT_IMX6_OLD;
	if (rvt->header.tag == HAB_TAG_RVT)
		return habv4_get_status(rvt);

	rvt = (void *)HABV4_RVT_IMX6_NEW;
	if (rvt->header.tag == HAB_TAG_RVT)
		return habv4_get_status(rvt);

	rvt = (void *)HABV4_RVT_IMX6UL;
	if (rvt->header.tag == HAB_TAG_RVT)
		return habv4_get_status(rvt);

	pr_err("ERROR - RVT not found!\n");

	return -EINVAL;
}

static int init_imx6_hab_get_status(void)
{
	int ret = 0;

	if (!cpu_is_mx6())
		/* can happen in multi-image builds and is not an error */
		return 0;

	ret = imx6_hab_get_status();

	/*
	 * Nobody will check the return value if there were HAB errors, but the
	 * initcall will fail spectaculously with a strange error message.
	 */
	if (ret == -EPERM)
		return 0;
	return ret;
}

/*
 * Need to run before MMU setup because i.MX6 ROM code is mapped near 0x0,
 * which will no longer be accessible when the MMU sets the zero page to
 * faulting.
 */
postconsole_initcall(init_imx6_hab_get_status);

int imx28_hab_get_status(void)
{
	const struct habv4_rvt *rvt = (void *)HABV4_RVT_IMX28;

	return habv4_get_status(rvt);
}

static int init_imx28_hab_get_status(void)
{
	int ret = 0;

	if (!cpu_is_mx28())
		/* can happen in multi-image builds and is not an error */
		return 0;

	ret = imx28_hab_get_status();

	/* nobody will check the return value if there were HAB errors, but the
	 * initcall will fail spectaculously with a strange error message. */
	if (ret == -EPERM)
		return 0;
	return ret;
}
/* i.MX28 ROM code can be run after MMU setup to make use of caching */
postmmu_initcall(init_imx28_hab_get_status);
