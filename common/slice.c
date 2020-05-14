// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "slice: " fmt

#include <common.h>
#include <init.h>
#include <slice.h>

/*
 * slices, the barebox idea of locking
 *
 * barebox has pollers which execute code in the background whenever one of the
 * delay functions (udelay, mdelay, ...) or is_timeout() are called. This
 * introduces resource problems when some device triggers a poller by calling
 * a delay function and then the poller code calls into the same device again.
 *
 * As an example consider a I2C GPIO expander which drives a LED which shall
 * be used as a heartbeat LED:
 *
 * poller -> LED on/off -> GPIO high/low -> I2C transfer
 *
 * The I2C controller has a timeout loop using is_timeout() and thus can trigger
 * a poller run. With this the following can happen during an unrelated I2C
 * transfer:
 *
 * I2C transfer -> is_timeout() -> poller -> LED on/off -> GPIO high/low -> I2C transfer
 *
 * We end up with issuing an I2C transfer during another I2C transfer and
 * things go downhill.
 *
 * Due to the lack of interrupts we can't do real locking in barebox. We use
 * a mechanism called slices instead. A slice describes a resource to which
 * other slices can be attached. Whenever a slice is needed it must be acquired.
 * Acquiring a slice never fails, it just increases the acquired counter of
 * the slice and its dependent slices. when a slice shall be used inside a
 * poller it must first be tested if the slice is already in use. If it is,
 * we can't do the operation on the slice now and must return and hope that
 * we have more luck in the next poller call.
 *
 * slices can be attached other slices as dependencies. In the example above
 * LED driver would add a dependency to the GPIO controller and the GPIO driver
 * would add a dependency to the I2C bus:
 *
 * GPIO driver probe:
 *
 * slice_depends_on(&gpio->slice, i2c_device_slice(i2cdev));
 *
 * LED driver probe:
 *
 * slice_depends_on(&led->slice, gpio_slice(gpio));
 *
 * The GPIO code would call slice_acquire(&gpio->slice) before doing any
 * operation on the GPIO chip providing this GPIO, likewise the I2C core
 * would call slice_acquire(&i2cbus->slice) before doing an operation on
 * this I2C bus.
 *
 * The heartbeat poller code would call slice_acquired(led_slice(led)) and
 * only continue when the slice is not acquired.
 */

/*
 * slices are not required to have a device and thus a name, but it really
 * helps debugging when it has one.
 */
static const char *slice_name(struct slice *slice)
{
	return slice->name;
}

static void __slice_acquire(struct slice *slice, int add)
{
	slice->acquired += add;

	if (slice->acquired < 0) {
		pr_err("Slice %s acquired count drops below zero\n",
			slice_name(slice));
		dump_stack();
		slice->acquired = 0;
		return;
	}
}

/**
 * slice_acquire: acquire a slice
 * @slice: The slice to acquire
 *
 * This acquires a slice and its dependencies
 */
void slice_acquire(struct slice *slice)
{
	__slice_acquire(slice, 1);
}

/**
 * slice_release: release a slice
 * @slice: The slice to release
 *
 * This releases a slice and its dependencies
 */
void slice_release(struct slice *slice)
{
	__slice_acquire(slice, -1);
}

/**
 * slice_acquired: test if a slice is acquired
 * @slice: The slice to test
 *
 * This tests if a slice is acquired. Returns true if it is, false otherwise
 */
bool slice_acquired(struct slice *slice)
{
	struct slice_entry *se;
	bool ret = false;

	if (slice->acquired > 0)
		return true;

	if (slice->acquired < 0) {
		pr_err("Recursive dependency detected in slice %s\n",
		       slice_name(slice));
		panic("Cannot continue");
	}

	slice->acquired = -1;

	list_for_each_entry(se, &slice->deps, list)
		if (slice_acquired(se->slice)) {
			ret = true;
			break;
		}

	slice->acquired = 0;

	return ret;
}

static void __slice_debug_acquired(struct slice *slice, int level)
{
	struct slice_entry *se;

	pr_debug("%*s%s: %d\n", level * 4, "",
		 slice_name(slice),
		 slice->acquired);

	list_for_each_entry(se, &slice->deps, list)
		__slice_debug_acquired(se->slice, level + 1);
}

/**
 * slice_debug_acquired: print the acquired state of a slice
 *
 * @slice: The slice to print
 *
 * This prints the acquired state of a slice and its dependencies.
 */
