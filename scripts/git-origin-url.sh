#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-0-Clause

set -euo pipefail

# Some very early hashes to identify the projects by
declare -A repos=(
    ["a3ffa97f40dc81f2d6b07ee964f2340fe0c1ba97"]="https://git.pengutronix.de/cgit/barebox"
    ["1da177e4c3f41524e886b7f1b8a0c1fc7321cac2"]="https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"
    ["cc8ed39b240180b58810784f844e253263594ac3"]="https://git.busybox.net/busybox"
    ["7e492d8258182e31c988bbf9917d4a3d41949d56"]="https://github.com/u-boot/u-boot"
    ["afe1be4dbdf142513f6ac1d92e6a20bdc4b20c80"]="https://github.com/systemd/systemd"
)

err() { echo "error: $* " >&2; exit 1; }
usage="Usage: $0 [-s] <path-to-remote-file> [local-file]

Generate an Origin-URL comment from a remote file with the assumption
that it is in a git repository checked out at the correct version
from which the code in question was originally copied."

snippet=""
while getopts "sh" opt; do
    case "$opt" in
        s)
            snippet="Snippet-"
            ;;
        h)
            echo "$usage"
            exit 0
            ;;
        *)
            err "$usage"
            ;;
	esac
done

shift $((OPTIND-1))

if [ $# -ne 1 ] && [ $# -ne 2 ]; then
    err "$usage"
fi

# Optional second argument to allow e.g. !git origin-url $remote_file % im vim
# In cases where local file has different extension then the original
dstbasename="$(basename ${2:-$1})"

[ -e "$1" ] || err "File not found"

filepath="$(realpath "$1")"

repo_root="$(git -C "$(dirname "$filepath")" rev-parse --show-toplevel 2>/dev/null \
    || err "Not inside a Git repository: $filepath")"

rel_path="$(realpath --relative-to="$repo_root" "$filepath")"

# newest commit that touched this file
file_commit="$(git -C "$repo_root" log -n1 --format=%H -- "$rel_path")"

origin_url=""

# find which repo this file ultimately came from
for commit in "${!repos[@]}"; do
    if git -C "$repo_root" merge-base --is-ancestor "$commit" "$file_commit" 2>/dev/null; then
        origin_url="${repos[$commit]}"
        break
    fi
done

[ -z "$origin_url" ] && err "No matching repo base found for file commit $file_commit"

# output uses the newest file commit, not the origin commit
if [[ $origin_url == https://github.com/* ]] ; then
    tree_url="$origin_url/blob/$file_commit/$rel_path"
else
    # assume cgit as fallback
    tree_url="$origin_url/tree/$rel_path?id=$file_commit"
fi

case "${dstbasename##*.}" in
    h|S|imxcfg|y|l|lds)
        comment_begin='/* '
        comment_end=' */'
        ;;
    c|dts*)
        comment_begin='// '
        comment_end=''
        ;;
    *)
        comment_begin='# '
        comment_end=''
        ;;
esac

comment() { echo "${comment_begin}${*}${comment_end}"; }

[ -z "${snippet}" ] || comment "SPDX-SnippetBegin"
comment "SPDX-${snippet}Comment: Origin-URL: ${tree_url}"
[ -z "${snippet}" ] || comment "SPDX-SnippetEnd"
