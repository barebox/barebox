/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QEMU_VIRT_COMMANDLINE_H_
#define QEMU_VIRT_COMMANDLINE_H_

struct device_node;

int qemu_virt_parse_commandline(struct device_node *root);

#endif
