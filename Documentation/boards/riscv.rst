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
and 128-bit CPU. It can run 32-bit barebox with this sample configuration:

.. literalinclude:: riscv/barebox-virt32.cfg

as well as 64-bit barebox with this configuration:

.. literalinclude:: riscv/barebox-virt64.cfg

``barebox-dt-2nd.img`` can be generated like with Qemu. Graphical
output and input are also supported.
To activate add::

    display0: { device: "simplefb", width: 800, height: 600 },
    input_device: "virtio",

into the config file.

See https://barebox.org/jsbarebox/?graphic=1 for a live example.

BeagleV
-------

barebox has second-stage support for the BeagleV Starlight::

  make ARCH=riscv starfive_defconfig
  make

Thie resulting ``./images/barebox-beaglev-starlight.img`` can be used as payload
to opensbi::

  git clone https://github.com/starfive-tech/opensbi
  cd opensbi
  export ARCH=riscv
  export PLATFORM=starfive/vic7100
  export FW_PAYLOAD_PATH=$BAREBOX/build/images/barebox-beaglev-starlight.img

  make ARCH=riscv
  ./fsz.sh ./build/platform/starfive/vic7100/firmware/fw_payload.bin fw_payload.bin.out
  ls -l $OPENSBI/build/platform/starfive/vic7100/firmware/fw_payload.bin.out

The resulting ``./platform/starfive/vic7100/firmware/fw_payload.bin.out`` can then
be flashed via Xmodem to the board::

  picocom -b 115200 /dev/ttyUSB0 --send-cmd "sx -vv" --receive-cmd "rx -vv"
  0:update uboot
  select the function: 0␤
  send file by xmodem
  ^A^S./platform/starfive/vic7100/firmware/fw_payload.bin.out␤

After reset, barebox should then boot to shell and attempt booting kernel ``Image``
and device tree ``jh7100-starlight.dtb`` from the first root partition with the same
partition as rootfs. Note that while barebox does take over some initialization,
because of lack of Linux drivers, it doesn't yet do everything. If you experience
boot hangs, you may need to disable devices (or extend the starfive-pwrseq driver
to initialize it for you).

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
