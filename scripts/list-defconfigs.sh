#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Output json formatted defconfig list for Github Action consumption

ARCH=${@:-*}

cd arch

archs=$(for arch in $ARCH; do
	ls -1 $arch/configs | xargs -i printf '{ "arch": "%s", "config": "%s" }\n' \
		"$arch" "{}" | paste -sd ',' -
done | paste -sd ',' -)

echo '{ "include" : '" [ $archs ] }"
