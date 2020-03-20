
#include <config.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>

#include <stdio.h>
#include <module.h>

/*
  A version of malloc/free/realloc written by Doug Lea and released to the
  public domain.  Send questions/comments/complaints/performance data
  to dl@cs.oswego.edu

* VERSION 2.6.6  Sun Mar  5 19:10:03 2000  Doug Lea  (dl at gee)

   Note: There may be an updated version of this malloc obtainable at
	   ftp://g.oswego.edu/pub/misc/malloc.c
	 Check before installing!

* Why use this malloc?

  This is not the fastest, most space-conserving, most portable, or
  most tunable malloc ever written. However it is among the fastest
  while also being among the most space-conserving, portable and tunable.
  Consistent balance across these factors results in a good general-purpose
  allocator. For a high-level description, see
     http://g.oswego.edu/dl/html/malloc.html

* Synopsis of public routines

  (Much fuller descriptions are contained in the program documentation below.)

  malloc(size_t n);
     Return a pointer to a newly allocated chunk of at least n bytes, or null
     if no space is available.
  free(Void_t* p);
     Release the chunk of memory pointed to by p, or no effect if p is null.
  realloc(Void_t* p, size_t n);
     Return a pointer to a chunk of size n that contains the same data
     as does chunk p up to the minimum of (n, p's size) bytes, or null
     if no space is available. The returned pointer may or may not be
     the same as p. If p is null, equivalent to malloc.  Unless the
     #define REALLOC_ZERO_BYTES_FREES below is set, realloc with a
     size argument of zero (re)allocates a minimum-sized chunk.
  memalign(size_t alignment, size_t n);
     Return a pointer to a newly allocated chunk of n bytes, aligned
     in accord with the alignment argument, which must be a power of
     two.
  valloc(size_t n);
     Equivalent to memalign(pagesize, n), where pagesize is the page
     size of the system (or as near to this as can be figured out from
     all the includes/defines below.)
  pvalloc(size_t n);
     Equivalent to valloc(minimum-page-that-holds(n)), that is,
     round up n to nearest pagesize.
  calloc(size_t unit, size_t quantity);
     Returns a pointer to quantity * unit bytes, with all locations
     set to zero.
  cfree(Void_t* p);
     Equivalent to free(p).
  malloc_trim(size_t pad);
     Release all but pad bytes of freed top-most memory back
     to the system. Return 1 if successful, else 0.
  malloc_usable_size(Void_t* p);
     Report the number usable allocated bytes associated with allocated
     chunk p. This may or may not report more bytes than were requested,
     due to alignment and minimum size constraints.
  malloc_stats();
     Prints brief summary statistics on stderr.
  mallinfo()
     Returns (by copy) a struct containing various summary statistics.
  mallopt(int parameter_number, int parameter_value)
     Changes one of the tunable parameters described below. Returns
     1 if successful in changing the parameter, else 0.

* Vital statistics:

  Alignment:                            8-byte
       8 byte alignment is currently hardwired into the design.  This
       seems to suffice for all current machines and C compilers.

  Assumed pointer representation:       4 or 8 bytes
       Code for 8-byte pointers is untested by me but has worked
       reliably by Wolfram Gloger, who contributed most of the
       changes supporting this.

  Assumed size_t  representation:       4 or 8 bytes
       Note that size_t is allowed to be 4 bytes even if pointers are 8.

  Minimum overhead per allocated chunk: 4 or 8 bytes
       Each malloced chunk has a hidden overhead of 4 bytes holding size
       and status information.

  Minimum allocated size: 4-byte ptrs:  16 bytes    (including 4 overhead)
			  8-byte ptrs:  24/32 bytes (including, 4/8 overhead)

       When a chunk is freed, 12 (for 4byte ptrs) or 20 (for 8 byte
       ptrs but 4 byte size) or 24 (for 8/8) additional bytes are
       needed; 4 (8) for a trailing size field
       and 8 (16) bytes for free list pointers. Thus, the minimum
       allocatable size is 16/24/32 bytes.

       Even a request for zero bytes (i.e., malloc(0)) returns a
       pointer to something of the minimum allocatable size.

  Maximum allocated size: 4-byte size_t: 2^31 -  8 bytes
			  8-byte size_t: 2^63 - 16 bytes

       It is assumed that (possibly signed) size_t bit values suffice to
       represent chunk sizes. `Possibly signed' is due to the fact
       that `size_t' may be defined on a system as either a signed or
       an unsigned type. To be conservative, values that would appear
       as negative numbers are avoided.
       Requests for sizes with a negative sign bit when the request
       size is treaded as a long will return null.

  Maximum overhead wastage per allocated chunk: normally 15 bytes

       Alignnment demands, plus the minimum allocatable size restriction
       make the normal worst-case wastage 15 bytes (i.e., up to 15
       more bytes will be allocated than were requested in malloc), with
       two exceptions:
	 1. Because requests for zero bytes allocate non-zero space,
	    the worst case wastage for a request of zero bytes is 24 bytes.
	 2. For requests >= mmap_threshold that are serviced via
	    mmap(), the worst case wastage is 8 bytes plus the remainder
	    from a system page (the minimal mmap unit); typically 4096 bytes.

* Limitations

    Here are some features that are NOT currently supported

    * No user-definable hooks for callbacks and the like.
    * No automated mechanism for fully checking that all accesses
      to malloced memory stay within their bounds.
    * No support for compaction.

* Synopsis of compile-time options:

    People have reported using previous versions of this malloc on all
    versions of Unix, sometimes by tweaking some of the defines
    below. It has been tested most extensively on Solaris and
    Linux. It is also reported to work on WIN32 platforms.
    People have also reported adapting this malloc for use in
    stand-alone embedded systems.

    The implementation is in straight, hand-tuned ANSI C.  Among other
    consequences, it uses a lot of macros.  Because of this, to be at
    all usable, this code should be compiled using an optimizing compiler
    (for example gcc -O2) that can simplify expressions and control
    paths.

  __STD_C                  (default: derived from C compiler defines)
     Nonzero if using ANSI-standard C compiler, a C++ compiler, or
     a C compiler sufficiently close to ANSI to get away with it.
  DEBUG                    (default: NOT defined)
     Define to enable debugging. Adds fairly extensive assertion-based
     checking to help track down memory errors, but noticeably slows down
     execution.
  REALLOC_ZERO_BYTES_FREES (default: NOT defined)
     Define this if you think that realloc(p, 0) should be equivalent
     to free(p). Otherwise, since malloc returns a unique pointer for
     malloc(0), so does realloc(p, 0).
  HAVE_MEMCPY               (default: defined)
     Define if you are not otherwise using ANSI STD C, but still
     have memcpy and memset in your C library and want to use them.
     Otherwise, simple internal versions are supplied.
  USE_MEMCPY               (default: 1 if HAVE_MEMCPY is defined, 0 otherwise)
     Define as 1 if you want the C library versions of memset and
     memcpy called in realloc and calloc (otherwise macro versions are used).
     At least on some platforms, the simple macro versions usually
     outperform libc versions.
  HAVE_MMAP                 (default: defined as 1)
     Define to non-zero to optionally make malloc() use mmap() to
     allocate very large blocks.
  HAVE_MREMAP                 (default: defined as 0 unless Linux libc set)
     Define to non-zero to optionally make realloc() use mremap() to
     reallocate very large blocks.
  malloc_getpagesize        (default: derived from system #includes)
     Either a constant or routine call returning the system page size.
  HAVE_USR_INCLUDE_MALLOC_H (default: NOT defined)
     Optionally define if you are on a system with a /usr/include/malloc.h
     that declares struct mallinfo. It is not at all necessary to
     define this even if you do, but will ensure consistency.
  INTERNAL_SIZE_T           (default: size_t)
     Define to a 32-bit type (probably `unsigned int') if you are on a
     64-bit machine, yet do not want or need to allow malloc requests of
     greater than 2^31 to be handled. This saves space, especially for
     very small chunks.
  INTERNAL_LINUX_C_LIB      (default: NOT defined)
     Defined only when compiled as part of Linux libc.
     Also note that there is some odd internal name-mangling via defines
     (for example, internally, `malloc' is named `mALLOc') needed
     when compiling in this case. These look funny but don't otherwise
     affect anything.
  WIN32                     (default: undefined)
     Define this on MS win (95, nt) platforms to compile in sbrk emulation.
  LACKS_UNISTD_H            (default: undefined if not WIN32)
     Define this if your system does not have a <unistd.h>.
  LACKS_SYS_PARAM_H         (default: undefined if not WIN32)
     Define this if your system does not have a <sys/param.h>.
  MORECORE                  (default: sbrk)
     The name of the routine to call to obtain more memory from the system.
  NULL          (default: -1)
     The value returned upon failure of MORECORE.
  MORECORE_CLEARS           (default 1)
     True (1) if the routine mapped to MORECORE zeroes out memory (which
     holds for sbrk).
  DEFAULT_TRIM_THRESHOLD
  DEFAULT_TOP_PAD
  DEFAULT_MMAP_THRESHOLD
  DEFAULT_MMAP_MAX
     Default values of tunable parameters (described in detail below)
     controlling interaction with host system routines (sbrk, mmap, etc).
     These values may also be changed dynamically via mallopt(). The
     preset defaults are those that give best performance for typical
     programs/systems.
  USE_DL_PREFIX             (default: undefined)
     Prefix all public routines with the string 'dl'.  Useful to
     quickly avoid procedure declaration conflicts and linker symbol
     conflicts with existing memory allocation routines.


*/

