Freescale i.MX OCOTP (On-Chip OTP)
==================================

Required properties:

* ``compatible``: ``fsl,imx6q-ocotp``
* ``reg``: physical register base and size

Deprecated properties:

* ``barebox,provide-mac-address``: Provide MAC addresses for Ethernet devices. This
  can be multiple entries in the form <&phandle regofs> to assign a MAC
  address to an Ethernet device. This has been deprecated in favor or the upstream
  nvmem cell binding.

Legacy example:

.. code-block:: none

  ocotp1: ocotp@021bc000 {
  	compatible = "fsl,imx6q-ocotp";
  	reg = <0x021bc000 0x4000>;
  	barebox,provide-mac-address = <&fec 0x620>;
  };

Upstream alternative:

.. code-block:: none

  &ocotp1 {
  	#address-cells = <1>;
  	#size-cells = <1>;

  	fec_mac_addr: mac-addr@88 {
  		reg = <0x88 6>;
  	};
  };

  &fec {
  	nvmem-cells = <&fec_mac_addr>;
  	nvmem-cell-names = "mac-address";
  };
