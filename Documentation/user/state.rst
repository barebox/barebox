.. _state_framework:

Barebox State Framework
=======================

Boards often have the need to store variable sets in persistent memory. barebox
could handle that with its regular environment. But the constraints for such a
variable set are often different from what the regular environment can do:

* compact binary format to make it suitable for small non-volatile memories
* atomic save/restore of the whole variable set
* redundancy

*state* is a framework to describe, access, store and restore a set of variables
and is prepared to exchange/share data between barebox and Linux userspace using
some kind of persistent memory.

Already known users of the *state* information are the :ref:`bootchooser` and
RAUC_.

.. _RAUC: https://rauc.readthedocs.io/en/latest/

barebox itself uses a *state* driver to access the variables in the
persistent memory. For the Linux run-time there is a userspace tool_ to do
the same.

.. _tool: https://git.pengutronix.de/cgit/tools/dt-utils/

To define a *state* variable set, a devicetree based description is used. Refer to
:ref:`barebox,state` for further details.

There are several software components involved, which are described in this
section.

.. _state_framework,backends:

Backends (e.g. Supported Memory Types)
--------------------------------------

Some non-volatile memory is need for storing a *state* variable set:

- Disk like devices: SD, (e)MMC, ATA
- all kinds of NAND and NOR flash memories (mtd)
- MRAM
- EEPROM
- all kind of SRAMs (backup battery assumed)

For classic MTDs (NOR/NAND/SRAM), a partition is required and understood by
the Linux kernel as well to define the location inside the device where to store
the *state* variable set. For non-MTDs (MRAM/EEPROM) a different approach is
required to define the location where to store the *state* variable set.

.. _state_framework,backend_types:

Backend-Types (e.g. *state* storage format)
-------------------------------------------

The *state* variable set itself can be stored in different ways. Currently two
formats are available, ``raw`` and ``dtb``.

Both serialize the *state* variable set differently.

.. note:: The ``raw`` serializer additionally supports an HMAC algorithm to
   detect manipulations. Refer to :ref:`HMAC <barebox,state_hmac>` for further
   details.

.. _state_framework,raw:

The ``raw`` *state* storage format
##################################

``raw`` means the *state* variable set is a simple binary data blob only. In
order to handle it, the *state* framework puts a management header in front of
the binary data blob with the following content and layout:

 +---------+---------+---------------------------------------------------+
 | Offset  |   Size  |    Content                                        |
 +---------+---------+---------------------------------------------------+
 |  0x00   | 4 bytes | 'magic value'                                     |
 +---------+---------+---------------------------------------------------+
 |  0x04   | 2 bytes | reserved, filled with zero bits                   |
 +---------+---------+---------------------------------------------------+
 |  0x06   | 2 bytes | byte count of binary data blob                    |
 +---------+---------+---------------------------------------------------+
 |  0x08   | 4 bytes | CRC32 of binary data blob (offset 0x10...)        |
 +---------+---------+---------------------------------------------------+
 |  0x0c   | 4 bytes | CRC32 of header (offset 0x00...0x0b)              |
 +---------+---------+---------------------------------------------------+
 | 0x10... |         | binary data blob                                  |
 +---------+---------+---------------------------------------------------+

- 'magic value' is an unsigned value with native endianness, refer to
  :ref:`'magic' property <barebox,state_magic>` about its value.
- 'byte count' is an unsigned value with native endianness
- 'binary data blob CRC32' is an unsigned value with native endianness
- 'header CRC32' is an unsigned value with native endianness

.. note:: the 32-bit CRC calculation uses the polynomial:

  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1

Since the binary data blob has no built-in description of the embedded *state*
variable set, it depends on an external layout definition to encode
and decode it correctly. A devicetree based description is used to describe the
embedded *state* variable set. Refer to
:ref:`Variable Subnodes <barebox,state_variable>` for further details.

.. important:: It is important to share this layout definition in all
   'worlds' which want to read or manipulate the *state* variable set. This
   includes offsets, sizes and endianesses of the binary data. Refer to
   :ref:`Configuring the state variable set <barebox,state_setup>` on how to
   setup barebox to ensure this is done automatically for devicetree based
   operating systems.

.. note:: When calculating the ``backend-stridesize`` take the header overhead
   into account. The header overhead is always 16 bytes.

