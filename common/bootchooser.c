/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 * Copyright (C) 2015 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt)	"bootchooser: " fmt

#include <bootchooser.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <command.h>
#include <libfile.h>
#include <common.h>
#include <malloc.h>
#include <printk.h>
#include <xfuncs.h>
#include <envfs.h>
#include <errno.h>
#include <fcntl.h>
#include <ioctl.h>
#include <libbb.h>
#include <state.h>
#include <stdio.h>
#include <init.h>
#include <crc.h>
#include <net.h>
#include <fs.h>
#include <reset_source.h>

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/err.h>

#define BOOTCHOOSER_PREFIX "global.bootchooser"

static char *available_targets;
static char *state_prefix;
static int global_default_attempts = 3;
static int global_default_priority = 1;
static int disable_on_zero_attempts;
static int retry;
static int last_boot_successful;

struct bootchooser {
	struct bootentry entry;
	struct list_head targets;
	struct bootchooser_target *last_chosen;

	struct state *state;
	char *state_prefix;

	int verbose;
	int dryrun;
};

struct bootchooser_target {
	struct bootchooser *bootchooser;
	struct list_head list;

	/* state */
	unsigned int priority;
	unsigned int remaining_attempts;
	int id;

	/* spec */
	char *name;
	unsigned int default_attempts;
	unsigned int default_priority;

	char *boot;

	char *prefix;
	char *state_prefix;
};

enum reset_attempts {
	RESET_ATTEMPTS_POWER_ON,
	RESET_ATTEMPTS_ALL_ZERO,
};

static unsigned long reset_attempts;

enum reset_priorities {
	RESET_PRIORITIES_ALL_ZERO,
};

static unsigned long reset_priorities;

static int bootchooser_target_ok(struct bootchooser_target *target, const char **reason)
{
	if (!target->priority) {
		if (reason)
			*reason = "Target disabled (priority = 0)";
		return false;
	}

	if (!target->remaining_attempts) {
		if (reason)
			*reason = "remaining attempts = 0";
		return false;
	}

	if (reason)
		*reason = "target OK";

	return true;
}

static void pr_target(struct bootchooser_target *target)
{
	const char *reason;
	int ok;

	ok = bootchooser_target_ok(target, &reason);

	printf("%s\n"
	       "    id: %u\n"
	       "    priority: %u\n"
	       "    default_priority: %u\n"
	       "    remaining attempts: %u\n"
	       "    default attempts: %u\n"
	       "    boot: '%s'\n",
	       target->name, target->id, target->priority, target->default_priority,
	       target->remaining_attempts, target->default_attempts,
	       target->boot);
	if (!ok)
		printf("    disabled due to %s\n", reason);
}

static int pr_setenv(struct bootchooser *bc, const char *fmt, ...)
{
	va_list ap;
	int ret = 0;
	char *str, *val;
	const char *oldval;

	va_start(ap, fmt);
	str = bvasprintf(fmt, ap);
	va_end(ap);

	if (!str)
		return -ENOMEM;

	val = strchr(str, '=');
	if (!val) {
		ret = -EINVAL;
		goto err;
	}

	*val++ = '\0';

	oldval = getenv(str);
	if (!oldval || strcmp(oldval, val)) {
		if (bc->state)
			ret = setenv(str, val);
		else
			ret = nvvar_add(str, val);
	}

err:
	free(str);

	return ret;
}

static const char *pr_getenv(const char *fmt, ...)
{
	va_list ap;
	char *str;
	const char *val;

	va_start(ap, fmt);
	str = bvasprintf(fmt, ap);
	va_end(ap);

	if (!str)
		return NULL;

	val = getenv(str);

	free(str);

	return val;
}

static int getenv_u32(const char *prefix, const char *name, uint32_t *retval)
{
	char *str;
	const char *val;

	str = xasprintf("%s.%s", prefix, name);

	val = getenv(str);

	free(str);

	if (!val)
		return -ENOENT;

	*retval = simple_strtoul(val, NULL, 0);

	return 0;
}

