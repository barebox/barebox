barebox TLV - Non-Volatile Factory Data Storage
===============================================

barebox TLV is a system to store and retrieve a device's (read-only)
meta-data from non-volatile memory.
It is intended to handle information that are usually only set in
the factory - like serial number, MAC-addresses, analog calibration
data, etc.
Data is stored in a tag-length-value format (hence the name) and read
from non-volatile memory during startup.
Unpacked values are stored in the devicetree ``chosen``-node.

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
         be16 tag; /* 2 bytes */
         be16 len; /* 2 bytes */
         u8 payload[];
     };

     struct binfile {
         be32 magic_version;
         be32 length_tlv; /* 4 bytes */
         be32 length_sig; /* 4 bytes */
         struct tlv tlvs[];
         be32 crc32;
     };

.. note::
  Even though the header has a ``length_sig`` field,
  there is currently no support for cryptographic signatures.

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

The tag range ``0x8000`` to ``0xFFFF`` is intended for custom extensions.
Parsing must be handled by board-specific extensions.

Data Types
----------

barebox defines a number of common data types.
These are defined in ``common/tlv/barebox.c``.

Board-specific extensions can add additional data types as needed.

TLV Binary Generation
---------------------

To generate a TLV binary the schema for the specific TLV must be defined.
Schemas are yaml-files that represent what the actual parser in barebox expects.

An example schema can be found in ``srcipts/bareboxtlv-generator/schema-example.yaml``.
This schema defines some well-known tags and two board-specific tags.

Afterwards another yaml-file with the data for the TLV binary is needed.
An example can be found in ``srcipts/bareboxtlv-generator/data-example.yaml``.

With these information in place a TLV binary can be created:

.. code-block:: shell

   ./bareboxtlv-generator.py --input-data data-example.yaml \
                             schema-example.yaml tlv.bin

.. note::
  The ``FactoryDataset`` class in ``bareboxtlv-generator.py``
   is intended to be used as a library.
