#!/bin/sh
#
# This is a wrapper for xz to compress the kernel image using appropriate
# compression options depending on the architecture.
#
# Author: Lasse Collin <lasse.collin@tukaani.org>
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

BCJ=
LZMA2OPTS=
XZ=${XZ:-xz}

case $SRCARCH in
	x86)            BCJ=--x86 ;;
	powerpc)        BCJ=--powerpc ;;
	ia64)           BCJ=--ia64; LZMA2OPTS=pb=4 ;;
	arm)            BCJ=--arm$S64 ;;
	sparc)          BCJ=--sparc ;;
esac

if grep -q '^CONFIG_THUMB2_BAREBOX=y$' include/config/auto.conf; then
	BCJ=--armthumb
fi

# clear BCJ filter if unsupported
if [ -n "${BCJ}" ]; then
	$XZ -H | grep -q -- $BCJ || BCJ=
fi

exec $XZ --check=crc32 $BCJ --lzma2=$LZMA2OPTS,dict=32MiB