void slice_debug_acquired(struct slice *slice)
{
	if (!slice_acquired(slice))
		return;

	__slice_debug_acquired(slice, 0);
}

/**
 * slice_depends_on: Add a dependency to a slice
 *
 * @slice: The slice to add the dependency to
 * @dep:   The slice @slice depends on
 *
 * This makes @slice dependent on @dep. In other words, acquiring @slice
 * will lead to also acquiring @dep.
 */
void slice_depends_on(struct slice *slice, struct slice *dep)
{
	struct slice_entry *se = xzalloc(sizeof(*se));

	pr_debug("Adding dependency %s (%d) to slice %s (%d)\n",
		 slice_name(dep), dep->acquired,
		 slice_name(slice), slice->acquired);

	se->slice = dep;

	list_add_tail(&se->list, &slice->deps);
}

static LIST_HEAD(slices);

/**
 * slice_init - initialize a slice before usage
 * @slice: The slice to initialize
 * @name:  The name of the slice
 *
 * This initializes a slice before usage. @name should be a meaningful name, when
 * a device context is available to the caller it is recommended to pass its
 * dev_name() as name argument.
 */
void slice_init(struct slice *slice, const char *name)
{
	INIT_LIST_HEAD(&slice->deps);
	slice->name = xstrdup(name);
	list_add_tail(&slice->list, &slices);
}

/**
 * slice_exit: Remove a slice
 * @slice: The slice to remove the dependency from
 */
void slice_exit(struct slice *slice)
{
	struct slice *s;
	struct slice_entry *se, *tmp;

	list_del(&slice->list);

	/* remove our dependencies */
	list_for_each_entry_safe(se, tmp, &slice->deps, list) {
		list_del(&se->list);
		free(se);
	}

	/* remove this slice from other slices dependencies */
	list_for_each_entry(s, &slices, list) {
		list_for_each_entry_safe(se, tmp, &s->deps, list) {
			if (se->slice == slice) {
				list_del(&se->list);
				free(se);
			}
		}
	}

	free(slice->name);
}

struct slice command_slice;

/**
 * command_slice_acquire - acquire the command slice
 *
 * The command slice is acquired by default. It is only released
 * at certain points we know it's safe to execute code in the
 * background, like when the shell is waiting for input.
 */
void command_slice_acquire(void)
{
	slice_acquire(&command_slice);
}

/**
 * command_slice_release - release the command slice
 */
void command_slice_release(void)
{
	slice_release(&command_slice);
}

static int command_slice_init(void)
{
	slice_init(&command_slice, "command");
	slice_acquire(&command_slice);
	return 0;
}

pure_initcall(command_slice_init);

#if defined CONFIG_CMD_SLICE

#include <command.h>
#include <getopt.h>

static void slice_info(void)
{
	struct slice *slice;
	struct slice_entry *se;

	list_for_each_entry(slice, &slices, list) {
		printf("slice %s: acquired=%d\n",
		       slice_name(slice), slice->acquired);
		list_for_each_entry(se, &slice->deps, list)
			printf("    dep: %s\n", slice_name(se->slice));
	}
}

static int __slice_acquire_name(const char *name, int add)
{
	struct slice *slice;

	list_for_each_entry(slice, &slices, list) {
		if (!strcmp(slice->name, name)) {
			__slice_acquire(slice, add);
			return 0;
		}
	}

	printf("No such slice: %s\n", name);

	return 1;
}

BAREBOX_CMD_HELP_START(slice)
BAREBOX_CMD_HELP_TEXT("slice debugging command")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-i", "Print information about slices")
BAREBOX_CMD_HELP_OPT ("-a <devname>", "acquire a slice")
BAREBOX_CMD_HELP_OPT ("-r <devname>", "release a slice")
BAREBOX_CMD_HELP_END

static int do_slice(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "a:r:i")) > 0) {
		switch (opt) {
		case 'a':
			return __slice_acquire_name(optarg, 1);
		case 'r':
			return __slice_acquire_name(optarg, -1);
		case 'i':
			slice_info();
			return 0;
		}
	}

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_START(slice)
	.cmd = do_slice,
	BAREBOX_CMD_DESC("debug slices")
	BAREBOX_CMD_OPTS("[-ari]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_slice_help)
BAREBOX_CMD_END
#endif
