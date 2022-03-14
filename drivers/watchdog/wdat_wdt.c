// SPDX-License-Identifier: GPL-2.0-only
/*
 * ACPI Hardware Watchdog (WDAT) driver.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 */
#include <common.h>
#include <acpi.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of.h>
#include <watchdog.h>

enum acpi_wdat_actions {
	ACPI_WDAT_RESET = 1,
	ACPI_WDAT_GET_CURRENT_COUNTDOWN = 4,
	ACPI_WDAT_GET_COUNTDOWN = 5,
	ACPI_WDAT_SET_COUNTDOWN = 6,
	ACPI_WDAT_GET_RUNNING_STATE = 8,
	ACPI_WDAT_SET_RUNNING_STATE = 9,
	ACPI_WDAT_GET_STOPPED_STATE = 10,
	ACPI_WDAT_SET_STOPPED_STATE = 11,
	ACPI_WDAT_GET_REBOOT = 16,
	ACPI_WDAT_SET_REBOOT = 17,
	ACPI_WDAT_GET_SHUTDOWN = 18,
	ACPI_WDAT_SET_SHUTDOWN = 19,
	ACPI_WDAT_GET_STATUS = 32,
	ACPI_WDAT_SET_STATUS = 33,
	ACPI_WDAT_ACTION_RESERVED = 34	/* 34 and greater are reserved */
};

enum acpi_wdat_instructions {
	ACPI_WDAT_READ_VALUE = 0,
	ACPI_WDAT_READ_COUNTDOWN = 1,
	ACPI_WDAT_WRITE_VALUE = 2,
	ACPI_WDAT_WRITE_COUNTDOWN = 3,
	ACPI_WDAT_INSTRUCTION_RESERVED = 4,	/* 4 and greater are reserved */
	ACPI_WDAT_PRESERVE_REGISTER = 0x80	/* Except for this value */
};

#define MAX_WDAT_ACTIONS ACPI_WDAT_ACTION_RESERVED

#define WDAT_DEFAULT_TIMEOUT	30

/* WDAT Instruction Entries (actions) */

struct __packed acpi_wdat_entry {
	u8				action;
	u8				instruction;
	u16				reserved;
	struct acpi_generic_address	register_region;
	u32				value;		/* Value used with Read/Write register */
	u32				mask;		/* Bitmask required for this register instruction */
};

/**
 * struct wdat_instruction - Single ACPI WDAT instruction
 * @entry: Copy of the ACPI table instruction
 * @reg: Register the instruction is accessing
 * @node: Next instruction in action sequence
 */
struct wdat_instruction {
	struct acpi_wdat_entry entry;
	void __iomem *reg;
	struct list_head node;
};

/**
 * struct wdat_wdt - ACPI WDAT watchdog device
 * @dev: Parent platform device
 * @wdd: Watchdog core device
 * @period: How long is one watchdog period in ms
 * @stopped_in_sleep: Is this watchdog stopped by the firmware in S1-S5
 * @stopped: Was the watchdog stopped by the driver in suspend
 * @instructions: An array of instruction lists indexed by an action number from
 *                the WDAT table. There can be %NULL entries for not implemented
 *                actions.
 */
struct wdat_wdt {
	struct watchdog		wdd;
	unsigned int		period;
	bool			stopped_in_sleep;
	bool			stopped;
	struct list_head	*instructions[MAX_WDAT_ACTIONS];
};

struct __packed acpi_table_wdat {
	struct acpi_table_header header;	/* Common ACPI table header */
	u32			 header_length;	/* Watchdog Header Length */
	u16			 pci_segment;	/* PCI Segment number */
	u8			 pci_bus;	/* PCI Bus number */
	u8			 pci_device;	/* PCI Device number */
	u8			 pci_function;	/* PCI Function number */
	u8			 reserved[3];
	u32			 timer_period;	/* Period of one timer count (msec) */
	u32			 max_count;	/* Maximum counter value supported */
	u32			 min_count;	/* Minimum counter value */
	u8			 flags;
	u8			 reserved2[3];
	u32			 nr_entries;	/* Number of watchdog entries that follow */
	struct acpi_wdat_entry   entries[];
};

