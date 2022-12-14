// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BOCHS_HW_H
#define BOCHS_HW_H

#include <linux/compiler.h>

#define VBE_DISPI_IOPORT_INDEX           0x01CE
#define VBE_DISPI_IOPORT_DATA            0x01CF

struct device;

int bochs_hw_probe(struct device *dev, void *fb_map, void __iomem *mmio);

#endif
