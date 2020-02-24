
#include <mach/imx-header.h>
#include <mach/imx_cpu_types.h>

static inline int cpu_is_mx8m(const struct config_data *data)
{
	return data->cpu_type == IMX_CPU_IMX8MQ || data->cpu_type == IMX_CPU_IMX8MM;
}

int parse_config(struct config_data *data, const char *filename);
