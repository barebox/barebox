#ifndef __STATE_H
#define __STATE_H

#include <of.h>

struct state;


struct state *state_new_from_node(struct device_node *node, bool readonly);
void state_release(struct state *state);

struct state *state_by_name(const char *name);
struct state *state_by_node(const struct device_node *node);

int state_load_no_auth(struct state *state);
int state_load(struct state *state);
int state_save(struct state *state);
void state_info(void);

int state_read_mac(struct state *state, const char *name, u8 *buf);

#endif /* __STATE_H */
