
BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME)).dtb.o
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB)

targets += dtbs
targets += $(dtb-y)

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files := *.dtb *.dtb.S