#ifndef DEFAULT_TRIM_THRESHOLD
#define DEFAULT_TRIM_THRESHOLD (128 * 1024)
#endif

/*
    M_TRIM_THRESHOLD is the maximum amount of unused top-most memory
      to keep before releasing via malloc_trim in free().

      Automatic trimming is mainly useful in long-lived programs.
      Because trimming via sbrk can be slow on some systems, and can
      sometimes be wasteful (in cases where programs immediately
      afterward allocate more large chunks) the value should be high
      enough so that your overall system performance would improve by
      releasing.

      The trim threshold and the mmap control parameters (see below)
      can be traded off with one another. Trimming and mmapping are
      two different ways of releasing unused memory back to the
      system. Between these two, it is often possible to keep
      system-level demands of a long-lived program down to a bare
      minimum. For example, in one test suite of sessions measuring
      the XF86 X server on Linux, using a trim threshold of 128K and a
      mmap threshold of 192K led to near-minimal long term resource
      consumption.

      If you are using this malloc in a long-lived program, it should
      pay to experiment with these values.  As a rough guide, you
      might set to a value close to the average size of a process
      (program) running on your system.  Releasing this much memory
      would allow such a process to run in memory.  Generally, it's
      worth it to tune for trimming rather tham memory mapping when a
      program undergoes phases where several large chunks are
      allocated and released in ways that can reuse each other's
      storage, perhaps mixed with phases where there are no such
      chunks at all.  And in well-behaved long-lived programs,
      controlling release of large blocks via trimming versus mapping
      is usually faster.

      However, in most programs, these parameters serve mainly as
      protection against the system-level effects of carrying around
      massive amounts of unneeded memory. Since frequent calls to
      sbrk, mmap, and munmap otherwise degrade performance, the default
      parameters are set to relatively high values that serve only as
      safeguards.

      The default trim value is high enough to cause trimming only in
      fairly extreme (by current memory consumption standards) cases.
      It must be greater than page size to have any useful effect.  To
      disable trimming completely, you can set to (unsigned long)(-1);


*/

#ifndef DEFAULT_TOP_PAD
#define DEFAULT_TOP_PAD        (0)
#endif

/*
    M_TOP_PAD is the amount of extra `padding' space to allocate or
      retain whenever sbrk is called. It is used in two ways internally:

      * When sbrk is called to extend the top of the arena to satisfy
	a new malloc request, this much padding is added to the sbrk
	request.

      * When malloc_trim is called automatically from free(),
	it is used as the `pad' argument.

      In both cases, the actual amount of padding is rounded
      so that the end of the arena is always a system page boundary.

      The main reason for using padding is to avoid calling sbrk so
      often. Having even a small pad greatly reduces the likelihood
      that nearly every malloc request during program start-up (or
      after trimming) will invoke sbrk, which needlessly wastes
      time.

      Automatic rounding-up to page-size units is normally sufficient
      to avoid measurable overhead, so the default is 0.  However, in
      systems where sbrk is relatively slow, it can pay to increase
      this value, at the expense of carrying around more memory than
      the program needs.

*/

#ifndef DEFAULT_MMAP_THRESHOLD
#define DEFAULT_MMAP_THRESHOLD (128 * 1024)
#endif

/*

    M_MMAP_THRESHOLD is the request size threshold for using mmap()
      to service a request. Requests of at least this size that cannot
      be allocated using already-existing space will be serviced via mmap.
      (If enough normal freed space already exists it is used instead.)

      Using mmap segregates relatively large chunks of memory so that
      they can be individually obtained and released from the host
      system. A request serviced through mmap is never reused by any
      other request (at least not directly; the system may just so
      happen to remap successive requests to the same locations).

      Segregating space in this way has the benefit that mmapped space
      can ALWAYS be individually released back to the system, which
      helps keep the system level memory demands of a long-lived
      program low. Mapped memory can never become `locked' between
      other chunks, as can happen with normally allocated chunks, which
      means that even trimming via malloc_trim would not release them.

      However, it has the disadvantages that:

	 1. The space cannot be reclaimed, consolidated, and then
	    used to service later requests, as happens with normal chunks.
	 2. It can lead to more wastage because of mmap page alignment
	    requirements
	 3. It causes malloc performance to be more dependent on host
	    system memory management support routines which may vary in
	    implementation quality and may impose arbitrary
	    limitations. Generally, servicing a request via normal
	    malloc steps is faster than going through a system's mmap.

      All together, these considerations should lead you to use mmap
      only for relatively large requests.

*/

#ifndef DEFAULT_MMAP_MAX
#define DEFAULT_MMAP_MAX       (0)
#endif

/*
    M_MMAP_MAX is the maximum number of requests to simultaneously
      service using mmap. This parameter exists because:

	 1. Some systems have a limited number of internal tables for
	    use by mmap.
	 2. In most systems, overreliance on mmap can degrade overall
	    performance.
	 3. If a program allocates many large regions, it is probably
	    better off using normal sbrk-based allocation routines that
	    can reclaim and reallocate normal heap memory. Using a
	    small value allows transition into this mode after the
	    first few allocations.

      Setting to 0 disables all use of mmap.  If HAVE_MMAP is not set,
      the default value is 0, and attempts to set it to non-zero values
      in mallopt will fail.
*/

/*
  INTERNAL_SIZE_T is the word-size used for internal bookkeeping
  of chunk sizes. On a 64-bit machine, you can reduce malloc
  overhead by defining INTERNAL_SIZE_T to be a 32 bit `unsigned int'
  at the expense of not being able to handle requests greater than
  2^31. This limitation is hardly ever a concern; you are encouraged
  to set this. However, the default version is the same as size_t.
*/

#ifndef INTERNAL_SIZE_T
#define INTERNAL_SIZE_T size_t
#endif

/*
  REALLOC_ZERO_BYTES_FREES should be set if a call to
  realloc with zero bytes should be the same as a call to free.
  Some people think it should. Otherwise, since this malloc
  returns a unique pointer for malloc(0), so does realloc(p, 0).
*/

/*   #define REALLOC_ZERO_BYTES_FREES */

/*
  Define HAVE_MMAP to optionally make malloc() use mmap() to
  allocate very large blocks.  These will be returned to the
  operating system immediately after a free().
*/

#define HAVE_MMAP 0	/* Not available for barebox */

/*
  Define HAVE_MREMAP to make realloc() use mremap() to re-allocate
  large blocks.  This is currently only possible on Linux with
  kernel versions newer than 1.3.77.
*/

#undef	HAVE_MREMAP	/* Not available for barebox */

/*

  This version of malloc supports the standard SVID/XPG mallinfo
  routine that returns a struct containing the same kind of
  information you can get from malloc_stats. It should work on
  any SVID/XPG compliant system that has a /usr/include/malloc.h
  defining struct mallinfo. (If you'd like to install such a thing
  yourself, cut out the preliminary declarations as described above
  and below and save them in a malloc.h file. But there's no
  compelling reason to bother to do this.)

  The main declaration needed is the mallinfo struct that is returned
  (by-copy) by mallinfo().  The SVID/XPG malloinfo struct contains a
  bunch of fields, most of which are not even meaningful in this
  version of malloc. Some of these fields are are instead filled by
  mallinfo() with other numbers that might possibly be of interest.

  HAVE_USR_INCLUDE_MALLOC_H should be set if you have a
  /usr/include/malloc.h file that includes a declaration of struct
  mallinfo.  If so, it is included; else an SVID2/XPG2 compliant
  version is declared below.  These must be precisely the same for
  mallinfo() to work.

*/

