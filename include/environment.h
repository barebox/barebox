/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_


#ifdef __BAREBOX__
/**
 * Managment of a environment variable
 */
struct variable_d {
	struct variable_d *next;	/**< List management */
	char data[0];			/**< variable length data */
};

struct env_context {
	struct env_context *parent;	/**< FIXME */
	struct variable_d *local;	/**< FIXME */
	struct variable_d *global;	/**< FIXME */
};

struct env_context *get_current_context(void);
char *var_val(struct variable_d *);
char *var_name(struct variable_d *);

#ifdef CONFIG_ENVIRONMENT_VARIABLES
const char *getenv(const char *);
int setenv(const char *, const char *);
#else
static inline char *getenv(const char *var)
{
	return NULL;
}

static inline int setenv(const char *var, const char *val)
{
	return 0;
}
#endif

int env_pop_context(void);
int env_push_context(void);

/* defaults to /dev/env0 */
extern char *default_environment_path;

int envfs_load(char *filename, char *dirname);
int envfs_save(char *filename, char *dirname);

int export(const char *);

struct stat;
int file_size_action(const char *, struct stat *, void *, int);
int file_save_action(const char *, struct stat *, void *, int);

#endif /* __BAREBOX__ */

/* This part is used for the host and the target */
struct action_data {
	int fd;
	const char *base;
	void *writep;
};
#define PAD4(x) ((x + 3) & ~3)

#endif	/* _ENVIRONMENT_H_ */

/**
 * @file
 * @brief Environment handling
 *
 * Important: This file will also be used on the host to create
 * the default environment when building the barebox binary.
 */
