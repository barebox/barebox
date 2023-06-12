#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only

CONTAINER=${CONTAINER:-ghcr.io/barebox/barebox/barebox-ci:latest}
export KCONFIG_ADD="test/kconfig/disable_target_tools.kconf $KCONFIG_ADD"

while getopts "c:uh" opt; do
	case "$opt" in
	h)
		echo "usage: $0 [-c CONTAINER] [command...]"
		echo "This script mounts the working directory into a container"
		echo "and runs the specified command inside or /bin/bash if no"
		echo "command was specified."
		echo "OPTIONS:"
		echo "  -c <container>    container image to use (default: $CONTAINER)"
		echo "  -u                pull container image updates"
		echo "  -h                This help text"

		exit 0
		;;
	u)
		update=1
		;;
	c)
		CONTAINER="$OPTARG"
		;;
	esac
done

shift $((OPTIND-1))

[ -n "$update" ] && podman pull "$CONTAINER"

pwd_real=$(realpath $PWD)

exec podman run -it -v "$PWD:$PWD:z" -v "$pwd_real:$pwd_real:z" --rm \
	-e TERM -e ARCH -e CONFIG -e JOBS -e LOGDIR -e REGEX \
	-e KCONFIG_ADD -w "$PWD" --userns=keep-id \
	-- "$CONTAINER" "${@:-/bin/bash}"
