obj-y += cpu.o
obj-y += interrupts.o

#
# start-arm920t.S and start-arm926ejs.S have been
# merged to start-arm920t.S. More start-*.S should
# be merged here and finally called start-arm.S
#
obj-$(CONFIG_ARM920T) += start-arm920t.o
obj-$(CONFIG_ARM926EJS) += start-arm920t.o
