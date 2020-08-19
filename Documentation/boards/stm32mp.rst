STMicroelectronics STM32MP
==========================

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
  barebox-stm32mp157c-lxa-mc1.img
  barebox-stm32mp157c-seeed-odyssey.img


Flashing barebox
----------------

An appropriate image for a SD-Card can be generated with following
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
      partition barebox-environment {
          image = "/dev/null"
          size = 1M
      }
  }

For eMMC, the boot partitions are used as the FSBL partitions and so the user
partitions may look like this:

  image @STM32MP_BOARD@.img {
      partition ssbl {
          image = "barebox-@STM32MP_BOARD@.img"
          size = 1M
      }
      partition barebox-environment {
          image = "/dev/null"
          size = 1M
      }
  }

The fsbl1 and fsbl2 can be flashed by writing to barebox ``/dev/mmcX.boot0`` and
``/dev/mmcX.boot1`` respectively or from a booted operating system.

Boot source selection
---------------------

The STM32MP BootROM samples three boot pins at reset. Usually BOOT1 is
pulled down and BOOT0 and BOOT2 are connected to a 2P DIP switch::

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
