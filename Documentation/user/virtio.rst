..
  SPDX-License-Identifier: GPL-2.0+

  Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
  Copyright (C) 2021, Ahmad Fatoum

.. _virtio_sect:

VirtIO Support
==============

This document describes the information about barebox support for VirtIO_
devices, including supported boards, build instructions, driver details etc.

What's VirtIO?
--------------

VirtIO is a virtualization standard for network and disk device drivers where
just the guest's device driver "knows" it is running in a virtual environment,
and cooperates with the hypervisor. This enables guests to get high performance
network and disk operations, and gives most of the performance benefits of
paravirtualization. In the barebox case, the guest is barebox itself, while the
virtual environment will normally be QEMU_ targets like ARM, MIPS, RISC-V or x86.

Status
------

VirtIO can use various different buses, aka transports as described in the
spec. While VirtIO devices are commonly implemented as PCI devices on x86,
embedded devices models like ARM/RISC-V, which does not normally come with
PCI support might use simple memory mapped device (MMIO) instead of the PCI
device. The memory mapped virtio device behaviour is based on the PCI device
specification. Therefore most operations including device initialization,
queues configuration and buffer transfers are nearly identical. Both MMIO
and non-legacy PCI are supported in barebox.

The VirtIO spec defines a lots of VirtIO device types, however at present only
block, network, console, input and RNG devices are supported.

Build Instructions
------------------

Building barebox for QEMU targets is no different from others.
For example, we can do the following with the CROSS_COMPILE environment
variable being properly set to a working toolchain for ARM::

  $ make multi_v7_defconfig
  $ make

Testing
-------

The following QEMU command line is used to get barebox up and running with
a VirtIO console on ARM::

  $ qemu-system-arm -m 256M -M virt -nographic             \
  	-kernel ./images/barebox-dt-2nd.img                \
  	-device virtio-serial-device                       \
  	-chardev socket,path=/tmp/foo,server,nowait,id=foo \
  	-device virtconsole,chardev=foo,name=console.foo

To access the console socket, you can use ``socat /tmp/foo -``.

Note the use of ``-kernel ./images/barebox-dt-2nd.img`` instead of
``-bios ./images/barebox-$BOARD.img``. ``-kernel`` will cause QEMU
to pass barebox a fixed-up device tree describing the ``virtio-mmio``
rings.

Except for the console, multiple instances of a VirtIO device can be created
by appending more '-device' parameters. For example to extend a MIPS
malta VM with one HWRNG and 2 block VirtIO PCI devices::

  $ qemu-system-mips -m 256M -M malta -serial stdio         \
    	-bios ./images/barebox-qemu-malta.img -monitor null \
  	-device virtio-rng-pci,disable-legacy=on            \
  	-drive if=none,file=image1.hdimg,format=raw,id=hd0  \
  	-device virtio-blk-pci,drive=hd0,disable-legacy=on  \
  	-drive if=none,file=image2.hdimg,format=raw,id=hd1  \
  	-device virtio-blk-pci,drive=hd1,disable-legacy=on

Note the use of ``disable-legacy=on``. barebox doesn't support legacy
or transitional VirtIO devices. Some versions of QEMU may need to
have ``,disable-modern=off`` specfied as well.

.. _VirtIO: http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
.. _qemu: https://www.qemu.org
