// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt)	"gpiolib: " fmt

#include <init.h>
#include <common.h>
#include <command.h>
#include <complete.h>
#include <gpio.h>
#include <of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/overflow.h>
#include <errno.h>
#include <malloc.h>

static LIST_HEAD(chip_list);

struct gpio_desc {
	struct gpio_chip *chip;
	char *label;
	const char *name;
	u32 flags; /* OR-d enum of_gpio_flags */
};

/*
 * This descriptor validation needs to be inserted verbatim into each
 * function taking a descriptor, so we need to use a preprocessor
 * macro to avoid endless duplication. If the desc is NULL it is an
 * optional GPIO and calls should just bail out.
 */
static int validate_desc(const struct gpio_desc *desc, const char *func)
{
	if (!desc)
		return 0;
	if (IS_ERR(desc)) {
		pr_warn("%s: invalid GPIO (errorpointer)\n", func);
		return PTR_ERR(desc);
	}

	return 1;
}

#define VALIDATE_DESC(desc) do { \
	int __valid = validate_desc(desc, __func__); \
	if (__valid <= 0) \
		return __valid; \
	} while (0)

#define VALIDATE_DESC_VOID(desc) do { \
	int __valid = validate_desc(desc, __func__); \
	if (__valid <= 0) \
		return; \
	} while (0)

static struct gpio_desc *gpio_desc;

static int gpio_desc_alloc(void)
{
	gpio_desc = xzalloc(sizeof(struct gpio_desc) * ARCH_NR_GPIOS);

	return 0;
}
pure_initcall(gpio_desc_alloc);

static inline bool gpiod_is_requested(struct gpio_desc *desc)
{
	return desc->flags & OF_GPIO_REQUESTED;
}

static int gpio_ensure_requested(struct gpio_desc *desc, int gpio)
{
	if (gpiod_is_requested(desc))
		return 0;

	return gpio_request(gpio, "gpio");
}

static struct gpio_desc *gpio_to_desc(unsigned gpio)
{
	if (gpio_is_valid(gpio))
		if (gpio_desc[gpio].chip)
			return &gpio_desc[gpio];

	pr_warning("invalid GPIO %d\n", gpio);

	return NULL;
}

static unsigned gpiodesc_chip_offset(const struct gpio_desc *desc)
{
	return (desc - gpio_desc) - desc->chip->base + desc->chip->gpio_offset;
}

static __maybe_unused struct gpio_desc *gpiochip_get_desc(struct gpio_chip *chip, int gpio)
{
	return gpio_desc + chip->base + gpio - chip->gpio_offset;
}

static int gpiod_is_active_low(const struct gpio_desc *desc)
{
	return desc->flags & OF_GPIO_ACTIVE_LOW;
}

static int gpio_adjust_value(const struct gpio_desc *desc,
			     int value)
{
	if (value < 0)
		return value;

	if (gpiod_is_active_low(desc))
		value = !value;

	return (bool)value;
}

static int gpiodesc_request(struct gpio_desc *desc, const char *label)
{
	int ret;

	if (gpiod_is_requested(desc)) {
		ret = -EBUSY;
		goto done;
	}

	ret = 0;

	if (desc->chip->ops->request) {
		ret = desc->chip->ops->request(desc->chip,
					     gpiodesc_chip_offset(desc));
		if (ret)
			goto done;
	}

	desc->flags = OF_GPIO_REQUESTED;
	desc->label = xstrdup(label);

done:
	if (ret)
		pr_err("_gpio_request: gpio-%td (%s) status %d\n",
		       desc - gpio_desc, label ? : "?", ret);

	return ret;
}

int gpio_find_by_label(const char *label)
{
	int i;

	for (i = 0; i < ARCH_NR_GPIOS; i++) {
		struct gpio_desc *info = &gpio_desc[i];

		if (!gpiod_is_requested(info) || !info->chip || !info->label)
			continue;

		if (!strcmp(info->label, label))
			return i;
	}

	return -ENOENT;
}

int gpio_find_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARCH_NR_GPIOS; i++) {
		struct gpio_desc *info = &gpio_desc[i];

		if (!info->chip || !info->name)
			continue;

		if (!strcmp(info->name, name))
			return i;
	}

	return -ENOENT;
}

int gpio_request(unsigned gpio, const char *label)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc) {
		pr_err("_gpio_request: gpio-%d (%s) status %d\n",
			 gpio, label ? : "?", -ENODEV);
		return -ENODEV;
	}

	return gpiodesc_request(desc, label);
}

