/*
 * Copyright (C) 2009 Carlo Caione <carlo@carlocaione.org>
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
 */

#include "rpi.h"

static void rpi_b_init(void)
{
	rpi_leds[0].gpio = 16;
	rpi_leds[0].active_low = 1;
	rpi_set_usbethaddr();
}

/* See comments in mbox.h for data source */
const struct rpi_model rpi_models[] = {
	RPI_MODEL(0, "Unknown model", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C0_2, "Model B (no P5)", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C0_3, "Model B (no P5)", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_4, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_5, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_6, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_A_7, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_8, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_9, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_d, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_e, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_f, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_PLUS, "Model B+", rpi_b_plus_init),
	RPI_MODEL(BCM2835_BOARD_REV_CM, "Compute Module", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_PLUS, "Model A+", NULL),
};
const size_t rpi_models_size = ARRAY_SIZE(rpi_models);
const char *rpi_model_string = "(BCM2835/ARM1176JZF-S)";
