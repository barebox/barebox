STMicroelectronics STM32MP
==========================

.. note::

  Support for the STM32MP architecure in barebox is still in progress.
  Bootstrapping an OS from mainline barebox is not yet supported.

The STM32MP is a line of 32-bit ARM SoCs. They reuse peripherals of the
STM32 line of microcontrollers and can have a STM32 MCU embedded as co-processor
as well.

The boot process of the STM32MP SoC is a two step process.
The first stage boot loader (FSBL) is loaded by the ROM code into the built-in
SYSRAM and executed. The FSBL sets up the SDRAM, install a secure monitor and
then the second stage boot loader (SSBL) is loaded into DRAM.

When building barebox, the resulting ``barebox-${board}.img`` file has the STM32
header preprended, so it can be loaded directly as SSBL by the ARM TF-A
(https://github.com/ARM-software/arm-trusted-firmware). Each entry point has a
header-less image ending in ``*.pblb`` as well.

Use of barebox as FSBL is not supported.

Building barebox
----------------

With multi-image and device trees, it's expected to have ``stm32mp_defconfig``
as sole defconfig for all STM32MP boards::

  make ARCH=arm stm32mp_defconfig

The resulting images will be placed under ``images/``:

::

  barebox-stm32mp157c-dk2.img


Flashing barebox
----------------

An appropriate image for the boot media can be generated with following
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
          image = "barebox-@STM32MP_BOARD@.img"
          size = 1M
      }
  }

Image can then be flashed on e.g. a SD-Card.

TODO
----

* Extend barebox MMCI support to support the SDMMC2
* Extend barebox DesignWare MAC support to support the stmmac
