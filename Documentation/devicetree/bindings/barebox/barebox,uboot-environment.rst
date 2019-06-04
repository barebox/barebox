U-Boot environment device
=========================

This driver provides a unified device exposing U-Boot environment
varaible data, sans the low-level parts. Resulting device is intended
to be used with corresponding filesystem driver to expose environment
data as a filesystem.

Required properties:

* ``compatible``: should be ``barebox,uboot-environment``
* ``device-path``: phandle of the partition the device environment is
  on (single partiton configuration)
* ``device-path-0`` and ``device-path-1``: phandle of the partition
  the environment is on (redundant configuration)

Example:

.. code-block:: none

  environment {
  	compatible = "barebox,uboot-environment";
	device-path-0 = &uboot_env_0;
	device-path-1 = &uboot_env_1;
  };
  
  &usdhc4 {
	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;

		uboot_env_0: partition@c0000 {
			label = "uboot-environment-0";
			reg = <0xc0000 0x4000>;
		};

		uboot_env_1: partition@cc800 {
			label = "uboot-environment-1";
			reg = <0xcc800 0x4000>;
		};
	};
  };
