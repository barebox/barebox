# Barebox QEMU whole-boot fuzzer

Run `make -C fuzz`. This builds the ARM64 QEMU virt target and starts each AFL++
test case as a read-only VirtIO block device in a new QEMU virtual machine.

Use `make -C fuzz run INPUT=corpus/SEED` to replay one input.
Use `make -C fuzz coverage INPUT=corpus/SEED` to create source coverage at
`fuzz/coverage/index.html`.

The source-local TCG plugin reports ARM64 guest basic-block edges to AFL++.