bool gpiod_slice_acquired(struct gpio_desc *desc)
{
	if (!desc)
		return false;

	return slice_acquired(&desc->chip->slice);
}

bool gpio_slice_acquired(unsigned gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	return gpiod_slice_acquired(desc);
}

static void gpiodesc_free(struct gpio_desc *desc)
{
	if (!gpiod_is_requested(desc))
		return;

	if (desc->chip->ops->free)
		desc->chip->ops->free(desc->chip, gpiodesc_chip_offset(desc));

	desc->flags = 0;
	free(desc->label);
	desc->label = NULL;
}

void gpio_free(unsigned gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	gpiodesc_free(desc);
}

/**
 * gpiod_put - dispose of a GPIO descriptor
 * @desc:	GPIO descriptor to dispose of
 *
 * No descriptor can be used after gpiod_put() has been called on it.
 */
void gpiod_put(struct gpio_desc *desc)
{
	if (!desc)
		return;

	gpiodesc_free(desc);
}
EXPORT_SYMBOL(gpiod_put);

/**
 * gpiod_put_array - dispose of multiple GPIO descriptors
 * @descs:	struct gpio_descs containing an array of descriptors
 */
void gpiod_put_array(struct gpio_descs *descs)
{
	unsigned int i;

	for (i = 0; i < descs->ndescs; i++)
		gpiod_put(descs->desc[i]);

	kfree(descs);
}
EXPORT_SYMBOL_GPL(gpiod_put_array);

static int gpio_do_set_config(struct gpio_chip *gc, unsigned int offset,
			      unsigned long config)
{
	if (!IS_ENABLED(CONFIG_GPIO_PINCONF))
		return -ENOSYS;
	if (!gc->ops->set_config)
		return -ENOTSUPP;

	return gc->ops->set_config(gc, offset, config);
}

/**
 * gpiod_set_config - sets @config for a GPIO
 * @desc: descriptor of the GPIO for which to set the configuration
 * @config: Same packed config format as generic pinconf
 *
 * Returns:
 * 0 on success, %-ENOTSUPP if the controller doesn't support setting the
 * configuration.
 */
int gpiod_set_config(struct gpio_desc *desc, unsigned long config)
{
	VALIDATE_DESC(desc);

	return gpio_do_set_config(desc->chip, gpiodesc_chip_offset(desc), config);
}
EXPORT_SYMBOL_GPL(gpiod_set_config);

/**
 * gpiod_set_raw_value() - assign a gpio's raw value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the raw value of the GPIO, i.e. the value of its physical line without
 * regard for its ACTIVE_LOW status.
 */
void gpiod_set_raw_value(struct gpio_desc *desc, int value)
{
	VALIDATE_DESC_VOID(desc);

	if (desc->chip->ops->set)
		desc->chip->ops->set(desc->chip, gpiodesc_chip_offset(desc), value);
}
EXPORT_SYMBOL(gpiod_set_raw_value);

void gpio_set_value(unsigned gpio, int value)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc)
		return;

	if (gpio_ensure_requested(desc, gpio))
		return;

	gpiod_set_raw_value(desc, value);
}
EXPORT_SYMBOL(gpio_set_value);

/**
 * gpiod_set_value() - assign a gpio's value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the logical value of the GPIO, i.e. taking its ACTIVE_LOW,
 * OPEN_DRAIN and OPEN_SOURCE flags into account.
 */
void gpiod_set_value(struct gpio_desc *desc, int value)
{
	VALIDATE_DESC_VOID(desc);
	gpiod_set_raw_value(desc, gpio_adjust_value(desc, value));
}
EXPORT_SYMBOL_GPL(gpiod_set_value);

void gpio_set_active(unsigned gpio, bool value)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc)
		return;

	gpiod_set_value(desc, value);
}
EXPORT_SYMBOL(gpio_set_active);

/**
 * gpiod_get_raw_value() - return a gpio's raw value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's raw value, i.e. the value of the physical line disregarding
 * its ACTIVE_LOW status, or negative errno on failure.
 */
int gpiod_get_raw_value(const struct gpio_desc *desc)
{
	VALIDATE_DESC(desc);

	if (!desc->chip->ops->get)
		return -ENOSYS;

	return desc->chip->ops->get(desc->chip, gpiodesc_chip_offset(desc));
}
EXPORT_SYMBOL_GPL(gpiod_get_raw_value);

