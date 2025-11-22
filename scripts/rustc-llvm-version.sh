#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/scripts/rustc-llvm-version.sh?id=8d3ae59288f1e7d58d76558a6ee96d533bc5019f
#
# Usage: $ ./rustc-llvm-version.sh rustc
#
# Print the LLVM version that the Rust compiler uses in a 6 digit form.

# Convert the version string x.y.z to a canonical up-to-6-digits form.
get_canonical_version()
{
	IFS=.
	set -- $1
	echo $((10000 * $1 + 100 * $2 + $3))
}

if output=$("$@" --version --verbose 2>/dev/null | grep -E 'LLVM.*[0-9]+\.[0-9]+\.[0-9]+'); then
	set -- $output
	get_canonical_version $3
else
	echo 0
	exit 1
fi
