Amlogic Meson
=============

barebox supports three families of Amlogic's 64-bit SoCs:

* GX: GXBB (S905), GXL (S905X, S905D, S905W, S805X, S805Y) and GXM (S912)
* G12: G12A (S905X2, S905D2, S905Y2), G12B (S922X, A311D) and SM1 (S905X3,
  S905D3)
* A9 (A311Y3)

barebox is started by the vendor's boot firmware as BL33 and produces
a single image when built, which can then be combined with an upstream
Linux device tree. Following boards have been tested:

- G12: Libre Computer AML-A311D-CC Alta
- A9: Amlogic's A311Y3 BY401 evaluation kit

Building barebox
----------------

``meson_defconfig`` builds barebox for all three families. The drivers are
part of ``multi_v8_defconfig`` as well::

  export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
  make meson_defconfig
  make

The vendor firmware can't pass a device tree to BL33, so the board's device
tree is appended to ``images/barebox-dt-2nd.img``. The upstream device trees
in ``dts/`` can be built by name, e.g. for the Alta:

.. code-block:: sh

  make dts/src/arm64/amlogic/meson-g12b-a311d-libretech-cc.dtb
  cat images/barebox-dt-2nd.img \
      dts/src/arm64/amlogic/meson-g12b-a311d-libretech-cc.dtb > bl33.img

``bl33.img`` still needs to be packaged into a boot image together with the
firmware. barebox will embed what it needs, like the mapping of the boot source to
an MMC device, to any device tree of a family as built-in overlay.

``barebox-dt-2nd.img`` relocates itself to the end of the memory node before
it reads the DRAM size from the firmware, so the memory node must not be
bigger than the DRAM on the board, else barebox hangs. This limitation is
to be addressed in future.

Amlogic G12
-----------

Boot process
^^^^^^^^^^^^

The BootROM loads BL2 from the boot medium into SRAM. BL2 sets up the DRAM
and loads the rest of the Firmware Image Package (FIP) that follows it: BL30
for the system co-processor, BL31, the secure monitor, and BL33. BL31 starts
BL33 at 0x01000000 with the Always-On UART set up as console.

barebox reads the DRAM size and the boot medium from ``AO_SEC_GP_CFG0``,
where the firmware leaves them. The DRAM BL31 keeps is reserved in the
upstream device trees (``secmon@5000000``). Use of barebox as BL2 is not
implemented.

The firmware is provided by Amlogic as blobs: BL2 and BL30 are built per board,
BL31 is a closed-source fork of TF-A. The amlogic-boot-fip_ repository has them
for many G12 boards, along with Amlogic's packaging tool and a script that builds
a boot image around any BL33.

.. _amlogic-boot-fip: https://github.com/LibreELEC/amlogic-boot-fip

Building the boot image
^^^^^^^^^^^^^^^^^^^^^^^

``build-fip.sh`` of amlogic-boot-fip_ takes the name of a board directory
there, ``aml-a311d-cc`` for the Alta, and the BL33 image:

.. code-block:: sh

  git clone https://github.com/LibreELEC/amlogic-boot-fip
  cd amlogic-boot-fip
  mkdir out
  ./build-fip.sh aml-a311d-cc $BAREBOX_BUILDDIR/bl33.img out/

The packaging tool is an x86-64 binary. The results are named as with U-Boot
as BL33:

``u-boot.bin``
  The boot image, BL2 followed by the FIP, for the SPI NOR and USB boot.

``u-boot.bin.sd.bin``
  ``u-boot.bin`` with an additional sector in front, for SD cards and eMMC.

``u-boot.bin.usb.bl2``, ``u-boot.bin.usb.tpl``
  ``u-boot.bin`` split after BL2, for Amlogic's USB tools.

``make -C aml-a311d-cc BL33=... O=... TMP=... COMPRESS_LZ4=1`` instead of
``build-fip.sh`` compresses BL33 with lz4, as the vendor does with U-Boot.

Boot image layout
~~~~~~~~~~~~~~~~~

======== ==============================================================
Offset   Content
======== ==============================================================
0x0      BL2, 64 KiB
0x10000  FIP header, 16 KiB: offset and size of every entry, relative to
         the header, and at 0x13fe0 the SHA-256 over bytes
         0x10010..0x13fe0
0x14000  DDR training firmware, then BL30, BL31 and BL33
======== ==============================================================

BL31 and BL33 have a 0x290 byte header in front: the ``@AML`` magic at 0x10,
the payload size at 0x20, the SHA-256 of the payload at 0x30 and that of
header bytes 0x10..0x270 at 0x270. Header and payload are padded to whole
512 byte blocks. The payload is the raw binary or an lz4 stream starting
with ``LZ4C``. The headers carry no signature. BL2 refuses boot images
bigger than 4 MiB.

Boot media
^^^^^^^^^^

The order in which the BootROM tries the boot media depends on the POC
strapping pins, see U-Boot's `Almlogic SoC boot flow <https://docs.u-boot-project.org/en/latest/board/amlogic/boot-flow.html>`_.
The Alta tries its SPI NOR first, then eMMC, SD and USB, so an image on
the SD card or eMMC is only booted once the flash holds no valid image.

SD card and eMMC
~~~~~~~~~~~~~~~~

The BootROM expects the boot image at sector 1, which leaves sector 0 to an
MBR partition table. To keep an existing one, write ``u-boot.bin.sd.bin``
without its first sector:

.. code-block:: sh

  dd if=out/u-boot.bin.sd.bin of=/dev/sdX bs=512 skip=1 seek=1 conv=fsync,notrunc

