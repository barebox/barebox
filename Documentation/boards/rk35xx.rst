RK35xx Rockchip SoCs
====================

Barebox features support for multiple SoCs of the Rockchip RK35xx
series, including RK3566, RK3568, RK3576 and RK3588.

Supported Boards
----------------

- Rockchip RK3568 EVB
- Rockchip RK3568 Bananapi R2 Pro
- Pine64 Quartz64 Model A
- Pine64 Pine Tab 2
- Radxa ROCK3 Model A
- Radxa ROCK5 (RK3588)
- Radxa CM3 (RK3566) IO Board
- Protonic MECSBC
- Protonic PRTPUK

The steps described in the following target the RK3568 and the RK3568 EVB but
generally apply to both SoCs and all boards.
Differences between the SoCs or boards are outlined where required.

Building
--------

The build process needs three binary files which have to be copied from the
`rkbin https://github.com/rockchip-linux/rkbin` repository to the barebox source tree:

.. code-block:: sh

  cp $RKBIN/bin/rk35/rk3568_bl31_v1.44.elf firmware/rk3568-bl31.bin
  cp $RKBIN/bin/rk35/rk3568_bl32_v2.15.bin firmware/rk3568-bl32.bin
  cp $RKBIN/bin/rk35/rk3568_ddr_1560MHz_v1.23.bin arch/arm/boards/rockchip-rk3568-evb/sdram-init.bin

With these barebox can be compiled as:

.. code-block:: sh

  make ARCH=arm rockchip_v8_defconfig
  make ARCH=arm

.. note:: When compiling OP-TEE yourself, use the tee.bin image as it has
  a header telling barebox where to load the image to.
  The tee-raw.bin image lacks a header and thus barebox will fallback to the hardcoded
  addresses expected by the vendor blobs in the rkbin repository.

.. note:: The RK3566 and RK3568 seem to share the bl31 and bl32 firmware files,
  whereas the memory initialization blob is different.

.. note:: The bl31 from the rkbin repository seems to be unable to handle
  device trees of a larger size (for example, if CONFIG_OF_OVERLAY_LIVE is
  enabled). Disable CONFIG_ARCH_ROCKCHIP_ATF_PASS_FDT in this case.

Creating a bootable SD card
---------------------------

A bootable SD card can be created with:

.. code-block:: sh

  dd if=images/barebox-rk3568-evb.img of=/dev/sdx bs=1024 seek=32

The barebox image is written to the raw device, so make sure the partitioning
doesn't conflict with the are barebox is written to. Starting the first
partition at offset 8MiB is a safe bet.

USB bootstrapping
-----------------

The RK3568 can be bootstrapped via USB for which the rk-usb-loader tool in the barebox
repository can be used. The tool takes the same images as written on SD cards:

.. code-block:: sh

  ./scripts/rk-usb-loader images/barebox-rk3568-evb.img

Note that the boot order of the RK3568 is not configurable. The SoC will only enter USB
MaskROM mode when no other bootsource contains a valid bootloader. This means to use USB
you have to make all other bootsources invalid by removing SD cards and shortcircuiting
eMMCs. The RK3568 EVB has a pushbutton to disable the eMMC.
On the Quartz64 boards, remove the eMMC module if present.

Console Output
--------------

The DDR-init in the rkbin repository will set up a serial console
at 1.5 MBaud, while barebox will set up the console with the baudrate it
has been configured with in DT and/or Kconfig, which may be different.

It can be useful for debugging to have the same baudrate for all components.

The barebox baudrate is defined by device tree::

  / {
    chosen {
      stdout-path = "serial0:1500000n8";
    };
  };

and ``CONFIG_BAUDRATE`` controls the default if no baud rate is specified
or the device tree has not been parsed yet:

.. code-block:: console

  $ scripts/config --file .config --set-str CONFIG_BAUDRATE 1500000


The DDR-init baudrate can be modified by setting a ``uart baudrate``
override in the ``ddrbin_param.txt`` file in the rkbin repository:

.. code-block:: diff

  diff --git a/tools/ddrbin_param.txt b/tools/ddrbin_param.txt
  index 0d0f53884a72..f71e09aafc4c 100644
  --- a/tools/ddrbin_param.txt
  +++ b/tools/ddrbin_param.txt
  @@ -11,7 +11,7 @@ lp5_freq=
   
   uart id=
   uart iomux=
  -uart baudrate=
  +uart baudrate=115200
   
   sr_idle=
   pd_idle=

And after that the ``ddrbin_tool`` binary can be used to apply this
modification to the relevant ddr init blob::

$ tools/ddrbin_tool rk3568 tools/ddrbin_param.txt bin/rk35/rk3568_ddr_1560MHz_v1.21.bin

Adding new SoC support
----------------------

On 64-bit Rockchip Platforms, the barebox prebootloader acts as
the latter part of BL2 to load TF-A and optionally OP-TEE and
then execution continues to barebox proper as BL33.

As the low-level setup of DRAM happens outside of barebox,
adding support for new SoCs is thus very straight-forward.

For base functionality, it probably requires only porting the
Linux clk and pinctrl drivers as well as the boilerplate needed
for every SoC. Reach out to the :ref:`mailing list <feedback>`
if you have unsupported hardware.
