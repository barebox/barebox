#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>

static struct device_d global_dev;

int global_add_parameter(struct param_d *param)
{
        return dev_add_parameter(&global_dev, param);
}

static struct device_d global_dev = {
        .name  = "global",
	.id    = "env",
        .map_base = 0,
        .size   = 0,
};

static struct driver_d global_driver = {
        .name  = "global",
        .probe = dummy_probe,
};

static int global_init(void)
{
	register_device(&global_dev);
        register_driver(&global_driver);
        return 0;
}

coredevice_initcall(global_init);

static int do_get( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev;
	struct param_d *param;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], NULL);
        if (!dev) {
                printf("no such device: %s\n", argv[1]);
                return 1;
        }

        param = dev_get_param(dev, argv[2]);
	printf("%s\n", param->value);
	printf("\n");

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
        char *endp;
        int ret;
	struct param_d *param;

	if (argc < 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], &endp);
        if (!dev) {
                printf("no such device: %s\n", argv[1]);
                return 1;
        }

	param = get_param_by_name(dev, argv[2]);
	if (!param) {
		printf("device %s has no parameter %s\n", dev->id, argv[2]);
		return 1;
	}

	ret = dev_set_param(dev, argv[2], argv[3]);

        if (ret)
                perror("set parameter");
        return ret;
}

U_BOOT_CMD(
	set,     4,     0,      do_set,
	"set      - set device variables\n",
	"\n"
);

