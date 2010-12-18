#ifndef __LED_H
#define __LED_H

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

#endif /* __LED_H */