int gpio_get_value(unsigned gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);
	int ret;

	if (!desc)
		return -ENODEV;

	ret = gpio_ensure_requested(desc, gpio);
	if (ret)
		return ret;

	return gpiod_get_raw_value(desc);
}
EXPORT_SYMBOL(gpio_get_value);

/**
 * gpiod_get_value() - return a gpio's value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's logical value, i.e. taking the ACTIVE_LOW status into
 * account, or negative errno on failure.
 */
int gpiod_get_value(const struct gpio_desc *desc)
{
	VALIDATE_DESC(desc);

	return gpio_adjust_value(desc, gpiod_get_raw_value(desc));
}
EXPORT_SYMBOL_GPL(gpiod_get_value);

int gpio_is_active(unsigned gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc)
		return -ENODEV;

	return gpiod_get_value(desc);
}
EXPORT_SYMBOL(gpio_is_active);

/**
 * gpiod_direction_output_raw - set the GPIO direction to output
 * @desc:	GPIO to set to output
 * @value:	initial output value of the GPIO
 *
 * Set the direction of the passed GPIO to output, such as gpiod_set_value() can
 * be called safely on it. The initial value of the output must be specified
 * as raw value on the physical line without regard for the ACTIVE_LOW status.
 *
 * Return 0 in case of success, else an error code.
 */
int gpiod_direction_output_raw(struct gpio_desc *desc, int value)
{
	VALIDATE_DESC(desc);

	if (!desc->chip->ops->direction_output)
		return -ENOSYS;

	return desc->chip->ops->direction_output(desc->chip,
					       gpiodesc_chip_offset(desc), value);
}
EXPORT_SYMBOL(gpiod_direction_output_raw);

int gpio_direction_output(unsigned gpio, int value)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);
	int ret;

	if (!desc)
		return -ENODEV;

	ret = gpio_ensure_requested(desc, gpio);
	if (ret)
		return ret;

	return gpiod_direction_output_raw(desc, value);
}
EXPORT_SYMBOL(gpio_direction_output);

static int gpio_set_config_with_argument(struct gpio_desc *desc,
					 enum pin_config_param mode,
					 u32 argument)
{
	unsigned long config;

	config = pinconf_to_config_packed(mode, argument);
	return gpio_do_set_config(desc->chip, gpiodesc_chip_offset(desc), config);
}

static int gpio_set_config_with_argument_optional(struct gpio_desc *desc,
						  enum pin_config_param mode,
						  u32 argument)
{
	int ret;

	ret = gpio_set_config_with_argument(desc, mode, argument);
	if (ret != -ENOTSUPP)
		return ret;
	return 0;
}

static int gpio_set_config(struct gpio_desc *desc, enum pin_config_param mode)
{
	return gpio_set_config_with_argument(desc, mode, 0);
}

static int gpio_set_bias(struct gpio_desc *desc)
{
	enum pin_config_param bias;
	unsigned int arg;

	if (desc->flags & OF_GPIO_PULL_DISABLE)
		bias = PIN_CONFIG_BIAS_DISABLE;
	else if (desc->flags & OF_GPIO_PULL_UP)
		bias = PIN_CONFIG_BIAS_PULL_UP;
	else if (desc->flags & OF_GPIO_PULL_DOWN)
		bias = PIN_CONFIG_BIAS_PULL_DOWN;
	else
		return 0;

	switch (bias) {
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_BIAS_PULL_UP:
		arg = 1;
		break;

	default:
		arg = 0;
		break;
	}

	return gpio_set_config_with_argument_optional(desc, bias, arg);
}

/**
 * gpiod_direction_output - set the GPIO direction to output
 * @desc:	GPIO to set to output
 * @value:	initial output value of the GPIO
 *
 * Set the direction of the passed GPIO to output, such as gpiod_set_value() can
 * be called safely on it. The initial value of the output must be specified
 * as the logical value of the GPIO, i.e. taking its ACTIVE_LOW status into
 * account.
 *
 * Return 0 in case of success, else an error code.
 */
