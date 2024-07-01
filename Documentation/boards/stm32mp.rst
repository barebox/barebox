STMicroelectronics STM32MP
==========================

The STM32MP is a line of 32-bit ARM SoCs. They reuse peripherals of the
STM32 line of microcontrollers and can have a STM32 MCU embedded as co-processor
as well.

The boot process of the STM32MP1 SoC is a two step process.
The first stage boot loader (FSBL) is loaded by the ROM code into the built-in
SYSRAM and executed. The FSBL sets up the SDRAM, install a secure monitor and
then the second stage boot loader (SSBL) is loaded into DRAM.

When building barebox, the resulting ``barebox-${board}.stm32`` file has the STM32
header preprended, so it can be loaded directly as SSBL by the ARM TF-A
(https://github.com/ARM-software/arm-trusted-firmware). Each entry point has a
header-less image ending in ``*.pblb`` as well. Additionally, there is
a ``barebox-stm32mp-generic.img``, which is a header-less image for
use as part of a Firmware Image Package (FIP).

barebox images are meant to be loaded by the ARM TF-A
(https://github.com/ARM-software/arm-trusted-firmware). FIP images are
mandatory for STM32MP1 since TF-A v2.7.

Use of barebox as FSBL is not implemented.

Building barebox
----------------

There's a single ``stm32mp_defconfig`` for all STM32MP boards::

  export ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
  make stm32mp_defconfig
  make

The resulting images will be placed under ``images/``::

  barebox-stm32mp-generic-bl33.img
  barebox-stm32mp13xx-dk.stm32
  barebox-stm32mp15xx-dkx.stm32
  barebox-stm32mp15x-ev1.stm32
  barebox-stm32mp157c-lxa-mc1.stm32
  barebox-prtt1a.stm32
  barebox-prtt1s.stm32
  barebox-prtt1c.stm32
  barebox-stm32mp157c-seeed-odyssey.stm32
  barebox-dt-2nd.img

In the above output, images with a ``.stm32`` extension feature the (legacy)
stm32image header. ``barebox-dt-2nd.img`` and ``barebox-stm32mp-generic-bl33.img``
are board-generic barebox images that receive an external device tree.

.. _stm32mp_fip:

Flashing barebox (FIP)
----------------------

After building barebox in ``$BAREBOX_BUILDDIR``, change directory to ARM
Trusted Firmware to build a FIP image. Example building STM32MP157C-DK2
with SP_min (no OP-TEE):

.. code:: bash

    make CROSS_COMPILE=arm-none-eabi- PLAT=stm32mp1 ARCH=aarch32 ARM_ARCH_MAJOR=7 \
        STM32MP_EMMC=1 STM32MP_EMMC_BOOT=1 STM32MP_SDMMC=1 STM32MP_SPI_NOR=1 \
        AARCH32_SP=sp_min \
        DTB_FILE_NAME=stm32mp157c-dk2.dtb \
        BL33=$BAREBOX_BUILDDIR/images/barebox-stm32mp-generic-bl33.img \
        BL33_CFG=$BAREBOX_BUILDDIR/arch/arm/dts/stm32mp157c-dk2.dtb \
        fip

For different boards, adjust ``DTB_FILENAME`` and ``BL33_CFG`` as appropriate.

If OP-TEE is used, ensure ``CONFIG_OPTEE_SIZE`` is set appropriately, so
early barebox code does not attempt accessing secure memory.

barebox can also be patched into an existing FIP image with ``fiptool``:

.. code:: bash

    fiptool update mmcblk0p3  \
      --nt-fw $BAREBOX_BUILDDIR/images/barebox-stm32mp-generic-bl33.img \
      --hw-config $BAREBOX_BUILDDIR/arch/arm/dts/stm32mp135f-dk.dtb

Flashing barebox (legacy stm32image)
------------------------------------

After building ARM Trusted Firmware with ``STM32MP_USE_STM32IMAGE=1``,
an appropriate image for a SD-Card can be generated with following
``genimage(1)`` config::

  image @STM32MP_BOARD@.img {
      hdimage {
          align = 1M
          gpt = "true"
      }
      partition fsbl1 {
          image = "tf-a-@STM32MP_BOARD@.stm32"
          size = 256K
      }
      partition fsbl2 {
          image = "tf-a-@STM32MP_BOARD@.stm32"
          size = 256K
      }
      partition ssbl {
          image = "barebox-@STM32MP_BOARD@.stm32"
          size = 1M
      }
      partition barebox-environment {
          image = "/dev/null"
          size = 1M
      }
  }

For eMMC, the boot partitions are used as the FSBL partitions and so the user
partitions may look like this::

  image @STM32MP_BOARD@.img {
      partition ssbl {
          image = "barebox-@STM32MP_BOARD@.stm32"
          size = 1M
      }
      partition barebox-environment {
          image = "/dev/null"
          size = 1M
      }
  }

The fsbl1 and fsbl2 can be flashed by writing to barebox ``/dev/mmcX.boot0`` and
``/dev/mmcX.boot1`` respectively or from a booted operating system.

Additionally, the eMMC's ``ext_csd`` register must be modified to activate the
boot acknowledge signal (``BOOT_ACK``) and to select a boot partition.

Assuming ``CONFIG_CMD_MMC_EXTCSD`` is enabled and the board shall boot from
``/dev/mmc1.boot1``::

  mmc_extcsd /dev/mmc1 -i 179 -v 0x50

The STM32MP1 BootROM does *not* support booting from eMMC without fast boot
acknowledge.

USB Bootstrap (DFU)
-------------------

The STM32MP1 can be strapped to boot from USB. After Power-On reset, it
should be detectable as ``STMicroelectronics STM Device in DFU Mode``
and can be uploaded to with ``dfu-util(1)``::

  dfu-util --alt 1 -D tf-a-stm32mp157c-my-board.stm32
  dfu-util --alt 3 -D bl3-firmware.fip
  dfu-util --alt 0 -e

The first command will talk to the BootROM and upload the first stage
bootloader (ARM Trusted Firmware-A) into on-chip SRAM.

When compiled with ``STM32MP_USB_PROGRAMMER=1``, TF-A v2.6 or higher
will seamlessly continue operation of the DFU gadget. The second
command will talk to TF-A to upload a Firmware Image Package, which
is a format bundling further firmware including barebox.

The final command will instruct TF-A to boot the loaded images.
This all happens in volatile memory. To persist images, use
normal barebox functionality like creating a DFU-gadget in barebox,
Fastboot/USB mass storage ... etc.

The FIP image containing barebox can be generated as described in
:ref:`stm32mp_fip`. Upstream TF-A doesn't support DFU for
SSBLs using the legacy stm32image format.

DFU mode can be forced via :ref:`reboot_mode` from a booted system with::

  tamp.reboot_mode.next=serial reset -w

Boot source selection
---------------------

The STM32MP BootROM samples three boot pins at reset. On official
eval kit, they are either connected to a 3P DIP switch or 2P (with
BOOT1 pulled down).

EV-1
^^^^
SW1 on the DK boards sets boot mode as follows::

       +-------+
       |   --- |
 BOOT2 |   O-- |
 BOOT1 | O --O |
 BOOT0 | N O-- |  <---- SD-Card
       +-------+

       +-------+
       |   --- |
 BOOT2 |   --O |
 BOOT1 | O O-- |
 BOOT0 | N --O |  <---- eMMC
       +-------+

       +-------+
       |   --- |
 BOOT2 |   --O |
 BOOT1 | O --O |
 BOOT0 | N --O |  <---- DFU on UART and USB OTG
       +-------+

DK-1/DK-2
^^^^^^^^^
Boot mode on the DK board is set via a dual DIP switch on the backside of the PCB
(under the HDMI port).

If the board is set up, so the HDMI side looks up and the micro-USB side is to
the left and the backside with the DIP-Switch is to the user, it can be configured
as follows:

       +-------+
 BOOT2 | O O-- |
 BOOT0 | N O-- |  <---- SDMMC
       +-------+
       +-------+
 BOOT2 | O O-- |
 BOOT0 | N --O |  <---- QSPI-NOR Flash
       +-------+
       +-------+
 BOOT2 | O --O |
 BOOT0 | N --O |  <---- DFU on UART and USB OTG
       +-------+

Boot status indicator
---------------------

The ROM code on the first Cortex-A7 core pulses the PA13 pad.
An error LED on this pad can be used to indicate boot status:

* **Boot Failure:** LED lights bright
* **UART/USB Boot:** LED blinks fast
* **Debug access:** LED lights weak
