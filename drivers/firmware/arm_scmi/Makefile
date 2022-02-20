# SPDX-License-Identifier: GPL-2.0-only
scmi-bus-y = bus.o
scmi-driver-y = driver.o
scmi-transport-y = shmem.o
scmi-transport-$(CONFIG_ARM_SMCCC) += smc.o
scmi-protocols-y = base.o reset.o clock.o voltage.o

scmi-module-objs := $(scmi-bus-y) $(scmi-driver-y) $(scmi-protocols-y) \
		    $(scmi-transport-y)
obj-$(CONFIG_ARM_SCMI_PROTOCOL) += scmi-module.o
