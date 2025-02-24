.. _devicetree_binding_mtd_partition:

Representing flash partitions in devicetree
===========================================

In addition to the upstream binding, barebox supports parsing OF partitions
for EEPROMs and for SD/MMC as well. SD/MMC OF partitions in barebox are
allowed to co-exist with GPT/MBR partitions as long as they don't overlap.

This is different from the Linux behavior where OF partitions have precedence
over GPT/MBR. For this reason, barebox also accepts the compatible
``barebox,fixed-partitions``, which is handled inside barebox identically to
``fixed-partitions``, but is ignored by Linux.

Additionally, barebox supports an extra optional property:

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

In addition to the upstream ``fixed-partitions`` compatible binding,
barebox also supports a ``barebox,fixed-partitions`` binding.

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

  sd {
  	partitions {
  		compatible = "barebox,fixed-partitions";
  		#address-cells = <1>;
  		#size-cells = <1>;

  		barebox-env@1000 {
  			label = "barebox-environment";
  			reg = <0x1000 0x20000>;
  		};
  	};
  };
