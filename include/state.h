#ifndef __STATE_H
#define __STATE_H

#include <of.h>

struct state;

int state_backend_dtb_file(struct state *state, const char *of_path,
		const char *path);
int state_backend_raw_file(struct state *state, const char *of_path,
		const char *path, off_t offset, size_t size);

struct state *state_new_from_node(struct device_node *node, char *path,
				  off_t offset, size_t max_size, bool readonly);
void state_release(struct state *state);

struct state *state_by_name(const char *name);
struct state *state_by_node(const struct device_node *node);
int state_get_name(const struct state *state, char const **name);

int state_load(struct state *state);
int state_save(struct state *state);
void state_info(void);

#endif /* __STATE_H */
