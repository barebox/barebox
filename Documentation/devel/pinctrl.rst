.. _devel_pinctrl:

Pinctrl drivers
===============

barebox configures pins from device tree states, referenced by
``pinctrl-N`` properties of the consumer node:

.. code-block:: dts

  uart0-pins {
          groups = "uart0";
          function = "uart";
          bias-disable;
  };

The pinctrl API is the one from Linux minus debugfs, pin ownership
tracking and static mapping tables. Unlike Linux, barebox keeps no
consumer handles or cached settings: a state is parsed on-demand when
it is selected. barebox additions are marked as such in the headers and
listed below.

Registering a controller
------------------------

Drivers ported from Linux call ``pinctrl_register_and_init()`` followed by
``pinctrl_enable()``, which applies the controller's own default state.
Older barebox drivers embed ``struct pinctrl_dev`` and call
``pinctrl_register()``, which does both.

Applying a state
----------------

Drivers with a custom binding may implement ``pinctrl_ops::set_state`` and
parse the node themselves.

The alternative is using ``GENERIC_PINCONF`` and have the node applied by
``pinctrl_map_set_state()``. It hands the node to the driver's
``dt_node_to_map`` if there is one, else to Linux'
``pinconf_generic_dt_subnode_to_map()``, and applies the resulting maps one
node at a time. Unlike Linux, it doesn't mux all nodes of a state before
configuring them, doesn't check that a group belongs to its function and
doesn't undo a partly applied state on error.

Drivers building groups and functions at probe time use
``pinctrl_generic_add_group()`` and ``pinmux_generic_add_pinfunction()``
behind ``GENERIC_PINCTRL_GROUPS`` and ``GENERIC_PINMUX_FUNCTIONS``.

barebox extensions
------------------

* ``pinmux_ops::pinmux_property_set``: taken from U-Boot for drivers that
  decode ``pinmux`` cells by pin number instead of implementing
  ``dt_node_to_map``. It muxes the pin one cell describes and returns its
  pin number for applying the node's pin configuration. The STM32 and
  AT91 PIO4 drivers use it as they were ported from U-Boot.
* Without ``pin_config_group_set``, group configuration falls back to
  ``pin_config_set`` for every pin of the group.
* ``pinmux_ops::gpio_get_direction`` complements ``gpio_set_direction``.

GPIO ranges
-----------

GPIO chips are tied to pin controllers through ``struct pinctrl_gpio_range``
as in Linux, either from the ``gpio-ranges`` property or by calling
``gpiochip_add_pin_range()``. GPIO drivers backed by a pin controller use
``gpiochip_generic_request()``, ``gpiochip_generic_free()`` and
``gpiochip_generic_config()``.

Unlike Linux, a ``gpio-ranges`` reference to a pin controller that isn't
registered yet is skipped instead of failing the GPIO chip. Group ranges
(``gpio-ranges-group-names``) aren't supported and are skipped with a warning.

Porting a Linux driver
----------------------

Import the Linux files verbatim with an ``SPDX-Comment: Origin-URL``. Then
convert the boilerplate mechanically, delete what barebox does not need and
adapt the rest. The barebox glue and the Kconfig/Makefile wiring come last.
One commit per step keeps the port most easily reviewable against Linux.

What is usually removed is interrupt support, suspend/resume and debugfs.
fwnode accessors need converting to ``of_`` by hand until barebox implements
that API. A driver with one GPIO chip per bank needs a device per bank,
as barebox gpiolib takes the chip's device tree node from its device.
Ports keeping ``.set_config = gpiochip_generic_config`` must
``select HAVE_GPIO_PINCONF``, so gpiolib will call it.
