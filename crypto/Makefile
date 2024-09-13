# SPDX-License-Identifier: GPL-2.0-only

obj-pbl-$(CONFIG_CRC32)	+= crc32.o
obj-pbl-$(CONFIG_CRC_ITU_T)	+= crc-itu-t.o
obj-$(CONFIG_CRC7)	+= crc7.o
obj-$(CONFIG_DIGEST)	+= digest.o
obj-$(CONFIG_DIGEST_CRC32_GENERIC)	+= crc32_digest.o
obj-$(CONFIG_DIGEST_HMAC_GENERIC)	+= hmac.o
obj-$(CONFIG_DIGEST_MD5_GENERIC)	+= md5.o
obj-$(CONFIG_DIGEST_SHA1_GENERIC)	+= sha1.o
obj-$(CONFIG_DIGEST_SHA224_GENERIC)	+= sha2.o
obj-$(CONFIG_DIGEST_SHA256_GENERIC)	+= sha2.o
pbl-y					+= sha2.o digest.o
obj-$(CONFIG_DIGEST_SHA384_GENERIC)	+= sha4.o
obj-$(CONFIG_DIGEST_SHA512_GENERIC)	+= sha4.o
obj-y	+= memneq.o

obj-$(CONFIG_CRYPTO_PBKDF2)	+= pbkdf2.o
obj-$(CONFIG_CRYPTO_RSA)	+= rsa.o
obj-$(CONFIG_CRYPTO_KEYSTORE)	+= keystore.o
obj-$(CONFIG_CRYPTO_BUILTIN_KEYS)      += public-keys.o

obj-$(CONFIG_JWT)		+= jwt.o

extra-$(CONFIG_CRYPTO_BUILTIN_KEYS) += public-keys.h

ifdef CONFIG_CRYPTO_BUILTIN_KEYS

$(obj)/public-keys.o: $(obj)/public-keys.h

CONFIG_CRYPTO_PUBLIC_KEYS := $(shell echo $(CONFIG_CRYPTO_PUBLIC_KEYS))
CONFIG_CRYPTO_PUBLIC_KEYS := $(foreach d,$(CONFIG_CRYPTO_PUBLIC_KEYS),"$(d)")

$(obj)/public-keys.h: FORCE
	$(call cmd,public_keys,$(CONFIG_CRYPTO_PUBLIC_KEYS))

endif