static int bootchooser_target_compare(struct list_head *a, struct list_head *b)
{
	struct bootchooser_target *bootchooser_a =
		list_entry(a, struct bootchooser_target, list);
	struct bootchooser_target *bootchooser_b =
		list_entry(b, struct bootchooser_target, list);

	/* order with descending priority */
	return bootchooser_a->priority >= bootchooser_b->priority ? -1 : 1;
}

/**
 * bootchooser_target_new - Create a new bootchooser target
 * @bc: The bootchooser
 * @name: The name of the new target
 *
 * Parses the variables associated with @name, creates a bootchooser
 * target from it and returns it.
 */
static struct bootchooser_target *bootchooser_target_new(struct bootchooser *bc,
							 const char *name)
{
	struct bootchooser_target *target = xzalloc(sizeof(*target));
	const char *val;
	int ret;

	target->name = xstrdup(name);
	target->prefix = basprintf("%s.%s", BOOTCHOOSER_PREFIX, name);
	target->state_prefix = basprintf("%s.%s", bc->state_prefix, name);
	target->default_attempts = global_default_attempts;
	target->default_priority = global_default_priority;

	getenv_u32(target->prefix, "default_priority",
			    &target->default_priority);
	getenv_u32(target->prefix, "default_attempts",
			    &target->default_attempts);

	ret = getenv_u32(target->state_prefix, "priority", &target->priority);
	if (ret) {
		pr_warn("Cannot read priority for target %s, using default %d\n",
		       target->name, target->default_priority);
		target->priority = target->default_priority;
	}

	ret = getenv_u32(target->state_prefix, "remaining_attempts", &target->remaining_attempts);
	if (ret) {
		pr_warn("Cannot read remaining attempts for target %s, using default %d\n",
		       target->name, target->default_attempts);
		target->remaining_attempts = target->default_attempts;
	}

	if (target->remaining_attempts && !target->priority) {
		pr_warn("Disabled target %s has remaining attempts %d, setting to 0\n",
			target->name, target->remaining_attempts);
		target->remaining_attempts = 0;
	}

	val = pr_getenv("%s.boot", target->prefix);
	if (!val)
		val = target->name;
	target->boot = xstrdup(val);

	return target;
}

/**
 * bootchooser_target_by_id - Return a target given its id
 *
 * Each target has an id, simply counted by the order they appear in
 * global.bootchooser.targets. We start counting at one to leave 0
 * for detection of uninitialized variables.
 */
static struct bootchooser_target *bootchooser_target_by_id(struct bootchooser *bc,
							   uint32_t id)
{
	struct bootchooser_target *target;

	list_for_each_entry(target, &bc->targets, list)
		if (target->id == id)
			return target;

	return NULL;
}

/**
 * bootchooser_target_disable - Disable a bootchooser target
 */
static void bootchooser_target_disable(struct bootchooser_target *target)
{
	target->priority = 0;
	target->remaining_attempts = 0;
}

/**
 * bootchooser_target_enabled - test if a target is enabled
 *
 * Returns true if a target is enabled, false if it's not.
 */
static bool bootchooser_target_enabled(struct bootchooser_target *target)
{
	return target->priority != 0;
}

/**
 * bootchooser_reset_attempts - reset remaining attempts of targets
 *
 * Reset the remaining_attempts counter of all enabled targets
 * to their default values.
 */
static void bootchooser_reset_attempts(struct bootchooser *bc)
{
	struct bootchooser_target *target;

	bootchooser_for_each_target(bc, target) {
		if (bootchooser_target_enabled(target))
			bootchooser_target_set_attempts(target, -1);
	}
}

/**
 * bootchooser_reset_priorities - reset priorities of targets
 *
 * Reset the priorities counter of all targets to their default
 * values.
 */
static void bootchooser_reset_priorities(struct bootchooser *bc)
{
	struct bootchooser_target *target;

	bootchooser_for_each_target(bc, target)
		bootchooser_target_set_priority(target, -1);
}

/**
 * bootchooser_get - get a bootchooser instance
 *
 * This evaluates the different globalvars and eventually state variables,
 * creates a bootchooser instance from it and returns it.
 */