int gpiod_direction_output(struct gpio_desc *desc, int value)
{
	int ret;

	VALIDATE_DESC(desc);

	if (IS_ENABLED(CONFIG_GPIO_PINCONF)) {
		if (desc->flags & (OF_GPIO_OPEN_DRAIN | OF_GPIO_SINGLE_ENDED)) {
			/* First see if we can enable open drain in hardware */
			ret = gpio_set_config(desc, PIN_CONFIG_DRIVE_OPEN_DRAIN);
			if (!ret)
				goto set_output_value;
			/* Emulate open drain by not actively driving the line high */
			if (value)
				return gpiod_direction_input(desc);
		} else if (desc->flags & OF_GPIO_SINGLE_ENDED) {
			ret = gpio_set_config(desc, PIN_CONFIG_DRIVE_OPEN_SOURCE);
			if (!ret)
				goto set_output_value;
			/* Emulate open source by not actively driving the line low */
			if (!value)
				return gpiod_direction_input(desc);
		} else {
			gpio_set_config(desc, PIN_CONFIG_DRIVE_PUSH_PULL);
		}

set_output_value:
		ret = gpio_set_bias(desc);
		if (ret)
			return ret;
	}

	return gpiod_direction_output_raw(desc, gpio_adjust_value(desc, value));
}

int gpio_direction_active(unsigned gpio, bool value)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc)
		return -ENODEV;

	return gpiod_direction_output(desc, value);
}
EXPORT_SYMBOL(gpio_direction_active);

/**
 * gpiod_direction_input - set the GPIO direction to input
 * @desc:	GPIO to set to input
 *
 * Set the direction of the passed GPIO to input, such as gpiod_get_value() can
 * be called safely on it.
 *
 * Return 0 in case of success, else an error code.
 */
int gpiod_direction_input(struct gpio_desc *desc)
{
	int ret;

	VALIDATE_DESC(desc);

	if (!desc->chip->ops->direction_input)
		return -ENOSYS;

	ret = desc->chip->ops->direction_input(desc->chip,
					       gpiodesc_chip_offset(desc));
	if (ret == 0 && IS_ENABLED(CONFIG_GPIO_PINCONF))
		ret = gpio_set_bias(desc);

	return ret;
}
EXPORT_SYMBOL(gpiod_direction_input);

int gpio_direction_input(unsigned gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);
	int ret;

	if (!desc)
		return -ENODEV;

	ret = gpio_ensure_requested(desc, gpio);
	if (ret)
		return ret;

	return gpiod_direction_input(desc);
}
EXPORT_SYMBOL(gpio_direction_input);

/**
 * gpiodesc_request_one - request GPIO descriptor
 * @desc: GPIO descriptor
 * @lflags: OF lookup flags for this GPIO or 0 if default
 * such as GPIOF_ACTIVE_LOW
 * @dflags: descriptor request flags (GPIOF_*) for this GPIO or 0 if default
 *
 * Function is used to request a GPIO descriptor and configure it for use.
 *
 * Returns:
 * 0 on success and a negative error code otherwise.
 */
static int gpiodesc_request_one(struct gpio_desc *desc, unsigned long lflags,
				unsigned long dflags, const char *label)
{
	int err;

	/*
	 * Not all of the flags below are mulit-bit, but, for the sake
	 * of consistency, the code is written as if all of them were.
	 */
	const bool active_low  = (dflags & GPIOF_ACTIVE_LOW) == GPIOF_ACTIVE_LOW;
	const bool dir_in      = (dflags & GPIOF_DIR_IN) == GPIOF_DIR_IN;
	const bool logical     = (dflags & GPIOF_LOGICAL) == GPIOF_LOGICAL;
	const bool init_active = (dflags & GPIOF_INIT_ACTIVE) == GPIOF_INIT_ACTIVE;
	const bool init_high   = (dflags & GPIOF_INIT_HIGH) == GPIOF_INIT_HIGH;

	err = gpiodesc_request(desc, label);
	if (err)
		return err;

	desc->flags |= lflags;
	if (active_low)
		desc->flags |= OF_GPIO_ACTIVE_LOW;

	if (dir_in)
		err = gpiod_direction_input(desc);
	else if (logical)
		err = gpiod_direction_output(desc, init_active);
	else
		err = gpiod_direction_output_raw(desc, init_high);

	if (err)
		gpiodesc_free(desc);

	return err;
}

/**
 * gpio_request_one - request a single GPIO with initial configuration
 * @gpio:	the GPIO number
 * @flags:	GPIO configuration as specified by GPIOF_*
 * @label:	a literal description string of this GPIO
 */
int gpio_request_one(unsigned gpio, unsigned long flags, const char *label)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	if (!desc)
		return -ENODEV;

	return gpiodesc_request_one(desc, 0, flags, label);
}
EXPORT_SYMBOL_GPL(gpio_request_one);

/**
 * gpio_request_array - request multiple GPIOs in a single call
 * @array:	array of the 'struct gpio'
 * @num:	how many GPIOs in the array
 */
