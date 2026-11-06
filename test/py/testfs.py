# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import os
import shutil
import stat
import subprocess


def mkcpio(inpath, outname):
    compress = outname.suffix == ".gz"

    find = subprocess.Popen(["find", inpath], stdout=subprocess.PIPE)

    with open(outname, "wb") as outfile:
        cpio = subprocess.Popen(["cpio", "-o", "-H", "newc"], stdin=find.stdout,
                                stdout=(subprocess.PIPE if compress else outfile))

        find.stdout.close()

        if compress:
            gzip = subprocess.Popen(["gzip"], stdin=cpio.stdout, stdout=outfile)
            cpio.stdout.close()
            gzip.wait()

        cpio.wait()
        find.wait()


def mktree(path):
    """Fill path with a copy of LICENSES/ and a few more entries

    The license texts are all smaller than a file system block usually is,
    so add a file that spans many blocks and doesn't end on a block
    boundary, a file of several pieces separated by holes that starts and
    ends with one, an empty file and two symlinks, one with a target too
    long to be stored inline.
    """
    licenses = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir,
                            "LICENSES")
    shutil.copytree(licenses, os.path.join(path, "LICENSES"))

    texts = []
    for root, dirs, files in sorted(os.walk(licenses)):
        for name in sorted(files):
            with open(os.path.join(root, name), "rb") as f:
                texts.append(f.read())

    with open(os.path.join(path, "all-licenses"), "wb") as f:
        f.write(b"".join(texts) * 8)

    # Blocks of zeroes are stored as holes
    with open(os.path.join(path, "sparse"), "wb") as f:
        for i in range(1, 9):
            f.seek(i << 17)
            f.write(texts[i % len(texts)])
        f.truncate(10 << 17)

    open(os.path.join(path, "empty"), "wb").close()
    os.symlink("LICENSES/preferred/GPL-2.0", os.path.join(path, "COPYING"))
    os.symlink("./" * 32 + "LICENSES/preferred/GPL-2.0",
               os.path.join(path, "COPYING-long"))


def ls_long(barebox, path):
    """Return the entries of a directory as listed by ls -l"""
    entries = {}
    for line in barebox.run_check(f"ls -l {path}"):
        mode, size, name, *link = line.split()
        entries[name] = (mode, size, link[1] if link else None)
    return entries


def check_tree(barebox, tree, mnt):
    """Check that barebox finds the host's directory tree at mnt

    Every directory must list the same entries with the host's modes,
    sizes and symlink targets, and every file must read back with the
    host's md5sum.
    """
    for root, dirs, files in os.walk(tree):
        rel = os.path.relpath(root, tree)
        target = mnt if rel == "." else f"{mnt}/{rel}"

        entries = ls_long(barebox, target)
        assert sorted(entries) == sorted(dirs + files), \
            f"Expected 'ls -l {target}' to list the image's entries"

        for name in dirs + files:
            st = os.lstat(os.path.join(root, name))
            mode, size, link = entries[name]
            expected = stat.filemode(st.st_mode)

            assert mode == expected, \
                f"Expected {target}/{name} to have mode {expected}"

            if stat.S_ISLNK(st.st_mode):
                expected = os.readlink(os.path.join(root, name))
                assert link == expected, \
                    f"Expected {target}/{name} to link to {expected}"
            elif stat.S_ISREG(st.st_mode):
                assert int(size) == st.st_size, \
                    f"Expected {target}/{name} to be {st.st_size} bytes"

        for name in files:
            with open(os.path.join(root, name), "rb") as f:
                expected = hashlib.md5(f.read()).hexdigest()

            out = barebox.run_check(f"md5sum {target}/{name}")
            assert out[0].split()[0] == expected, \
                f"Expected md5sum of {target}/{name} to match the host's"


def check_image(barebox, image, tree, mnt):
    """Mount image with -o loop at mnt and check it holds the host's tree"""
    barebox.run_check(f"mkdir -p {mnt}")
    barebox.run_check(f"mount -o loop {image} {mnt}")

    try:
        check_tree(barebox, tree, mnt)
    finally:
        barebox.run(f"umount {mnt}")
