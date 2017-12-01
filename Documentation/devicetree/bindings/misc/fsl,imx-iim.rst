Freescale i.MX IIM (Ic Identification Module)
=============================================

Required properties:

* ``compatible``: ``fsl,imx27-iim``, ``fsl,imx51-iim``
* ``reg``: physical register base and size

Optional properties:

* ``barebox,provide-mac-address``: Provide MAC addresses for Ethernet devices. This
  can be multiple entries in the form <&phandle bankno fuseofs> to assign a MAC
  address to an Ethernet device.

Example:

.. code-block:: none

  iim: iim@83f98000 {
  	compatible = "fsl,imx51-iim", "fsl,imx27-iim";
  	reg = <0x83f98000 0x4000>;
  	barebox,provide-mac-address = <&fec 1 9>;
  };
