DIGI a9m2440
============

This CPU card is based on a Samsung S3C2440 CPU. The card is shipped with:

  * S3C2440\@400 MHz or 533 MHz (ARM920T/ARMv4T)
  * 16.9344 MHz crystal reference
  * SDRAM 32/64/128 MiB

     * Samsung K4M563233E-EE1H (one or two devices for 32 MiB or 64 MiB)

       * 2M x 32bit x 4 Banks Mobile SDRAM
       * CL2\@100 MHz (CAS/RAS delay 19ns)
       * 105 MHz max
       * column address size is 9 bits
       *  Row cycle time: 69ns

     * Samsung K4M513233C-DG75 (one or two devices for 64 MiB or 128 MiB)

       * 4M x 32bit x 4 Banks Mobile SDRAM
       * CL2\@100MHz (CAS/RAS delay 18ns)
       * 111 MHz max
       * column address size is 9 bits
       * Row cycle time: 63ns

     * 64ms refresh period (4k)
     * 90 pin FBGA
     * 32 bit data bits
     * Extended temperature range (-25°C...85°C)

  * NAND Flash 32/64/128 MiB

     * Samsung KM29U512T (NAND01GW3A0AN6)

       * 64 MiB 3,3V 8-bit
       * ID: 0xEC, 0x76, 0x??, 0xBD

     * Samsung KM29U256T

       * 32 MiB 3,3V 8-bit
       * ID: 0xEC, 0x75, 0x??, 0xBD

     * ST Micro

       * 128 MiB 3,3V 8-bit
       * ID: 0x20, 0x79

     * 30ns/40ns/20ns

  * I2C interface, 100 KHz and 400 KHz

     * Real Time Clock

       * Dallas DS1337
       * address 0x68

     * EEPROM

       * ST M24LC64
       * address 0x50
       * 16bit addressing

  * LCD interface
  * Touch Screen interface
  * Camera interface
  * I2S interface
  * AC97 Audio-CODEC interface
  * SD card interface
  * 3 serial RS232 interfaces
  * Host and device USB interface, USB1.1 compliant
  * Ethernet interface

    * 10Mbps, Cirrus Logic, CS8900A (on the CPU card)

  * SPI interface
  * JTAG interface

How to get the binary image
---------------------------

Configure with the default configuration:

.. code-block:: sh

  make ARCH=arm a9m2440_defconfig

Build the binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv4compiler

**NOTE:** replace the armv4compiler with your ARM v4 cross compiler.
