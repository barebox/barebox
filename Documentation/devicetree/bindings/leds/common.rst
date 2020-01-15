Common leds properties
======================

* ``linux,default-trigger``, ``barebox,default-trigger``:  This parameter, if present, is a
    string defining the trigger assigned to the LED.  Current triggers are:

    * ``heartbeat`` - LED flashes at a constant rate
    * ``panic`` - LED turns on when barebox panics
    * ``net`` - LED indicates network activity (tx and rx)
    * ``net-rx`` - LED indicates network activity (rx only)
    * ``net-tx`` - LED indicates network activity (tx only)

* ``label``: The label for this LED. If omitted, the label is taken
  from the node name (excluding the unit address).

* ``panic-indicator`` - This property specifies that the LED should be used as a
 panic indicator.
