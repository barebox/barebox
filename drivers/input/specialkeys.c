// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2019 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <restart.h>
#include <poweroff.h>
#include <init.h>
#include <input/input.h>
#include <dt-bindings/input/linux-event-codes.h>

static void input_specialkeys_notify(struct input_notifier *in,
				     struct input_event *ev)
{
	switch (ev->code) {
	case KEY_RESTART:
		pr_info("Triggering reset due to special key.\n");
		restart_machine(0);
		break;

	case KEY_POWER:
		pr_info("Triggering poweroff due to special key.\n");
		poweroff_machine(0);
		break;
	}

	pr_debug("ignoring code: %d\n", ev->code);
}

static struct input_notifier notifier;

static int input_specialkeys_init(void)
{
	notifier.notify = input_specialkeys_notify;
	return input_register_notfier(&notifier);
}
late_initcall(input_specialkeys_init);