struct bootchooser *bootchooser_get(void)
{
	struct bootchooser *bc;
	struct bootchooser_target *target;
	char *targets, *str, *freep = NULL, *delim;
	int ret = -EINVAL, id = 1;
	uint32_t last_chosen;
	static int attempts_resetted;

	bc = xzalloc(sizeof(*bc));

	if (*state_prefix) {
		if (IS_ENABLED(CONFIG_STATE)) {
			char *state_devname;

			delim = strchr(state_prefix, '.');
			if (!delim) {
				pr_err("state_prefix '%s' has invalid format\n",
				       state_prefix);
				goto err;
			}
			state_devname = xstrndup(state_prefix, delim - state_prefix);
			bc->state_prefix = xstrdup(state_prefix);
			bc->state = state_by_name(state_devname);
			if (!bc->state) {
				free(state_devname);
				pr_err("Cannot get state '%s'\n",
				       state_devname);
				ret = -ENODEV;
				goto err;
			}
			free(state_devname);
		} else {
			pr_err("State disabled, cannot use nv.state_prefix=%s\n",
			       state_prefix);
			ret = -ENODEV;
			goto err;
		}
	} else {
		bc->state_prefix = xstrdup("nv.bootchooser");
	}

	INIT_LIST_HEAD(&bc->targets);

	freep = targets = xstrdup(available_targets);

	while (1) {
		str = strsep(&targets, " ");
		if (!str || !*str)
			break;

		target = bootchooser_target_new(bc, str);
		if (!IS_ERR(target)) {
			target->id = id;
			list_add_sort(&target->list, &bc->targets,
				      bootchooser_target_compare);
		}

		id++;
	}

	if (id == 1) {
		pr_err("Target list $global.bootchooser.targets is empty\n");
		goto err;
	}

	if (list_empty(&bc->targets)) {
		pr_err("No targets could be initialized\n");
		goto err;
	}

	free(freep);

	if (test_bit(RESET_PRIORITIES_ALL_ZERO, &reset_priorities)) {
		int priority = 0;

		bootchooser_for_each_target(bc, target)
			priority += target->priority;

		if (!priority) {
			pr_info("All targets disabled, re-enabling them\n");
			bootchooser_reset_priorities(bc);
		}
	}

	if (test_bit(RESET_ATTEMPTS_POWER_ON, &reset_attempts) &&
	    reset_source_get() == RESET_POR && !attempts_resetted) {
		pr_info("Power-on Reset, resetting remaining attempts\n");
		bootchooser_reset_attempts(bc);
		attempts_resetted = 1;
	}

	if (test_bit(RESET_ATTEMPTS_ALL_ZERO, &reset_attempts)) {
		int attempts = 0;

		bootchooser_for_each_target(bc, target)
			attempts += target->remaining_attempts;

		if (!attempts) {
			pr_info("All enabled targets have 0 remaining attempts, resetting them\n");
			bootchooser_reset_attempts(bc);
		}
	}

	ret = getenv_u32(bc->state_prefix, "last_chosen", &last_chosen);
	if (!ret && last_chosen > 0) {
		bc->last_chosen = bootchooser_target_by_id(bc, last_chosen);
		if (!bc->last_chosen)
			pr_warn("Last booted target with id %d does not exist\n", last_chosen);
	}

	if (bc->last_chosen && last_boot_successful)
		bootchooser_target_set_attempts(bc->last_chosen, -1);

	if (disable_on_zero_attempts) {
		bootchooser_for_each_target(bc, target) {
			if (!target->remaining_attempts) {
				pr_info("target %s has 0 remaining attempts, disabling\n",
					target->name);
				bootchooser_target_disable(target);
			}
		}

	}

	return bc;

err:
	free(freep);
	free(bc);

	return ERR_PTR(ret);
}

/**
 * bootchooser_save - save a bootchooser to the backing store
 * @bc:		The bootchooser instance to save
 *
 * Return: 0 for success, negative error code otherwise
 */
