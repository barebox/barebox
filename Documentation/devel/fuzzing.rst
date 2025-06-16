Fuzzing barebox
===============

As described in the :ref:`security` chapter, some parts of barebox need to
deal with untrusted inputs. To aid in finding and fixing issues that might
be exploited, barebox can be built with LLVM's libfuzzer to exercise
these security-critical parsers.

Building
^^^^^^^^

The barebox sandbox architecture has support for libfuzzer when compiled with
LLVM. The ``libfuzzer_defconfig`` enables it as well as different hardening
options to crash barebox on detection of memory safety issues::

  $ export LLVM=1 # or e.g. LLVM=-19, if clang is called clang-19
  $ make libfuzzer_defconfig
  $ make -j$(nproc)
  # [snip]
  images built:
  barebox
  fuzz-filetype
  fuzz-fit
  fuzz-fs
  fuzz-dtb
  fuzz-fdt-compatible
  fuzz-partitions

All fuzzers generated are symlinks to the same barebox executable. barebox
will detect that it was invoked via symlink and switch to fuzzing mode.

Fuzzing
^^^^^^^

Fuzzers can be run directly or by invoked the main barebox binary with the
``--fuzz`` option. The latter is mostly useful for debugging.

Examples of running the fuzzers::

  # Just run the fuzzer with no corpus
  images/fuzz-filetype

  # Multi-threaded fuzzing is recommended as is using a corpus
  images/fuzz-dtb -rss_limit_mb=10000 -max_len=51200 -jobs=64 \
	../barebox-fuzz-corpora/dtb

  # Some fuzzers still leak, so disable leak detection till resolved
  images/fuzz-fit -max_total_time=600 -rss_limit_mb=20000 -max_len=128000 -detect_leaks=0

  # Debug a crash
  gdb --args images/fuzz-fit crash-$HASH

When a crash is detected, libfuzzer will create a ``crash-$HASH`` file
that can be passed instead of the corpus directory to run the fuzz test
once.

Corpora
^^^^^^^

We maintain a corpus for every fuzz test on
`Github <https://github.com/barebox/barebox-fuzz-corpora>`_.

This helps bootstrap the fuzzer, so it can exercise new paths more quickly.

Determining Source Code Coverage
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::
	Coverage instrumentation is currently only supported with LLVM
        and sandbox.

To collect coverage information, barebox must be built with ``CONFIG_GCOV=y``.
The linking process will take much longer than usual, but once done, running
barebox will produce coverage information.

.. code-block:: bash

	images/fuzz-filetype -max_total_time=60 -max_len=2048

After the process exists regularly (i.e., not aborted with ctrl+C!),
it will produce a ``default.profraw`` file, which needs to be further
processed:

.. code-block:: bash

	make coverage-html

This will produce a ``${KBUILD_OUTPUT}/coverage_html/`` directory, which can be
inspected by a web browser:

.. code-block:: bash

	firefox coverage_html/index.html

Adding a fuzzer
^^^^^^^^^^^^^^^

The barebox integration of libfuzzer is a bit unusual; barebox supplies
its own ``main()`` and calls into libfuzzer instead of the over way round.

This allows us to write fuzz tests naturally inline without having
to setup things beforehand as barebox will have already executed all
of its initcalls for example.

To add a new fuzz test, just add a function next to the parser that
parses a memory buffer::

  #include <fuzz.h>

  static int fuzz_dtb(const u8 *data, size_t size)
  {
  	struct device_node *np;

  	np = of_unflatten_dtb_const(data, size);
  	if (!IS_ERR(np))
  		of_delete_node(np);

  	return 0;
  }
  fuzz_test("dtb", fuzz_dtb);


.. note:: Fuzz tests should not leak memory, otherwise
 the fuzzing process may abort eventually due to memory exhaustion.

This function than needs to be registered by name in
``images/Makefile.sandbox``::

  fuzzer-$(CONFIG_OFTREE)	+= dtb

Searching the source tree for ``fuzz_test`` will show more examples,
e.g. how to wrap the received buffer in a ramdisk to interface
with code that requires block devices.

When adding a new fuzzing test, please also `submit a pullrequest
with a corpus <https://github.com/barebox/barebox-fuzz-corpora/compare>_.
