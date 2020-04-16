/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PWM_H
#define __PWM_H

#include <dt-bindings/pwm/pwm.h>

struct pwm_device;
struct device_d;

#define	PWM_POLARITY_NORMAL	0

/*
 * struct pwm_state - state of a PWM channel
 * @period_ns: PWM period (in nanoseconds)
 * @duty_ns: PWM duty cycle (in nanoseconds)
 * @polarity: PWM polarity
 * @p_enable: PWM enabled status
 */
struct pwm_state {
	unsigned int period_ns;
	unsigned int duty_ns;
	unsigned int polarity;
	unsigned int p_enable;
};

/*
 * pwm_request - request a PWM device
 */
struct pwm_device *pwm_request(const char *pwmname);

struct pwm_device *of_pwm_request(struct device_node *np, const char *con_id);

/*
 * pwm_free - free a PWM device
 */
void pwm_free(struct pwm_device *pwm);

/*
 * pwm_init_state - prepare a new state from device tree args
 */
void pwm_init_state(const struct pwm_device *pwm,
		    struct pwm_state *state);

/*
 * pwm_config - change a PWM device configuration
 */
int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state);

/*
 * pwm_config - change a PWM device configuration
 */
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns);

/*
 * pwm_enable - start a PWM output toggling
 */
int pwm_enable(struct pwm_device *pwm);

/*
 * pwm_disable - stop a PWM output toggling
 */
void pwm_disable(struct pwm_device *pwm);

unsigned int pwm_get_period(struct pwm_device *pwm);

struct pwm_chip;

/**
 * pwm_get_state() - retrieve the current PWM state
 * @pwm: PWM device
 * @state: state to fill with the current PWM state
 */
void pwm_get_state(const struct pwm_device *pwm, struct pwm_state *state);

/**
 * pwm_apply_state() - apply the passed PWM state
 * @pwm: PWM device
 * @state: state to apply to pwm device
 */
int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state);

/**
 * struct pwm_ops - PWM operations
 * @request: optional hook for requesting a PWM
 * @free: optional hook for freeing a PWM
 * @apply: apply specified pwm state
 */
struct pwm_ops {
	int (*request)(struct pwm_chip *chip);
	void (*free)(struct pwm_chip *chip);
	int (*apply)(struct pwm_chip *chip, const struct pwm_state *state);
};

/**
 * struct pwm_chip - abstract a PWM
 * @id: The id of this pwm
 * @devname: unique identifier for this pwm
 * @ops: The callbacks for this PWM
 * @state: current state of the PWM
 */
struct pwm_chip {
	int			id;
	const char		*devname;
	const struct pwm_ops	*ops;

	struct pwm_state	state;
};

int pwmchip_add(struct pwm_chip *chip, struct device_d *dev);
int pwmchip_remove(struct pwm_chip *chip);

#endif /* __PWM_H */
