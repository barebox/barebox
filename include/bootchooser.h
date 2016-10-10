#ifndef __BOOTCHOOSER_H
#define __BOOTCHOOSER_H

#include <of.h>
#include <boot.h>

struct bootchooser;
struct bootchooser_target;

struct bootchooser *bootchooser_get(void);
int bootchooser_save(struct bootchooser *bootchooser);
int bootchooser_put(struct bootchooser *bootchooser);

void bootchooser_info(struct bootchooser *bootchooser);

struct bootchooser_target *bootchooser_get_last_chosen(struct bootchooser *bootchooser);
const char *bootchooser_target_name(struct bootchooser_target *target);
struct bootchooser_target *bootchooser_target_by_name(struct bootchooser *bootchooser,
						      const char *name);
void bootchooser_target_force_boot(struct bootchooser_target *target);

int bootchooser_create_bootentry(struct bootentries *entries);

int bootchooser_target_set_attempts(struct bootchooser_target *target, int attempts);
int bootchooser_target_set_priority(struct bootchooser_target *target, int priority);

void bootchooser_last_boot_successful(void);

struct bootchooser_target *bootchooser_target_first(struct bootchooser *bootchooser);
struct bootchooser_target *bootchooser_target_next(struct bootchooser *bootchooser,
					       struct bootchooser_target *cur);

#define bootchooser_for_each_target(bootchooser, target) \
	for (target = bootchooser_target_first(bootchooser); target; \
	     target = bootchooser_target_next(bootchooser, target))

#endif /* __BOOTCHOOSER_H */
