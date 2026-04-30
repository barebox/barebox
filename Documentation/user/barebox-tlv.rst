barebox TLV - Non-Volatile Factory Data Storage
===============================================

barebox TLV ("Tag Length Value" format) is a system to store and
retrieve a device's (read-only) meta-data from non-volatile memory.
It is intended to handle information that are usually only set in
the factory - like serial number, MAC-addresses, analog calibration
data, etc.
Data is stored in a tag-length-value format (hence the name) and read
from non-volatile memory during startup.
Unpacked values are stored in the devicetree ``chosen``-node.

.. graphviz::
  :align: center

   digraph tlv_usage {
     node [shape=box]

     soc_id      [label="SOC ID\n(read from fuses)"]
     tlv_pubkey  [label="TLV Public Key\n(compiled into barebox)"]
     signed_tlv  [label="Signed TLV data\n(read from eeprom)"]
     device_tree [label="Device-Tree"]

     tlv_pubkey  -> signed_tlv  [label="Verifies"]
     soc_id      -> signed_tlv  [label="Compared against" dir=both]
     signed_tlv  -> device_tree [label="Updates"]
   }

barebox TLV consists of two components:

* The parser inside barebox (``common/tlv``).
  This code parses the non-volatile memory during startup.
* A Python-Tool (``scripts/bareboxtlv-generator``).
  This script is intended to generate the TLV binaries in the factory.

Data Format
-----------

The TLV binary has the following format:

.. code-block:: C

     struct tlv {
         be16 tag;
         be16 len;
         u8 payload[];
     };

     struct sig {
         u8 spki_hash_prefix[4];
         u8 signature[];
     };

     struct tlv_format {
         be32 magic;
         be32 length_tlv;   /* in bytes */
         be16 reserved;     /* must be 0 */
         be16 length_sig;   /* in bytes */
         struct tlv tlvs[];
         struct sig signature; /* omitted if length_sig == 0 */
         be32 crc;
     };

Tags
----

Tags are defined as 32-bit integers.
A tag defines the following attributes:

* **Data format:**
  The data format can be something like ``string``, ``decimal`` or
  ``serial-number``.
* **Name of the entry:**
  This is the name under which the value is stored in ``chosen``-node.

The tag range ``0x0000`` to ``0x7FFF`` is intended for common tags,
that can be used in every schema.
These common tags are defined in ``common/tlv/barebox.c``.

.. csv-table:: TLV predefined tags
  :header: "ID", "Name", "Description"
  :widths: 10, 20, 30

	0x0002, device-hardware-release, "Detailed release information string for the device"
	0x0003, factory-timestamp, "UNIX timestamp of fabrication"
	0x0004, device-serial-number, "Device serial number string"
	0x0005, modification, "Modification: 0: Device unmodified; 1: undocumented modifications"
	0x0006, featureset, "A comma separated list of features"
	0x0007, pcba-serial-number, "Printed Circuit Board Assembly serial number string"
	0x0008, pcba-hardware-release, "Printed Circuit Board Assembly hardware release"
	0x0011, ethernet-address, "A list of Ethernet addresses or a single Ethernet address"
	0x0012, ethernet-address, "A sequence of subsequent Ethernet addresses, by number and starting address"
	0x0024, bound-soc-uid, "Reject TLV if supplied binary data does not match UID SoC register"

The tag range ``0x8000`` to ``0xFFFF`` is intended for custom extensions.
Parsing must be handled by board-specific extensions.

Signature
---------

If ``length_sig`` is zero, signature is disabled.  The ``signature`` section of
the file is exactly ``length_sig`` bytes long.  This includes the
``spki_hash_prefix``, followed by the signature itself.  ``spki_hash_prefix``
is the first four bytes of the sha256 hash of the *Subject Public Key Info* and
its purpose is to allow for quicker matching of a signed TLV with its
corresponding public key during signature verification.

The ``signature`` shall be verified on all fields before the ``signature`` field,
but with ``length_sig`` overwritten with 0,
hashed with sha256
and signed using any of the supported algorithms (currently RSA and some ECDSA curves).

Note that ECDSA signatures are not DER encoded but rather plain concatenation
of r and s (each extended to the size of the ECDSA-key).

Data Types
----------

barebox defines a number of common data types.
These are defined in ``common/tlv/barebox.c``.

Board-specific extensions can add additional data types as needed.

TLV Binary Generation
---------------------

To generate a TLV binary the schema for the specific TLV must be defined.
Schemas are yaml-files that represent what the actual parser in barebox expects.

An example schema can be found in ``scripts/bareboxtlv-generator/schema-example.yaml``.
This schema defines some well-known tags and two board-specific tags.