int bootchooser_save(struct bootchooser *bc)
{
	struct bootchooser_target *target;
	int ret;

	if (bc->last_chosen)
		pr_setenv(bc, "%s.last_chosen=%d", bc->state_prefix,
			  bc->last_chosen->id);

	list_for_each_entry(target, &bc->targets, list) {
		ret = pr_setenv(bc, "%s.remaining_attempts=%d",
				target->state_prefix,
				target->remaining_attempts);
		if (ret)
			return ret;

		ret = pr_setenv(bc, "%s.priority=%d",
				target->state_prefix, target->priority);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_STATE) && bc->state) {
		ret = state_save(bc->state);
		if (ret) {
			pr_err("Cannot save state: %s\n", strerror(-ret));
			return ret;
		}
	} else {
		ret = nvvar_save();
		if (ret) {
			pr_err("Cannot save nv variables: %s\n", strerror(-ret));
			return ret;
		}
	}

	return 0;
}

/**
 * bootchooser_put - release a bootchooser instance
 * @bc: The bootchooser instance
 *
 * This releases a bootchooser instance and the memory associated with it.
 */
int bootchooser_put(struct bootchooser *bc)
{
	struct bootchooser_target *target, *tmp;
	int ret;

	ret = bootchooser_save(bc);
	if (ret)
		pr_err("Failed to save bootchooser state: %s\n", strerror(-ret));

	list_for_each_entry_safe(target, tmp, &bc->targets, list) {
		free(target->boot);
		free(target->prefix);
		free(target->state_prefix);
		free(target->name);
		free(target);
	}

	free(bc);

	return ret;
}

/**
 * bootchooser_info - Show information about a bootchooser instance
 * @bc: The bootchooser
 */
void bootchooser_info(struct bootchooser *bc)
{
	struct bootchooser_target *target;
	const char *reason;
	int count = 0;

	printf("Good targets (first will be booted next):\n");
	list_for_each_entry(target, &bc->targets, list) {
		if (bootchooser_target_ok(target, NULL)) {
			count++;
			pr_target(target);
		}
	}

	if (!count)
		printf("none\n");

	count = 0;

	printf("\nDisabled targets:\n");
	list_for_each_entry(target, &bc->targets, list) {
		if (!bootchooser_target_ok(target, &reason)) {
			count++;
			pr_target(target);
		}
	}

	if (!count)
		printf("none\n");

	printf("\nlast booted target: %s\n", bc->last_chosen ?
	       bc->last_chosen->name : "unknown");
}

/**
 * bootchooser_get_target - get the target that shall be booted next
 * @bc:		The bootchooser
 *
 * This is the heart of the bootchooser. This function selects the next
 * target to boot and returns it. The remaining_attempts counter of the
 * selected target is decreased and the bootchooser state is saved to the
 * backend.
 *
 * Return: The next target
 */
struct bootchooser_target *bootchooser_get_target(struct bootchooser *bc)
{
	struct bootchooser_target *target;

	list_for_each_entry(target, &bc->targets, list) {
		if (bootchooser_target_ok(target, NULL))
			goto found;
	}

	pr_err("No valid targets found:\n");
	list_for_each_entry(target, &bc->targets, list)
		pr_target(target);

	return ERR_PTR(-ENOENT);

found:
	target->remaining_attempts--;

	if (bc->verbose)
		pr_info("name=%s decrementing remaining_attempts to %d\n",
			target->name, target->remaining_attempts);

	if (bc->verbose)
		pr_info("selected target '%s', boot '%s'\n", target->name, target->boot);

	bc->last_chosen = target;

	bootchooser_save(bc);

	return target;
}

/**
 * bootchooser_target_name - get the name of a target
 * @target:	The target
 *
 * Given a bootchooser target this function returns its name.
 *
 * Return: The name of the target
 */
const char *bootchooser_target_name(struct bootchooser_target *target)
{
	return target->name;
}

/**
 * bootchooser_target_by_name - get a target from name
 * @bc:		The bootchooser
 * @name:	The name of the target to retrieve
 *
 * Given a name this function returns the corresponding target.
 *
 * Return: The target if found, NULL otherwise
 */
