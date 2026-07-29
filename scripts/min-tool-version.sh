#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Print the minimum supported version of the given tool.
# When you raise the minimum version, please update
# Documentation/process/changes.rst as well.

set -e

if [ $# != 1 ]; then
	echo "Usage: $0 toolname" >&2
	exit 1
fi

case "$1" in
binutils)
	echo 2.30.0
	;;
gcc)
	echo 8.1.0
	;;
llvm)
	echo 15.0.0
	;;
*)
	echo "$1: unknown tool" >&2
	exit 1
	;;
esac
