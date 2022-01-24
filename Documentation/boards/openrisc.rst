OpenRISC
========

Optaining an OpenRISC toolchain
-------------------------------

Toolchain binaries can be obtained from openrisc.io or our github releases page.
Instructions for building the different toolchains can be found on openrisc.io
or Stafford's toolchain build and release scripts.

See:

  * https://github.com/stffrdhrn/gcc/releases
  * https://github.com/stffrdhrn/or1k-toolchain-build
  * https://openrisc.io/software

Example of downloading and installing a toolchain::

  $ curl --remote-name --location \
    https://github.com/stffrdhrn/gcc/releases/download/or1k-10.0.0-20190723/or1k-elf-10.0.0-20190723.tar.xz
  $ tar -xf or1k-elf-10.0.0-20190723.tar.xz
  $ export PATH=$PATH:$PWD/or1k-elf/bin

Running OpenRISC barebox on qemu
--------------------------------

Running barebox on qemu is similar to running linux on qemu see more details on
the qemu wiki site at https://wiki.qemu.org/Documentation/Platforms/OpenRISC

Compile the qemu emulator::

  $ git clone https://gitlab.com/qemu-project/qemu.git
  $ cd qemu
  $ mkdir build ; cd build
  $ ../configure \
    --target-list="or1k-softmmu" \
    --enable-fdt \
    --disable-kvm \
    --disable-xen \
    --disable-xkbcommon \
    --enable-debug \
    --enable-debug-info
  $ make

Next compile barebox::

  $ make ARCH=openrisc defconfig
  ...
  $ make ARCH=openrisc CROSS_COMPILE=or1k-elf-

Run barebox::

  $ <path to qemu source>/build/or1k-softmmu/qemu-system-or1k \
    -cpu or1200 \
    -M or1k-sim \
    -kernel /home/shorne/work/openrisc/barebox/barebox \
    -net nic -net tap,ifname=tap0,script=no,downscript=no \
    -serial mon:stdio -nographic -gdb tcp::10001 \
    -m 32


  barebox 2021.02.0-00120-g763c6fee7-dirty #14 Thu Mar 4 05:13:51 JST 2021


  Board: or1ksim
  mdio_bus: miibus0: probed
  malloc space: 0x01b80000 -> 0x01f7ffff (size 4 MiB)

  Hit any to stop autoboot:    3
  barebox@or1ksim:/

or1ksim
-------

Compile or1ksim emulator:

.. code-block:: console

 $ cd ~/
 $ git clone https://github.com/openrisc/or1ksim
 $ cd or1ksim
 $ ./configure
 $ make

Create minimal or1ksim.cfg file:

.. code-block:: none

 section cpu
   ver = 0x12
   cfgr = 0x20
   rev = 0x0001
 end

 section memory
   name = "RAM"
   type = unknown
   baseaddr = 0x00000000
   size = 0x02000000
   delayr = 1
   delayw = 2
 end

 section uart
   enabled = 1
   baseaddr = 0x90000000
   irq = 2
   16550 = 1
   /* channel = "tcp:10084" */
   channel = "xterm:"
 end

 section ethernet
   enabled = 1
   baseaddr = 0x92000000
   irq = 4
   rtx_type = "tap"
   tap_dev = "tap0"
 end

Run or1ksim:

.. code-block:: console

 $ ~/or1ksim/sim -f or1ksim.cfg barebox
