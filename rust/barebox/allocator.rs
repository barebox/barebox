// SPDX-License-Identifier: GPL-2.0

//! Allocator support for barebox.

use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;
use core::ptr;

pub use bindings;

/// The barebox global memory allocator.
pub struct Bmalloc;

/// Calls `aligned_realloc` to alloc a new object aligned to `new_layout`'s alignment.
///
/// # Safety
///
/// - `ptr` can be either null or a pointer which has been allocated by this allocator.
/// - `new_layout` must have a non-zero size.
pub(crate) unsafe fn realloc_aligned(ptr: *mut u8, layout: Layout) -> *mut u8 {
    // SAFETY:
    // - `ptr` is either null or a pointer returned from a previous `k{re}alloc()` by the
    //   function safety requirement.
    // - `align` is greater than 0 since it's from `layout.align()` (which cannot be zero according
    //   to the function safety requirement)
    // - `size` is greater than 0 since it's from `layout.size()` (which cannot be zero according
    //   to the function safety requirement)
    unsafe {
        bindings::aligned_realloc(ptr.cast::<c_void>(), layout.align(), layout.size()).cast::<u8>()
    }
}

// SAFETY: `aligned_realloc()` returns memory with the requested alignment that stays valid
// and unaliased until passed to `free()`.
unsafe impl GlobalAlloc for Bmalloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        // SAFETY: `ptr::null_mut()` is null and `layout` has a non-zero size by the function safety
        // requirement.
        unsafe { realloc_aligned(ptr::null_mut(), layout) }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        // SAFETY: By the function safety requirement, `ptr` was returned by a
        // previous allocation of this allocator and is not used afterwards.
        unsafe {
            bindings::free(ptr.cast::<c_void>());
        }
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        // SAFETY:
        // - `new_size`, when rounded up to the nearest multiple of `layout.align()`, will not
        //   overflow `isize` by the function safety requirement.
        // - `layout.align()` is a proper alignment (i.e. not zero and must be a power of two).
        let layout = unsafe { Layout::from_size_align_unchecked(new_size, layout.align()) };

        // SAFETY:
        // - `ptr` is either null or a pointer allocated by this allocator by the function safety
        //   requirement.
        // - the size of `layout` is not zero because `new_size` is not zero by the function safety
        //   requirement.
        unsafe { realloc_aligned(ptr, layout) }
    }

    // Allocations are already zeroed with CONFIG_INIT_ON_ALLOC_DEFAULT_ON.
    #[cfg(CONFIG_INIT_ON_ALLOC_DEFAULT_ON)]
    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        // SAFETY: `ptr::null_mut()` is null and `layout` has a non-zero size by the function safety
        // requirement.
        unsafe { realloc_aligned(ptr::null_mut(), layout) }
    }
}

#[global_allocator]
static ALLOCATOR: Bmalloc = Bmalloc;

/// Required by alloc; see <https://github.com/rust-lang/rust/pull/138961>.
///
/// # Safety
///
/// Never call this: it only exists so that `alloc` finds the symbol.
#[rustc_std_internal_symbol]
pub unsafe fn __rust_no_alloc_shim_is_unstable_v2() {}
/// Required by alloc; see <https://github.com/rust-lang/rust/pull/138961>.
///
/// The default OOM hook calls this to decide between a panic and a
/// non-unwinding abort. [`__rust_alloc_error_handler`] below is used
/// instead, so the value is only ever read by code we do not link.
///
/// # Safety
///
/// Never call this: it only exists so that `alloc` finds the symbol.
#[rustc_std_internal_symbol]
pub unsafe fn __rust_alloc_error_handler_should_panic_v2() -> u8 {
    0
}

/// Static used by older `alloc` instead of [`__rust_no_alloc_shim_is_unstable_v2`].
#[allow(non_upper_case_globals)]
#[rustc_std_internal_symbol]
pub static __rust_no_alloc_shim_is_unstable: u8 = 0;
/// Static used by older `alloc` instead of [`__rust_alloc_error_handler_should_panic_v2`].
#[allow(non_upper_case_globals)]
#[rustc_std_internal_symbol]
pub static __rust_alloc_error_handler_should_panic: u8 = 0;

/// Error Handler called on OOM
#[rustc_std_internal_symbol]
pub fn __rust_alloc_error_handler(size: usize, align: usize) -> ! {
    panic!(
        "OOM: Unable to allocate {} bytes at {} alignment!",
        size, align
    );
}
