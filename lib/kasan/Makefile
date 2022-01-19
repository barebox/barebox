# SPDX-License-Identifier: GPL-2.0-only

obj-y += generic_report.o generic.o report.o common.o test_kasan.o
KASAN_SANITIZE_generic_report.o := n
KASAN_SANITIZE_generic.o := n
KASAN_SANITIZE_report.o := n
KASAN_SANITIZE_common.o := n

CC_FLAGS_KASAN_RUNTIME := $(call cc-option, -fno-conserve-stack)
CC_FLAGS_KASAN_RUNTIME += -fno-stack-protector

CFLAGS_generic_report.o := $(CC_FLAGS_KASAN_RUNTIME)
CFLAGS_generic.o := $(CC_FLAGS_KASAN_RUNTIME)
CFLAGS_report.o := $(CC_FLAGS_KASAN_RUNTIME)
CFLAGS_common.o := $(CC_FLAGS_KASAN_RUNTIME)
