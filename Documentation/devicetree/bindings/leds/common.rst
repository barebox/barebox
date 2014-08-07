Common leds properties
======================

* ``linux,default-trigger``, ``barebox,default-trigger``:  This parameter, if present, is a
    string defining the trigger assigned to the LED.  Current triggers are:

    * ``heartbeat`` - LED flashes at a constant rate
    * ``panic`` - LED turns on when barebox panics
    * ``net`` - LED indicates network activity

* ``label``: The label for this LED. If omitted, the label is taken
  from the node name (excluding the unit address).
