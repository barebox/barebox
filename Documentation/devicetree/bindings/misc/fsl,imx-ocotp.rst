Freescale i.MX OCOTP (On-Chip OTP)
==================================

Required properties:

* ``compatible``: ``fsl,imx6q-ocotp``
* ``reg``: physical register base and size

Optional properties:

* ``barebox,provide-mac-address``: Provide MAC addresses for Ethernet devices. This
  can be multiple entries in the form <&phandle regofs> to assign a MAC
  address to an Ethernet device.

Example:

.. code-block:: none

  ocotp1: ocotp@021bc000 {
  	compatible = "fsl,imx6q-ocotp";
  	reg = <0x021bc000 0x4000>;
  	barebox,provide-mac-address = <&fec 0x620>;
  };
