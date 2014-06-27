.. _pbl:

PreBootLoader images (PBL)
==========================

Traditionally barebox generates a raw uncompressed binary. PBL is an effort to
create self extracting compressed images instead. This helps on some boards
where storage space is sparse. Another usecase of PBL is on SoCs on which the
ROM code loads the initial bootloader to (limited) SRAM. With self extracting
binaries, more binary space becomes available.

PBL is available for ARM and MIPS. It can be enabled in ``make menuconfig`` with
the ``[*] Pre-Bootloader image`` option.

The user visible difference is that with PBL support ``barebox.bin`` is no longer
the final binary image, but instead ``arch/$ARCH/pbl/zbarebox.bin``. Use the
``barebox-flash-image`` link which always points to the correct image.

Technical background
--------------------

Normal object files are added to the make system using ``obj-y += file.o``.
With PBL the build system recurses into the source directories a second
time, this time all files specified with ``pbl-y += file.o`` are compiled.
This way source code can be shared between regular barebox and PBL. A special
case is ``lwl-y += file.o`` which expands to ``obj-y`` when PBL is disabled
and to ``pbl-y`` when PBL is enabled.

**HINT:** for getting an overview over the binaries, disassemble barebox.bin
(``make barebox.S``) with or without PBL support and also disassemble the
PBL (``make arch/$ARCH/pbl/zbarebox.S``)
