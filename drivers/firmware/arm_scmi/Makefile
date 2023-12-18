# SPDX-License-Identifier: GPL-2.0-only
scmi-bus-y = bus.o
scmi-core-objs := $(scmi-bus-y)

scmi-driver-y = driver.o
scmi-transport-$(CONFIG_ARM_SCMI_HAVE_SHMEM) = shmem.o
scmi-transport-$(CONFIG_ARM_SCMI_TRANSPORT_SMC) += smc.o
scmi-transport-$(CONFIG_ARM_SCMI_HAVE_MSG) += msg.o
scmi-transport-$(CONFIG_ARM_SCMI_TRANSPORT_OPTEE) += optee.o
scmi-protocols-y = base.o clock.o power.o reset.o sensors.o voltage.o
scmi-module-objs := $(scmi-driver-y) $(scmi-protocols-y) $(scmi-transport-y)

obj-$(CONFIG_ARM_SCMI_PROTOCOL) += scmi-core.o
obj-$(CONFIG_ARM_SCMI_PROTOCOL) += scmi-module.o

obj-$(CONFIG_ARM_SCMI_POWER_DOMAIN) += scmi_pm_domain.o