struct bootchooser_target *bootchooser_target_by_name(struct bootchooser *bc,
						      const char *name)
{
	struct bootchooser_target *target;

	bootchooser_for_each_target(bc, target)
		if (!strcmp(target->name, name))
			return target;
	return NULL;
}

/**
 * bootchooser_target_set_attempts - set remaining attempts of a target
 * @target:	The target to change
 * @attempts:	The number of attempts
 *
 * This sets the number of remaining attempts for a bootchooser target.
 * If @attempts is < 0 then the remaining attempts is reset to the default
 * value.
 *
 * Return: 0 for success, negative error code otherwise
 */
int bootchooser_target_set_attempts(struct bootchooser_target *target, int attempts)
{
	if (attempts >= 0)
		target->remaining_attempts = attempts;
	else
		target->remaining_attempts = target->default_attempts;

	return 0;
}

/**
 * bootchooser_target_set_priority - set priority of a target
 * @target:	The target to change
 * @priority:	The priority
 *
 * This sets the priority of a bootchooser target. If @priority is < 0
 * then the priority reset to the default value.
 *
 * Return: 0 for success, negative error code otherwise
 */
int bootchooser_target_set_priority(struct bootchooser_target *target, int priority)
{
	if (priority >= 0)
		target->priority = priority;
	else
		target->priority = target->default_priority;

	return 0;
}

/**
 * bootchooser_target_first - get the first target from a bootchooser
 * @bc:		The bootchooser
 *
 * Gets the first target from a bootchooser, used for the bootchooser
 * target iterator.
 *
 * Return: The first target or NULL if no target exists
 */
struct bootchooser_target *bootchooser_target_first(struct bootchooser *bc)
{
	return list_first_entry_or_null(&bc->targets,
					struct bootchooser_target, list);
}

/**
 * bootchooser_target_next - get the next target from a bootchooser
 * @bc:		The bootchooser
 * @target:	The current target
 *
 * Gets the next target from a bootchooser, used for the bootchooser
 * target iterator.
 *
 * Return: The first target or NULL if no more targets exist
 */
struct bootchooser_target *bootchooser_target_next(struct bootchooser *bc,
					       struct bootchooser_target *target)
{
	struct list_head *next = target->list.next;

	if (next == &bc->targets)
		return NULL;

	return list_entry(next, struct bootchooser_target, list);
}

/**
 * bootchooser_last_boot_successful - tell that the last boot was successful
 *
 * This tells bootchooser that the last boot was successful.
 */
void bootchooser_last_boot_successful(void)
{
	last_boot_successful = true;
}

/**
 * bootchooser_get_last_chosen - get the target which was chosen last time
 * @bc:		The bootchooser
 *
 * Bootchooser stores the id of the target which was last booted in
 * <state_prefix>.last_chosen. This function returns the target associated
 * with this id.
 *
 * Return: The target which was booted last time
 */
struct bootchooser_target *bootchooser_get_last_chosen(struct bootchooser *bc)
{
	if (!bc->last_chosen)
		return ERR_PTR(-ENODEV);

	return bc->last_chosen;
}

static int bootchooser_boot_one(struct bootchooser *bc, int *tryagain)
{
	char *system;
	struct bootentries *entries;
	struct bootentry *entry;
	struct bootchooser_target *target;
	int ret = 0;

	entries = bootentries_alloc();

	target = bootchooser_get_target(bc);
	if (IS_ERR(target)) {
		ret = PTR_ERR(target);
		*tryagain = 0;
		goto out;
	}

	system = basprintf("bootchooser.active=%s", target->name);
	globalvar_add_simple("linux.bootargs.bootchooser", system);
	free(system);

	ret = bootentry_create_from_name(entries, target->boot);
	if (ret <= 0) {
		printf("Nothing bootable found on '%s'\n", target->boot);
		*tryagain = 1;
		ret = -ENODEV;
		goto out;
	}

	last_boot_successful = false;

	ret = -ENOENT;

	bootentries_for_each_entry(entries, entry) {
		ret = boot_entry(entry, bc->verbose, bc->dryrun);
		if (!ret) {
			*tryagain = 0;
			goto out;
		}
	}

	*tryagain = 1;
out:
	globalvar_set_match("linux.bootargs.bootchooser", NULL);

	bootentries_free(entries);

	return ret;
}

