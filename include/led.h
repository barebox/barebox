#ifndef __LED_H
#define __LED_H

#include <linux/list.h>

#include <errno.h>
#include <of.h>

struct led {
	void (*set)(struct led *, unsigned int value);
	int max_value;
	char *name;
	int num;
	struct list_head list;

	int blink;
	int flash;
	unsigned int *blink_states;
	int blink_nr_states;
	int blink_next_state;
	uint64_t blink_next_event;
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
int led_blink(struct led *led, unsigned int on_ms, unsigned int off_ms);
int led_blink_pattern(struct led *led, const unsigned int *pattern,
		      unsigned int pattern_len);
int led_flash(struct led *led, unsigned int duration_ms);
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
	LED_TRIGGER_DEFAULT_ON,
	LED_TRIGGER_MAX,
	LED_TRIGGER_INVALID = LED_TRIGGER_MAX,
};

enum trigger_type {
	TRIGGER_ENABLE,
	TRIGGER_DISABLE,
	TRIGGER_FLASH,
};

#ifdef CONFIG_LED_TRIGGERS
int led_set_trigger(enum led_trigger trigger, struct led *led);
void led_trigger_disable(struct led *led);
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

void led_triggers_show_info(void);
const char *trigger_name(enum led_trigger trigger);
enum led_trigger trigger_by_name(const char *name);

void led_of_parse_trigger(struct led *led, struct device_node *np);

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
void led_gpio_rgb_unregister(struct gpio_rgb_led *led);
#else
static inline int led_gpio_rgb_register(struct gpio_rgb_led *led)
{
	return -ENOSYS;
}

static inline void led_gpio_rgb_unregister(struct gpio_rgb_led *led)
{
}
#endif

#endif /* __LED_H */