.. _state_framework,dtb:

The ``dtb`` *state* storage format
##################################

.. note:: The ``dtb`` backend type isn't well tested. Use the ``raw`` backend
          when in doubt.

The ``dtb`` backend type stores the *state* variable set as a devicetree binary
blob. This is exactly the original devicetree description of the *state* variable
set itself, but additionally contains the actual values of the variable set.
Unlike the ``raw`` *state* backend the ``dtb`` *state* backend can describe itself.

.. _state_framework,backend_storage_type:

Backend Storage Types (e.g. media storage layout)
-------------------------------------------------

The serialized data (``raw`` or ``dtb``) can be stored to different backend
storage types. These types are dedicated to different memory types.

Currently two backend storage type implementations do exist, ``circular`` and
``direct``.

The state framework can select the correct backend storage type depending on the
backend medium. Media requiring erase operations (NAND, NOR flash) defaults to
the ``circular`` backend storage type automatically. In contrast EEPROMs and
RAMs are candidates for the ``direct`` backend storage type.

Direct Storage Backend
######################

This kind of backend storage type is intended to be used with persistent RAMs or
EEPROMs.
These media are characterized by:

- memory cells can be simply written at any time (no previous erase required)
- memory cells can be written as often as required (unlimted or very high endurance)
- can be written on a byte-by-byte manner

Example: MRAM with 64 bytes at device's offset 0:

.. code-block:: text

    0                                                                 0x3f
    +-------------------------------------------------------------------+
    |                                                                   |
    +-------------------------------------------------------------------+

Writing the *state* variable set always happens at the same offset:

.. code-block:: text

    0                                                                 0x3f
    +-------------------------------------------+-----------------------+
    |                 copy                      |                       |
    +-------------------------------------------+-----------------------+

.. important:: The ``direct`` storage backend needs 8 bytes of additional space
   per *state* variable set for its meta data.

Circular Storage Backend
########################

This kind of backend storage type is intended to be used with regular flash memory devices.

Flash memories are characterized by:

- only erased memory cells can be written with new data
- written data cannot be written twice (at least not for modern flash devices)
- erase can happen on eraseblock sizes only (detectable, physical value)
- an eraseblock only supports a limited number of write-erase-cycles (as low as a few thousand cycles)

The purpose of the ``circular`` backend storage type is to save erase cycles
which may wear out the flash's eraseblocks. This type instead incrementally fills
an eraseblock with updated data and only when an eraseblock
is fully written, it erases it and starts over writing new data to the same
eraseblock again.

**NOR type flash memory is additionally characterized by**

- can be written on a byte-by-byte manner

.. _state_framework,nor:

Example: NOR type flash memory with 64 kiB eraseblock size

.. code-block:: text

    0                                                                0x0ffff
    +--------------------------------------------------------------------+
    |                                                                    |
    +--------------------------------------------------------------------+
    |<--------------------------- eraseblock --------------------------->|

Writing the *state* variable set the very first time:

.. code-block:: text

    0
    +------------+------------
    |   copy     |
    |    #1      |
    +------------+------------
    |<- stride ->|
    |<---- eraseblock -------

'copy#1' will be used.

Changing the *state* variable set the first time (e.g. writing it the second time):

.. code-block:: text

    0
    +------------+------------+------------
    |   copy     |   copy     |
    |    #1      |    #2      |
    +------------+------------+------------
    |<- stride ->|<- stride ->|
    |<------------- eraseblock -----------

'copy#2' will now be used and 'copy#1' will be ignored.

Changing the *state* variable set the n-th time:

.. code-block:: text

    0                                                                0x0ffff
    +------------+------------+-------- -------+------------+------------+
    |   copy     |   copy     |                |    copy    |   copy     |
    |    #1      |    #2      |                |    #n-1    |    #n      |
    +------------+------------+-------- -------+------------+------------+
    |<- stride ->|<- stride ->|                |<- stride ->|<- stride ->|
    |<---------------------------- eraseblock -------------------------->|

'copy#n' will now be used and all other copies will be ignored.

The next time the *state* variable set changes again, the whole block will be
erased and the *state* variable set gets stored at the first position inside
the eraseblock again. This reduces the need for a flash memory erase by factors.