int gpio_request_array(const struct gpio *array, size_t num)
{
	int i, err;

	for (i = 0; i < num; i++, array++) {
		err = gpio_request_one(array->gpio, array->flags, array->label);
		if (err)
			goto err_free;
	}
	return 0;

err_free:
	while (i--)
		gpio_free((--array)->gpio);
	return err;
}
EXPORT_SYMBOL_GPL(gpio_request_array);

/**
 * gpio_free_array - release multiple GPIOs in a single call
 * @array:	array of the 'struct gpio'
 * @num:	how many GPIOs in the array
 */
void gpio_free_array(const struct gpio *array, size_t num)
{
	while (num--)
		gpio_free((array++)->gpio);
}
EXPORT_SYMBOL_GPL(gpio_free_array);

int gpio_array_to_id(const struct gpio *array, size_t num, u32 *val)
{
	u32 id = 0;
	int ret, i;

	if (num > 32)
		return -EOVERFLOW;

	ret = gpio_request_array(array, num);
	if (ret)
		return ret;

	/* Wait until logic level will be stable */
	udelay(5);
	for (i = 0; i < num; i++) {
		ret = gpio_is_active(array[i].gpio);
		if (ret < 0)
			goto free_array;
		if (ret)
			id |= 1UL << i;
	}

	*val = id;
	ret = 0;

free_array:
	gpio_free_array(array, num);
	return ret;
}
EXPORT_SYMBOL(gpio_array_to_id);

static int gpiochip_find_base(int ngpio)
{
	int i;
	int spare = 0;
	int base = -ENOSPC;

	for (i = ARCH_NR_GPIOS - 1; i >= 0; i--) {
		struct gpio_chip *chip = gpio_desc[i].chip;

		if (!chip) {
			spare++;
			if (spare == ngpio) {
				base = i;
				break;
			}
		} else {
			spare = 0;
			i -= chip->ngpio - 1;
		}
	}

	if (gpio_is_valid(base))
		debug("%s: found new base at %d\n", __func__, base);
	return base;
}


#ifdef CONFIG_OF_GPIO

static int of_hog_gpio(struct device_node *np, struct gpio_chip *chip,
		       unsigned int idx)
{
	struct device_node *chip_np = chip->dev->of_node;
	unsigned long flags = 0;
	u32 gpio_cells, gpio_num, of_flags;
	int ret;
	const char *name = NULL;

	ret = of_property_read_u32(chip_np, "#gpio-cells", &gpio_cells);
	if (ret)
		return ret;

	/*
	 * Support for GPIOs that don't have #gpio-cells set to 2 is
	 * not implemented
	 */
	if (WARN_ON(gpio_cells != 2))
		return -ENOTSUPP;

	ret = of_property_read_u32_index(np, "gpios", idx * gpio_cells,
					 &gpio_num);
	if (ret)
		return ret;

	ret = of_property_read_u32_index(np, "gpios", idx * gpio_cells + 1,
					 &of_flags);
	if (ret)
		return ret;

	/*
	 * Note that, in order to be compatible with Linux, the code
	 * below interprets 'output-high' as to mean 'output-active'.
	 * That is, when processed for active-low GPIO, it will result
	 * in output being asserted logically 'active', but physically
	 * 'low'.
	 *
	 * Conversely it means that specifying 'output-low' for
	 * 'active-low' GPIO would result in 'high' level observed on
	 * the corresponding pin
	 *
	 */
	if (of_property_read_bool(np, "input"))
		flags |= GPIOF_DIR_IN;
	else if (of_property_read_bool(np, "output-low"))
		flags |= GPIOF_OUT_INIT_INACTIVE;
	else if (of_property_read_bool(np, "output-high"))
		flags |= GPIOF_OUT_INIT_ACTIVE;
	else
		return -EINVAL;

	/* The line-name is optional and if not present the node name is used */
	ret = of_property_read_string(np, "line-name", &name);
	if (ret < 0)
		name = np->name;

	return gpiodesc_request_one(gpiochip_get_desc(chip, gpio_num),
				    of_flags, flags, name);
}

static int of_gpiochip_scan_hogs(struct gpio_chip *chip)
{
	struct device_node *np;
	int ret, i;

	for_each_available_child_of_node(chip->dev->of_node, np) {
		if (!of_property_read_bool(np, "gpio-hog"))
			continue;

		for (ret = 0, i = 0;
		     !ret;
		     ret = of_hog_gpio(np, chip, i), i++)
			;
		/*
		 * We ignore -EOVERFLOW because it serves us as an
		 * indicator that there's no more GPIOs to handle.
		 */
		if (ret < 0 && ret != -EOVERFLOW)
			return ret;
	}

	return 0;
}

