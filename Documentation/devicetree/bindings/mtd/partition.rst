.. _devicetree_binding_mtd_partition:

Representing flash partitions in devicetree
===========================================

In addition to the upstream binding, another property is added:

Optional properties:

* ``partuuid``: The global partition UUID for this partition.
  For GPT partitions, the partuuid is the 16-byte GPT Partition UUID (e.g.
  ``de6f4f5c-c055-4374-09f7-8c6821dfb60e``).
  For MBR partitions, the partuuid is the 4-byte disk identifier
  followed by a dash and the partition number (starting with 1, e.g.
  ``c9178f9d-01``).

  The partuuid is case-insensitive.

Additionally, barebox also supports partitioning the eMMC boot partitions if
the partition table node is named appropriately:

* ``partitions`` : user partition
* ``boot0-partitions`` : boot0 partition
* ``boot1-partitions`` : boot1 partition

``boot0-partitions`` and ``boot1-partitions`` are deprecated. Use ``partitions-boot1``
and ``partitions-boot2`` instead which is supported under Linux as well.

Examples:

.. code-block:: none

  / {
  	partitions {
  		compatible = "fixed-partitions";
  		#address-cells = <1>;
  		#size-cells = <1>;

  		state_part: state {
  			partuuid = "16367da7-c518-499f-9aad-e1f366692365";
  		};
  	};
  };

  emmc@1 {
  	boot0-partitions {
  		compatible = "fixed-partitions";
  		#address-cells = <1>;
  		#size-cells = <1>;

  		barebox@0 {
  			label = "barebox";
  			reg = <0x0 0x300000>;
  		};
  	};
  };
