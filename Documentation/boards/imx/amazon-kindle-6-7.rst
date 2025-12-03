Amazon Kindle 6/7 (Paperwhite 2/3, Kindle 7th gen and Voyage)
=============================================================

Four models of the Amazon Kindle e-Book readers are considered as members
of the 6th and 7th generation:

* DP75SDI "PINOT_WFO" (256MB) "Kindle Paperwhite 2" 6th gen
* DP75SDI "Muscat WFO" (512MB) "Kindle Paperwhite 3" 7th gen
* WP63GW (512MB) "Kindle" 7th gen
* NM460GZ (512MB) "Voyage" 7th gen

Those e-book readers share a common set of hardware:

* Freescale i.MX6SL SOC
* 256MB or 512MB of LPDDR2
* eMMC storage
* MAX77696A PMIC

The devices boot up in internal boot mode from eMMC boot partition 1
and are shipped with a vendor modified u-boot imximage based on u-boot
v2009.08.

According to the availability of source code tarballs on the amazon website
"Source Code Notice for Kindle E-Readers and Fire Tablets", factory image
updates seem to have been terminated between 2021 and 2023.

This device is battery-powered and there is no way to switch the device off.
When the device is inactive, the Kindle software will first reduce the
power consumption to a few milliamps of battery power, after some minutes
the power consumption is further reduced to sub-milliamp level. Keeping
iomux pullups switched on at standby may significantly drain your battery.

Building barebox
----------------

``make kindle-gen-6-7_defconfig`` should get you a working config.

Uploading barebox
-----------------

Similar to the 4/5 gen devices, console access to gen 6/7 kindles is
available via the Micro-USB connector when a 30k Ohm resistor to GND
is used on the sense pin: USB D- will become TXD, D+ RXD.

From the uboot shell, the barebox image may be chainloaded e.g. like

   .. code-block:: console

        $ bist
        $ loady 0x80000000
        $ go 0x80001000

Booting the devices from USB is more demanding and will likely require
opening of the device and desoldering of a metal shield.
All devices feature just a single power button which may be used to reset
the device. Unlike the models of the 4th generation where a separate
button is available to boot from USB, such buttons do not exist here.
Button pads on the PCB are not populated and USB boot mode by
pin-strapping seems to be disabled by fuse settings. To put the device
into USB boot mode, the eMMC may be disabled temporarily.

1. Connect the Kindle to your host computer with a USB cable.
2. Disable the eMMC
   (for the DP75SDI pull the left resistor left of TP806/TP807 (below
   TP914) facing the eMMC to zero).
3. Power down the device by holding the power button until the power LED goes
   dark (about 12-15 seconds).
4. Release the power button.
5. Free the eMMC pin.
6. After 10-20 seconds, a new USB device named ``SE Blank MEGREZ`` should
   appear on your host computer.
7. Use imx-usb-loader to boot from USB.

   .. code-block:: console

        $ scripts/imx/imx-usb-loader images/barebox-kindle6-dp75sdi.img

A USB serial ACM console will be launched by a barebox init script
for 10 seconds after boot.

Installing barebox
------------------

Barebox may be used as drop-in replacement for the shipped bootloader, when
the imximg fits into 384000 bytes, compression like IMAGE_COMPRESSION_XZKERN
might be required for this. When installing the barebox imximg on
the eMMC, take care not to overwrite the vendor supplied serial numbers and
other data stored on the eMMC. All 6th and 7th gen devices seem to share the
same disk layout. E.g. just write the imx-header and the application section:

.. code-block:: console

        $ loady -t usbserial
        $ memcpy -s barebox-kindle6-dp75sdi.img -d /dev/mmc1.boot0 0x400 0x400 0x5dc00
