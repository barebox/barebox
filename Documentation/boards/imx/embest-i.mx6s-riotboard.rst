Embest RIoTboard
================

RIoTboard Standard kit
----------------------

http://www.embest-tech.com/prod_view.aspx?TypeId=74&Id=345&FId=t3:74:3

General Features:
  * Product Dimensions: 75mm x 120mm
  * Operating Temperature: 0 ~ 50℃
  * Operating Humidity: 20% ~ 90% (non-condensing)
  * DC-in：5V/4A

Processor:
  * i.MX 6Solo based on ARM Cortex™-A9
  * 32 Kbyte L1 instruction buffer
  * 32 Kbyte L1 data buffer
  * Private counter and watchdog
  * Cortex-A9 NEON MPE (media processing engine) coprocessor

On-Board Memories:
  * 4GByte eMMC
  * 2*512MB DDR3 SDRAM

One-Board Interfaces/Buttons:

  * Audio input/output interfaces
  * A LVDS interface
  * A HDMI interface
  * A LCD display interface (expansion interface)
  * A digital camera interface
  * A MIPI interface
  * 4 TTL-level serial interfaces (one for debugging, the rest for expansion)
  * 4 high-speed USB2.0 host interfaces (480Mbps)
  * A high-speed USB2.0 OTG interface (480Mbps)
  * A SD card slot
  * A TF card slot
  * A 10/100M/1Gbps RJ45 interface
  * 2 I2C interfaces (expansion interfaces)
  * 2 SPI interfaces (expansion interfaces)
  * 3 PWM interfaces (expansion interfaces)
  * A GPIO interface (expansion interfaces)
  * A JTAG interface
  * A boot-mode switch
  * 4 LEDs (1 system LED, 2 custom LED, 1 open SDA LED)
  * A reset button

How to build barebox for Embest RIoTboard
-----------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm imx_v7_defconfig

Build the binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv7compiler

.. note:: replace ``armv7compiler`` with your ARM v7 cross compiler prefix,
 e.g.: ``arm-linux-gnueabihf-``

The resulting binary image to be flashed will be ``images/barebox-embest-imx6s-riotboard.img``.

Replacing U-Boot with barebox
-----------------------------

  1. Connect to the boards's UART (115200 8N1);

  2. Turn board's power on;

  3. Wait ``Hit any key to stop autoboot: 2`` prompt and press any key;

  4. Upload barebox image to the board via tftp and start it

.. code-block:: none

  MX6Solo RIoTboard U-Boot > setenv ethaddr d2:2d:ce:88:f4:f1
  MX6Solo RIoTboard U-Boot > tftpboot 0x20800000 192.168.23.4:ore-barebox-riotboard
  ----phy_addr=0x4, id= 0x4dd072
  FEC: Link is Up 796d
  Using FEC0 device
  TFTP from server 192.168.23.4; our IP address is 192.168.1.103
  Filename 'ore-barebox-riotboard'.
  Load address: 0x20800000
  Loading: ## Warning: gatewayip needed but not set
  ## Warning: gatewayip needed but not set
  ############################################
  done
  Bytes transferred = 637947 (9bbfb hex)
  MX6Solo RIoTboard U-Boot > go 0x20800000

  ## Starting application at 0x20800000 ...

  barebox 2019.04.0-00151-g86762248de #391 Mon Apr 29 14:41:58 CEST 2019

  Board: RIoTboard i.MX6S
  detected i.MX6 Solo revision 1.1
  i.MX reset reason POR (SRSR: 0x00000001)
  mdio_bus: miibus0: probed
  imx-usb 2184200.usb@2184200.of: USB EHCI 1.00
  imx-esdhc 2194000.usdhc@2194000.of: registered as mmc1
  imx-esdhc 2198000.usdhc@2198000.of: registered as mmc2
  imx-esdhc 219c000.usdhc@219c000.of: registered as mmc3
  imx-ipuv3 2400000.ipu@2400000.of: IPUv3H probed
  imx-hdmi 120000.hdmi@120000.of: Detected HDMI controller 0x13:0x1a:0xa0:0xc1
  netconsole: registered as netconsole-1
  malloc space: 0x2fefb5e0 -> 0x4fdf6bbf (size 511 MiB)
  mmc3: detected MMC card version 4.41
  mmc3: registered mmc3.boot0
  mmc3: registered mmc3.boot1
  mmc3: registered mmc3
  partition mmc3.3 not completely inside device mmc3
  mmc3: Failed to register partition 3 on mmc3 (-22)
  envfs: no envfs (magic mismatch) - envfs never written?
  running /env/bin/init...

  Hit m for menu or any key to stop autoboot:    2

  type exit to get to the menu
  barebox@RIoTboard i.MX6S:/
..

  5. Install barebox to the eMMC

.. code-block:: none

  barebox@RIoTboard i.MX6S:/ cp /mnt/tftp/ore-barebox-riotboard /dev/mmc3.barebox
..
