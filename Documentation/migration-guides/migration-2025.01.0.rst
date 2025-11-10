Release v2025.01.0
==================

Layerscape
----------

Support for the NXP-specific PPA Secure Monitor has been removed in
favor of ARM Trusted Firmware-A as the former has been long unmaintained.

I2C Adapter numbering
---------------------

``i2c0`` to ``i2c31`` are now reserved for I2C adapters with device tree
aliases. Do not rely on a fixed numbering for other non-aliased adapters.

OP-TEE for Rockchip
-------------------

The BL32 firmware images are now called ``firmware/rk3XXX-bl32.bin``
instead of ``firmware/rk3XXX-op-tee.bin``. Raw OP-TEE binaries without
a header now use the load address the vendor blobs are hardcoded to
run at instead of the previous ``0x200000``. To override, use an OP-TEE
image with a header specifying the correct load address.

TFTP
----

The default TFTP window size has been reverted to 1 as many network
drivers couldn't cope with the increased throughput.
This can be reverted by setting ``CONFIG_FS_TFTP_MAX_WINDOW_SIZE``
or at runtime via the ``global.tftp.windowsize`` variable.
