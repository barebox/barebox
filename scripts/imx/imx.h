
#include <mach/imx-header.h>
#include <mach/imx_cpu_types.h>

static inline int cpu_is_mx8m(const struct config_data *data)
{
	switch (data->cpu_type) {
	case IMX_CPU_IMX8MQ:
	case IMX_CPU_IMX8MM:
	case IMX_CPU_IMX8MP:
		return true;
	default:
		return false;
	}
}

int parse_config(struct config_data *data, const char *filename);
