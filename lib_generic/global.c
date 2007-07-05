#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <asm-generic/errno.h>

static struct device_d global_dev;

int global_add_parameter(struct param_d *param)
{
        return dev_add_parameter(&global_dev, param);
}

static int my_probe(struct device_d *dev)
{
	return 0;
}

static struct device_d global_dev = {
        .name  = "global",
	.id    = "env",
        .map_base = 0,
        .size   = 0,
};

static struct driver_d global_driver = {
        .name  = "global",
        .probe = my_probe,
};

static int global_init(void)
{
        global_driver.probe = my_probe;
	register_device(&global_dev);
        register_driver(&global_driver);
        return 0;
}

coredevice_initcall(global_init);

static int do_get( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev;
        char *endp, *str;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], &endp);
        if (!dev) {
                printf("no such device: %s\n", argv[1]);
                return 1;
        }

        str = dev_get_param(dev, argv[2]);
        printf("%s\n",str);
        return 0;
}

U_BOOT_CMD(
	get,     3,     0,      do_get,
	"get      - get device variables\n",
	"\n"
);

static int do_set( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev;
        char *endp, *val;
        int ret;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], &endp);
        if (!dev) {
                printf("no such device: %s\n", argv[1]);
                return 1;
        }

        val = malloc(strlen(argv[3] + 1));
        if (!val)
                return -ENOMEM;

        strcpy(val, argv[3]);
        ret = dev_set_param(dev, argv[2], val);
        if (ret)
                printf("failed\n");
        return ret;
}

U_BOOT_CMD(
	set,     4,     0,      do_set,
	"set      - set device variables\n",
	"\n"
);