Afterwards another yaml-file with the data for the TLV binary is needed.
An example can be found in ``scripts/bareboxtlv-generator/data-example.yaml``.

.. graphviz::

   digraph tlv_generator {
     node [shape=record fontname="Monospace"]

     schema_yaml [label="{schema.yaml |
           magic: 0x61bb95f3
         \lmax_size: 0x1000
         \ltags:
         \l\  factory-timestamp:
         \l\    tag: 0x0003
         \l\    format: \"decimal\"
         \l\    length: 8
         \l\    example: 1636451762
         \l\  featureset:
         \l\    tag: 0x0006
         \l\    format: \"string\"
         \l\    example: \"base\"
         \l\    purpose: For later use.
         }" style=dashed]

     data_yaml [label="{data.yaml |
           factory-timestamp: 1636451762
         \lfeatureset: \"base\"\l
         }"]

     tlv_key     [label="{tlv.key | PRIVATE KEY}"]
     generator   [label="barebox-tlv-generator.py"]
     signed_bin  [label="{TLV_signed.bin | Signed TLV data}"]

     schema_yaml -> generator
     data_yaml   -> generator
     tlv_key     -> generator
     generator   -> signed_bin
   }

With these information in place a TLV binary can be created:

.. code-block:: shell

   ./bareboxtlv-generator.py --input-data data-example.yaml \
                             schema-example.yaml tlv.bin

To additionally sign the TLV, supply a private key using the ``--sign KEY`` option.

As ``bareboxtlv-generator.py`` internally uses ``openssl pkeyutl`` for
accessing the private key, when using OpenSSL 3, any provider, such as pkcs11,
that is correctly configured, can be used as KEY.

.. note::
  The ``FactoryDataset`` class in ``bareboxtlv-generator.py``
   is intended to be used as a library.

Data Location
-------------

The generated ``tlv.bin`` file has to be stored on the device in a known location.
This location can for example be described inside the devicetree of the device.

.. code-block:: dts
    :emphasize-lines: 8,10

    &eeprom1 {
      partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        tlv_partition: partition@0 {
          compatible = "barebox,tlv-v1";
          label = "barebox_tlv";
          reg = <0x0 0x1000>;
        };
      };
    };

The ``compatible`` defines the format to parse the TLV data as and the ``reg``
describes the size of the data.

Custom TLV format
-----------------

A custom TLV format can be created for example like this:

.. code-block:: c
   :linenos:

    #include <string.h>
    #include <tlv/tlv.h>
    #include <common.h>

    #define TLV_MAGIC_CUSTOM_V1_SIGNED 0xe3573cd3

    static struct tlv_mapping custom_tlv_v1_mappings[] = {
      /* UNIX timestamp of fabrication */
      { 0x0003, tlv_format_timestamp, "factory-timestamp" },
      /* Device serial number string */
      { 0x0004, tlv_handle_serial, "device-serial-number" },
      /* a comma separated list of features */
      { 0x0006, tlv_format_str, "featureset" },
      /* Printed Circuit Board Assembly serial number string */
      { 0x0007, tlv_format_str, "pcba-serial-number" },
      /* Reject TLV if supplied binary data does not match UID SoC register */
      { 0x0024, tlv_bind_soc_uid, "bound-soc-uid" },
      /* Custom key */
      { 0x8001, tlv_format_str, "custom-key"},
      { /* sentintel */ },
    };

    static struct tlv_mapping *mappings[] = {
      custom_tlv_v1_mappings,
      NULL
    };

    static struct of_device_id matches[] = {
      { .compatible = "custom,tlv-v1" },
      { /* sentinel */ }
    };

    static struct tlv_decoder custom_tlv_v1 = {
      .magic = TLV_MAGIC_CUSTOM_V1_SIGNED,
      .driver.name = "custom-tlv-v1",
      .driver.of_compatible = matches,
      .mappings = mappings,
      .signature_keyring = "tlv-custom",
    };

    static int custom_tlv_v1_register(void)
    {
      return tlv_register_decoder(&custom_tlv_v1);
    }
    of_populate_initcall(custom_tlv_v1_register);

* line 7-21: Every possible mapping, that is used must be listed here.
* line 19: The mapping includes a custom tag.
  All tags above ``0x8000`` are reserved for custom use.
* line 29: The compatible string of the partition, that will contain the data.
* line 5,34: Some randomly generated 32bit value to uniquely identify the
  mapping-table.
* line 38: The keyring tlv-stange should be used to validate the signature.
  Keys for the keyring are specified in the barebox config
  ``CONFIG_CRYPTO_PUBLIC_KEYS`` with for example:
  ``keyring=tlv-custom:__ENV__TLV_KEY``.
* line 41-45: Registers the format into barebox.
