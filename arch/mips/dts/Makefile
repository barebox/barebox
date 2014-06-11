
BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

dtb-y += ${BUILTIN_DTB}.dtb

.SECONDARY: $(obj)/$(BUILTIN_DTB).dtb.S
.SECONDARY: $(patsubst %,$(obj)/%.S,$(dtb-y))

targets += dtbs
targets += $(dtb-y)

extra-y += $(dtb-y)

clean-files := *.dtb *.dtb.S