**NAND type flash memory is additionally characterized by**

- organized in pages (size is a detectable, physical value)
- writes can only happen in multiples of the page size (which much less than the eraseblock size)
- partially writing a page can be limited in count or be entirely forbidden (in
  the case of *MLC* NANDs)

Example: NAND type flash memory with 128 kiB eraseblock size and 2 kiB page
size and a 2 kiB write size

.. code-block:: text

    0                                                             0x20000
    +------+------+------+------+---- ----+------+------+------+------+
    | page | page | page | page |         | page | page | page | page |
    |  #1  |  #2  |  #3  |  #4  |         | #61  | #62  | #63  | #64  |
    +------+------+------+------+---- ----+------+------+------+------+
    |<-------------------------- eraseblock ------------------------->|

Writing the *state* variable set the very first time:

.. code-block:: text

    |<--- page #1---->|
    +-------+---------+--
    | copy  |         |
    |  #1   |         |
    +-------+---------+--
    |<---- eraseblock ---

'copy#1' will be used.

Changing the *state* variable set the first time (e.g. writing it the second time):

.. code-block:: text

    |<-- page #1 -->|<-- page #2 -->|
    +-------+-------+-------+-------+----
    | copy  |       | copy  |       |
    |  #1   |       |  #2   |       |
    +-------+-------+-------+-------+----
    |<--------- eraseblock --------------

'copy#2' will now be used and 'copy#1' will be ignored.

Changing the *state* variable set the 64th time:

.. code-block:: text

    |<-- page #1 -->|<-- page #2 -->|        |<- page #63 -->|<- page #64 -->|
    +-------+-------+-------+-------+--    --+-------+-------+-------+-------+
    | copy  |       | copy  |       |        | copy  |       | copy  |       |
    |  #1   |       |  #2   |       |        |  #63  |       |  #64  |       |
    +-------+-------+-------+-------+--    --+-------+-------+-------+-------+
    |<----------------------------- eraseblock ----------------------------->|

'copy#n' will now be used and all other copies will be ignored.

The next time the *state* variable set changes again, the eraseblock will be
erased and the *state* variable set gets stored at the first position inside
the eraseblock again. This significantly reduces the need for a block erases.

.. important:: One copy of the *state* variable set is limited to the page size
   of the used backend (e.g. NAND type flash memory)

Redundant *state* variable set copies
-------------------------------------

To avoid data loss when changing the *state* variable set, more than one
*state* variable set copy can be stored into the backend. Whenever the *state*
variable set changes, only one *state* variable set copy gets changed at a time.
In the case of an interruption and/or power loss resulting into an incomplete
write to the backend, the system can fall back to a different *state* variable
set copy (previous *state* variable set).

Direct Storage Backend Redundancy
#################################

For this kind of backend storage type a value for the stride size must be
defined by the developer (refer to
:ref:`backend-stridesize <barebox,state_backend_stridesize>`).

It always stores **three** redundant copies of the backend-type. Keep this in
mind when calculating the stride size and defining the backend size (e.g. the
size of a partition).

.. code-block:: text

    +----------------+------+----------------+------+----------------+------+
    | redundant copy | free | redundant copy | free | redundant copy | free |
    +----------------+------+----------------+------+----------------+------+
    |<---- stride size ---->|<---- stride size ---->|<---- stride size ---->|

.. important:: The ``direct`` storage backend needs 8 bytes of additional space
   per *state* variable set for its meta data. Keep this in mind when calculating
   the stridesize. For example, if your variable set needs 8 bytes, the ``raw``
   header needs 16 bytes and the ``direct`` storage backend additionally 8 bytes.
   The full space for one *state* variable set is now 8 + 16 + 8 = 32 bytes.

Circular Storage Backend Redundancy
###################################

**NOR type flash memory**

Redundant copies of the *state* variable set are stored based on the memory's
eraseblocks and this size is automatically detected at run-time.
It needs a stride size as well, because a NOR type flash memory can be written
on a byte-by-byte manner.
In contrast to the ``direct`` storage backend redundancy, the
stride size for the ``circular`` storage backend redundancy defines the
side-by-side location of the *state* variable set copies.

