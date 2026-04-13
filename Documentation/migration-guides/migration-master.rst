:orphan:

register_simplefb parameter
---------------------------

The ``register_simplefb`` parameter on framebuffer devices has changed from
boolean to an enum with values ``disabled``, ``enabled``, and ``stdout-path``.

Scripts that **read** the parameter will now receive ``"disabled"`` or
``"enabled"`` instead of ``"0"`` or ``"1"``.

Scripts that **write** ``"0"`` or ``"1"`` continue to work.