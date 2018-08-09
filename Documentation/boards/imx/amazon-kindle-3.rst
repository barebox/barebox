Amazon Kindle 3 "Kindle Keyboard" Model No. D00901
==================================================

This e-book reader is based on a Freescale i.MX35 SOC.
The device is equiped with:

* 256MiB synchronous dynamic RAM
* 4GiB eMMC
* a MC13892 PMIC

The device boots in internal boot mode from eMMC and is shipped with a
vendor modified u-boot imximage.

To upload and run a new bootloader the device can be put into USB-downloader
mode by the SOC microcode when Vol+ is pressed during startup. A new USB
device "SE Blank RINGO" should appear, barebox may be uploaded using

.. code-block:: console

        $ scripts/imx/imx-usb-loader barebox.imximg

Note: a USB serial ACM console will be launched by a barebox init script
when the cursor select key is pressed during startup (e.g. before running
imx-usb-loader)

Barebox may be used as drop-in replacement for the shipped bootloader.
When installing the barebox imximg on the eMMC take care not to overwrite
the partition table and vendor supplied serial numbers stored on the eMMC.
e.g. just write the imx-header and the application section:

.. code-block:: sh

        memcpy -b -s barebox.imximg -d /dev/disk0.imx_header 1024 0 1024
        memcpy -b -s barebox.imximg -d /dev/disk0.self 4096 0 195584