.. code-block:: text

    |<X>|<X>|...
    +--------------------------------+--------------------------------+--
    |C#1|C#2|C#3|C#4|C#5|            |C#1|C#2|C#3|C#4|C#5|            |
    +--------------------------------+--------------------------------+--
    |<--------- eraseblock --------->|<--------- eraseblock --------->|<-
    |<------- redundant area ------->|<------- redundant area ------->|<-

*<X>* defines the stride size, *C#1*, *C#2* the *state* variable set copies.

Since these kinds of MTD devices are partitioned, its a good practice to always
reserve multiple eraseblocks for the barebox's *state* feature. Keep in mind:
Even NOR type flash memories can be worn out.

**NAND type flash memory**

Redundant copies of the *state* variable set are stored based on the memory's
eraseblocks and this size is automatically detected at run-time.

.. code-block:: text

    +------+------+--- ---+------+------+------+------+--- ---+------+------+--
    | copy | copy |       | copy | copy | copy | copy |       | copy | copy |
    |  #1  |  #2  |       | #63  | #64  |  #1  |  #2  |       | #63  | #64  |
    +------+------+--- ---+------+------+------+------+--- ---+------+------+--
    |<----------- eraseblock ---------->|<----------- eraseblock ---------->|<-
    |<-------- redundant area --------->|<-------- redundant area --------->|<-

Since these kinds of MTD devices are partitioned, its a good practice to always
reserve multiple eraseblocks for the barebox's *state* feature. Keep in mind:
NAND type flash memories can be worn out, factory bad blocks can exist from the
beginning.

Handling of Bad Blocks
----------------------

NAND type flash memory can have factory bad eraseblocks and more bad
eraseblocks can appear over the life time of the memory. They are detected by
the MTD layer, marked as bad and never used again.

.. important:: If NAND type flash memory should be used as a backend, at least
   three eraseblocks are used to keep three redundant copies of the *state*
   variable set. You should add some spare eraseblocks to the backend
   partition by increasing the partition's size to a suitable value to handle
   factory bad eraseblocks and worn-out eraseblocks.

Examples
--------

The following examples intends to show how to setup and interconnect all
required components for various non-volatile memories.

All examples use just one *state* variable of type *uint8* named ``variable``
to keep them simple. For the ``raw`` backend-type it means one *state*
variable set has a size of 17 bytes (16 bytes header plus one byte variables).

.. note:: The mentioned ``aliases`` and the *state* variable set node entries
   are members of the devicetree's root node.

.. note:: For a more detailed description of the used *state* variable set
   properties here, refer to :ref:`barebox,state`.

NOR flash memories
##################

This type of memory can be written on a single byte/word basis (depending on its bus
width), but must be erased prior writing the same byte/word again and erasing
must happen on an eraseblock basis. Typical eraseblock sizes are 128 kiB or
(much) larger for parallel NOR flashes and 4 kiB or larger for serial NOR
flashes.

From the Linux kernel perspective this type of memory is a *Memory Technologie
Device* (aka 'MTD') and handled by barebox in the same manner. It needs a
partition configuration.

The following devicetree node entry defines some kind of NOR flash memory and
a partition at a specific offset to be used as the backend for the
*state* variable set.

.. code-block:: text

	norflash@0 {
		backend_state_nor: partition@120000 {
			[...]
		};
	};

With this 'backend' definition its possible to define the *state* variable set
content, its backend-type and *state* variable set layout.

.. code-block:: text

	aliases {
		state = &state_nor;
	};

	state_nor: nor_state_memory {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "barebox,state";
		magic = <0x512890a0>;
		backend-type = "raw";
		backend = <&backend_state_nor>;
		backend-storage-type = "circular";
		backend-stridesize = <32>;

		variable {
			reg = <0x0 0x1>;
			type ="uint8";
			default = <0x1>;
		};
	};

NAND flash memories
###################

This type of memory can be written on a *page* base (typically 512 bytes,
2048 or (much) larger), but must be erased prior writing the same page again and
erasing must happen on an eraseblock base. Typical eraseblock sizes are
64 kiB or (much) larger.

From the Linux kernel perspective this type of memory is a *Memory Technologie
Device* (aka 'MTD') and handled by barebox in the same manner. It needs a
partition configuration.

