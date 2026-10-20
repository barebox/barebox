.. _devicetree-barebox-assert-available:

barebox built-in overlay assertions
===================================

``barebox,assert-available`` in the top-level node of a built-in device tree
overlay lists, as paths, the base device tree nodes the overlay needs.
barebox applies the overlay only if all of them exist **and** are enabled::

   / {
   	compatible = "myvendor,myboard";
   	barebox,assert-available = "/soc/mmc@5b010000";
   };

This is how an overlay says it is for hardware only some board variants have
and that it is of no use in part; fragments are otherwise applied one by one,
with the ones that find no target skipped. Nodes the overlay only patches
don't belong here: a property on a node nothing probes has no effect.

The paths have to be written out, as a foreign device tree has no symbols;
``BASE_PATH(label)`` provides them where the overlay names a base device tree
with ``DTBO_BASE_<overlay>``. barebox ignores the property outside a built-in
overlay, i.e. in ``of_overlay_apply_tree()`` or the ``of_overlay`` command.
