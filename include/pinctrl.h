#ifndef PINCTRL_H
#define PINCTRL_H

struct pinctrl_device;

struct pinctrl_ops {
	int (*set_state)(struct pinctrl_device *, struct device_node *);
	int (*set_direction)(struct pinctrl_device *, unsigned int, bool);
	int (*get_direction)(struct pinctrl_device *, unsigned int);
};

struct pinctrl_device {
	struct device_d *dev;
	struct pinctrl_ops *ops;
	struct list_head list;
	struct device_node *node;
	unsigned int base, npins;
};

int pinctrl_register(struct pinctrl_device *pdev);
void pinctrl_unregister(struct pinctrl_device *pdev);

#ifdef CONFIG_PINCTRL
int pinctrl_select_state(struct device_d *dev, const char *state);
int pinctrl_select_state_default(struct device_d *dev);
int of_pinctrl_select_state(struct device_node *np, const char *state);
int of_pinctrl_select_state_default(struct device_node *np);
int pinctrl_gpio_direction_input(unsigned pin);
int pinctrl_gpio_direction_output(unsigned int pin);
int pinctrl_gpio_get_direction(unsigned pin);
#else
static inline int pinctrl_select_state(struct device_d *dev, const char *state)
{
	return -ENODEV;
}

static inline int pinctrl_select_state_default(struct device_d *dev)
{
	return -ENODEV;
}

static inline int of_pinctrl_select_state(struct device_node *np, const char *state)
{
	return -ENODEV;
}

static inline int of_pinctrl_select_state_default(struct device_node *np)
{
	return -ENODEV;
}

static inline int pinctrl_gpio_direction_input(unsigned pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_direction_output(unsigned int pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_get_direction(unsigned pin)
{
	return -ENOTSUPP;
}
#endif

#endif /* PINCTRL_H */
