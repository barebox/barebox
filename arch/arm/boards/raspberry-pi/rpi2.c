/*
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
 */

#include "rpi.h"

const struct rpi_model rpi_models[] = {
	RPI_MODEL(0, "Unknown model", NULL),
	RPI_MODEL(BCM2836_BOARD_REV_2_B, "2 Model B", rpi_b_plus_init),
};
const size_t rpi_models_size = ARRAY_SIZE(rpi_models);
const char *rpi_model_string = "(BCM2836/CORTEX-A7)";
