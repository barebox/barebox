RISC-V
======

Running RISC-V barebox on qemu
------------------------------

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
      -nographic -M erizo -bios <path to barebox sources >/barebox.bin \
      -serial stdio -monitor none -trace file=/dev/null
  Switch to console [cs0]
  
  
  barebox 2018.12.0-00148-g60e49c4e16 #1 Tue Dec 18 01:12:29 MSK 2018
  
  
  Board: generic Erizo SoC board
  malloc space: 0x80100000 -> 0x801fffff (size 1 MiB)
  running /env/bin/init...
  /env/bin/init not found
  barebox:/


Running RISC-V barebox on DE0-Nano FPGA board
---------------------------------------------

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
