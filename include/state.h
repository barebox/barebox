#ifndef __STATE_H
#define __STATE_H

#include <of.h>

struct state;

#if IS_ENABLED(CONFIG_STATE)

struct state *state_new_from_node(struct device_node *node, bool readonly);
void state_release(struct state *state);

struct state *state_by_name(const char *name);
struct state *state_by_node(const struct device_node *node);

int state_load_no_auth(struct state *state);
int state_load(struct state *state);
int state_save(struct state *state);
void state_info(void);

int state_read_mac(struct state *state, const char *name, u8 *buf);

#else /* #if IS_ENABLED(CONFIG_STATE) */

static inline struct state *state_new_from_node(struct device_node *node,
						bool readonly)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct state *state_by_name(const char *name)
{
	return NULL;
}

static inline struct state *state_by_node(const struct device_node *node)
{
	return NULL;
};

static inline int state_load(struct state *state)
{
	return -ENOSYS;
}

static inline int state_save(struct state *state)
{
	return -ENOSYS;
}

static inline int state_read_mac(struct state *state, const char *name, u8 *buf)
{
	return -ENOSYS;
}

#endif /* #if IS_ENABLED(CONFIG_STATE) / #else */

#endif /* __STATE_H */