/* #define HAVE_USR_INCLUDE_MALLOC_H */


/* SVID2/XPG mallinfo structure */

struct mallinfo
{
	int arena;		/* total space allocated from system */
	int ordblks;		/* number of non-inuse chunks */
	int smblks;		/* unused -- always zero */
	int hblks;		/* number of mmapped regions */
	int hblkhd;		/* total space in mmapped regions */
	int usmblks;		/* unused -- always zero */
	int fsmblks;		/* unused -- always zero */
	int uordblks;		/* total allocated space */
	int fordblks;		/* total non-inuse space */
	int keepcost;		/* top-most, releasable (via malloc_trim) space */
};

/* SVID2/XPG mallopt options */

#define M_MXFAST  1		/* UNUSED in this malloc */
#define M_NLBLKS  2		/* UNUSED in this malloc */
#define M_GRAIN   3		/* UNUSED in this malloc */
#define M_KEEP    4		/* UNUSED in this malloc */

/* mallopt options that actually do something */

#define M_TRIM_THRESHOLD    -1
#define M_TOP_PAD           -2
#define M_MMAP_THRESHOLD    -3
#define M_MMAP_MAX          -4

/*
  Access to system page size. To the extent possible, this malloc
  manages memory from the system in page-size units.

  The following mechanics for getpagesize were adapted from
  bsd/gnu getpagesize.h
*/

#define malloc_getpagesize	4096

/*
  Type declarations
*/

struct malloc_chunk
{
	INTERNAL_SIZE_T prev_size;	/* Size of previous chunk (if free). */
	INTERNAL_SIZE_T size;	/* Size in bytes, including overhead. */
	struct malloc_chunk *fd;	/* double links -- used only if free. */
	struct malloc_chunk *bk;
};

typedef struct malloc_chunk *mchunkptr;

/*

   malloc_chunk details:

    (The following includes lightly edited explanations by Colin Plumb.)

    Chunks of memory are maintained using a `boundary tag' method as
    described in e.g., Knuth or Standish.  (See the paper by Paul
    Wilson ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a
    survey of such techniques.)  Sizes of free chunks are stored both
    in the front of each chunk and at the end.  This makes
    consolidating fragmented chunks into bigger chunks very fast.  The
    size fields also hold bits representing whether chunks are free or
    in use.

    An allocated chunk looks like this:


    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Size of previous chunk, if allocated            | |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             User data starts here...                          .
	    .                                                               .
	    .             (malloc_usable_space() bytes)                     .
	    .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Size of chunk                                     |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


    Where "chunk" is the front of the chunk for the purpose of most of
    the malloc code, but "mem" is the pointer that is returned to the
    user.  "Nextchunk" is the beginning of the next contiguous chunk.

    Chunks always begin on even word boundaries, so the mem portion
    (which is returned to the user) is also on an even word boundary, and
    thus double-word aligned.

    Free chunks are stored in circular doubly-linked lists, and look like this:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Size of previous chunk                            |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Forward pointer to next chunk in list             |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Back pointer to previous chunk in list            |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	    |             Unused space (may be 0 bytes long)                .
	    .                                                               .
	    .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
	    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    The P (PREV_INUSE) bit, stored in the unused low-order bit of the
    chunk size (which is always a multiple of two words), is an in-use
    bit for the *previous* chunk.  If that bit is *clear*, then the
    word before the current chunk size contains the previous chunk
    size, and can be used to find the front of the previous chunk.
    (The very first chunk allocated always has this bit set,
    preventing access to non-existent (or non-owned) memory.)

    Note that the `foot' of the current chunk is actually represented
    as the prev_size of the NEXT chunk. (This makes it easier to
    deal with alignments etc).

    The two exceptions to all this are

     1. The special chunk `top', which doesn't bother using the
	trailing size field since there is no
	next contiguous chunk that would have to index off it. (After
	initialization, `top' is forced to always exist.  If it would
	become less than MINSIZE bytes long, it is replenished via
	malloc_extend_top.)

     2. Chunks allocated via mmap, which have the second-lowest-order
	bit (IS_MMAPPED) set in their size fields.  Because they are
	never merged or traversed from any other chunk, they have no
	foot size or inuse information.

    Available chunks are kept in any of several places (all declared below):

    * `av': An array of chunks serving as bin headers for consolidated
       chunks. Each bin is doubly linked.  The bins are approximately
       proportionally (log) spaced.  There are a lot of these bins
       (128). This may look excessive, but works very well in
       practice.  All procedures maintain the invariant that no
       consolidated chunk physically borders another one. Chunks in
       bins are kept in size order, with ties going to the
       approximately least recently used chunk.

       The chunks in each bin are maintained in decreasing sorted order by
       size.  This is irrelevant for the small bins, which all contain
       the same-sized chunks, but facilitates best-fit allocation for
       larger chunks. (These lists are just sequential. Keeping them in
       order almost never requires enough traversal to warrant using
       fancier ordered data structures.)  Chunks of the same size are
       linked with the most recently freed at the front, and allocations
       are taken from the back.  This results in LRU or FIFO allocation
       order, which tends to give each chunk an equal opportunity to be
       consolidated with adjacent freed chunks, resulting in larger free
       chunks and less fragmentation.

    * `top': The top-most available chunk (i.e., the one bordering the
       end of available memory) is treated specially. It is never
       included in any bin, is used only if no other chunk is
       available, and is released back to the system if it is very
       large (see M_TRIM_THRESHOLD).

    * `last_remainder': A bin holding only the remainder of the
       most recently split (non-top) chunk. This bin is checked
       before other non-fitting chunks, so as to provide better
       locality for runs of sequentially allocated chunks.

    *  Implicitly, through the host system's memory mapping tables.
       If supported, requests greater than a threshold are usually
       serviced via calls to mmap, and then later released via munmap.

*/

/*  sizes, alignments */

#define SIZE_SZ                (sizeof(INTERNAL_SIZE_T))
#define MALLOC_ALIGNMENT       (SIZE_SZ + SIZE_SZ)
#define MALLOC_ALIGN_MASK      (MALLOC_ALIGNMENT - 1)
#define MINSIZE                (sizeof(struct malloc_chunk))

/* conversion from malloc headers to user pointers, and back */

#define chunk2mem(p)   ((void*)((char*)(p) + 2*SIZE_SZ))
#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))

/* pad request bytes into a usable size */

