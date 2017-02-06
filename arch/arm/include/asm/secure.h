#ifndef __ASM_ARM_SECURE_H
#define __ASM_ARM_SECURE_H

#ifndef __ASSEMBLY__

int armv7_secure_monitor_install(void);
int __armv7_secure_monitor_install(void);
void armv7_switch_to_hyp(void);

extern unsigned char secure_monitor_init_vectors[];

enum arm_security_state {
	ARM_STATE_SECURE,
	ARM_STATE_NONSECURE,
	ARM_STATE_HYP,
};

#ifdef CONFIG_ARM_SECURE_MONITOR
enum arm_security_state bootm_arm_security_state(void);
const char *bootm_arm_security_state_name(enum arm_security_state state);
#else
static inline enum arm_security_state bootm_arm_security_state(void)
{
	return ARM_STATE_SECURE;
}

static inline const char *bootm_arm_security_state_name(
	enum arm_security_state state)
{
	return "secure";
}
#endif

#endif /* __ASSEMBLY__ */

#define ARM_SECURE_STACK_SHIFT		10
#define ARM_SECURE_MAX_CPU		8

#endif /* __ASM_ARM_SECURE_H */
