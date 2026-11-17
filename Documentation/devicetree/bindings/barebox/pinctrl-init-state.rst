init pin control state
======================

With ``CONFIG_PINCTRL_INIT_STATE``, a device's ``init`` pin control state is
applied instead of ``default`` before its driver probes and ``default``
afterwards, like in Linux. This keeps pins from being driven before the
driver is ready for it.

A ``barebox,init`` state takes precedence over ``init``. Linux doesn't know
it, so it's for pins that only barebox needs to treat this way.

Unlike Linux, an init state is applied even without a ``default`` state and
then stays. A state the driver selected during probe is kept.

.. code-block:: none

   &flash {
        /* drive WP# inactive before the flash driver probes */
        pinctrl-names = "barebox,init";
        pinctrl-0 = <&flash_wp_gpio_pins>;
   };
