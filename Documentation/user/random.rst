Random Number Generator support
===============================

Barebox provides two types of RNG sources - PRNG and HWRNG:

- "A pseudorandom number generator (PRNG), also known as a deterministic random
  bit generator (DRBG),[1] is an algorithm for generating a sequence of numbers
  whose properties approximate the properties of sequences of random numbers.
  The PRNG-generated sequence is not truly random, because it is completely
  determined by a relatively small set of initial values, called the PRNG's seed
  (which may include truly random values). Although sequences that are closer to
  truly random can be generated using hardware random number generators."
  Pseudorandom number generator. https://en.wikipedia.org/wiki/Pseudorandom_number_generator (2017.05.08).
  The PRNG used by Barebox is LCG (linear congruential generator) non cryptographically
  secure, so please use with caution.

- The HWRNG framework is software that makes use of a special hardware feature on
  your CPU, SoC or motherboard. It can‘t provide any guarantee about cryptographic
  security of used HW. Please refer to vendor documentation and/or RNG certification.

API
^^^

.. code-block:: c

        /* seed the PRNG. */
        void srand(unsigned int seed);

        /* Fill the buffer with PRNG bits. */
        void get_random_bytes(void *buf, int len);

        /* Fill the buffer with bits provided by HWRNG.
         * This function may fail with a message “error: no HWRNG available!”
         * in case HWRNG is not available or HW got some runtime error.
         * If barebox is compiled with CONFIG_ALLOW_PRNG_FALLBACK,
         * then get_crypto_bytes() will print “warning: falling back to Pseudo RNG source!”
         * and use PRNG instead of returning error.
         */
        int get_crypto_bytes(void *buf, int len);

User interface
^^^^^^^^^^^^^^

- /dev/hwrng0
  provides access to first available HWRNG. To examine this source you can use:

.. code-block:: sh

  md -s /dev/hwrng0

- /dev/prng
  provides access to PRNG. To examine this source you can use:

.. code-block:: sh

  md -s /dev/prng

To seed PRNG from user space the :ref:`command_seed` is provided. For example:

.. code-block:: sh

  seed 12345
  md -s /dev/prng
