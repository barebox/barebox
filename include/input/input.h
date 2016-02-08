#ifndef __INPUT_H
#define __INPUT_H

#include <linux/types.h>
#include <linux/list.h>
#include <dt-bindings/input/linux-event-codes.h>

struct input_event {
	uint16_t code;
	uint16_t value;
};

struct input_device {
	struct list_head list;
	DECLARE_BITMAP(keys, KEY_CNT);
};

void input_report_key_event(struct input_device *idev, unsigned int code, int value);

int input_device_register(struct input_device *);
void input_device_unregister(struct input_device *);

void input_key_get_status(unsigned long *keys, int bits);

struct input_notifier {
	void (*notify)(struct input_notifier *in, struct input_event *event);
	struct list_head list;
};

int input_register_notfier(struct input_notifier *in);
void input_unregister_notfier(struct input_notifier *in);

#endif /* __INPUT_H */

