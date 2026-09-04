# Barebox whole-boot fuzzer

Run `make -C fuzz`. The Makefile builds Barebox's native sandbox target,
uses the checked-in `fuzz/corpus`, and starts AFL++ QEMU mode.

Each AFL++ input is passed unchanged as the read-only block device
`/dev/fuzzdisk`. Barebox detects the filesystem and partitions, routes them to
the enabled filesystem parsers, and tries to boot them. It also passes the raw
input to `bootm` with ordinary and forced-signature verification. This covers
filesystem and image formats.

The default build enables `CONFIG_FUZZ_INSECURE_PARTIAL_DIGEST`, reducing CRC,
hash, and RSA comparisons so mutations can reach code behind integrity checks.
Rebuild candidates with exact checks before treating them as findings:

```sh
make -C fuzz clean
make -C fuzz PARTIAL_DIGESTS=0 build
```

Run `make -C fuzz coverage` to rebuild Barebox with Clang source coverage,
replay the checked-in corpus and AFL++ queue, and create the line-coverage report
at `fuzz/coverage/barebox.coverage_html/index.html`.

Run `make -C fuzz run <path of your seed>` to run a specific seed with barebox

Fuzzing requires `afl-fuzz` and its matching `afl-qemu-trace`. Coverage also
requires Clang, LLVM, LLD, and `genhtml`
