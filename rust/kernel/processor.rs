// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/processor.rs?id=8d3ae59288f1e7d58d76558a6ee96d533bc5019f

//! Processor related primitives.
//!
//! C header: [`include/linux/processor.h`](srctree/include/linux/processor.h)

/// Lower CPU power consumption or yield to a hyperthreaded twin processor.
///
/// It also happens to serve as a compiler barrier.
#[inline]
pub fn cpu_relax() {
    // SAFETY: Always safe to call.
    unsafe { bindings::cpu_relax() }
}
