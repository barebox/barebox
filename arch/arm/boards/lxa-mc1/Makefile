# SPDX-License-Identifier: GPL-2.0-only

obj-y += board.o

# Overlay onto the upstream device tree, which barebox doesn't build in
overlay-y += stm32mp157c-lxa-mc1.dtbo
DTBO_BASE_stm32mp157c-lxa-mc1 := stm32mp157c-lxa-mc1-symbols

clean-files := *.dtb *.dtbo *.dtbo.S .*.dtso .*.dtc .*.pre .*.dts *-paths.h
