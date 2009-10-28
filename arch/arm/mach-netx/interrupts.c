#include <common.h>

#include <mach/netx-regs.h>

int timer_init (void)
{
	/* disable timer initially */
	GPIO_REG(GPIO_COUNTER_CTRL(0)) = 0;
	/* Reset the timer value to zero */
	GPIO_REG(GPIO_COUNTER_CURRENT(0)) = 0;
	GPIO_REG(GPIO_COUNTER_MAX(0)) = 0xffffffff;
	GPIO_REG(GPIO_COUNTER_CTRL(0)) = COUNTER_CTRL_RUN;

	return 0;
}

/* current timestamp in ms */
static unsigned long timestamp = 0;

static unsigned long last_timer = 0;

/* We can't detect overruns here since we don't know how often
 * we get called. The only thing we can do is to make sure that
 * time moves forward.
 */
ulong get_timer (ulong start)
{
	unsigned long cur_timer = GPIO_REG(GPIO_COUNTER_CURRENT(0));
	unsigned long time_inc;


	if(cur_timer > last_timer)
		time_inc = (cur_timer - last_timer) / 100000;
	else
		time_inc = ((0xffffffff - last_timer) + cur_timer) / 100000;

	if(time_inc)
		last_timer = cur_timer;

	timestamp += time_inc;

	return timestamp - start;
}

void mdelay(unsigned long msec)
{
	unsigned long now = get_timer(0);

	while( get_timer(0) < now + msec );
}

void udelay(unsigned long usec)
{
	unsigned long start, end, msec = usec / 1000;

	if(msec)
		mdelay(msec);

	usec -= msec * 1000;

	start = GPIO_REG(GPIO_COUNTER_CURRENT(0));
	end = start + usec * 100;

	if(end < start) {
		/* wait for overrun */
		while( GPIO_REG(GPIO_COUNTER_CURRENT(0)) > start);
	}

	while( GPIO_REG(GPIO_COUNTER_CURRENT(0)) < end);
}
