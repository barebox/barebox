# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
ifneq ($(BUILTIN_DTB),)
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o
endif

obj-$(CONFIG_BOARD_K200) += k200.dtb.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts
