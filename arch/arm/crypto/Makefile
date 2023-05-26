# SPDX-License-Identifier: GPL-2.0-only
#
# Arch-specific CryptoAPI modules.
#

obj-$(CONFIG_DIGEST_SHA1_ARM) += sha1-arm.o
obj-$(CONFIG_DIGEST_SHA256_ARM) += sha256-arm.o

sha1-arm-y	:= sha1-armv4-large.o sha1_glue.o
sha256-arm-y	:= sha256-core.o sha256_glue.o

obj-$(CONFIG_DIGEST_SHA1_ARM64_CE) += sha1-ce.o
sha1-ce-y := sha1-ce-glue.o sha1-ce-core.o

obj-$(CONFIG_DIGEST_SHA256_ARM64_CE) += sha2-ce.o
sha2-ce-y := sha2-ce-glue.o sha2-ce-core.o

quiet_cmd_perl = PERL    $@
      cmd_perl = $(PERL) $(<) > $(@)

$(src)/sha256-core.S_shipped: $(src)/sha256-armv4.pl
	$(call cmd,perl)
