RISC-V
======

QEMU Virt
---------

barebox supports both the qemu riscv32 and riscv64 ``-M virt`` boards::

  make ARCH=riscv virt64_defconfig
  qemu-system-riscv64 -M virt -serial stdio -kernel build/images/barebox-dt-2nd.img

Replace ``64`` by ``32`` for 32-bit build. :ref:`virtio_sect` over MMIO is supported and
can be used for e.g. an extra console or to pass in a virtio-blk device::

  qemu-system-riscv64 -M virt -serial stdio                                \
  	-kernel ./images/barebox-dt-2nd.img                                \
  	-device virtio-rng-device                                          \
  	-drive if=none,file=./images/barebox-dt-2nd.img,format=raw,id=hd0  \
  	-device virtio-blk-device,drive=hd0                                \
  	-device virtio-serial-device                                       \
  	-chardev socket,path=/tmp/foo,server,nowait,id=foo           	   \
  	-device virtconsole,chardev=foo,name=console.foo

  barebox 2021.02.0 #27 Sun Mar 14 10:08:09 CET 2021
  
  
  Board: riscv-virtio,qemu
  malloc space: 0x83dff820 -> 0x87bff03f (size 62 MiB)
  
  barebox@riscv-virtio,qemu:/ filetype /dev/virtioblk0
  /dev/virtioblk0: RISC-V Linux image (riscv-linux)

Note that the ``board-dt-2nd.img`` uses the Linux RISC-V kernel image
format and boot protocol. It thus requires the device tree to be passed
from outside in ``a1`` and must be loaded at an offset as indicated in
the header for the initial stack to work. Using the ``-kernel`` option
in Qemu or booting from bootloaders that can properly boot Linux will
take care of this.

TinyEMU
-------

TinyEMU can emulate a qemu-virt like machine with a RISC-V 32-, 64-
and 128-bit CPU. It can run barebox with this sample configuration::

  /* temu barebox-virt64.cfg */
  {
      version: 1,
      machine: "riscv64",
      memory_size: 256,
      bios: "bbl64.bin",
      kernel: "./images/barebox-dt-2nd.img",
  }

``barebox-dt-2nd.img`` can be generated like with Qemu. Graphical
output is also supported, but virtio input support is still missing.
To activate add::

    display0: { device: "simplefb", width: 800, height: 600 },

into the config file.

Erizo
-----

Running on qemu
~~~~~~~~~~~~~~~

Obtain RISC-V GCC/Newlib Toolchain,
see https://github.com/riscv/riscv-tools/blob/master/README.md
for details. The ``build.sh`` script from ``riscv-tools`` should
create toolchain.

Next compile qemu emulator::

  $ git clone -b 20180409.erizo https://github.com/miet-riscv-workgroup/riscv-qemu
  $ cd riscv-qemu
  $ cap="no" ./configure \
    --extra-cflags="-Wno-maybe-uninitialized" \
    --audio-drv-list="" \
    --disable-attr \
    --disable-blobs \
    --disable-bluez \
    --disable-brlapi \
    --disable-curl \
    --disable-curses \
    --disable-docs \
    --disable-kvm \
    --disable-spice \
    --disable-sdl \
    --disable-vde \
    --disable-vnc-sasl \
    --disable-werror \
    --enable-trace-backend=simple \
    --disable-stack-protector \
    --target-list=riscv32-softmmu,riscv64-softmmu
  $ make


Next compile barebox::

  $ make erizo_generic_defconfig ARCH=riscv
  ...
  $ make ARCH=riscv CROSS_COMPILE=<path to your riscv toolchain>/riscv32-unknown-elf-

Run barebox::

  $ <path to riscv-qemu source>/riscv32-softmmu/qemu-system-riscv32 \
      -nographic -M erizo -bios ./images/barebox-erizo-generic.img \
      -serial stdio -monitor none -trace file=/dev/null
  Switch to console [cs0]
  
  
  barebox 2018.12.0-00148-g60e49c4e16 #1 Tue Dec 18 01:12:29 MSK 2018
  
  
  Board: generic Erizo SoC board
  malloc space: 0x80100000 -> 0x801fffff (size 1 MiB)
  running /env/bin/init...
  /env/bin/init not found
  barebox:/


Running on DE0-Nano FPGA board
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See https://github.com/open-design/riscv-soc-cores/ for instructions
on DE0-Nano bitstream generation and loading.

Connect to board's UART with your favorite serial communication software
(e.g. minicom) and check 'nmon> ' prompt (nmon runs from onchip ROM).

Next close your communication software and use ./scripts/nmon-loader
to load barebox image into board's DRAM, e.g.

  # ./scripts/nmon-loader barebox.erizo.nmon /dev/ttyUSB0 115200

Wait several munutes for 'nmon> ' prompt.

Next, start barebox from DRAM:

  nmon> g 80000000
  Switch to console [cs0]
  
  
  barebox 2018.12.0-00148-g60e49c4e16 #1 Tue Dec 18 01:12:29 MSK 2018
  
  
  Board: generic Erizo SoC board
  malloc space: 0x80100000 -> 0x801fffff (size 1 MiB)
  running /env/bin/init...
  /env/bin/init not found
  barebox:/
