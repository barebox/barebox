Amazon Kindle 4/5 (Wi-Fi/No-Touch, Touch and Paperwhite)
========================================================

The Kindle Models No. D01100 (Kindle Wi-Fi, also known as No-Touch or K4NT),
D01200 (Kindle Touch)
and EY21 (Paperwhite) are refered as the Kindle 4th and 5th generation.
Those e-book readers share a common set of hardware:

* a Freescale i.MX50 SOC
* 2 or 4GiB eMMC
* a MC13892 PMIC

The older readers D01100 and D01200 use 256MiB of LPDDR1,
while the newer EY21 uses 256MiB of LPDDR2.

The devices boot up in internal boot mode from an eMMC boot partition and
are shipped with a vendor modified u-boot imximage based on u-boot v2009.08.

This device is battery-powered and there is no way to switch the device off.
When the device is inactive, the Kindle software will first reduce the
power consumption to a few milliamps of battery power, after some minutes
the power consumption is further reduced to about 550 microamps. Switching
on iomux pullups may significantly reduce your standby-time.

Building barebox
----------------

``make kindle-mx50_defconfig`` should get you a working config.

Uploading barebox
-----------------

To upload and run a new bootloader, the older devices can be put into
USB bootloader mode by the SoC microcode:

1. Connect the Kindle to your host computer with a USB cable.
2. Power down the device by holding the power button until the power LED goes
   dark (about 10 seconds).
4. Hold the power button, and hold down a device-specific special key:
   * the fiveway down button on the model D01100
   * the home button on model D01200
4. Then release the power button, but still hold the special key.
5. A new USB device named ``NS Blank CODEX`` should appear on your host computer.
   You can now release the special button.
7. Finally, upload barebox to the Kindle by using:

   .. code-block:: console

        $ scripts/imx/imx-usb-loader barebox-kindle-d01100.img
        $ scripts/imx/imx-usb-loader barebox-kindle-d01200.img

Additionally, a USB serial ACM console will be launched by a barebox init script
when:

* the cursor select key is pressed during startup of model D01100
* the home button is pressed within a second after startup of model D01200.
  (If you press the home button during startup, you will enter USB boot mode.)
* the EY21 has no keys to press, a USB console will be launched for 10s.

Barebox may be used as drop-in replacement for the shipped bootloader, when
the imximg fits into 258048 bytes. When installing the barebox imximg on
the eMMC, take care not to overwrite the vendor supplied serial numbers stored
on the eMMC,
e.g. for the D01100 just write the imx-header and the application section:

.. code-block:: console

        $ loady -t usbserial
        $ memcpy -b -s barebox-kindle-d01100.img -d /dev/disk0.boot0.imx_header 1024 0 2048
        $ memcpy -b -s barebox-kindle-d01100.img -d /dev/disk0.boot0.self 4096 0 253952
