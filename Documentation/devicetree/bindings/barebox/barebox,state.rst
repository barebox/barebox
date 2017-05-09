.. _barebox,state:

barebox state
=============

Overview
--------


Boards often have the need to store variables in persistent memory.
The constraints are often different from what the regular environment
can do:

* compact binary format to make it suitable for small EEPROMs/MRAMs
* atomic save/restore of the whole variable set
* redundancy

``barebox,state`` is a framework to describe, access, store and
restore a set of variables. A state variable set can be fully
described in a devicetree node. This node could be part of the regular
devicetree blob or it could be an extra devicetree solely for the
state. The state variable set contains variables of different types
and a place to store the variable set.

A state node contains a description of a set of variables along with a
place where the variables are stored.

Required properties:

* ``compatible``: should be ``barebox,state``;
* ``magic``: A 32bit number used as a magic to identify the state
* ``backend``: contains a phandle to the device/partition which holds the
  actual state data.
* ``backend-type``: should be ``raw`` or ``dtb``.
* additionally a state node must have an alias in the /aliases/ node pointing
  to it.

Optional properties:

* ``algo``: A HMAC algorithm used to detect manipulation of the data
  or header, sensible values follow this pattern ``hmac(<HASH>)``,
  e.g. ``hmac(sha256)``. Only used for ``raw``.
* ``backend-stridesize``: Maximum size per copy of the data. Only important for
  non-MTD devices
* ``backend-storage-type``: Normally the correct storage type is detected auto-
  matically. The circular backend supports the option ``noncircular`` to fall
  back to an old storage format.

Variable nodes
--------------

These are subnodes of a state node each describing a single
variable. The node name may end with ``@<ADDRESS>``, but the suffix is
stripped from the variable name.

State variables have a type. Currenty supported types are: ``uint8``,
``uint32``, ``enum32``, ``mac`` address or ``string`` (fixed length string).
Variable length strings are not planned.

Required properties:

* ``reg``: Standard ``reg`` property with ``#address-cells = <1>`` and
  ``#size-cells = <1>``. Defines the ``offset`` and ``size`` of the
  variable in the ``raw`` backend. ``size`` must fit the node
  ``type``. Variables are not allowed to overlap.
* ``type``: Should be ``uint8``, ``uint32``, ``int32``. ``enum32``, ``mac``
  or ``string`` for the type of the variable
* ``names``: For ``enum32`` values only, this specifies the values
  possible for ``enum32``.

Optional properties:

* ``default``: The default value if the variable cannot be read from
  storage. For ``enum32`` values it is an integer representing an
  offset into the names array.

Example::

  /aliases {
  	state = &state;
  };

  state: state {
  	magic = <0x27031977>;
  	compatible = "barebox,state";
  	backend-type = "raw";
  	backend = <&state_part>;

  	foo {
  		reg = <0x00 0x4>;
  		type = "uint32";
  		default = <0x0>;
  	};

  	bar {
  		reg = <0x10 0x4>;
  		type = "enum32";
  		names = "baz", "qux";
  		default = <1>;
  	};
  };

  &nand_flash {
  	partitions {
  		compatible = "fixed-partitions";
  		#address-cells = <1>;
  		#size-cells = <1>;
  		state_part: state@10000 {
  			label = "state";
  			reg = <0x10000 0x10000>;
  		};
  	};
  };

Variable Types
--------------

* ``uint8``:
* ``uint32``:
* ``int32``:
* ``enum32``: The ``default`` value is an integer representing an
  offset into the names array.
* ``mac``:
* ``string``: The length of the string excluding the trailing 0 is
  determined by the length given in the ``reg`` property.

Backends
--------

Currently two backends exist. The raw backend is a very compact format
consisting of a magic value for identification, the raw values and a
CRC. Two copies are maintained for making sure that during update the
storage device still contains a valid state. The dtb backend stores
the state as a devicetree binary blob. This is exactly the original
devicetree description of the state itself, but additionally contains
the actual values of the variables. Unlike the raw state backend the
dtb state backend can describe itself.

HMAC
----

With the optional property ``algo = "hmac(<HASH>)";`` a HMAC algorithm
can be defined to detect unauthorized modification of the state's
header and/or data. For this to work the HMAC and the selected hash
algorithm have to be compiled into barebox.

The shared secret for the HMAC is requested via
``keystore_get_secret()``, using the state's name, from the barebox
simple keystore. It's up to the developer to populate the keystore via
``keystore_set_secret()`` in beforehand.

Frontend
--------

As frontend a state instance is a regular barebox device which has
device parameters for the state variables. With this the variables can
be accessed like normal shell variables. The ``state`` command is used
to save/restore a state to the backend device.

After initializing the variable can be accessed with ``$state.foo``.
``state -s`` stores the state to eeprom.
