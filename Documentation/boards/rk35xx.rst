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
- QNAP TS-433 NAS
- Protonic RK356x
- Protonic PRTPUK

.. toctree::
  :glob:
  :maxdepth: 1

  rk35xx/*

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

Rockchip Secure Boot
--------------------

Rockchip Secure Boot is a Rockchip feature to ensure the integrity and
legitimacy of the firmware running on a device. The Bootrom verifies that the
header of a loaded Rockchip image contains a public key that matches a sha265
hash that is stored in the OTP fuses of the SoC and a signature that can be
verified with this public key.

This allows to verify that the image has been signed by the owner of the
private key that was authorized when secure boot was activated on the board.

The Rockchip header especially contains the public key as a Modulus, Exponent,
and NP triple, and hashes over each code sector in this image. The signature
comprises the entire rkimage header except for the signature itself. Thus the
signature verifies that the hashes over the code sectors are trustworthy and,
indirectly, the code sectors are trustworthy ::

    0x0 +---------------------------------+      -
        | Magic                           |      |
   0x78 +---------------------------------+      |
        | sector                          |--+   | Signed Area
        | hash                            +  |   |
  0x200 +---------------------------------+  |   |
        | Modulus                         |  |   |
        | Exponent                        |  |   |
        | NP                              |  |   |
  0x600 +---------------------------------+  |   -
        | Signature                       |  |
  0x800 +---------------------------------+  |
        | ...                             |  |
        +---------------------------------+  |
        | Barebox Prebootloader (PBL)     |<-+
        | Piggydata (Main Barebox Binary) |
        +---------------------------------+

The public key is not stored in the OTP due to size reasons, but instead a
sha256 hash over the public key as contained in the rkimage is stored in the
OTP. The Bootrom verifies that the hash in the OTP matches the hash of the
public key.

Signing Rockchip Images
^^^^^^^^^^^^^^^^^^^^^^^

The Bootrom allows to use RSA-2048 or RSA-4096 keys for verification.

Generate an RSA key pair for image signing and verification::

  $ openssl genrsa -f4 -out <key.pem> 4096

Enable ``CONFIG_ARCH_ROCKCHIP_IMAGE_SIGNED`` and let
``CONFIG_ARCH_ROCKCHIP_IMAGE_SIGN_KEY`` point to your generated PEM file.

The Rockchip images generated by the barebox build are now signed with the
configured key.

The ``rk-otp.sh`` script prints the sha256 hash of a key in a PEM file or a key
embedded into an rkimage. You may verify that the image is actually signed with
your key by comparing the output of the following two commands::

  $ scripts/rk-otp.sh <key.pem>
  $ scripts/rk-otp.sh <rkimage>

Secure Boot
^^^^^^^^^^^

The barebox ``rksecure`` command allows to burn a hash into the OTP and enable
secure boot at runtime.

Since the secure OTP are only accessible from the secure world, barebox cannot
directly access the OTP area. In order to write or read the secure OTP from
barebox, you need an OP-TEE with the rksecure PTA. See the OP-TEE documentation
how to enable this PTA.

.. note::

   The OP-TEE rksecure PTA is only available in Upstream OP-TEE.
   Rockchips Downstream OP-TEE blob does not support writing the OTP area and
   enabling Secure Boot.

Enable the ``rksecure`` command by enabling the following config variables::

  CONFIG_CMD_RKSECURE
  CONFIG_OPTEE_RKSECURE

The Bootrom also boots signed images if secure boot is not enabled. Thus, you
may boot the same image used with and without secure boot enabled devices.

You may use the ``rksecure`` command to retrieve the current secure boot status
and the burned hash from a device::

  barebox$ rksecure -i

To enable secure boot on a device, run the following command. ``<hash>`` is the
hash returned by the ``rk-otp.sh`` script and ``<bits>`` needs to match the bit
length of the RSA key that you use for image signing::

  barebox$ rksecure -x <hash> -b <bits> -l

.. warn::

   Once you executed this command on a device, the Bootrom will
   reject any image that has not been signed with the appropriate RSA key.

You may check the secure boot status again with ``rksecure -i`` and try
booting an unsigned or corrupted image to verify secure boot is actually
enabled.
