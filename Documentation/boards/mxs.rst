Freescale MXS
=============

Freescale i.MXs or MXS are a SoC family which consists of the i.MX23
and the i.MX28. These are quite different from the regular i.MX SoCs
and thus are represented by their own architecture in both the kernel
and in barebox.

Bootlets
--------

Traditionally these SoCs need the Freescale bootlets source and the
*elf2sb2* binary to set up the PMIC and SDRAM, and to build a bootable
image out of the barebox binary.
Since the bootlets are board-specific and the source code is only
hardly customisable, each vendor usually has their own slightly different
version of the bootlets. Booting with the Freescale bootlets is not
described here, refer to the bootlet source code or your vendor's
documentation instead.

Barebox has a port of the bootlets integrated into its source, which is
derived from the U-Boot bootlet code written by Marek Vasut.

Most MXS boards in the barebox tree have been ported to use barebox bootlets and
image generation, and we recommend this approach for new boards too.

Booting Freescale i.MXs
-----------------------

The Freescale MXS SoCs have a multi staged boot process which needs
different images composed out of different binaries. The ROM executes
a so called *bootstream* which can contain multiple executables.

The first executable (the prepare stage) is only a small binary executed in
SRAM, which has a limited size of 128 KB. Its purpose is to setup the internal
PMIC and the SDRAM, and then jump back to the MXS ROM code, which then maps the
second executable (the full bootloader) into SDRAM and executes it.
In case of barebox, the bootstream (``*-bootstream.img``) is composed of the
self extracting barebox image (``*.pblx``) and the prepare stage
(``prep_*.pblb``). The file name of those images reflects the name of the
respective entry points.

The bootstream image itself is useful for USB boot, but for booting from
SD cards or NAND a BCB header has to be prepended to the image. In case
of SD boot the image is named ``*-sd.img``.

Bootstream images can be unencrypted or encrypted. Depending on the OCOTP fuses
of your chip, you might need the one or the other to boot the board.

Since some of the bootstream images are encrypted, they are not suitable for
2nd stage execution. For this purpose a 2nd stage image with the name
``*-2nd.img`` is generated.

Booting from USB
----------------

If enabled in *menuconfig* → *System Type*, barebox builds the *imx-usb-loader*
tool (derived from the *sbloader* tool from the rockbox project), which can
load images onto MXS SoCs over USB. (Refer to the documentation of your board
how to get it into USB boot mode.)

If the board is connected to the PC and started in USB boot mode, it should
show up in lsusb::

  Bus 001 Device 098: ID 15a2:004f Freescale Semiconductor, Inc. i.MX28 SystemOnChip in RecoveryMode

The bootstream images can then directly be booted with::

  ./scripts/imx-usb-loader images/barebox-karo-tx28-bootstream.img

You might require appropriate udev rules or *sudo* to gain the rights to
access the USB device.

Booting from SD cards
---------------------

The SD images are suitable for booting from SD cards. The MXS SoCs require a
special partition of type 0x53 (OnTrack DM6 Aux) which contains the BCB header.
The partitioning can be created with the following fdisk sequence (using
*/dev/sdg* as an example for the SD card)::

  $ fdisk /dev/sdg

  Welcome to fdisk (util-linux 2.25.1).
  Changes will remain in memory only, until you decide to write them.
  Be careful before using the write command.


  Command (m for help): o
  Created a new DOS disklabel with disk identifier 0xd7e5d328.

  Command (m for help): n
  Partition type
     p   primary (0 primary, 0 extended, 4 free)
     e   extended (container for logical partitions)
  Select (default p): p
  Partition number (1-4, default 1): 1
  First sector (2048-7829503, default 2048):
  Last sector, +sectors or +size{K,M,G,T,P} (2048-7829503, default 7829503): +1M

  Created a new partition 1 of type 'Linux' and of size 1 MiB.

  Command (m for help): t
  Selected partition 1
  Hex code (type L to list all codes): 53
  Changed type of partition 'Linux' to 'OnTrack DM6 Aux3'.

  Command (m for help): w

After writing the new partition table, the image can be written directly to
the first partition::

  cat images/barebox-karo-tx28-sd.img > /dev/sdg1

.. note::

  For some unknown reason the BCB header is
  inside a partition, but contains the sector number of the raw device from
  which the rest of the image is read. With standard settings, booting from
  SD card only works if the partition containing the bootloader starts at
  sector 2048 (the standard for fdisk). See the *-p* parameter to the
  ``scripts/mxsboot`` tool to change this sector number in the image.

Booting second stage
--------------------

The second stage images can be started with the barebox :ref:`command_bootm` command or
just jumped into using the :ref:`command_go` command.

MXS boards
----------

Not all supported boards have a description here.

.. toctree::
  :glob:
  :maxdepth: 1

  mxs/*
