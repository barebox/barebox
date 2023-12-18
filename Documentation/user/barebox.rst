barebox
=======

Getting barebox
---------------

barebox is released on a monthly basis. The version numbers use the format
YYYY.MM.N, so 2014.06.0 is the monthly release for June 2014. Stable releases
are done as needed to fix critical problems and are indicated by incrementing
the suffix (for example 2014.06.1).

All releases can be downloaded from:

https://www.barebox.org/download/

Development versions of barebox are accessible via Git. A local repository clone
can be checked out as follows:

.. code-block:: sh

  $ git clone git://git.pengutronix.de/git/barebox.git
  Cloning into 'barebox'...
  remote: Counting objects: 113356, done.
  remote: Compressing objects: 100% (25177/25177), done.
  remote: Total 113356 (delta 87910), reused 111155 (delta 85935)
  Receiving objects: 100% (113356/113356), 33.13 MiB | 183.00 KiB/s, done.
  Resolving deltas: 100% (87910/87910), done.
  Checking connectivity... done.
  Checking out files: 100% (5651/5651), done.

By default, the master branch is checked out. If you want to develop for
barebox, this is the right branch to send patches against.

If you want to see which patches are already selected for the next release,
you can look at the ``next`` branch:

.. code-block:: sh

  $ git checkout -b next origin/remotes/next

A web interface to the repository is available at
https://git.pengutronix.de/cgit/barebox

.. _configuration:

Configuration
-------------

barebox uses Kconfig from the Linux kernel as a configuration tool,
where all configuration is done via the ``make`` command. Before running
it you have to specify your architecture with the ``ARCH`` environment
variable and the cross compiler with the ``CROSS_COMPILE`` environment
variable. Currently, ``ARCH`` must be one of:

* arm
* mips
* openrisc
* ppc
* riscv
* sandbox
* x86

``CROSS_COMPILE`` should be the prefix of your cross compiler. This can
either contain the full path or, if the cross compiler binary is
in your $PATH, just the prefix.

Either export ``ARCH`` and ``CROSS_COMPILE`` once before working on barebox:

.. code-block:: sh

  export ARCH=arm
  export CROSS_COMPILE=/path/to/arm-cortexa8-linux-gnueabihf-
  make ...

or add them to each invocation of the ``make`` command:

.. code-block:: sh

  ARCH=arm CROSS_COMPILE=/path/to/arm-cortexa8-linux-gnueabihf- make ...

For readability, ARCH/CROSS_COMPILE are skipped from the following examples.

Configuring for a board
^^^^^^^^^^^^^^^^^^^^^^^

All configuration files can be found under the ``arch/${ARCH}/configs/``
directory. For an overview of possible Make targets for your architecture,
type:

.. code-block:: sh

  make help

Your output from ``make help`` will be based on the architecture you've
selected via the ``ARCH`` variable. So if, for example, you had selected:

.. code-block:: sh

  export ARCH=mips

your help output would represent all of the generic (architecture-independent)
targets, followed by the MIPS-specific ones:

.. code-block:: sh

  make [ARCH=mips] help
  ...
  ... list of generic targets ...
  ...
  Architecture specific targets (mips):
    No architecture specific help defined for mips

    ath79_defconfig          - Build for ath79
    bcm47xx_defconfig        - Build for bcm47xx
    gxemul-malta_defconfig   - Build for gxemul-malta
    loongson-ls1b_defconfig  - Build for loongson-ls1b
    qemu-malta_defconfig     - Build for qemu-malta
    xburst_defconfig         - Build for xburst

barebox supports building for multiple boards with a single config. If you
can't find your board in the list, it may be supported by one of the multi-board
configs. As an example, this is the case for tegra_v7_defconfig and imx_v7_defconfig.
Select your config with ``make <yourboard>_defconfig``:

.. code-block:: sh

  make imx_v7_defconfig

The configuration can be further customized with one of the configuration frontends
with the most popular being ``menuconfig``:

.. code-block:: sh

  make menuconfig

barebox uses the same configuration and build system as Linux (Kconfig,
Kbuild), so you can use all the kernel config targets you already know, e.g.
``make xconfig``, ``make allyesconfig`` etc.

Configuring and compiling "out-of-tree"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before going any further, it's worth knowing how you can do all your barebox
configuration and compilation "out of tree"; that is, how you can keep your
source directory pristine and have all output from the various ``make`` commands
generated in a separate build directory.

Once you check out your barebox source directory, and before you do any
configuration or building, set the environment variable ``KBUILD_OUTPUT``
to point to your intended output directory, as in:

.. code-block:: sh

  export KBUILD_OUTPUT=.../my_barebox_build_directory

From that point on, all of the ``make`` commands you run in your source
directory will generate their output in your specified output directory.
Not only does this keep your source directory clean, but it allows several
developers to share the same source directory while doing all their own
configuration and building in their own individual build directories.

.. note::

   To do out-of-tree builds, your source tree must be absolutely clean
   of all generated artifacts from previous configurations and builds.
   In other words, if you had earlier done any configuration or building
   in that source tree that dumped its results into the same source tree
   directory, you need to do the equivalent of a ``make distclean`` before
   using that source directory for any out-of-tree builds.

Compilation
-----------

After barebox has been :ref:`configured <configuration>` it can be compiled
simply with:

.. code-block:: sh

  make

The resulting binary varies depending on the board barebox is compiled for.
Without :ref:`multi_image` support the ``barebox-flash-image`` link will point
to the binary for flashing/uploading to the board. With :ref:`multi_image` support
the compilation process will finish with a list of images built under ``images/``::

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

.. _second_stage:

Starting barebox
-----------------

Bringing barebox to a board for the first time is highly board specific, see your
board documentation for initial bringup.

For ARM and RISC-V, the barebox build can additionally generate a generic DT image
(enable ``CONFIG_BOARD_ARM_GENERIC_DT`` or ``CONFIG_BOARD_RISCV_GENERIC_DT``,
respectively). The resulting ``images/barebox-dt-2nd.img`` can be booted just
like a Linux kernel that is passed an external device tree. For example:

.. code-block:: console

  U-Boot: tftp $kernel_addr barebox-dt-2nd.img
  U-Boot: tftp $fdt_addr my-board.dtb
  U-Boot: bootz $kernel_addr - $fdt_addr # On 32-bit ARM
  U-Boot: booti $kernel_addr - $fdt_addr # for other platforms

Another option is to generate a FIT image containing the generic DT image and a
matching device tree with ``mkimage``:

.. code-block:: console

  sh: mkimage --architecture arm \
      --os linux \
      --type kernel \
      --fit auto \
      --load-address $kernel_addr_r \
      --compression none \
      --image images/barebox-dt-2nd.img \
      --device-tree arch/${ARCH}/dts/my-board.dtb \
      barebox-dt-2nd.fit

This FIT image can then be loaded by U-Boot and executed just like a regular
Linux kernel:

.. code-block:: console

  U-Boot: tftp $fit_addr barebox-dt-2nd.fit
  U-Boot: bootm $fit_addr

Make sure that the address in ``$fit_addr`` is different from the
``$kernel_addr_r`` passed to ``mkimage`` as the load address of the Kernel
image. Otherwise U-Boot may attempt to overwrite the FIT image with the barebox
image contained within.

For non-DT enabled-bootloaders or other architectures, often the normal barebox
binaries can also be used as they are designed to be startable second stage
from another bootloader, where possible. For example, if you have U-Boot running
on your board, you can start barebox with U-Boot's ``bootm`` command. The bootm
command doesn't support the barebox binaries directly, they first have to be
converted to uImage format using the mkimage tool provided with U-Boot:

.. code-block:: console

  sh: mkimage -n barebox -A arm -T kernel -C none -a 0x80000000 -d \
      build/images/barebox-freescale-imx53-loco.img barebox.uImage

U-Boot expects the start address of the binary to be given in the image using the
``-a`` option. The address depends on the board and must be an address which isn't
used by U-Boot. You can pick the same address you would use for generating a kernel
image for that board. The image can then be started with ``bootm``:

.. code-block:: console

  U-Boot: tftp $load_addr barebox.uImage
  U-Boot: bootm $load_addr

With barebox already running on your board, this can be used to chainload
another barebox. For instance, if you mounted a TFTP server to ``/mnt/tftp``
(see :ref:`filesystems_tftp` for how to do that), chainload barebox with:

.. code-block:: console

  bootm /mnt/tftp/barebox.bin

At least ``barebox.bin`` (with :ref:`pbl` support enabled ``images/*.pblb``)
should be startable second stage. The final binaries (``images/*.img``) may or may not
be startable second stage as it may have SoC specific headers which prevent running second
stage. barebox will usually have handlers in-place to skip these headers, so
it can chainload itself regardless.

First Steps
-----------

This is a typical barebox startup log:

.. code-block:: console

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
by pressing a key, you will find yourself at the :ref:`shell <hush>`.

At the shell type ``help`` for a list of supported commands. ``help <command>`` shows
the usage for a particular command. barebox has tab completion which will complete
your command. Arguments to commands are also completed depending on the command. If
a command expects a file argument only files will be offered as completion. Other
commands will only complete devices or devicetree nodes.

Building barebox tools
----------------------

The normal barebox build results in one or more barebox images (cf. :ref:`multi_image`)
and a number of tools built from its ``scripts/`` directory.

Most tools are used for the barebox build itself: e.g. the device tree compiler,
the Kconfig machinery and the different image formatting tools that wrap barebox,
so it may be loaded by the boot ROM of the relevant SoCs.

In addition to these barebox also builds host and target tools that are useful
outside of barebox build: e.g. to manipulate the environment or to load an
image over a boot ROM's USB recovery protocol. These tools may link against
libraries, which are detected using ``PKG_CONFIG`` and ``CROSS_PKG_CONFIG``
for native and cross build respectively. Their default values are::

  PKG_CONFIG=pkg-config
  CROSS_PKG_CONFIG=${CROSS_COMPILE}pkg-config

These can be overridden using environment or make variables.

As use of pkg-config both for host and target tool in the same build can
complicate build system integration. There are two ``ARCH=sandbox`` configuration
to make this more straight forward:

Host Tools
^^^^^^^^^^

The ``hosttools_defconfig`` will compile standalone host tools for the
host (build) system. To build the USB loaders, ``PKG_CONFIG`` needs to know
about ``libusb-1.0``. This config won't build any target tools.

.. code-block:: console

  export ARCH=sandbox
  make hosttools_defconfig
  make scripts

Target Tools
^^^^^^^^^^^^

The ``targettools_defconfig`` will cross-compile standalone target tools for the
target system.  To build the USB loaders, ``CROSS_PKG_CONFIG`` needs to know
about ``libusb-1.0``. This config won't build any host tools, so it's ok to
set ``CROSS_PKG_CONFIG=pkg-config`` if ``pkg-config`` is primed for target
use. Example:

.. code-block:: console

  export ARCH=sandbox CROSS_COMPILE=aarch64-linux-gnu-
  export CROSS_PKG_CONFIG=pkg-config
  make targettools_defconfig
  make scripts