static int bootchooser_boot(struct bootentry *entry, int verbose, int dryrun)
{
	struct bootchooser *bc = container_of(entry, struct bootchooser,
						       entry);
	int ret, tryagain;

	bc->verbose = verbose;
	bc->dryrun = dryrun;

	do {
		ret = bootchooser_boot_one(bc, &tryagain);

		if (!retry)
			break;
	} while (tryagain);

	return ret;
}

static void bootchooser_release(struct bootentry *entry)
{
	struct bootchooser *bc = container_of(entry, struct bootchooser,
						       entry);

	bootchooser_put(bc);
}

/**
 * bootchooser_create_bootentry - create a boot entry
 * @entries:	The list of bootentries
 *
 * This adds a bootchooser to the list of boot entries. Called
 * by the 'boot' code.
 *
 * Return: The number of entries added to the list
 */
int bootchooser_create_bootentry(struct bootentries *entries)
{
	struct bootchooser *bc = bootchooser_get();

	if (IS_ERR(bc))
		return PTR_ERR(bc);

	bc->entry.boot = bootchooser_boot;
	bc->entry.release = bootchooser_release;
	bc->entry.title = xstrdup("bootchooser");
	bc->entry.description = xstrdup("bootchooser");

	bootentries_add_entry(entries, &bc->entry);

	return 1;
}

static const char * const reset_attempts_names[] = {
	[RESET_ATTEMPTS_POWER_ON] = "power-on",
	[RESET_ATTEMPTS_ALL_ZERO] = "all-zero",
};

static const char * const reset_priorities_names[] = {
	[RESET_PRIORITIES_ALL_ZERO] = "all-zero",
};

static int bootchooser_init(void)
{
	state_prefix = xstrdup("");
	available_targets = xstrdup("");

	globalvar_add_simple_bool("bootchooser.disable_on_zero_attempts", &disable_on_zero_attempts);
	globalvar_add_simple_bool("bootchooser.retry", &retry);
	globalvar_add_simple_string("bootchooser.targets", &available_targets);
	globalvar_add_simple_string("bootchooser.state_prefix", &state_prefix);
	globalvar_add_simple_int("bootchooser.default_attempts", &global_default_attempts, "%u");
	globalvar_add_simple_int("bootchooser.default_priority", &global_default_priority, "%u");
	globalvar_add_simple_bitmask("bootchooser.reset_attempts", &reset_attempts,
				  reset_attempts_names, ARRAY_SIZE(reset_attempts_names));
	globalvar_add_simple_bitmask("bootchooser.reset_priorities", &reset_priorities,
				  reset_priorities_names, ARRAY_SIZE(reset_priorities_names));
	return 0;
}
device_initcall(bootchooser_init);

BAREBOX_MAGICVAR_NAMED(global_bootchooser_disable_on_zero_attempts,
		       global.bootchooser.disable_on_zero_attempts,
		       "bootchooser: Disable target when remaining attempts counter reaches 0");
BAREBOX_MAGICVAR_NAMED(global_bootchooser_retry,
		       global.bootchooser.retry,
		       "bootchooser: Try again when booting a target fails");
BAREBOX_MAGICVAR_NAMED(global_bootchooser_targets,
		       global.bootchooser.targets,
		       "bootchooser: Space separated list of target names");
BAREBOX_MAGICVAR_NAMED(global_bootchooser_default_attempts,
		       global.bootchooser.default_attempts,
		       "bootchooser: Default number of attempts for a target");
BAREBOX_MAGICVAR_NAMED(global_bootchooser_default_priority,
		       global.bootchooser.default_priority,
		       "bootchooser: Default priority for a target");
BAREBOX_MAGICVAR_NAMED(global_bootchooser_state_prefix,
		       global.bootchooser.state_prefix,
		       "bootchooser: state name prefix, empty for nv backend");