/*
 * of_gpiochip_set_names - Set GPIO line names using OF properties
 * @chip: GPIO chip whose lines should be named, if possible
 *
 * Looks for device property "gpio-line-names" and if it exists assigns
 * GPIO line names for the chip. The memory allocated for the assigned
 * names belong to the underlying firmware node and should not be released
 * by the caller.
 */
static int of_gpiochip_set_names(struct gpio_chip *chip)
{
	struct device_node *np = dev_of_node(chip->dev);
	const char **names;
	int ret, i, count;

	count = of_property_count_strings(np, "gpio-line-names");
	if (count < 0)
		return 0;

	names = kcalloc(count, sizeof(*names), GFP_KERNEL);
	if (!names)
		return -ENOMEM;

	ret = of_property_read_string_array(np, "gpio-line-names",
					    names, count);
	if (ret < 0) {
		kfree(names);
		return ret;
	}

	/*
	 * Since property 'gpio-line-names' cannot contains gaps, we
	 * have to be sure we only assign those pins that really exists
	 * since chip->ngpio can be less.
	 */
	if (count > chip->ngpio)
		count = chip->ngpio;

	for (i = 0; i < count; i++) {
		/*
		 * Allow overriding "fixed" names provided by the GPIO
		 * provider. The "fixed" names are more often than not
		 * generic and less informative than the names given in
		 * device properties.
		 */
		if (names[i] && names[i][0])
			gpio_desc[chip->base + i].name = names[i];
	}

	free(names);

	return 0;
}

/**
 * of_gpio_simple_xlate - translate gpiospec to the GPIO number and flags
 * @gc:		pointer to the gpio_chip structure
 * @gpiospec:	GPIO specifier as found in the device tree
 * @flags:	a flags pointer to fill in
 *
 * This is simple translation function, suitable for the most 1:1 mapped
 * GPIO chips. This function performs only one sanity check: whether GPIO
 * is less than ngpios (that is specified in the gpio_chip).
 */