The first partition can safely start at 8 MiB. On eMMC, the BootROM tries
the user area first and then the ``boot0`` and ``boot1`` partitions.

The standard PARTITION_CONFIG parameter in the eMMC's ext_csd is ignored.

SPI NOR
~~~~~~~

In the flash, the boot image starts at offset 0, so ``u-boot.bin`` is
written as is.

Erasing the first 64 KiB of the flash makes the BootROM boot from eMMC or SD
instead. It's a good idea to keep a copy of the original flash contents before
doing so.

USB boot
~~~~~~~~

Boards with a USB boot button make USB the first boot medium while it's held
at power-up. The BootROM then waits on the USB OTG port (``1b8e:c003`` on A9)
for an image, which pyamlboot_ can load::

  boot-g12.py out/u-boot.bin

.. _pyamlboot: https://github.com/superna9999/pyamlboot

Chainloading
^^^^^^^^^^^^

During development, a bootloader already on the board can start barebox
without a new boot image. From U-Boot, use ``booti`` with U-Boot's device
tree::

  => dhcp
  => tftpboot 0x1000000 barebox-dt-2nd.img
  => booti 0x1000000 - ${fdtcontroladdr}

``go`` doesn't work, as it jumps with U-Boot's MMU and caches enabled.

From barebox, ``bootm`` can start ``barebox-dt-2nd.img``.

Amlogic GX
----------

The GX family boots like the G12 and has the same drivers, except for the pin
controller.

amlogic-boot-fip_ has the firmware for many GX boards. BL31 starts BL33 at
0x01000000 with ``uart_AO`` at 115200 baud as console. barebox reads the
DRAM size and the boot medium from ``AO_SEC_GP_CFG0``, which is laid out as
on the G12, but at 0xc8100240. The upstream device trees reserve the first
16 MiB of DRAM (``hwrom@0``) and where BL31 and BL32 stay with the different
firmware versions.

BL33 is again ``barebox-dt-2nd.img`` with the device tree appended, e.g. for
the Libre Computer AML-S905X-CC (Le Potato):

.. code-block:: sh

  make dts/src/arm64/amlogic/meson-gxl-s905x-libretech-cc.dtb
  cat images/barebox-dt-2nd.img \
      dts/src/arm64/amlogic/meson-gxl-s905x-libretech-cc.dtb > bl33.img

The AML-S905X-CC device tree describes 2 GiB of DRAM, so it can't be used
for the 1 GiB variant of the board. The board is ``lepotato`` in
amlogic-boot-fip_:

.. code-block:: sh

  ./build-fip.sh lepotato $BAREBOX_BUILDDIR/bl33.img out/

The results are named as for the G12 and SD card images can also be written
to eMMC, see `SD card and eMMC`_. ``u-boot.bin.usb.bl2`` holds the 48 KiB BL2 of
the GX, which pyamlboot_'s ``boot.py`` loads over USB together with
``u-boot.bin.usb.tpl``. The BootROM tries SPI NOR first if the POC strap is
low, then eMMC, NAND, SD and USB. Unlike most G12 device trees, those of
several Libre Computer GX boards enable the SPI NOR.

Amlogic A9
----------

The A9 (A311Y3) is supported the same way, though the upstream device trees
lack most nodes so far. It has been tested on Amlogic's BY401 board, booted
over USB, from SD and from eMMC.

Boot process
^^^^^^^^^^^^

The BootROM loads BL2, which loads BL2E and BL2X. They set up the DRAM,
start BL30 on the Always-On CPU, BL31 and BL32 (OP-TEE) and finally BL33 at
address 0, with UART_B of the Always-On domain at 921600 baud as console.
All of these seem to only be available as blobs.

barebox reads the DRAM size and the boot medium from
``SYSCTRL_SEC_STATUS_REG4`` and ``SYSCTRL_SEC_STATUS_REG2``. DRAM beyond
3.5 GiB continues at 4 GiB. barebox reserves BL31 and BL32 where the
firmware reports them, unless the device tree does already, and stops the
SoC watchdog the firmware leaves running.
The built-in overlay adds a ``/chosen`` node with the console at 921600 baud.

Console
^^^^^^^

The console is the Always-On UART, ``uart_AO``, at 115200 baud on the GX and
G12 and UART_B at 921600 baud on the A9/BY401, as the firmware set it up.

Pin controllers and GPIOs
^^^^^^^^^^^^^^^^^^^^^^^^^

The pin controllers of the G12 and A9 mux the pins as the device tree says
and provide the GPIOs. The GX has no pin controller driver yet, so barebox
relies on the pin setup of the boot firmware there.

SD and eMMC
^^^^^^^^^^^

SD cards run in high speed at 50 MHz at most, as barebox doesn't switch to
1.8 V for the UHS modes. eMMCs use HS200 if the device tree allows it and
``CONFIG_MCI_TUNING`` is enabled, else high speed, or DDR with
``mmc-ddr-1_8v``. HS400 is not supported.

Watchdog
^^^^^^^^

The SoC watchdog is ``wdog0``, with a timeout of at most 65 seconds. If the
firmware left it running, barebox reports it as running and extends its
timeout to the maximum, ``wdog0.autoping=1`` keeps it fed. On the A9,
barebox stops it at startup instead.

Limitations
-----------

* There are no power domain drivers, the domains stay as the firmware left
them.

* Ethernet and USB support is not upstream yet, and barebox has no update handlers
for the boot media.

* This port is still work-in-progress. Especially the A9 support in mainline
Linux is still evolving and drivers will be ported to barebox as they
are upstreamed into Linux.

* The GX family support has not been (re-)tested on real hardware yet.
