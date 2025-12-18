#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

if [ "$#" -lt 2 ]; then
    2>&1 echo "USAGE: $0 OUTPUT INPUT..."
    exit 2
fi

${LD} -shared --gc-sections --whole-archive -o "$@" || {
    2>&1 echo
    2>&1 echo "Link check failed. Relocations to outside EFI runtime code?"
    exit 1;
}