The following devicetree node entry defines some kind of NAND flash memory and
a partition at a specific offset inside it to be used as the backend for the
*state* variable set.

.. code-block:: text

	nandflash@0 {
		backend_state_nand: partition@500000 {
			[...]
		};
	};

With this 'backend' definition its possible to define the *state* variable set
content, its backend-type and *state* variable layout.

.. code-block:: text

	aliases {
		state = &state_nand;
	};

	state_nand: nand_state_memory {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "barebox,state";
		magic = <0xab67421f>;
		backend-type = "raw";
		backend = <&backend_state_nand>;
		backend-storage-type = "circular";

		variable {
			reg = <0x0 0x1>;
			type ="uint8";
			default = <0x1>;
		};
	};

SD/eMMC and ATA
###############

The following devicetree node entry defines some kind of SD/eMMC memory and
a partition at a specific offset inside it to be used as the backend for the
*state* variable set. Note that currently there is no support for on-disk
partition tables. Instead, a ofpart partition description must be used. You
have to make sure that this partition does not conflict with any other partition
in the partition table.

.. code-block:: text

	backend_state_sd: part@100000 {
		label = "state";
		reg = <0x100000 0x20000>;
	};

With this 'backend' definition its possible to define the *state* variable set
content, its backend-type and *state* variable layout.

.. code-block:: text

	aliases {
		state = &state_sd;
	};

	state_sd: state_memory {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "barebox,state";
		magic = <0xab67421f>;
		backend-type = "raw";
		backend = <&backend_state_sd>;
		backend-stridesize = <0x40>;

		variable {
			reg = <0x0 0x1>;
			type ="uint8";
			default = <0x1>;
		};
	};

SRAM
####

This type of memory can be written on a byte base and there is no need for an
erase prior writing a new value.

From the Linux kernel perspective this type of memory is a *Memory Technologie
Device* (aka 'MTD') and handled by barebox in the same manner. It needs a
partition definition.

The following devicetree node entry defines some kind of SRAM memory and
a partition at a specific offset inside it to be used as the backend for the
*state* variable set.

.. code-block:: text

	sram@0 {
		backend_state_sram: partition@10000 {
			[...]
		};
	};

With this 'backend' definition its possible to define the *state* variable set
content, its backend-type and *state* variable layout.

.. code-block:: text

	aliases {
		state = &state_sram;
	};

	state_sram: sram_state_memory {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "barebox,state";
		magic = <0xab67421f>;
		backend-type = "raw";
		backend = <&backend_state_sram>;
		backend-storage-type = "direct";
		backend-stridesize = <32>;

		variable {
			reg = <0x0 0x1>;
			type ="uint8";
			default = <0x1>;
		};
	};

EEPROM
######

This type of memory can be written on a byte base and must be erased prior
writing, but in contrast to the other flash memories, an EEPROM does the erase
of the address to be written to by its own, so its transparent to the
application.

While from the Linux kernel perspective this type of memory does not support
partitions, barebox and the *state* userland tool will use partition definitions
on an EEPROM memory as well, to exactly define the location in a generic manner
within the EEPROM.

.. code-block:: text

	eeprom@50 {
		partitions {
			compatible = "fixed-partitions";
			#size-cells = <1>;
			#address-cells = <1>;
			backend_state_eeprom: eeprom_state_memory@400 {
				reg = <0x400 0x100>;
				label = "state-eeprom";
			};
		};
	};
};

With this 'backend' definition its possible to define the *state* variable set
content, its backend-type and *state* variable layout.

.. code-block:: text

	aliases {
		state = &state_eeprom;
	};

	state_eeprom: eeprom_memory {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "barebox,state";
		magic = <0x344682db>;
		backend-type = "raw";
		backend = <&backend_state_eeprom>;
		backend-storage-type = "direct";
		backend-stridesize = <32>;

		variable {
			reg = <0x0 0x1>;
			type ="uint8";
			default = <0x1>;
		};
	};

Frontend
--------

As frontend a *state* instance is a regular barebox device which has
device parameters for the *state* variables. With this the variables can
be accessed like normal shell variables. The ``state`` command is used
to save/restore a *state* variable set to the backend device.

After initializing the variable can be accessed with ``$state.foo``.
``state -s`` stores the *state* to the backend device.