static int of_gpio_simple_xlate(struct gpio_chip *gc,
				const struct of_phandle_args *gpiospec,
				u32 *flags)
{
	int gpio = gpiospec->args[0] - gc->gpio_offset;

	/*
	 * We're discouraging gpio_cells < 2, since that way you'll have to
	 * write your own xlate function (that will have to retrieve the GPIO
	 * number and the flags from a single gpio cell -- this is possible,
	 * but not recommended).
	 */
	if (WARN_ON(gc->of_gpio_n_cells < 2))
		return -EINVAL;

	if (WARN_ON(gpiospec->args_count < gc->of_gpio_n_cells))
		return -EINVAL;

	if (gpio < 0 || gpio >= gc->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gc->base + gpio;
}

static int of_gpiochip_add(struct gpio_chip *chip)
{
	struct device_node *np;
	int ret;

	np = dev_of_node(chip->dev);
	if (!np)
		return 0;

	if (!chip->ops->of_xlate)
		chip->ops->of_xlate = of_gpio_simple_xlate;

	/*
	 * Separate check since the 'struct gpio_ops' is always the same for
	 * every 'struct gpio_chip' of the same instance (e.g. 'struct
	 * imx_gpio_chip').
	 */
	if (chip->ops->of_xlate == of_gpio_simple_xlate)
		chip->of_gpio_n_cells = 2;

	if (chip->of_gpio_n_cells > MAX_PHANDLE_ARGS)
		return -EINVAL;

	ret = of_gpiochip_set_names(chip);
	if (ret)
		return ret;

	return of_gpiochip_scan_hogs(chip);
}
#else
static int of_gpiochip_add(struct gpio_chip *chip)
{
	return 0;
}
#endif

#ifdef CONFIG_OFDEVICE
static const char *gpio_suffixes[] = {
	"gpios",
	"gpio",
};

static struct property *of_find_gpio_property(struct device_node *np,
					      const char *_con_id)
{
	struct property *pp = NULL;
	char *con_id;
	int i;

	for (i = 0; i < ARRAY_SIZE(gpio_suffixes); i++) {
		if (_con_id)
			con_id = basprintf("%s-%s", _con_id, gpio_suffixes[i]);
		else
			con_id = basprintf("%s", gpio_suffixes[i]);

		if (!con_id)
			return ERR_PTR(-ENOMEM);

		pp = of_find_property(np, con_id, NULL);
		free(con_id);

		if (pp)
			return pp;
	}

	return NULL;
}

/* Linux compatibility helper: Get a GPIO descriptor from device tree */
struct gpio_desc *dev_gpiod_get_index(struct device *dev,
			struct device_node *np,
			const char *con_id, int index,
			enum gpiod_flags dflags,
			const char *label)
{
	struct gpio_desc *desc = NULL;
	enum of_gpio_flags of_flags;
	struct property *pp;
	char *buf = NULL;
	int gpio;
	int ret;

	if (!np)
		return ERR_PTR(-ENODEV);

	pp = of_find_gpio_property(np, con_id);
	if (!pp)
		return ERR_PTR(-ENOENT);

	gpio = of_get_named_gpio_flags(np, pp->name, index, &of_flags);
	if (!gpio_is_valid(gpio))
		return ERR_PTR(gpio < 0 ? gpio : -EINVAL);

	desc = gpio_to_desc(gpio);

	buf = NULL;

	if (!label) {
		if (con_id)
			label = buf = basprintf("%s-%s", dev_name(dev), con_id);
		else
			label = dev_name(dev);
	}

	ret = gpiodesc_request_one(desc, of_flags, dflags, label);
	free(buf);

	return ret ? ERR_PTR(ret): desc;
}

/**
 * gpiod_count - return the number of GPIOs associated with a device / function
 *		or -ENOENT if no GPIO has been assigned to the requested function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @_con_id:	function within the GPIO consumer
 */
int gpiod_count(struct device *dev, const char *con_id)
{
	struct device_node *np = dev_of_node(dev);
	struct property *pp;

	if (!np)
		return -ENODEV;

	pp = of_find_gpio_property(np, con_id);
	if (!pp)
		return -ENOENT;

	return of_gpio_named_count(np, pp->name);
}
EXPORT_SYMBOL_GPL(gpiod_count);

/**
 * gpiod_get_array - obtain multiple GPIOs from a multi-index GPIO function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * This function acquires all the GPIOs defined under a given function.
 *
 * Return a struct gpio_descs containing an array of descriptors, -ENOENT if
 * no GPIO has been assigned to the requested function, or another IS_ERR()
 * code if an error occurred while trying to acquire the GPIOs.
 */
struct gpio_descs *__must_check gpiod_get_array(struct device *dev,
						const char *con_id,
						enum gpiod_flags flags)
{
	struct gpio_desc *desc;
	struct gpio_descs *descs;
	int count;

	count = gpiod_count(dev, con_id);
	if (count < 0)
		return ERR_PTR(count);

	descs = kzalloc(struct_size(descs, desc, count), GFP_KERNEL);
	if (!descs)
		return ERR_PTR(-ENOMEM);

	for (descs->ndescs = 0; descs->ndescs < count; descs->ndescs++) {
		desc = dev_gpiod_get_index(dev, dev_of_node(dev), con_id,
					   descs->ndescs, flags, NULL);
		if (IS_ERR(desc)) {
			gpiod_put_array(descs);
			return ERR_CAST(desc);
		}

		descs->desc[descs->ndescs] = desc;
	}

	return descs;
}
EXPORT_SYMBOL_GPL(gpiod_get_array);

#endif

static int gpiod_set_array_value_complex(bool raw,
					 unsigned int array_size,
					 struct gpio_desc **desc_array,
					 struct gpio_array *array_info,
					 unsigned long *value_bitmap)
{
	int i;

	BUG_ON(array_info != NULL);

	for (i = 0; i < array_size; i++)
		gpiod_set_value(desc_array[i], test_bit(i, value_bitmap));

	return 0;
}

/**
 * gpiod_set_array_value() - assign values to an array of GPIOs
 * @array_size: number of elements in the descriptor array / value bitmap
 * @desc_array: array of GPIO descriptors whose values will be assigned
 * @array_info: information on applicability of fast bitmap processing path
 * @value_bitmap: bitmap of values to assign
 *
 * Set the logical values of the GPIOs, i.e. taking their ACTIVE_LOW status
 * into account. NOTE: This function has no special handling for GPIOs
 * in the same bank that could've been set atomically: GPIO sequencing
 * is not guaranteed to always remain in the same order.
 */
int gpiod_set_array_value(unsigned int array_size,
			  struct gpio_desc **desc_array,
			  struct gpio_array *array_info,
			  unsigned long *value_bitmap)
{
	if (!desc_array)
		return -EINVAL;
	return gpiod_set_array_value_complex(false, array_size,
					     desc_array, array_info,
					     value_bitmap);
}
EXPORT_SYMBOL_GPL(gpiod_set_array_value);

int gpiochip_add(struct gpio_chip *chip)
{
	int i;

	if (chip->base >= 0) {
		for (i = 0; i < chip->ngpio; i++) {
			if (gpio_desc[chip->base + i].chip)
				return -EBUSY;
		}
	} else {
		chip->base = gpiochip_find_base(chip->ngpio);
		if (chip->base < 0)
			return -ENOSPC;
	}

	slice_init(&chip->slice, dev_name(chip->dev));

	list_add_tail(&chip->list, &chip_list);

	for (i = chip->base; i < chip->base + chip->ngpio; i++)
		gpio_desc[i].chip = chip;

	return of_gpiochip_add(chip);
}

void gpiochip_remove(struct gpio_chip *chip)
{
	list_del(&chip->list);
}

struct gpio_chip *gpio_get_chip_by_dev(struct device *dev)
{
	struct gpio_chip *chip;

	list_for_each_entry(chip, &chip_list, list) {
		if (chip->dev == dev)
			return chip;
	}

	return NULL;
}

struct gpio_chip *of_gpio_get_chip_by_alias(const char *alias)
{
	struct gpio_chip *chip;
	struct device_node *np;

	np = of_find_node_by_alias(NULL, alias);
	if (!np)
		return NULL;

	of_device_ensure_probed(np);

	list_for_each_entry(chip, &chip_list, list) {
		if (dev_of_node(chip->dev) == np)
			return chip;
	}

	return NULL;
}

int gpio_get_num(struct device *dev, int gpio)
{
	struct gpio_chip *chip;

	if (!dev)
		return -ENODEV;

	chip = gpio_get_chip_by_dev(dev);
	if (!chip)
		return -EPROBE_DEFER;

	return chip->base + gpio;
}

struct gpio_chip *gpio_get_chip(int gpio)
{
	struct gpio_desc *desc = gpio_to_desc(gpio);

	return desc ? desc->chip : NULL;
}

#ifdef CONFIG_CMD_GPIO
static int do_gpiolib(int argc, char *argv[])
{
	struct gpio_chip *chip = NULL;
	int i;

	if (argc > 2)
		return COMMAND_ERROR_USAGE;

	if (argc > 1) {
		struct device *dev;

		dev = find_device(argv[1]);
		if (!dev)
			return -ENODEV;

		chip = gpio_get_chip_by_dev(dev);
		if (!chip)
			return -EINVAL;
	}

	for (i = 0; i < ARCH_NR_GPIOS; i++) {
		struct gpio_desc *desc = &gpio_desc[i];
		bool requested, active_low;
		int val = -1, dir = -1;
		int idx;

		if (!desc->chip)
			continue;

		if (chip && chip != desc->chip)
			continue;

		/* print chip information and header on first gpio */
		if (desc->chip->base == i) {
			printf("\nGPIOs %u-%u, chip %s:\n",
				desc->chip->base,
				desc->chip->base + desc->chip->ngpio - 1,
				dev_name(desc->chip->dev));
			printf("             %-3s %-3s %-9s %-20s %-20s\n", "dir", "val", "requested", "name", "label");
		}

		idx = i - desc->chip->base;

		if (desc->chip->ops->get_direction)
			dir = desc->chip->ops->get_direction(desc->chip, idx);
		if (desc->chip->ops->get)
			val = desc->chip->ops->get(desc->chip, idx);

		requested  = gpiod_is_requested(desc);
		active_low = gpiod_is_active_low(desc);

		printf("  GPIO %4d: %-3s %-3s %-9s %-20s %-20s\n", chip ? idx : i,
			(dir < 0) ? "unk" : ((dir == GPIOF_DIR_IN) ? "in" : "out"),
			(val < 0) ? "unk" : ((val == 0) ? "lo" : "hi"),
		        requested ? (active_low ? "active low" : "true") : "false",
			desc->name ? desc->name : "",
			desc->label ? desc->label : "");
	}

	return 0;
}

BAREBOX_CMD_START(gpioinfo)
	.cmd		= do_gpiolib,
	BAREBOX_CMD_DESC("list registered GPIOs")
	BAREBOX_CMD_OPTS("[CONTROLLER]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
#endif
