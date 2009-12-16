obj-y	+= armlinux.o
obj-y	+= _ashldi3.o
obj-y	+= _ashrdi3.o
obj-y	+= cache.o
obj-y	+= div0.o
obj-y	+= _divsi3.o
obj-y	+= _modsi3.o
obj-y	+= _udivsi3.o
obj-y	+= _umodsi3.o
obj-y	+= _lshrdi3.o
obj-y	+= findbit.o
obj-y	+= arm.o
obj-y	+= io-readsb.o
obj-y	+= io-readsw-armv4.o
obj-y	+= io-readsl.o
obj-y	+= io-writesb.o
obj-y	+= io-writesw-armv4.o
obj-y	+= io-writesl.o
obj-$(CONFIG_AEABI)				+= __aeabi_idivmod.o
obj-$(CONFIG_AEABI)				+= __aeabi_uidivmod.o
obj-$(CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS)	+= memcpy.o
obj-$(CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS)	+= memset.o

obj-$(CONFIG_MODULES) += module.o
extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds

