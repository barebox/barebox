#ifndef __LED_H
#define __LED_H

#include <errno.h>

struct led {
	void (*set)(struct led *, unsigned int value);
	int max_value;
	char *name;
	int num;
	struct list_head list;
};

struct led *led_by_number(int no);
struct led *led_by_name(const char *name);
struct led *led_by_name_or_number(const char *str);

static inline int led_get_number(struct led *led)
{
	return led->num;
}

int led_set_num(int num, unsigned int value);
int led_set(struct led *led, unsigned int value);
int led_register(struct led *led);
void led_unregister(struct led *led);
void led_unregister(struct led *led);

/* LED trigger support */
enum led_trigger {
	LED_TRIGGER_PANIC,
	LED_TRIGGER_HEARTBEAT,
	LED_TRIGGER_NET_RX,
	LED_TRIGGER_NET_TX,
	LED_TRIGGER_NET_TXRX,
	LED_TRIGGER_MAX,
};

enum trigger_type {
	TRIGGER_ENABLE,
	TRIGGER_DISABLE,
	TRIGGER_FLASH,
};

#ifdef CONFIG_LED_TRIGGERS
int led_set_trigger(enum led_trigger trigger, struct led *led);
void led_trigger(enum led_trigger trigger, enum trigger_type);
#else
static inline int led_set_trigger(enum led_trigger trigger, struct led *led)
{
	return 0;
}

static inline void led_trigger(enum led_trigger trigger, enum trigger_type type)
{
}
#endif

int led_get_trigger(enum led_trigger trigger);

/* gpio LED support */
struct gpio_led {
	int gpio;
	bool active_low;
	struct led led;
};

struct gpio_bicolor_led {
	int gpio_c0, gpio_c1;
	bool active_low;
	struct led led;
};

struct gpio_rgb_led {
	int gpio_r, gpio_g, gpio_b;
	bool active_low;
	struct led led;
};

#ifdef CONFIG_LED_GPIO
int led_gpio_register(struct gpio_led *led);
void led_gpio_unregister(struct gpio_led *led);
#else
static inline int led_gpio_register(struct gpio_led *led)
{
	return -ENOSYS;
}

static inline void led_gpio_unregister(struct gpio_led *led)
{
}
#endif

#ifdef CONFIG_LED_GPIO_BICOLOR
int led_gpio_bicolor_register(struct gpio_bicolor_led *led);
void led_gpio_bicolor_unregister(struct gpio_bicolor_led *led);
#else
static inline int led_gpio_bicolor_register(struct gpio_bicolor_led *led)
{
	return -ENOSYS;
}

static inline void led_gpio_bicolor_unregister(struct gpio_bicolor_led *led)
{
}
#endif

#ifdef CONFIG_LED_GPIO_RGB
int led_gpio_rgb_register(struct gpio_rgb_led *led);
void led_gpio_rgb_unregister(struct gpio_led *led);
#else
static inline int led_gpio_rgb_register(struct gpio_rgb_led *led)
{
	return -ENOSYS;
}

static inline void led_gpio_rgb_unregister(struct gpio_led *led)
{
}
#endif

#endif /* __LED_H */
