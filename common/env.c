#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>

static ulong env_size;
static char *env_name_spec;

struct variable_d {
        struct variable_d *next;
        char data[0];
};

#define VARIABLE_D_SIZE(name, value) (sizeof(struct variable_d) + strlen(name) + strlen(value) + 2)

static struct variable_d *env_list;

static char *var_val(struct variable_d *var)
{
        return &var->data[strlen(var->data) + 1];
}

static char *var_name(struct variable_d *var)
{
        return var->data;
}

char *getenv (const char *name)
{
        struct variable_d *var;

        if (!env_list) {
                printf("getenv(%s): not initialized\n",name);
                return NULL;
        }

        var = env_list->next;

        while (var) {
                if (!strcmp(var_name(var), name))
                        return var_val(var);
		var = var->next;
	}
	return 0;
}

void setenv (const char *name, const char *value)
{
        struct variable_d *var = env_list;
        struct variable_d *newvar = NULL;

        if (!env_list) {
                printf("setenv(%s): not initialized\n",name);
                return;
        }

        if (value) {
                newvar = malloc(VARIABLE_D_SIZE(name, value));
                if (!newvar) {
                        printf("cannot setenv: out of mem\n");
                        return;
                }
                strcpy(&newvar->data[0], name);
                strcpy(&newvar->data[strlen(name) + 1], value);
                newvar->next = NULL;
        }

        while (var->next) {
                if (!strcmp(var->next->data, name)) {
                        if (value) {
                                newvar->next = var->next->next;
                                free(var->next);
                                var->next = newvar;
                                return;
                        } else {
                                struct variable_d *tmp;
                                tmp = var->next;
                                var->next = var->next->next;
                                free(var->next);
                                return;
                        }
                }
                var = var->next;
        }
        var->next = newvar;
}

int add_env_spec(char *spec)
{
        char *env;
        char *data;
        int err;
        unsigned long crc;
        struct memarea_info info;

        printf("%s\n",__FUNCTION__);

        if (spec_str_to_info(spec, &info)) {
                printf("-ENOPARSE\n");
                return -1;
        }

        /* Do it the simple way for now: Assume that the whole
         * environment sector plus the environment list fits in
         * memory.
         */

        env_size = info.size;
        env_name_spec = spec;

        env = malloc(env_size);
        if (!env) {
                err = -ENOMEM;
                goto err_out;
        }

        env_list = malloc(sizeof(struct variable_d));
        if (!env_list) {
                err = -ENOMEM;
                goto err_out;
        }

        env_list->next = NULL;

        err = dev_read(info.device, env, env_size, info.start, 0);
        if (err != env_size)
                goto err_out;

        /* Make sure we don't go crazy with a corrupted environment */
        *(env + env_size - 1) = 0;
        *(env + env_size - 2) = 0;

        crc = *(ulong *)env;
        if (crc != crc32(0, env + sizeof(ulong), env_size - sizeof(ulong))) {
                printf("environment is corrupt\n");
                err = -ENODEV;
                goto err_out;
        }

        data = env + sizeof(env);

        while(*data) {
                char *delim = strchr(data, '=');
                int len = strlen(data);
                printf("data: %s\n", data);

                if (!delim)
                        goto err_out;

                *delim = 0;
                setenv(data, delim + 1);
                data += len + 1;
        }

        free(env);

        return 0;
err_out:
        printf("error\n");
	return -1;
}

int saveenv(void)
{
        struct variable_d *var = env_list->next;
        struct memarea_info info;
        unsigned char *env;
        int left, ret = 0, ofs;

        if (spec_str_to_info(env_name_spec, &info)) {
                printf("-ENOPARSE\n");
                return -1;
        }

        env = malloc(env_size);
        if (!env) {
                printf("out of memory\n");
                ret = -ENOMEM;
                goto err_out;
        }

        memset(env, 0, env_size);

        left = env_size - 2;
        ofs = 4;

        while (var) {
                int nlen = strlen(var_name(var));
                int vlen = strlen(var_val(var));

                if (ofs + nlen + 1 + vlen + 2 >= env_size) {
                        printf("no space left in environment\n");
                        ret = -ENOSPC;
                        goto err_out;
                }
                strcpy(env + ofs, var_name(var));
                ofs += nlen;
                *(env + ofs) = '=';
                ofs++;
                strcpy(env + ofs, var_val(var));
                ofs += vlen + 1;

                var = var->next;
        }

        *(ulong *)env = crc32(0, env + sizeof(ulong), env_size - sizeof(ulong));

        ret = dev_erase(info.device, info.size, info.start);
        if (ret) {
                printf("unable to erase\n");
                goto err_out;
        }

        ret = dev_write(info.device, env, info.size, info.start, 0);
        if (ret < 0) {
                printf("unable to write\n");
                goto err_out;
       }

        free(env);

        return 0;

err_out:
        if (env)
                free(env);

        return ret;
}

int do_printenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct variable_d *var = env_list->next;

	if (argc == 2) {
                char *val = getenv(argv[1]);
                if (val) {
                        printf("%s=%s\n", argv[1], val);
                        return 0;
                }
                printf("## Error: \"%s\" not defined\n", argv[1]);
                return -EINVAL;
        }

        while (var) {
                printf("%s=%s\n", var_name(var), var_val(var));
                var = var->next;
        }

        return 0;
}

U_BOOT_CMD(
	printenv, CONFIG_MAXARGS, 1,	do_printenv,
	"printenv- print environment variables\n",
	"\n    - print values of all environment variables\n"
	"printenv name ...\n"
	"    - print value of environment variable 'name'\n"
);

int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	setenv(argv[1], argv[2]);

        return 0;
}

U_BOOT_CMD(
	setenv, CONFIG_MAXARGS, 0,	do_setenv,
	"setenv  - set environment variables\n",
	"name value ...\n"
	"    - set environment variable 'name' to 'value ...'\n"
	"setenv name\n"
	"    - delete environment variable 'name'\n"
);

int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf ("Saving Environment to %s...\n", env_name_spec);

	return saveenv();
}

U_BOOT_CMD(
	saveenv, 1, 0,	do_saveenv,
	"saveenv - save environment variables to persistent storage\n",
	NULL
);