#define request2size(req) \
 (((long)((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) < \
  (long)(MINSIZE + MALLOC_ALIGN_MASK)) ? MINSIZE : \
   (((req) + (SIZE_SZ + MALLOC_ALIGN_MASK)) & ~(MALLOC_ALIGN_MASK)))

/* Check if m has acceptable alignment */

#define aligned_OK(m)    (((unsigned long)((m)) & (MALLOC_ALIGN_MASK)) == 0)

/*
  Physical chunk operations
*/

/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */

#define PREV_INUSE 0x1

/* size field is or'ed with IS_MMAPPED if the chunk was obtained with mmap() */

#define IS_MMAPPED 0x2

/* Bits to mask off when extracting size */

#define SIZE_BITS (PREV_INUSE|IS_MMAPPED)

/* Ptr to next physical malloc_chunk. */

#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->size & ~PREV_INUSE) ))

/* Ptr to previous physical malloc_chunk */

#define prev_chunk(p)\
   ((mchunkptr)( ((char*)(p)) - ((p)->prev_size) ))

/* Treat space at ptr + offset as a chunk */

#define chunk_at_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))

/*
  Dealing with use bits
*/

/* extract p's inuse bit */

#define inuse(p)\
((((mchunkptr)(((char*)(p))+((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)

/* extract inuse bit of previous chunk */

#define prev_inuse(p)  ((p)->size & PREV_INUSE)

/* check for mmap()'ed chunk */

#define chunk_is_mmapped(p) ((p)->size & IS_MMAPPED)

/* set/clear chunk as in use without otherwise disturbing */

#define set_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size |= PREV_INUSE

#define clear_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size &= ~(PREV_INUSE)

/* check/set/clear inuse bits in known places */

#define inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size & PREV_INUSE)

#define set_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size |= PREV_INUSE)

#define clear_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size &= ~(PREV_INUSE))

/*
  Dealing with size fields
*/

/* Get size, ignoring use bits */

#define chunksize(p)          ((p)->size & ~(SIZE_BITS))

/* Set size at head, without disturbing its use bit */

#define set_head_size(p, s)   ((p)->size = (((p)->size & PREV_INUSE) | (s)))

/* Set size/use ignoring previous bits in header */

#define set_head(p, s)        ((p)->size = (s))

/* Set size at footer (only when chunk is not in use) */

#define set_foot(p, s)   (((mchunkptr)((char*)(p) + (s)))->prev_size = (s))

/*
   Bins

    The bins, `av_' are an array of pairs of pointers serving as the
    heads of (initially empty) doubly-linked lists of chunks, laid out
    in a way so that each pair can be treated as if it were in a
    malloc_chunk. (This way, the fd/bk offsets for linking bin heads
    and chunks are the same).

    Bins for sizes < 512 bytes contain chunks of all the same size, spaced
    8 bytes apart. Larger bins are approximately logarithmically
    spaced. (See the table below.) The `av_' array is never mentioned
    directly in the code, but instead via bin access macros.

    Bin layout:

    64 bins of size       8
    32 bins of size      64
    16 bins of size     512
     8 bins of size    4096
     4 bins of size   32768
     2 bins of size  262144
     1 bin  of size what's left

    There is actually a little bit of slop in the numbers in bin_index
    for the sake of speed. This makes no difference elsewhere.

    The special chunks `top' and `last_remainder' get their own bins,
    (this is implemented via yet more trickery with the av_ array),
    although `top' is never properly linked to its bin since it is
    always handled specially.

*/

#define NAV             128	/* number of bins */

typedef struct malloc_chunk *mbinptr;

/* access macros */

#define bin_at(i)      ((mbinptr)((char*)&(av_[2*(i) + 2]) - 2*SIZE_SZ))
#define next_bin(b)    ((mbinptr)((char*)(b) + 2 * sizeof(mbinptr)))
#define prev_bin(b)    ((mbinptr)((char*)(b) - 2 * sizeof(mbinptr)))

/*
   The first 2 bins are never indexed. The corresponding av_ cells are instead
   used for bookkeeping. This is not to save space, but to simplify
   indexing, maintain locality, and avoid some initialization tests.
*/

#define top            (bin_at(0)->fd)	/* The topmost chunk */
#define last_remainder (bin_at(1))	/* remainder from last split */

/*
   Because top initially points to its own bin with initial
   zero size, thus forcing extension on the first malloc request,
   we avoid having any special code in malloc to check whether
   it even exists yet. But we still need to in malloc_extend_top.
*/

#define initial_top    ((mchunkptr)(bin_at(0)))

/* Helper macro to initialize bins */

#define IAV(i)  bin_at(i), bin_at(i)

static mbinptr av_[NAV * 2 + 2] = {
	NULL, NULL,
	IAV (0), IAV (1), IAV (2), IAV (3), IAV (4), IAV (5), IAV (6), IAV (7),
	IAV (8), IAV (9), IAV (10), IAV (11), IAV (12), IAV (13), IAV (14),
	IAV (15),
	IAV (16), IAV (17), IAV (18), IAV (19), IAV (20), IAV (21), IAV (22),
	IAV (23),
	IAV (24), IAV (25), IAV (26), IAV (27), IAV (28), IAV (29), IAV (30),
	IAV (31),
	IAV (32), IAV (33), IAV (34), IAV (35), IAV (36), IAV (37), IAV (38),
	IAV (39),
	IAV (40), IAV (41), IAV (42), IAV (43), IAV (44), IAV (45), IAV (46),
	IAV (47),
	IAV (48), IAV (49), IAV (50), IAV (51), IAV (52), IAV (53), IAV (54),
	IAV (55),
	IAV (56), IAV (57), IAV (58), IAV (59), IAV (60), IAV (61), IAV (62),
	IAV (63),
	IAV (64), IAV (65), IAV (66), IAV (67), IAV (68), IAV (69), IAV (70),
	IAV (71),
	IAV (72), IAV (73), IAV (74), IAV (75), IAV (76), IAV (77), IAV (78),
	IAV (79),
	IAV (80), IAV (81), IAV (82), IAV (83), IAV (84), IAV (85), IAV (86),
	IAV (87),
	IAV (88), IAV (89), IAV (90), IAV (91), IAV (92), IAV (93), IAV (94),
	IAV (95),
	IAV (96), IAV (97), IAV (98), IAV (99), IAV (100), IAV (101), IAV (102),
	IAV (103),
	IAV (104), IAV (105), IAV (106), IAV (107), IAV (108), IAV (109),
	IAV (110), IAV (111),
	IAV (112), IAV (113), IAV (114), IAV (115), IAV (116), IAV (117),
	IAV (118), IAV (119),
	IAV (120), IAV (121), IAV (122), IAV (123), IAV (124), IAV (125),
	IAV (126), IAV (127)
};

/* field-extraction macros */

#define first(b) ((b)->fd)
#define last(b)  ((b)->bk)

/*
  Indexing into bins
*/

#define bin_index(sz)                                                          \
(((((unsigned long)(sz)) >> 9) ==    0) ?       (((unsigned long)(sz)) >>  3): \
 ((((unsigned long)(sz)) >> 9) <=    4) ?  56 + (((unsigned long)(sz)) >>  6): \
 ((((unsigned long)(sz)) >> 9) <=   20) ?  91 + (((unsigned long)(sz)) >>  9): \
 ((((unsigned long)(sz)) >> 9) <=   84) ? 110 + (((unsigned long)(sz)) >> 12): \
 ((((unsigned long)(sz)) >> 9) <=  340) ? 119 + (((unsigned long)(sz)) >> 15): \
 ((((unsigned long)(sz)) >> 9) <= 1364) ? 124 + (((unsigned long)(sz)) >> 18): \
					  126)
/*
  bins for chunks < 512 are all spaced 8 bytes apart, and hold
  identically sized chunks. This is exploited in malloc.
*/

#define MAX_SMALLBIN         63
#define MAX_SMALLBIN_SIZE   512
#define SMALLBIN_WIDTH        8

#define smallbin_index(sz)  (((unsigned long)(sz)) >> 3)

/*
   Requests are `small' if both the corresponding and the next bin are small
*/

#define is_small_request(nb) (nb < MAX_SMALLBIN_SIZE - SMALLBIN_WIDTH)

/*
    To help compensate for the large number of bins, a one-level index
    structure is used for bin-by-bin searching.  `binblocks' is a
    one-word bitvector recording whether groups of BINBLOCKWIDTH bins
    have any (possibly) non-empty bins, so they can be skipped over
    all at once during during traversals. The bits are NOT always
    cleared as soon as all bins in a block are empty, but instead only
    when all are noticed to be empty during traversal in malloc.
*/

#define BINBLOCKWIDTH     4	/* bins per block */

#define binblocks      (bin_at(0)->size) /* bitvector of nonempty blocks */

/* bin<->block macros */

#define idx2binblock(ix)    ((unsigned)1 << (ix / BINBLOCKWIDTH))
#define mark_binblock(ii)   (binblocks |= idx2binblock(ii))
#define clear_binblock(ii)  (binblocks &= ~(idx2binblock(ii)))

/* ----------------------------------------------------------------------- */

/*  Other static bookkeeping data */

/* variables holding tunable values */
#ifndef __BAREBOX__
static unsigned long trim_threshold = DEFAULT_TRIM_THRESHOLD;
static unsigned int n_mmaps_max = DEFAULT_MMAP_MAX;
static unsigned long mmap_threshold = DEFAULT_MMAP_THRESHOLD;
#endif
static unsigned long top_pad = DEFAULT_TOP_PAD;

/* The first value returned from sbrk */
static char *sbrk_base = (char*)(-1);

/* The maximum memory obtained from system via sbrk */
static unsigned long max_sbrked_mem;

/* The maximum via either sbrk or mmap */
static unsigned long max_total_mem;

/* internal working copy of mallinfo */
static struct mallinfo current_mallinfo;

/* The total memory obtained from system via sbrk */
#define sbrked_mem  (current_mallinfo.arena)

/* Tracking mmaps */

static unsigned long mmapped_mem;

/*
  Macro-based internal utilities
*/


/*
  Linking chunks in bin lists.
  Call these only with variables, not arbitrary expressions, as arguments.
*/

/*
  Place chunk p of size s in its bin, in size order,
  putting it ahead of others of same size.
*/

#define frontlink(P, S, IDX, BK, FD)                                          \
{                                                                             \
  if (S < MAX_SMALLBIN_SIZE)                                                  \
  {                                                                           \
    IDX = smallbin_index(S);                                                  \
    mark_binblock(IDX);                                                       \
    BK = bin_at(IDX);                                                         \
    FD = BK->fd;                                                              \
    P->bk = BK;                                                               \
    P->fd = FD;                                                               \
    FD->bk = BK->fd = P;                                                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    IDX = bin_index(S);                                                       \
    BK = bin_at(IDX);                                                         \
    FD = BK->fd;                                                              \
    if (FD == BK) mark_binblock(IDX);                                         \
    else                                                                      \
    {                                                                         \
      while (FD != BK && S < chunksize(FD)) FD = FD->fd;                      \
      BK = FD->bk;                                                            \
    }                                                                         \
    P->bk = BK;                                                               \
    P->fd = FD;                                                               \
    FD->bk = BK->fd = P;                                                      \
  }                                                                           \
}

/* take a chunk off a list */

#define unlink(P, BK, FD)                                                     \
{                                                                             \
  BK = P->bk;                                                                 \
  FD = P->fd;                                                                 \
  FD->bk = BK;                                                                \
  BK->fd = FD;                                                                \
}                                                                             \

/* Place p as the last remainder */

#define link_last_remainder(P)                                                \
{                                                                             \
  last_remainder->fd = last_remainder->bk =  P;                               \
  P->fd = P->bk = last_remainder;                                             \
}

/* Clear the last_remainder bin */

#define clear_last_remainder \
  (last_remainder->fd = last_remainder->bk = last_remainder)

/* Routines dealing with mmap(). */

/*
  Extend the top-most chunk by obtaining memory from system.
  Main interface to sbrk (but see also malloc_trim).
*/
static void malloc_extend_top(INTERNAL_SIZE_T nb)
{
	char *brk;		/* return value from sbrk */
	INTERNAL_SIZE_T front_misalign;	/* unusable bytes at front of sbrked space */
	INTERNAL_SIZE_T correction;	/* bytes for 2nd sbrk call */
	char *new_brk;		/* return of 2nd sbrk call */
	INTERNAL_SIZE_T top_size;	/* new size of top chunk */

	mchunkptr old_top = top;	/* Record state of old top */
	INTERNAL_SIZE_T old_top_size = chunksize(old_top);
	char *old_end = (char *) (chunk_at_offset(old_top, old_top_size));

	/* Pad request with top_pad plus minimal overhead */

	INTERNAL_SIZE_T sbrk_size = nb + top_pad + MINSIZE;
	unsigned long pagesz = malloc_getpagesize;

	/* If not the first time through, round to preserve page boundary */
	/* Otherwise, we need to correct to a page size below anyway. */
	/* (We also correct below if an intervening foreign sbrk call.) */

	if (sbrk_base != (char*)(-1))
		sbrk_size = (sbrk_size + (pagesz - 1)) & ~(pagesz - 1);

	brk = (char*)(sbrk(sbrk_size));

	/* Fail if sbrk failed or if a foreign sbrk call killed our space */
	if (brk == (char*)(NULL) || (brk < old_end && old_top != initial_top))
		return;

	sbrked_mem += sbrk_size;

	if (brk == old_end) {	/* can just add bytes to current top */
		top_size = sbrk_size + old_top_size;
		set_head (top, top_size | PREV_INUSE);
	} else {
		if (sbrk_base == (char*)(-1))	/* First time through. Record base */
			sbrk_base = brk;
		else		/* Someone else called sbrk().  Count those bytes as sbrked_mem. */
			sbrked_mem += brk - (char*)old_end;

		/* Guarantee alignment of first new chunk made from this space */
		front_misalign =
			(unsigned long) chunk2mem(brk) & MALLOC_ALIGN_MASK;
		if (front_misalign > 0) {
			correction = (MALLOC_ALIGNMENT) - front_misalign;
			brk += correction;
		} else
			correction = 0;

		/* Guarantee the next brk will be at a page boundary */

		correction += ((((unsigned long) (brk + sbrk_size)) +
				(pagesz - 1)) & ~(pagesz - 1)) -
			((unsigned long) (brk + sbrk_size));

		/* Allocate correction */
		new_brk = (char*) (sbrk(correction));
		if (new_brk == (char*)(NULL))
			return;

		sbrked_mem += correction;

		top = (mchunkptr) brk;
		top_size = new_brk - brk + correction;
		set_head (top, top_size | PREV_INUSE);

		if (old_top != initial_top) {

			/* There must have been an intervening foreign sbrk call. */
			/* A double fencepost is necessary to prevent consolidation */

			/* If not enough space to do this, then user did something very wrong */
			if (old_top_size < MINSIZE) {
				set_head (top, PREV_INUSE);	/* will force null return from malloc */
				return;
			}

			/* Also keep size a multiple of MALLOC_ALIGNMENT */
			old_top_size = (old_top_size -
					3 * SIZE_SZ) & ~MALLOC_ALIGN_MASK;
			set_head_size (old_top, old_top_size);
			chunk_at_offset (old_top, old_top_size)->size =
				SIZE_SZ | PREV_INUSE;
			chunk_at_offset (old_top, old_top_size + SIZE_SZ)->size =
				SIZE_SZ | PREV_INUSE;
			/* If possible, release the rest. */
			if (old_top_size >= MINSIZE)
				free(chunk2mem (old_top));
		}
	}

	if ((unsigned long) sbrked_mem > (unsigned long) max_sbrked_mem)
		max_sbrked_mem = sbrked_mem;
	if ((unsigned long) (mmapped_mem + sbrked_mem) > (unsigned long) max_total_mem)
		max_total_mem = mmapped_mem + sbrked_mem;
}

/* Main public routines */

/*
  Malloc Algorithm:

    The requested size is first converted into a usable form, `nb'.
    This currently means to add 4 bytes overhead plus possibly more to
    obtain 8-byte alignment and/or to obtain a size of at least
    MINSIZE (currently 16 bytes), the smallest allocatable size.
    (All fits are considered `exact' if they are within MINSIZE bytes.)

    From there, the first successful of the following steps is taken:

      1. The bin corresponding to the request size is scanned, and if
	 a chunk of exactly the right size is found, it is taken.

      2. The most recently remaindered chunk is used if it is big
	 enough.  This is a form of (roving) first fit, used only in
	 the absence of exact fits. Runs of consecutive requests use
	 the remainder of the chunk used for the previous such request
	 whenever possible. This limited use of a first-fit style
	 allocation strategy tends to give contiguous chunks
	 coextensive lifetimes, which improves locality and can reduce
	 fragmentation in the long run.

      3. Other bins are scanned in increasing size order, using a
	 chunk big enough to fulfill the request, and splitting off
	 any remainder.  This search is strictly by best-fit; i.e.,
	 the smallest (with ties going to approximately the least
	 recently used) chunk that fits is selected.

      4. If large enough, the chunk bordering the end of memory
	 (`top') is split off. (This use of `top' is in accord with
	 the best-fit search rule.  In effect, `top' is treated as
	 larger (and thus less well fitting) than any other available
	 chunk since it can be extended to be as large as necessary
	 (up to system limitations).

      5. If the request size meets the mmap threshold and the
	 system supports mmap, and there are few enough currently
	 allocated mmapped regions, and a call to mmap succeeds,
	 the request is allocated via direct memory mapping.

      6. Otherwise, the top of memory is extended by
	 obtaining more space from the system (normally using sbrk,
	 but definable to anything else via the MORECORE macro).
	 Memory is gathered from the system (in system page-sized
	 units) in a way that allows chunks obtained across different
	 sbrk calls to be consolidated, but does not require
	 contiguous memory. Thus, it should be safe to intersperse
	 mallocs with other sbrk calls.


      All allocations are made from the the `lowest' part of any found
      chunk. (The implementation invariant is that prev_inuse is
      always true of any allocated chunk; i.e., that each allocated
      chunk borders either a previously allocated and still in-use chunk,
      or the base of its memory arena.)
*/
void *malloc(size_t bytes)
{
	mchunkptr victim;	/* inspected/selected chunk */
	INTERNAL_SIZE_T victim_size;	/* its size */
	int idx;		/* index for bin traversal */
	mbinptr bin;		/* associated bin */
	mchunkptr remainder;	/* remainder from a split */
	long remainder_size;	/* its size */
	int remainder_index;	/* its bin index */
	unsigned long block;	/* block traverser bit */
	int startidx;		/* first bin of a traversed block */
	mchunkptr fwd;		/* misc temp for linking */
	mchunkptr bck;		/* misc temp for linking */
	mbinptr q;		/* misc temp */

	INTERNAL_SIZE_T nb;

	if ((long) bytes < 0)
		return NULL;

	nb = request2size(bytes); /* padded request size; */

	/* Check for exact match in a bin */

	if (is_small_request(nb)) { /* Faster version for small requests */
		idx = smallbin_index(nb);

		/* No traversal or size check necessary for small bins.  */

		q = bin_at(idx);
		victim = last(q);

		/* Also scan the next one, since it would have a remainder < MINSIZE */
		if (victim == q) {
			q = next_bin(q);
			victim = last(q);
		}
		if (victim != q) {
			victim_size = chunksize(victim);
			unlink(victim, bck, fwd);
			set_inuse_bit_at_offset(victim, victim_size);
			return chunk2mem(victim);
		}
		idx += 2;	/* Set for bin scan below. We've already scanned 2 bins. */
	} else {
		idx = bin_index(nb);
		bin = bin_at(idx);

		for (victim = last(bin); victim != bin; victim = victim->bk) {
			victim_size = chunksize(victim);
			remainder_size = victim_size - nb;

			if (remainder_size >= (long)MINSIZE) {	/* too big */
				--idx;	/* adjust to rescan below after checking last remainder */
				break;
			}

			else if (remainder_size >= 0) {	/* exact fit */
				unlink(victim, bck, fwd);
				set_inuse_bit_at_offset(victim, victim_size);
				return chunk2mem(victim);
			}
		}
		++idx;
	}

	/* Try to use the last split-off remainder */

	if ((victim = last_remainder->fd) != last_remainder) {
		victim_size = chunksize(victim);
		remainder_size = victim_size - nb;

		if (remainder_size >= (long)MINSIZE) {	/* re-split */
			remainder = chunk_at_offset(victim, nb);
			set_head(victim, nb | PREV_INUSE);
			link_last_remainder(remainder);
			set_head(remainder, remainder_size | PREV_INUSE);
			set_foot(remainder, remainder_size);
			return chunk2mem(victim);
		}

		clear_last_remainder;

		if (remainder_size >= 0) {	/* exhaust */
			set_inuse_bit_at_offset(victim, victim_size);
			return chunk2mem(victim);
		}
		/* Else place in bin */
		frontlink(victim, victim_size, remainder_index, bck, fwd);
	}

	/*
	   If there are any possibly nonempty big-enough blocks,
	   search for best fitting chunk by scanning bins in blockwidth units.
	 */
	if ((block = idx2binblock (idx)) <= binblocks) {
		/* Get to the first marked block */
		if ((block & binblocks) == 0) {
			/* force to an even block boundary */
			idx = (idx & ~(BINBLOCKWIDTH - 1)) + BINBLOCKWIDTH;
			block <<= 1;
			while ((block & binblocks) == 0) {
				idx += BINBLOCKWIDTH;
				block <<= 1;
			}
		}

		/* For each possibly nonempty block ... */
		for (;;) {
			startidx = idx;	/* (track incomplete blocks) */
			q = bin = bin_at(idx);

			/* For each bin in this block ... */
			do {
				/* Find and use first big enough chunk ... */
				for (victim = last(bin); victim != bin;
				     victim = victim->bk) {
					victim_size = chunksize(victim);
					remainder_size = victim_size - nb;

					if (remainder_size >= (long)MINSIZE) {	/* split */
						remainder =
							chunk_at_offset (victim,
									 nb);
						set_head(victim,
							nb | PREV_INUSE);
						unlink(victim, bck, fwd);
						link_last_remainder(remainder);
						set_head(remainder,
							remainder_size |
							PREV_INUSE);
						set_foot(remainder,
							remainder_size);
						return chunk2mem(victim);
					} else if (remainder_size >= 0) { /* take */
						set_inuse_bit_at_offset(victim,
									victim_size);
						unlink(victim, bck, fwd);
						return chunk2mem(victim);
					}
				}
				bin = next_bin (bin);
			} while ((++idx & (BINBLOCKWIDTH - 1)) != 0);

			/* Clear out the block bit. */
			do {	/* Possibly backtrack to try to clear a partial block */
				if ((startidx & (BINBLOCKWIDTH - 1)) == 0) {
					binblocks &= ~block;
					break;
				}
				--startidx;
				q = prev_bin(q);
			} while (first(q) == q);

			/* Get to the next possibly nonempty block */

			if ((block <<= 1) <= binblocks && (block != 0)) {
				while ((block & binblocks) == 0) {
					idx += BINBLOCKWIDTH;
					block <<= 1;
				}
			} else
				break;
		}
	}

	/* Try to use top chunk */

	/* Require that there be a remainder, ensuring top always exists  */
	if ((remainder_size = chunksize (top) - nb) < (long) MINSIZE) {
		/* Try to extend */
		malloc_extend_top(nb);
		if ((remainder_size = chunksize(top) - nb) < (long) MINSIZE)
			return NULL;	/* propagate failure */
	}

	victim = top;
	set_head(victim, nb | PREV_INUSE);
	top = chunk_at_offset(victim, nb);
	set_head(top, remainder_size | PREV_INUSE);
	return chunk2mem(victim);
}

/*
  free() algorithm :

    cases:

       1. free(0) has no effect.

       2. If the chunk was allocated via mmap, it is release via munmap().

       3. If a returned chunk borders the current high end of memory,
	  it is consolidated into the top, and if the total unused
	  topmost memory exceeds the trim threshold, malloc_trim is
	  called.

       4. Other chunks are consolidated as they arrive, and
	  placed in corresponding bins. (This includes the case of
	  consolidating with the current `last_remainder').
*/
void free(void *mem)
{
	mchunkptr p;		/* chunk corresponding to mem */
	INTERNAL_SIZE_T hd;	/* its head field */
	INTERNAL_SIZE_T sz;	/* its size */
	int idx;		/* its bin index */
	mchunkptr next;		/* next contiguous chunk */
	INTERNAL_SIZE_T nextsz;	/* its size */
	INTERNAL_SIZE_T prevsz;	/* size of previous contiguous chunk */
	mchunkptr bck;		/* misc temp for linking */
	mchunkptr fwd;		/* misc temp for linking */
	int islr;		/* track whether merging with last_remainder */

	if (!mem)		/* free(0) has no effect */
		return;

	p = mem2chunk(mem);
	hd = p->size;


	sz = hd & ~PREV_INUSE;
	next = chunk_at_offset(p, sz);
	nextsz = chunksize(next);

	if (next == top) { /* merge with top */
		sz += nextsz;

		if (!(hd & PREV_INUSE)) {	/* consolidate backward */
			prevsz = p->prev_size;
			p = chunk_at_offset(p, -((long) prevsz));
			sz += prevsz;
			unlink (p, bck, fwd);
		}

		set_head(p, sz | PREV_INUSE);
		top = p;
#ifdef USE_MALLOC_TRIM
		if ((unsigned long) (sz) >= (unsigned long)trim_threshold)
			malloc_trim(top_pad);
#endif
		return;
	}

	set_head(next, nextsz); /* clear inuse bit */

	islr = 0;

	if (!(hd & PREV_INUSE)) {	/* consolidate backward */
		prevsz = p->prev_size;
		p = chunk_at_offset(p, -((long) prevsz));
		sz += prevsz;

		if (p->fd == last_remainder) /* keep as last_remainder */
			islr = 1;
		else
			unlink(p, bck, fwd);
	}

	if (!(inuse_bit_at_offset(next, nextsz))) { /* consolidate forward */
		sz += nextsz;

		if (!islr && next->fd == last_remainder) { /* re-insert last_remainder */
			islr = 1;
			link_last_remainder(p);
		} else
			unlink(next, bck, fwd);
	}


	set_head(p, sz | PREV_INUSE);
	set_foot(p, sz);
	if (!islr)
		frontlink(p, sz, idx, bck, fwd);
}

/*
  Realloc algorithm:

    Chunks that were obtained via mmap cannot be extended or shrunk
    unless HAVE_MREMAP is defined, in which case mremap is used.
    Otherwise, if their reallocation is for additional space, they are
    copied.  If for less, they are just left alone.

    Otherwise, if the reallocation is for additional space, and the
    chunk can be extended, it is, else a malloc-copy-free sequence is
    taken.  There are several different ways that a chunk could be
    extended. All are tried:

       * Extending forward into following adjacent free chunk.
       * Shifting backwards, joining preceding adjacent space
       * Both shifting backwards and extending forward.
       * Extending into newly sbrked space

    Unless the #define REALLOC_ZERO_BYTES_FREES is set, realloc with a
    size argument of zero (re)allocates a minimum-sized chunk.

    If the reallocation is for less space, and the new request is for
    a `small' (<512 bytes) size, then the newly unused space is lopped
    off and freed.

    The old unix realloc convention of allowing the last-free'd chunk
    to be used as an argument to realloc is no longer supported.
    I don't know of any programs still relying on this feature,
    and allowing it would also allow too many other incorrect
    usages of realloc to be sensible.
*/
void *realloc(void *oldmem, size_t bytes)
{
	INTERNAL_SIZE_T nb;	/* padded request size */

	mchunkptr oldp;		/* chunk corresponding to oldmem */
	INTERNAL_SIZE_T oldsize;	/* its size */

	mchunkptr newp;		/* chunk to return */
	INTERNAL_SIZE_T newsize;	/* its size */
	void *newmem;		/* corresponding user mem */

	mchunkptr next;		/* next contiguous chunk after oldp */
	INTERNAL_SIZE_T nextsize;	/* its size */

	mchunkptr prev;		/* previous contiguous chunk before oldp */
	INTERNAL_SIZE_T prevsize;	/* its size */

	mchunkptr remainder;	/* holds split off extra space from newp */
	INTERNAL_SIZE_T remainder_size;	/* its size */

	mchunkptr bck;		/* misc temp for linking */
	mchunkptr fwd;		/* misc temp for linking */

#ifdef REALLOC_ZERO_BYTES_FREES
	if (bytes == 0) {
		free(oldmem);
		return NULL;
	}
#endif

	if ((long)bytes < 0)
		return NULL;

	/* realloc of null is supposed to be same as malloc */
	if (!oldmem)
		return malloc(bytes);

	newp = oldp = mem2chunk(oldmem);
	newsize = oldsize = chunksize(oldp);


	nb = request2size(bytes);


	if ((long)(oldsize) < (long)(nb)) {

		/* Try expanding forward */

		next = chunk_at_offset(oldp, oldsize);
		if (next == top || !inuse(next)) {
			nextsize = chunksize(next);

			/* Forward into top only if a remainder */
			if (next == top) {
				if ((long)(nextsize + newsize) >=
				    (long)(nb + MINSIZE)) {
					newsize += nextsize;
					top = chunk_at_offset(oldp, nb);
					set_head (top,
						  (newsize - nb) | PREV_INUSE);
					set_head_size(oldp, nb);
					return chunk2mem(oldp);
				}
			}

			/* Forward into next chunk */
			else if (((long) (nextsize + newsize) >= (long) (nb))) {
				unlink(next, bck, fwd);
				newsize += nextsize;
				goto split;
			}
		} else {
			next = NULL;
			nextsize = 0;
		}

		/* Try shifting backwards. */

		if (!prev_inuse(oldp)) {
			prev = prev_chunk(oldp);
			prevsize = chunksize(prev);

			/* try forward + backward first to save a later consolidation */

			if (next) {
				/* into top */
				if (next == top) {
					if ((long)
					    (nextsize + prevsize + newsize) >=
					    (long)(nb + MINSIZE)) {
						unlink (prev, bck, fwd);
						newp = prev;
						newsize += prevsize + nextsize;
						newmem = chunk2mem(newp);
						memcpy(newmem, oldmem,
							oldsize - SIZE_SZ);
						top = chunk_at_offset(newp, nb);
						set_head(top,
							  (newsize -
							   nb) | PREV_INUSE);
						set_head_size(newp, nb);
						return newmem;
					}
				}

				/* into next chunk */
				else if (((long)(nextsize + prevsize + newsize)
					  >= (long)(nb))) {
					unlink(next, bck, fwd);
					unlink(prev, bck, fwd);
					newp = prev;
					newsize += nextsize + prevsize;
					newmem = chunk2mem(newp);
					memcpy(newmem, oldmem,
						oldsize - SIZE_SZ);
					goto split;
				}
			}

			/* backward only */
			if (prev && (long)(prevsize + newsize) >= (long)nb) {
				unlink(prev, bck, fwd);
				newp = prev;
				newsize += prevsize;
				newmem = chunk2mem(newp);
				memcpy(newmem, oldmem, oldsize - SIZE_SZ);
				goto split;
			}
		}

		/* Must allocate */

		newmem = malloc(bytes);

		if (!newmem)	/* propagate failure */
			return NULL;

		/* Avoid copy if newp is next chunk after oldp. */
		/* (This can only happen when new chunk is sbrk'ed.) */

		if ((newp = mem2chunk(newmem)) == next_chunk(oldp)) {
			newsize += chunksize(newp);
			newp = oldp;
			goto split;
		}

		/* Otherwise copy, free, and exit */
		memcpy(newmem, oldmem, oldsize - SIZE_SZ);
		free(oldmem);
		return newmem;
	}


split:	/* split off extra room in old or expanded chunk */

	if (newsize - nb >= MINSIZE) {	/* split off remainder */
		remainder = chunk_at_offset(newp, nb);
		remainder_size = newsize - nb;
		set_head_size(newp, nb);
		set_head(remainder, remainder_size | PREV_INUSE);
		set_inuse_bit_at_offset(remainder, remainder_size);
		free (chunk2mem(remainder)); /* let free() deal with it */
	} else {
		set_head_size(newp, newsize);
		set_inuse_bit_at_offset(newp, newsize);
	}

	return chunk2mem(newp);
}

/*
  memalign algorithm:

    memalign requests more than enough space from malloc, finds a spot
    within that chunk that meets the alignment request, and then
    possibly frees the leading and trailing space.

    The alignment argument must be a power of two. This property is not
    checked by memalign, so misuse may result in random runtime errors.

    8-byte alignment is guaranteed by normal malloc calls, so don't
    bother calling memalign with an argument of 8 or less.

    Overreliance on memalign is a sure way to fragment space.
*/
void *memalign(size_t alignment, size_t bytes)
{
	INTERNAL_SIZE_T nb;	/* padded  request size */
	char *m;		/* memory returned by malloc call */
	mchunkptr p;		/* corresponding chunk */
	char *brk;		/* alignment point within p */
	mchunkptr newp;		/* chunk to return */
	INTERNAL_SIZE_T newsize;	/* its size */
	INTERNAL_SIZE_T leadsize;	/* leading space before alignment point */
	mchunkptr remainder;	/* spare room at end to split off */
	long remainder_size;	/* its size */

	if ((long) bytes < 0)
		return NULL;

	/* If need less alignment than we give anyway, just relay to malloc */

	if (alignment <= MALLOC_ALIGNMENT)
		return malloc(bytes);

	/* Otherwise, ensure that it is at least a minimum chunk size */

	if (alignment < MINSIZE)
		alignment = MINSIZE;

	/* Call malloc with worst case padding to hit alignment. */

	nb = request2size(bytes);
	m = (char*)(malloc (nb + alignment + MINSIZE));

	if (!m)
		return NULL;	/* propagate failure */

	p = mem2chunk(m);

	if ((((unsigned long)(m)) % alignment) == 0) {	/* aligned */
	} else {		/* misaligned */

		/*
		   Find an aligned spot inside chunk.
		   Since we need to give back leading space in a chunk of at
		   least MINSIZE, if the first calculation places us at
		   a spot with less than MINSIZE leader, we can move to the
		   next aligned spot -- we've allocated enough total room so that
		   this is always possible.
		 */

		brk = (char*) mem2chunk(((unsigned long) (m + alignment - 1)) &
					  -((signed) alignment));
		if ((long)(brk - (char*)(p)) < MINSIZE)
			brk = brk + alignment;

		newp = (mchunkptr)brk;
		leadsize = brk - (char*)(p);
		newsize = chunksize(p) - leadsize;

		/* give back leader, use the rest */

		set_head(newp, newsize | PREV_INUSE);
		set_inuse_bit_at_offset(newp, newsize);
		set_head_size(p, leadsize);
		free(chunk2mem(p));
		p = newp;
	}

	/* Also give back spare room at the end */

	remainder_size = chunksize(p) - nb;

	if (remainder_size >= (long)MINSIZE) {
		remainder = chunk_at_offset(p, nb);
		set_head(remainder, remainder_size | PREV_INUSE);
		set_head_size(p, nb);
		free (chunk2mem(remainder));
	}

	return chunk2mem(p);
}

/*
 *
 * calloc calls malloc, then zeroes out the allocated chunk.
 *
 */
void *calloc(size_t n, size_t elem_size)
{
	mchunkptr p;
	INTERNAL_SIZE_T csz;
	INTERNAL_SIZE_T sz = n * elem_size;
	void *mem;

	/* check if expand_top called, in which case don't need to clear */
	mchunkptr oldtop = top;
	INTERNAL_SIZE_T oldtopsize = chunksize(top);

	if ((long)n < 0)
		return NULL;

	mem = malloc(sz);

	if (!mem)
		return NULL;
	else {
		p = mem2chunk(mem);

		/* Two optional cases in which clearing not necessary */
		csz = chunksize(p);

		if (p == oldtop && csz > oldtopsize) {
			/* clear only the bytes from non-freshly-sbrked memory */
			csz = oldtopsize;
		}

		memset(mem, 0, csz - SIZE_SZ);
		return mem;
	}
}

/* Utility to update current_mallinfo for malloc_stats and mallinfo() */

static void malloc_update_mallinfo(void)
{
	int i;
	mbinptr b;
	mchunkptr p;

#ifdef DEBUG
	mchunkptr q;
#endif

	INTERNAL_SIZE_T avail = chunksize(top);
	int navail = ((long)(avail) >= (long)MINSIZE) ? 1 : 0;

	for (i = 1; i < NAV; ++i) {
		b = bin_at (i);
		for (p = last(b); p != b; p = p->bk) {
#ifdef DEBUG
			for (q = next_chunk(p);
			     q < top && inuse(q)
			     && (long) (chunksize(q)) >= (long)MINSIZE;
			     q = next_chunk(q))
#endif
				avail += chunksize(p);
			navail++;
		}
	}

	current_mallinfo.ordblks = navail;
	current_mallinfo.uordblks = sbrked_mem - avail;
	current_mallinfo.fordblks = avail;
#if HAVE_MMAP
	current_mallinfo.hblks = n_mmaps;
#endif
	current_mallinfo.hblkhd = mmapped_mem;
	current_mallinfo.keepcost = chunksize(top);

}

/*
  malloc_stats:

    Prints on the amount of space obtain from the system (both
    via sbrk and mmap), the maximum amount (which may be more than
    current if malloc_trim and/or munmap got called), the maximum
    number of simultaneous mmap regions used, and the current number
    of bytes allocated via malloc (or realloc, etc) but not yet
    freed. (Note that this is the number of bytes allocated, not the
    number requested. It will be larger than the number requested
    because of alignment and bookkeeping overhead.)
 */
/*
 * mallinfo returns a copy of updated current mallinfo.
 */
void malloc_stats(void)
{
	malloc_update_mallinfo();
	printf("Maximum system memory: %u\n", (unsigned int)(max_total_mem));
	printf("Current system memory: %u\n",
		(unsigned int)(sbrked_mem + mmapped_mem));
	printf("in use: %u\n",
		(unsigned int)(current_mallinfo.uordblks + mmapped_mem));
#if HAVE_MMAP
	printf("Maximum mmap'ed mmap regions: %u\n",
		 (unsigned int) max_n_mmaps);
#endif
}

/*

History:

    V2.6.6 Sun Dec  5 07:42:19 1999  Doug Lea  (dl at gee)
      * return null for negative arguments
      * Added Several WIN32 cleanups from Martin C. Fong <mcfong@yahoo.com>
	 * Add 'LACKS_SYS_PARAM_H' for those systems without 'sys/param.h'
	  (e.g. WIN32 platforms)
	 * Cleanup up header file inclusion for WIN32 platforms
	 * Cleanup code to avoid Microsoft Visual C++ compiler complaints
	 * Add 'USE_DL_PREFIX' to quickly allow co-existence with existing
	   memory allocation routines
	 * Set 'malloc_getpagesize' for WIN32 platforms (needs more work)
	 * Use 'assert' rather than 'ASSERT' in WIN32 code to conform to
	   usage of 'assert' in non-WIN32 code
	 * Improve WIN32 'sbrk()' emulation's 'findRegion()' routine to
	   avoid infinite loop
      * Always call 'fREe()' rather than 'free()'

    V2.6.5 Wed Jun 17 15:57:31 1998  Doug Lea  (dl at gee)
      * Fixed ordering problem with boundary-stamping

    V2.6.3 Sun May 19 08:17:58 1996  Doug Lea  (dl at gee)
      * Added pvalloc, as recommended by H.J. Liu
      * Added 64bit pointer support mainly from Wolfram Gloger
      * Added anonymously donated WIN32 sbrk emulation
      * Malloc, calloc, getpagesize: add optimizations from Raymond Nijssen
      * malloc_extend_top: fix mask error that caused wastage after
	foreign sbrks
      * Add linux mremap support code from HJ Liu

    V2.6.2 Tue Dec  5 06:52:55 1995  Doug Lea  (dl at gee)
      * Integrated most documentation with the code.
      * Add support for mmap, with help from
	Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Use last_remainder in more cases.
      * Pack bins using idea from  colin@nyx10.cs.du.edu
      * Use ordered bins instead of best-fit threshold
      * Eliminate block-local decls to simplify tracing and debugging.
      * Support another case of realloc via move into top
      * Fix error occurring when initial sbrk_base not word-aligned.
      * Rely on page size for units instead of SBRK_UNIT to
	avoid surprises about sbrk alignment conventions.
      * Add mallinfo, mallopt. Thanks to Raymond Nijssen
	(raymond@es.ele.tue.nl) for the suggestion.
      * Add `pad' argument to malloc_trim and top_pad mallopt parameter.
      * More precautions for cases where other routines call sbrk,
	courtesy of Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Added macros etc., allowing use in linux libc from
	H.J. Lu (hjl@gnu.ai.mit.edu)
      * Inverted this history list

    V2.6.1 Sat Dec  2 14:10:57 1995  Doug Lea  (dl at gee)
      * Re-tuned and fixed to behave more nicely with V2.6.0 changes.
      * Removed all preallocation code since under current scheme
	the work required to undo bad preallocations exceeds
	the work saved in good cases for most test programs.
      * No longer use return list or unconsolidated bins since
	no scheme using them consistently outperforms those that don't
	given above changes.
      * Use best fit for very large chunks to prevent some worst-cases.
      * Added some support for debugging

    V2.6.0 Sat Nov  4 07:05:23 1995  Doug Lea  (dl at gee)
      * Removed footers when chunks are in use. Thanks to
	Paul Wilson (wilson@cs.texas.edu) for the suggestion.

    V2.5.4 Wed Nov  1 07:54:51 1995  Doug Lea  (dl at gee)
      * Added malloc_trim, with help from Wolfram Gloger
	(wmglo@Dent.MED.Uni-Muenchen.DE).

    V2.5.3 Tue Apr 26 10:16:01 1994  Doug Lea  (dl at g)

    V2.5.2 Tue Apr  5 16:20:40 1994  Doug Lea  (dl at g)
      * realloc: try to expand in both directions
      * malloc: swap order of clean-bin strategy;
      * realloc: only conditionally expand backwards
      * Try not to scavenge used bins
      * Use bin counts as a guide to preallocation
      * Occasionally bin return list chunks in first scan
      * Add a few optimizations from colin@nyx10.cs.du.edu

    V2.5.1 Sat Aug 14 15:40:43 1993  Doug Lea  (dl at g)
      * faster bin computation & slightly different binning
      * merged all consolidations to one part of malloc proper
	 (eliminating old malloc_find_space & malloc_clean_bin)
      * Scan 2 returns chunks (not just 1)
      * Propagate failure in realloc if malloc returns 0
      * Add stuff to allow compilation on non-ANSI compilers
	  from kpv@research.att.com

    V2.5 Sat Aug  7 07:41:59 1993  Doug Lea  (dl at g.oswego.edu)
      * removed potential for odd address access in prev_chunk
      * removed dependency on getpagesize.h
      * misc cosmetics and a bit more internal documentation
      * anticosmetics: mangled names in macros to evade debugger strangeness
      * tested on sparc, hp-700, dec-mips, rs6000
	  with gcc & native cc (hp, dec only) allowing
	  Detlefs & Zorn comparison study (in SIGPLAN Notices.)

    Trial version Fri Aug 28 13:14:29 1992  Doug Lea  (dl at g.oswego.edu)
      * Based loosely on libg++-1.2X malloc. (It retains some of the overall
	 structure of old version,  but most details differ.)

*/

EXPORT_SYMBOL(malloc);
EXPORT_SYMBOL(calloc);
EXPORT_SYMBOL(free);
EXPORT_SYMBOL(realloc);