#define ACPI_WDAT_ENABLED           (1)
#define ACPI_WDAT_STOPPED           0x80

#define IO_COND(instr, is_pio, is_mmio) do {					\
	const struct acpi_generic_address *gas = &instr->entry.register_region; \
	unsigned long port = (unsigned long __force)instr->reg;			\
	if (gas->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {			\
		is_mmio;							\
	} else if (gas->space_id == ACPI_ADR_SPACE_SYSTEM_IO) {			\
		is_pio;								\
	}									\
} while (0)

static unsigned int read8(const struct wdat_instruction *instr)
{
	IO_COND(instr, return inb(port), return readb(instr->reg));
	return 0xff;
}

static unsigned int read16(const struct wdat_instruction *instr)
{
	IO_COND(instr, return inw(port), return readw(instr->reg));
	return 0xffff;
}

static unsigned int read32(const struct wdat_instruction *instr)
{
	IO_COND(instr, return inl(port), return readl(instr->reg));
	return 0xffffffff;
}

static void write8(u8 val, const struct wdat_instruction *instr)
{
	IO_COND(instr, outb(val,port), writeb(val, instr->reg));
}

static void write16(u16 val, const struct wdat_instruction *instr)
{
	IO_COND(instr, outw(val,port), writew(val, instr->reg));
}

static void write32(u32 val, const struct wdat_instruction *instr)
{
	IO_COND(instr, outl(val,port), writel(val, instr->reg));
}

