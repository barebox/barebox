#ifndef __STATE_H
#define __STATE_H

struct state;

int state_backend_dtb_file(struct state *state, const char *path);
int state_backend_raw_file(struct state *state, const char *path,
		off_t offset, size_t size);

struct state *state_new_from_fdt(const char *name, void *fdt);
struct state *state_new_from_node(const char *name, struct device_node *node);

struct state *state_by_name(const char *name);
struct state *state_by_node(const struct device_node *node);
int state_get_name(const struct state *state, char const **name);

int state_load(struct state *state);
int state_save(struct state *state);
void state_info(void);

#endif /* __STATE_H */
