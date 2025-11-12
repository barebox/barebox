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
