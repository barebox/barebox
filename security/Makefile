# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_SECURITY_POLICY)		+= policy.o
obj-$(CONFIG_SECURITY_POLICY_NAMES)	+= sconfig_names.o
obj-$(CONFIG_CRYPTO_KEYSTORE)	+= keystore.o
obj-$(CONFIG_JWT)		+= jwt.o
obj-pbl-$(CONFIG_HAVE_OPTEE)	+= optee.o
obj-$(CONFIG_BLOBGEN)		+= blobgen.o
obj-$(CONFIG_PASSWORD)		+= password.o

ifdef CONFIG_PASSWORD

ifeq ($(CONFIG_PASSWORD_DEFAULT),"")
define filechk_passwd
	echo "static const char default_passwd[] = \"\";"
endef
else
define filechk_passwd
(							\
echo -n "static const char default_passwd[] = \"";	\
cat $(shell find $(srctree) -name $(CONFIG_PASSWORD_DEFAULT) -type f) | tr -d '\n';\
echo "\";"						\
)
endef
endif

include/generated/passwd.h: FORCE
	$(call filechk,passwd)

$(obj)/password.o: include/generated/passwd.h
endif # CONFIG_PASSWORD
