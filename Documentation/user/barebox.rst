barebox
=======

Getting barebox
---------------

barebox is released on a monthly basis. The version numbers use the format
YYYY.MM.N, so 2014.06.0 is the monthly release for June 2014. Stable releases
are done as needed to fix critical problems and are indicated by incrementing
the suffix (for example 2014.06.1).

All releases can be downloaded from:

http://www.barebox.org/download/

Development versions of barebox are accessible via git. A local repository clone
can be created using git::

  $ git clone git://git.pengutronix.de/git/barebox.git
  Cloning into 'barebox'...
  remote: Counting objects: 113356, done.
  remote: Compressing objects: 100% (25177/25177), done.
  remote: Total 113356 (delta 87910), reused 111155 (delta 85935)
  Receiving objects: 100% (113356/113356), 33.13 MiB | 183.00 KiB/s, done.
  Resolving deltas: 100% (87910/87910), done.
  Checking connectivity... done.
  Checking out files: 100% (5651/5651), done.

After this, make sure to check out needed branch. If you want to
develop for barebox, it's better to check the the next branch::

  $ git checkout -b next origin/remotes/next

A web interface to the repository is available at
http://git.pengutronix.de/?p=barebox.git.

.. _configuration:

Configuration
-------------

barebox uses Kconfig from the Linux Kernel as a configuration tool.
All configuration is accessible via the 'make' command. Before running
it you have to specify your architecture with the ARCH environment
variable and the cross compiler with the CROSS_COMPILE environment
variable. ARCH has to be one of:

* arm
* blackfin
* mips
* nios2
* openrisc
* ppc
* sandbox
* x86

CROSS_COMPILE should be the prefix of your cross compiler. This can
either contain the full path or, if the cross compiler binary is
in your $PATH, just the prefix.

Either export ARCH and CROSS_COMPILE once before working on barebox::

  export ARCH=arm
  export CROSS_COMPILE=/path/to/arm-cortexa8-linux-gnueabihf-
  make ...

or add them before each 'make' command::

  ARCH=arm CROSS_COMPILE=/path/to/arm-cortexa8-linux-gnueabihf- make ...

For readability, ARCH/CROSS_COMPILE are skipped from the following examples.

Configuring for a board
^^^^^^^^^^^^^^^^^^^^^^^

All configuration files can be found under arch/$ARCH/configs/. For an
overview type::

  make help

  ...
  Architecture specific targets (mips):
    No architecture specific help defined for mips

    loongson-ls1b_defconfig  - Build for loongson-ls1b
    ritmix-rzx50_defconfig   - Build for ritmix-rzx50
    tplink-mr3020_defconfig  - Build for tplink-mr3020
    dlink-dir-320_defconfig  - Build for dlink-dir-320
    qemu-malta_defconfig     - Build for qemu-malta

barebox supports building for multiple boards with a single config. If you
can't find your board in the list, it may be supported by one of the multi-board
configs. As an example, this is the case for tegra_v7_defconfig and imx_v7_defconfig.
Select your config with ``make <yourboard>_defconfig``::

  make imx_v7_defconfig

The configuration can be further customized with one of the configuration frontends
with the most popular being ``menuconfig``::

  make menuconfig

barebox used the same configuration system as Linux, so you can use
all the things you know, e.g. ``make xconfig``, ``make allyesconfig`` etc.

Compilation
-----------

After barebox has been :ref:`configured <configuration>` it can be compiled::

  make

The resulting binary varies depending on the board barebox is compiled for.
Without :ref:`multi_image` support the 'barebox-flash-image' link will point
to the binary for flashing/uploading to the board. With :ref:`multi_image` support
the compilation process will finish with a list of images built under images/::

  images built:
  barebox-freescale-imx51-babbage.img
  barebox-genesi-efikasb.img
  barebox-freescale-imx53-loco.img
  barebox-freescale-imx53-loco-r.img
  barebox-freescale-imx53-vmx53.img
  barebox-tq-mba53-512mib.img
  barebox-tq-mba53-1gib.img
  barebox-datamodul-edm-qmx6.img
  barebox-guf-santaro.img
  barebox-gk802.img

Starting barebox
-----------------

Bringing barebox to a board for the first time is highly board specific, see your
board documentation for initial bringup.

barebox binaries are, where possible, designed to be startable second stage from another
bootloader. For example, if you have U-Boot running on your board, you can start barebox
with U-Boot's 'go' command::

  U-Boot: tftp $load_addr barebox.bin
  U-Boot: go $load_addr

With barebox already running on your board, this can be used to chainload another barebox::

  bootm /mntf/tftp/barebox.bin

At least ``barebox.bin`` (with :ref:`pbl` support enabled ``arch/$ARCH/pbl/zbarebox.bin``)
should be startable second stage. The flash binary (``barebox-flash-image``) may or may not
be startable second stage as it may have SoC specific headers which prevent running second
stage.

First Steps
-----------

This is a typical barebox startup log::

  barebox 2014.06.0-00232-g689dc27-dirty #406 Wed Jun 18 00:25:17 CEST 2014


  Board: Genesi Efika MX Smartbook
  detected i.MX51 revision 3.0
  mc13xxx-spi mc13892@00: Found MC13892 ID: 0x0045d0 [Rev: 2.0a]
  m25p80 m25p800: sst25vf032b (4096 Kbytes)
  ata0: registered /dev/ata0
  imx-esdhc 70004000.esdhc: registered as 70004000.esdhc
  imx-esdhc 70008000.esdhc: registered as 70008000.esdhc
  imx-ipuv3 40000000.ipu: IPUv3EX probed
  netconsole: registered as cs2
  malloc space: 0xabe00000 -> 0xafdfffff (size 64 MiB)
  mmc1: detected SD card version 2.0
  mmc1: registered mmc1
  barebox-environment environment-sd.7: setting default environment path to /dev/mmc1.barebox-environment
  running /env/bin/init...

  Hit any key to stop autoboot:  3

  barebox@Genesi Efika MX Smartbook:/

Without intervention, barebox will continue booting after 3 seconds. If interrupted
by pressing a key, you will find yourself on the :ref:`shell <hush>`.

On the shell type ``help`` for a list of supported commands. ``help <command>`` shows
the usage for a particular command. barebox has tab completion which will complete
your command. Arguments to commands are also completed depending on the command. If
a command expects a file argument only files will be offered as completion. Other
commands will only complete devices or devicetree nodes.
