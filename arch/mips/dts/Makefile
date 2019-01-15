BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
ifneq ($(BUILTIN_DTB),)
obj-dtb-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o
endif

pbl-dtb-$(CONFIG_BOARD_8DEVICES_LIMA) += qca4531-8devices-lima.dtb.o
pbl-dtb-$(CONFIG_BOARD_BLACK_SWIFT) += black-swift.dtb.o
pbl-dtb-$(CONFIG_BOARD_CI20) += img-ci20.dtb.o
pbl-dtb-$(CONFIG_BOARD_DLINK_DIR320) += dlink-dir-320.dtb.o
pbl-dtb-$(CONFIG_BOARD_DPTECHNICS_DPT_MODULE) += ar9331-dptechnics-dpt-module.dtb.o
pbl-dtb-$(CONFIG_BOARD_LOONGSON_TECH_LS1B) += loongson-ls1b.dtb.o
pbl-dtb-$(CONFIG_BOARD_QEMU_MALTA) += qemu-malta.dtb.o
pbl-dtb-$(CONFIG_BOARD_RZX50) += rzx50.dtb.o
pbl-dtb-$(CONFIG_BOARD_TPLINK_MR3020) += tplink-mr3020.dtb.o
pbl-dtb-$(CONFIG_BOARD_TPLINK_WDR4300) += ar9344-tl-wdr4300-v1.7.dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.lzo
