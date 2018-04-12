.. _barebox,state:

Barebox state
=============

A *state* variable set can be fully described as a devicetree based *state* node.
This *state* node could be part of the regular platform's devicetree blob or it
could be an extra devicetree solely for the *state*.

Devicetree *state* Node
-----------------------

A *state* node contains a description of a set of variables along with a
place where the variable set gets stored.

Required Properties
###################

* ``compatible``: should be ``barebox,state``
* ``magic``: a 32bit number
* ``backend``: phandle to persistent memory
* ``backend-type``: defines the *state* variable set storage format
* additionally a *state* node must have an alias in the ``/aliases`` node pointing
  to it.

.. _barebox,state_magic:

The ``magic`` property is a unique number which identifies the *state* variable
set's variable types and their layout. It should be kept stable as long as the
variable types and the layout are kept stable. It should also be kept stable
if new trailing variables are added to the existing layout to be backward
compatible. Only if the *state* variable set's variable types and/or their layout
change, the ``magic`` property's number must be changed to be unique again
with the new *state* variable set's content.

.. important:: You should not use the values 0x2354fdf3 and 0x14fa2d02 for your
   magic value. They're already reserved by the ``direct`` and ``circular``
   storage backends.

The ``backend`` property uses the *phandle* mechanism to link the *state* to
a real persistent memory. Refer :ref:`Backend <state_framework,backends>` for
supported persistent memories.

The ``backend-type`` should be ``raw`` or ``dtb``. Refer
:ref:`Backend Types <state_framework,backend_types>` for further details.

Optional Properties
###################

* ``backend-stridesize``: stride counted in bytes. See note below.
* ``backend-storage-type``: Defines the backend storage type to ``direct``,
  ``circular`` or ``noncircular``. If the backend memory needs to be erased
  prior a write it defaults to the ``circular`` storage backend type, for backend
  memories like RAMs or EEPROMs it defaults to the ``direct`` storage backend type.
* ``algo``: A HMAC algorithm used to detect manipulation of the data
  or header, sensible values follow this pattern ``hmac(<HASH>)``,
  e.g. ``hmac(sha256)``. Only available for the ``backend-type`` ``raw``.
* ``keep-previous-content``: Check if a the bucket meta magic field contains
  other data than the magic value. If so, the backend will not write the state
  to prevent unconditionally overwrites of existing data.

.. note:: For the ``backend-storage-type`` the keyword ``noncircular`` is still
   supported as a fall back to an old storage format. Recommendation is to not
   use this type anymore.

.. _barebox,state_backend_stridesize:

The ``backend-stridesize`` is still optional but required whenever the
underlying backend doesn't provide an information how to pad an instance of a
*state* variable set. This is valid for all underlying backends which support
writes on a byte-by-byte manner or don't have eraseblocks (EEPROM, SRAM and NOR
type flash backends).
The ``backend-stridesize`` value is used by the ``direct`` backend storage type
to place the redundant *state* variable set copies side by side in the backend.
And it's used by the ``circular`` backend storage type to place the *state*
variable set copies side by side into the eraseblock.
You should calculate the ``backend-stridesize`` value very carefully based on
the used ``backend-type``, the size of the used backend (e.g. partition size
for example) and its eraseblock size. Refer
:ref:`Backend Types <state_framework,backend_types>`.

.. note:: It might be useful to add some spare space to the
   ``backend-stridesize`` to ensure the ability to extend the *state* variable
   set later on.

.. _barebox,state_variable:

Variable Subnodes
-----------------

These are subnodes of a *state* node each describing a single
variable. The node name may end with ``@<ADDRESS>``, but the suffix is
stripped from the variable name.

State variables have a type. Currenty supported types are: ``uint8``,
``uint32``, ``enum32``, ``mac`` address or ``string`` (fixed length string).
Variable length strings are not planned.

Required Properties
###################

