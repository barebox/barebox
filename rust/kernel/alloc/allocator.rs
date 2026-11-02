// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/alloc/allocator.rs?id=038d61fd642278bab63ee8ef722c50d10ab01e8f

//! Allocator support.
//!
//! Documentation for the kernel's memory allocators can found in the "Memory Allocation Guide"
//! linked below. For instance, this includes the concept of "get free page" (GFP) flags and the
//! typical application of the different kernel allocators.
//!
//! Reference: <https://docs.kernel.org/core-api/memory-allocation.html>

use core::alloc::Layout;
use core::ptr;
use core::ptr::NonNull;

use crate::alloc::{AllocError, Allocator};
use crate::bindings;

const ARCH_KMALLOC_MINALIGN: usize = bindings::CONFIG_MALLOC_ALIGNMENT as usize;

/// The contiguous kernel allocator.
///
/// `Kmalloc` is typically used for physically contiguous allocations up to page size, but also
/// supports larger allocations up to `bindings::KMALLOC_MAX_SIZE`, which is hardware specific.
///
/// For more details see [self].
pub struct Kmalloc;

impl Kmalloc {
    /// Returns a [`Layout`] that makes [`Kmalloc`] fulfill the requested size and alignment of
    /// `layout`.
    pub fn aligned_layout(layout: Layout) -> Layout {
        // The alignment is passed on to `aligned_realloc()` in `realloc()` below.
        layout.pad_to_align()
    }
}

// SAFETY: `realloc` guarantees that
// - memory remains valid until it is explicitly freed,
// - passing a pointer to a valid memory allocation is OK,
// - `aligned_realloc` honours the alignment of the requested `Layout`, so `realloc` satisfies the
//   `Allocator` guarantees.
unsafe impl Allocator for Kmalloc {
    const MIN_ALIGN: usize = ARCH_KMALLOC_MINALIGN;

    #[inline]
    unsafe fn realloc(
        ptr: Option<NonNull<u8>>,
        layout: Layout,
        old_layout: Layout,
    ) -> Result<NonNull<[u8]>, AllocError> {
        let layout = Kmalloc::aligned_layout(layout);
        let size = layout.size();
        let ptr = match ptr {
            Some(ptr) => {
                if old_layout.size() == 0 {
                    ptr::null_mut()
                } else {
                    ptr.as_ptr()
                }
            }
            None => ptr::null_mut(),
        };

        // SAFETY:
        // - `bindings::aligned_realloc` only requires that `ptr` is NULL or valid and that
        //   `layout.align()` is a power of two, which `Layout` guarantees.
        // - `ptr` is either NULL or valid by the safety requirements of this function.
        let raw_ptr = unsafe {
            // If `size == 0` and `ptr != NULL` the memory behind the pointer is freed.
            bindings::aligned_realloc(ptr.cast::<crate::ffi::c_void>(), layout.align(), size)
                .cast::<u8>()
        };

        let ptr = if size == 0 {
            crate::alloc::dangling_from_layout(layout)
        } else {
            NonNull::new(raw_ptr).ok_or(AllocError)?
        };

        Ok(NonNull::slice_from_raw_parts(ptr, size))
    }
}
