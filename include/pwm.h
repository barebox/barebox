#ifndef __PWM_H
#define __PWM_H

struct pwm_device;
struct device_d;

/*
 * pwm_request - request a PWM device
 */
struct pwm_device *pwm_request(const char *pwmname);

/*
 * pwm_free - free a PWM device
 */
void pwm_free(struct pwm_device *pwm);

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

struct pwm_chip;

/**
 * struct pwm_ops - PWM operations
 * @request: optional hook for requesting a PWM
 * @free: optional hook for freeing a PWM
 * @config: configure duty cycles and period length for this PWM
 * @enable: enable PWM output toggling
 * @disable: disable PWM output toggling
 */
struct pwm_ops {
	int (*request)(struct pwm_chip *chip);
	void (*free)(struct pwm_chip *chip);
	int (*config)(struct pwm_chip *chip, int duty_ns,
						int period_ns);
	int (*enable)(struct pwm_chip *chip);
	void (*disable)(struct pwm_chip *chip);
};

/**
 * struct pwm_chip - abstract a PWM
 * @devname: unique identifier for this pwm
 * @ops: The callbacks for this PWM
 * @duty_ns: The duty cycle of the PWM, in nano-seconds
 * @period_ns: The period of the PWM, in nano-seconds
 */
struct pwm_chip {
	const char		*devname;
	struct pwm_ops		*ops;
	int			duty_ns;
	int			period_ns;
};

int pwmchip_add(struct pwm_chip *chip, struct device_d *dev);
int pwmchip_remove(struct pwm_chip *chip);

#endif /* __PWM_H */