* ``reg``: Standard ``reg`` property with ``#address-cells = <1>`` and
  ``#size-cells = <1>``. Defines the ``offset`` and ``size`` of the
  variable in the ``raw`` backend. ``size`` **must fit** the node
  ``type``. Variables are not allowed to overlap.
* ``type``: Should be ``uint8``, ``uint32``, ``enum32``, ``mac``
  or ``string`` for the type of the variable
* ``names``: For ``enum32`` values only, this specifies the possible values for
  ``enum32``.

Optional Properties
###################

* ``default``: The default value if the variable's content cannot be read from
  the backend. For ``enum32`` values it is an integer representing an offset
  into the names array.

.. note:: Since the ``default`` property is optional, keep in mind you may need
   a valid default value if other instances (like the bootchooser for example)
   depends on it. Due to this, a ``default`` might be a required property instead.

Variable Examples
#################

``uint8``:

.. code-block:: text

   uint8_example@0 {
       reg = <0x0 0x1>;
       type = "uint8";
       default = <0x00>;
   };

``uint32``:

.. code-block:: text

   uint32_example@0 {
       reg = <0x0 0x4>;
       type = "uint32";
       default = <100>;
   };

``enum32``:

.. code-block:: text

   enum_example@0 {
       reg = <0x0 0x4>;
       type = "enum32";
       names = "value#1", "value#2";
       default = <1>; /* selects "value#2" as the default */
   };

``mac``:

.. code-block:: text

   mac_example@0 {
       reg = <0x0 0x6>;
       type = "mac";
   };

Since a 'MAC' is a unique system identifier it makes no sense for a default
value here. It must be set individually at run-time instead.

``string``:

.. code-block:: text

   name {
       reg = <0x0 0x10>;
       type = "string";
   };

In this example the length of the string is limited to 16 characters.

.. _barebox,state_hmac:

HMAC
----

With the optional property ``algo = "hmac(<HASH>)";`` an HMAC algorithm
can be defined to detect unauthorized modification of the state's variable set
header and/or data. For this to work the HMAC and the selected hash
algorithm have to be compiled into barebox.

The shared secret for the HMAC is requested via
``keystore_get_secret()``, using the state's name, from the barebox
simple keystore. It's up to the developer to populate the keystore via
``keystore_set_secret()`` in beforehand. Refer :ref:`command_keystore` for
further details.

.. _barebox,state_setup:

Configuring the *state* variable set
------------------------------------

Since the *state* variable set is intended to be shared between the bootloader
and the kernel, the view to the *state* variable set must be the same in both
worlds.

This can be achieved by defining all *state* variable set related definitions
inside the barebox's devicetree only. It's **not** required to keep and maintain
the same information inside the Linux kernel's devicetree again.

When barebox is instructed to load and forward a devicetree to a Linux kernel
to be started, it "silently" copies all *state* variable set related definitions
from its own devicetree into the Linux kernel devicetree. This way both worlds
behave the same when *state* variable sets should be read or modified.

In order to enable barebox to copy the required information to a dedicated
location inside the Linux kernel devicetree the name of the memory node to
store the *state* variable set must be the same in the barebox's devicetree
and the operating system's devicetree.

With this "interconnection" barebox extends the operating system's devicetree
with:

- the layout and variable definition of the *state* variable set (in case of
  the ``raw`` backend-type)
- the store definition (backend type, backend storage type and so on)
- partitioning information for the persistent memory in question (on demand)
- the connection between the backend and the memory (device, partition)

Example:

Lets assume the barebox's devicetree uses the name ``persistent_state_memory@01``
to define its own *state* variable set backend.

Barebox's devicetree defines:

.. code-block:: text

   persistent_state_memory@01 {
       compatible = "somevalue";
       reg = <1>;

       #address-cells = <1>;
       #size-cells = <1>;

       state: partition@0 {
            label = "state";
            reg = <0x0 0x100>;
       };
   };

The operating system's devicetree defines instead:

.. code-block:: text

   persistent_state_memory@01 {
       compatible = "somevalue";
       reg = <1>;
   };