static int wdat_wdt_read(struct wdat_wdt *wdat,
		       const struct wdat_instruction *instr, u32 *value)
{
	const struct acpi_generic_address *gas = &instr->entry.register_region;

	switch (gas->access_width) {
	case 1:
		*value = read8(instr);
		break;
	case 2:
		*value = read16(instr);
		break;
	case 3:
		*value = read32(instr);
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(wdat->wdd.hwdev, "Read %#x from 0x%08llx\n", *value,
		gas->address);

	return 0;
}

static int wdat_wdt_write(struct wdat_wdt *wdat,
			const struct wdat_instruction *instr, u32 value)
{
	const struct acpi_generic_address *gas = &instr->entry.register_region;

	switch (gas->access_width) {
	case 1:
		write8((u8)value, instr);
		break;
	case 2:
		write16((u16)value, instr);
		break;
	case 3:
		write32(value, instr);
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(wdat->wdd.hwdev, "Wrote %#x to 0x%08llx\n", value,
		gas->address);

	return 0;
}

static int wdat_wdt_run_action(struct wdat_wdt *wdat, unsigned int action,
			       u32 param, u32 *retval)
{
	struct wdat_instruction *instr;

	if (action >= ARRAY_SIZE(wdat->instructions)) {
		dev_dbg(wdat->wdd.hwdev, "Invalid action %#x\n", action);
		return -EINVAL;
	}

	if (!wdat->instructions[action]) {
		dev_dbg(wdat->wdd.hwdev, "Unsupported action %#x\n", action);
		return -EOPNOTSUPP;
	}

	dev_dbg(wdat->wdd.hwdev, "Running action %#x\n", action);

	/* Run each instruction sequentially */
	list_for_each_entry(instr, wdat->instructions[action], node) {
		const struct acpi_wdat_entry *entry = &instr->entry;
		const struct acpi_generic_address *gas;
		u32 flags, value, mask, x, y;
		bool preserve;
		int ret;

		gas = &entry->register_region;

		preserve = entry->instruction & ACPI_WDAT_PRESERVE_REGISTER;
		flags = entry->instruction & ~ACPI_WDAT_PRESERVE_REGISTER;
		value = entry->value;
		mask = entry->mask;

		switch (flags) {
		case ACPI_WDAT_READ_VALUE:
			ret = wdat_wdt_read(wdat, instr, &x);
			if (ret)
				return ret;
			x >>= gas->bit_offset;
			x &= mask;
			if (retval)
				*retval = x == value;
			break;

		case ACPI_WDAT_READ_COUNTDOWN:
			ret = wdat_wdt_read(wdat, instr, &x);
			if (ret)
				return ret;
			x >>= gas->bit_offset;
			x &= mask;
			if (retval)
				*retval = x;
			break;

		case ACPI_WDAT_WRITE_VALUE:
			x = value & mask;
			x <<= gas->bit_offset;
			if (preserve) {
				ret = wdat_wdt_read(wdat, instr, &y);
				if (ret)
					return ret;
				y = y & ~(mask << gas->bit_offset);
				x |= y;
			}
			ret = wdat_wdt_write(wdat, instr, x);
			if (ret)
				return ret;
			break;

		case ACPI_WDAT_WRITE_COUNTDOWN:
			x = param;
			x &= mask;
			x <<= gas->bit_offset;
			if (preserve) {
				ret = wdat_wdt_read(wdat, instr, &y);
				if (ret)
					return ret;
				y = y & ~(mask << gas->bit_offset);
				x |= y;
			}
			ret = wdat_wdt_write(wdat, instr, x);
			if (ret)
				return ret;
			break;

		default:
			dev_err(wdat->wdd.hwdev, "Unknown instruction: %u\n",
				flags);
			return -EINVAL;
		}
	}

	return 0;
}

static void wdat_wdt_boot_status(struct wdat_wdt *wdat)
{
	u32 boot_status = 0;
	int ret;

	ret = wdat_wdt_run_action(wdat, ACPI_WDAT_GET_STATUS, 0, &boot_status);
	if (ret && ret != -EOPNOTSUPP) {
		dev_err(wdat->wdd.hwdev, "Failed to read boot status\n");
		return;
	}

	/* Clear the boot status in case BIOS did not do it */
	ret = wdat_wdt_run_action(wdat, ACPI_WDAT_SET_STATUS, 0, NULL);
	if (ret && ret != -EOPNOTSUPP)
		dev_err(wdat->wdd.hwdev, "Failed to clear boot status\n");
}

static int wdat_wdt_start(struct watchdog *wdd)
{
	struct wdat_wdt *wdat = container_of(wdd, struct wdat_wdt, wdd);

	return wdat_wdt_run_action(wdat, ACPI_WDAT_SET_RUNNING_STATE, 0, NULL);
}

static int wdat_wdt_stop(struct watchdog *wdd)
{
	struct wdat_wdt *wdat = container_of(wdd, struct wdat_wdt, wdd);

	return wdat_wdt_run_action(wdat, ACPI_WDAT_SET_STOPPED_STATE, 0, NULL);
}

static void wdat_wdt_set_running(struct wdat_wdt *wdat)
{
	u32 running = 0;
	int ret;

	ret = wdat_wdt_run_action(wdat, ACPI_WDAT_GET_RUNNING_STATE, 0,
				  &running);
	if (ret && ret != -EOPNOTSUPP)
		dev_err(wdat->wdd.hwdev, "Failed to read running state\n");

	dev_dbg(wdat->wdd.hwdev, "Running state: %d\n", running);

	wdat->wdd.running = running ? WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;
}

static int wdat_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct wdat_wdt *wdat = container_of(wdd, struct wdat_wdt, wdd);
	unsigned int periods;
	int ret;

	if (timeout == 0)
		return wdat_wdt_stop(wdd);

	periods = timeout * 1000 / wdat->period;
	ret = wdat_wdt_run_action(wdat, ACPI_WDAT_SET_COUNTDOWN, periods, NULL);
	if (ret)
		return ret;

	if (wdat->wdd.running == WDOG_HW_NOT_RUNNING) {
		wdat_wdt_start(wdd);
		wdat->wdd.running = WDOG_HW_RUNNING;
	}

	return wdat_wdt_run_action(wdat, ACPI_WDAT_RESET, 0, NULL);
}

static int wdat_wdt_enable_reboot(struct wdat_wdt *wdat)
{
	int ret;

	/*
	 * WDAT specification says that the watchdog is required to reboot
	 * the system when it fires. However, it also states that it is
	 * recommeded to make it configurable through hardware register. We
	 * enable reboot now if it is configrable, just in case.
	 */
	ret = wdat_wdt_run_action(wdat, ACPI_WDAT_SET_REBOOT, 0, NULL);
	if (ret && ret != -EOPNOTSUPP) {
		dev_err(wdat->wdd.hwdev,
			"Failed to enable reboot when watchdog triggers\n");
		return ret;
	}

	return 0;
}

static int wdat_wdt_probe(struct device_d *const dev)
{
	const struct acpi_wdat_entry *entries;
	struct acpi_table_wdat *tbl;
	struct wdat_wdt *wdat;
	int i, ret;

	dev_dbg(dev, "driver initializing...\n");

	tbl = (struct acpi_table_wdat __force *)dev_request_mem_region_by_name(dev, "SDT");
	if (IS_ERR(tbl)) {
		dev_err(dev, "no SDT resource available: %pe\n", tbl);
		return PTR_ERR(tbl);
	}

	dev_dbg(dev, "SDT is at 0x%p\n", tbl);

	wdat = xzalloc(sizeof(*wdat));

	/* WDAT specification wants to have >= 1ms period */
	if (tbl->timer_period < 1) {
		dev_dbg(dev, "timer_period is less than 1: %d\n", tbl->timer_period);
		return -EINVAL;
	}
	if (tbl->min_count > tbl->max_count) {
		dev_dbg(dev, "min_count must be greater than max_count\n");
		return -EINVAL;
	}

	wdat->period = tbl->timer_period;
	wdat->stopped_in_sleep = tbl->flags & ACPI_WDAT_STOPPED;
	wdat->wdd.set_timeout = wdat_wdt_set_timeout;
	wdat->wdd.hwdev = dev;
	wdat->wdd.timeout_max = U32_MAX;

	entries = tbl->entries;

	for (i = 0; i < tbl->nr_entries; i++) {
		const struct acpi_generic_address *gas;
		struct wdat_instruction *instr;
		struct list_head *instructions;
		struct resource *res;
		unsigned int action;
		struct resource r;

		action = entries[i].action;
		if (action >= MAX_WDAT_ACTIONS) {
			dev_dbg(dev, "Skipping unknown action: %u\n", action);
			continue;
		}

		instr = xzalloc(sizeof(*instr));

		INIT_LIST_HEAD(&instr->node);
		instr->entry = entries[i];

		gas = &entries[i].register_region;

		memset(&r, 0, sizeof(r));
		r.start = gas->address;
		r.end = r.start + ACPI_ACCESS_BYTE_WIDTH(gas->access_width) - 1;
		if (gas->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
			res = request_iomem_region(dev_name(dev), r.start, r.end);
		} else if (gas->space_id == ACPI_ADR_SPACE_SYSTEM_IO) {
			res = request_ioport_region(dev_name(dev), r.start, r.end);
		} else {
			dev_dbg(dev, "Unsupported address space: %d\n",
				gas->space_id);
			continue;
		}

		/*
		 * Some entries have the same gas->address.
		 * We want the action but can't request the region multiple times.
		 */
		if (IS_ERR(res) && (PTR_ERR(res) != -EBUSY))
			return PTR_ERR(res);

		instr->reg = IOMEM(r.start);

		instructions = wdat->instructions[action];
		if (!instructions) {
			instructions = xzalloc(sizeof(*instructions));
			if (!instructions)
				return -ENOMEM;

			INIT_LIST_HEAD(instructions);
			wdat->instructions[action] = instructions;
		}

		list_add_tail(&instr->node, instructions);
	}

	wdat_wdt_boot_status(wdat);
	wdat_wdt_set_running(wdat);

	ret = wdat_wdt_enable_reboot(wdat);
	if (ret)
		return ret;

	return watchdog_register(&wdat->wdd);
}


static struct acpi_driver wdat_wdt_driver = {
	.signature = "WDAT",
	.driver = {
		.name = "wdat-wdt",
		.probe = wdat_wdt_probe,
	}
};
device_acpi_driver(wdat_wdt_driver);
