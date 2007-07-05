#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>

struct variable_d {
        struct variable_d *next;
        char data[0];
};

#define VARIABLE_D_SIZE(name, value) (sizeof(struct variable_d) + strlen(name) + strlen(value) + 2)

static struct variable_d _env_list;
static struct variable_d *env_list = &_env_list;

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

	if (strchr(name, '.')) {
		char *ret = 0;
		char *devstr = strdup(name);
		char *par = strchr(devstr, '.');
		struct device_d *dev;
		*par = 0;
		dev = get_device_by_id(devstr);
		if (dev) {
				par++;
				ret = dev_get_param(dev, par);
		}
		free(devstr);
		return ret;
	}

        var = env_list->next;

        while (var) {
                if (!strcmp(var_name(var), name))
                        return var_val(var);
		var = var->next;
	}
	return 0;
}

int setenv (const char *name, const char *value)
{
        struct variable_d *var = env_list;
        struct variable_d *newvar = NULL;
	char *par;

	if ((par = strchr(name, '.'))) {
		struct device_d *dev;
		*par++ = 0;
		dev = get_device_by_id(name);
		if (dev) {
			int ret = dev_set_param(dev, par, value);
			if (ret < 0)
				perror("set parameter");
			errno = 0;
		} else {
		    errno = -ENODEV;
		    perror("set parameter");
                }
		return errno;
	}

        if (value) {
                newvar = xzalloc(VARIABLE_D_SIZE(name, value));
                strcpy(&newvar->data[0], name);
                strcpy(&newvar->data[strlen(name) + 1], value);
        }

        while (var->next) {
                if (!strcmp(var->next->data, name)) {
                        if (value) {
                                newvar->next = var->next->next;
                                free(var->next);
                                var->next = newvar;
                                return 0;
                        } else {
                                struct variable_d *tmp;
                                tmp = var->next;
                                var->next = var->next->next;
                                free(var->next);
                                return 0;
                        }
                }
                var = var->next;
        }
        var->next = newvar;

	return 0;
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

U_BOOT_CMD_START(printenv)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_printenv,
	.usage		= "printenv- print environment variables\n",
	U_BOOT_CMD_HELP(
	"\n    - print values of all environment variables\n"
	"printenv name ...\n"
	"    - print value of environment variable 'name'\n")
U_BOOT_CMD_END

#ifdef CONFIG_SIMPLE_PARSER
int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	setenv(argv[1], argv[2]);

        return 0;
}

U_BOOT_CMD_START(setenv)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_setenv,
	.usage		= "setenv  - set environment variables\n",
	U_BOOT_CMD_HELP(
	"name value ...\n"
	"    - set environment variable 'name' to 'value ...'\n"
	"setenv name\n"
	"    - delete environment variable 'name'\n"
U_BOOT_CMD_END

#endif

