#ifndef PINCTRL_H
#define PINCTRL_H

struct pinctrl_device;

struct pinctrl_ops {
	int (*set_state)(struct pinctrl_device *, struct device_node *);
};

struct pinctrl_device {
	struct device_d *dev;
	struct pinctrl_ops *ops;
	struct list_head list;
	struct device_node *node;
};

int pinctrl_register(struct pinctrl_device *pdev);
void pinctrl_unregister(struct pinctrl_device *pdev);

#ifdef CONFIG_PINCTRL
int pinctrl_select_state(struct device_d *dev, const char *state);
int pinctrl_select_state_default(struct device_d *dev);
#else
static inline int pinctrl_select_state(struct device_d *dev, const char *state)
{
	return -ENODEV;
}

static inline int pinctrl_select_state_default(struct device_d *dev)
{
	return -ENODEV;
}
#endif

#endif /* PINCTRL_H */
