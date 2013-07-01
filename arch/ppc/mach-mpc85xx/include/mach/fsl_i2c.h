/*
 * Copyright 2013 GE Intelligent Platforms, Inc
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
 */
void fsl_i2c_init(void __iomem *i2c, int speed, int slaveadd);
int fsl_i2c_read(void __iomem *i2c, uint8_t dev, uint addr, int alen,
		uint8_t *data, int length);
void fsl_i2c_stop(void __iomem *i2c);
