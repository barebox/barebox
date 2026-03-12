# SPDX-License-Identifier: GPL-2.0-or-later

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
