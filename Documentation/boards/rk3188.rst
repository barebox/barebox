Rockchip RK3188
===============

Radxa Rock
----------

Radxa Rock is a small SBC based on Rockchip RK3188 SoC.
See http://radxa.com/Rock for additional information.

Building
^^^^^^^^

.. code-block:: sh

  make ARCH=arm radxa_rock_defconfig
  make ARCH=arm

Creating bootable SD card
^^^^^^^^^^^^^^^^^^^^^^^^^

This will require a DRAM setup blob and additional utilities, see above.

Card layout (block == 0x200 bytes).

============   ==========================================
Block Number   Name
============   ==========================================
0x0000         DOS partition table
0x0040         RK bootinfo (BootROM check sector)
0x0044         DRAM setup routine
0x005C         Bootloader (barebox)
0x0400         Barebox environment
0x0800         Free space start
============   ==========================================

Instructions.

* Make 2 partitions on SD for boot and root filesystems.
* Checkout and compile https://github.com/apxii/rkboottools
* Get some RK3188 bootloader from https://github.com/neo-technologies/rockchip-bootloader
* Run "rk-splitboot RK3188Loader(L)_V2.13.bin" command. (for example).
  You will get FlashData file with others. It's a DRAM setup blob.
* Otherwise it can be borrowed from RK U-boot sources from
  https://github.com/linux-rockchip/u-boot-rockchip/blob/u-boot-rk3188/tools/rk_tools/3188_LPDDR2_300MHz_DDR3_300MHz_20130830.bin
* Run "rk-makebootable FlashData barebox-radxa-rock.bin rrboot.bin"
* Insert SD card and run "dd if=rrboot.bin of=</dev/sdcard> bs=$((0x200)) seek=$((0x40))"
* SD card is ready
