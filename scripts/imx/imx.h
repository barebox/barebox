
#include <imx/imx-header.h>
#include <imx/imx_cpu_types.h>

static inline int cpu_is_mx8m(const struct config_data *data)
{
	switch (data->cpu_type) {
	case IMX_CPU_IMX8MQ:
	case IMX_CPU_IMX8MM:
	case IMX_CPU_IMX8MN:
	case IMX_CPU_IMX8MP:
		return true;
	default:
		return false;
	}
}

static inline bool flexspi_image(const struct config_data *data)
{
	/*
	 *           | FlexSPI-FCFB  | FlexSPI-IVT
	 * -----------------------------------------
	 * i.MX8MM   |   0x0         |  0x1000
	 * i.MX8MN/P |   0x400       |  0x0
	 */

	return data->image_flexspi_ivt_offset ||
	       data->image_flexspi_fcfb_offset;
}

int parse_config(struct config_data *data, const char *filename);
