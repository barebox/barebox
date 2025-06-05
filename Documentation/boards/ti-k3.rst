TI K3 based boards
==================

The TI K3 is a line of 64-bit ARM SoCs. The build and boot process differs between the
different SoC types. This document contains the general prerequisites for all K3 SoCs.
Refer to the section for the actual SoC type for building and booting barebox.

Prerequisites
-------------

There are several binary blobs required for building barebox for TI K3 SoCs. Find them
in git://git.ti.com/processor-firmware/ti-linux-firmware.git. The repository is assumed
to be checked out at ``firmware/ti-linux-firmware``. Alternatively the barebox repository
has a ti-linux-firmware submodule which checks out at the correct place. The K3 SoCs boot
from a FAT partition on SD/eMMC cards. During the next steps the files are copied to
``$TI_BOOT``. This is assumed to be an empty directory. After the build process copy its
contents to a FAT filesystem on an SD/eMMC card.

TI K3 SoC specific documentation
================================

* :ref:`ti_k3_am62x`
* :ref:`ti_k3_am62lx`
