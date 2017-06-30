# Font handling

font-objs := fonts.o

font-objs-$(CONFIG_FONT_8x16)      += font_8x16.o
font-objs-$(CONFIG_FONT_8x8)       += font_8x8.o
font-objs-$(CONFIG_FONT_7x14)      += font_7x14.o
font-objs-$(CONFIG_FONT_MINI_4x6)  += font_mini_4x6.o
font-objs-$(CONFIG_FONT_CUSTOM_16X)+= font_custom_16x.o
font-objs-$(CONFIG_FONT_6x8)       += font_6x8.o

font-objs += $(font-objs-y)

obj-$(CONFIG_FONTS)         += font.o
