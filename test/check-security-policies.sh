#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Exercise the standalone security policy configurator targets
#
# Usage: test/check-security-policies.sh DEFCONFIG [NPOLICIES]
#
# The security_%config targets collect the policies themselves instead
# of taking them from a build, so run them in a fresh tree, both
# in-tree and out-of-tree, before anything else has generated policy
# lists.

set -e

defconfig="$1"
npolicies="$2"

if [ -z "$defconfig" ]; then
	echo >&2 "usage: $0 DEFCONFIG [NPOLICIES]"
	exit 2
fi

cd "$(dirname "$0")/.."

# Leftovers from an earlier build would be taken for collected policies
# and hide a collector that no longer runs, so start from a clean tree.
make mrproper

builddir="$(mktemp -d -p . build-sconfig.XXXXXX)"
policies="$(mktemp)"
trap 'rm -rf "$builddir" "$policies"' EXIT

for O in "" "$builddir"; do
	objtree="${O:-.}"

	make O="$O" "$defconfig"

	# security_listconfigs exits successfully on an empty list, so
	# count the policies instead of trusting the exit code.
	make O="$O" security_listconfigs >"$policies"
	cat "$policies"
	n="$(grep -c '\.sconfig$' "$policies" || :)"
	if [ "$n" -eq 0 ]; then
		echo >&2 "$0: no policies collected"
		exit 1
	fi
	if [ -n "$npolicies" ] && [ "$n" -ne "$npolicies" ]; then
		echo >&2 "$0: collected $n policies, expected $npolicies"
		exit 1
	fi

	# Fails as well when a committed .sconfig is out of date with Sconfig.
	make O="$O" security_checkconfigs

	# The only target here that goes through the security_%config rule
	# and thus the config-build dispatch. stdin is redirected, which
	# makes the frontend take defaults instead of prompting.
	make O="$O" security_oldconfig </dev/null

	# Configuring policies must not drag a build along: the .sconfig
	# files are preprocessed as they are, so neither the policies
	# themselves nor anything else, common/version.o here standing in
	# for the rest of the tree, is compiled.
	for o in common/version.o $(sed -n 's/\.sconfig$/.sconfig.o/p' "$policies"); do
		if [ -e "$objtree/$o" ]; then
			echo >&2 "$0: configuring policies built $o"
			exit 1
		fi
	done

	make O="$O" mrproper
done

# The CI container runs as root, while the checkout belongs to the user
# the runner uses, so git refuses the repository over its ownership. git
# diff doesn't fail loudly in that case: it falls back to --no-index and
# exits 129 with a usage message instead of comparing anything.
srctree_git() {
	git -c safe.directory="$PWD" "$@"
}

if ! srctree_git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
	echo >&2 "$0: not a git work tree, cannot look for modified policies"
	exit 1
fi

# Catches security_oldconfig rewriting a committed policy.
srctree_git diff --exit-code -- '*.sconfig'
