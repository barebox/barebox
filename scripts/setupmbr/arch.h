
/* we need the one from the host */
#include <endian.h>
#include <stdint.h>

/* Byte-orders.  */
#define swap16(x)	\
({ \
   uint16_t _x = (x); \
   (uint16_t) ((_x << 8) | (_x >> 8)); \
})

#define swap32(x)	\
({ \
   uint32_t _x = (x); \
   (uint32_t) ((_x << 24) \
                    | ((_x & (uint32_t) 0xFF00UL) << 8) \
                    | ((_x & (uint32_t) 0xFF0000UL) >> 8) \
                    | (_x >> 24)); \
})

#define swap64(x)	\
({ \
   uint64_t _x = (x); \
   (uint64_t) ((_x << 56) \
                    | ((_x & (uint64_t) 0xFF00ULL) << 40) \
                    | ((_x & (uint64_t) 0xFF0000ULL) << 24) \
                    | ((_x & (uint64_t) 0xFF000000ULL) << 8) \
                    | ((_x & (uint64_t) 0xFF00000000ULL) >> 8) \
                    | ((_x & (uint64_t) 0xFF0000000000ULL) >> 24) \
                    | ((_x & (uint64_t) 0xFF000000000000ULL) >> 40) \
                    | (_x >> 56)); \
})

#if __BYTE_ORDER == __BIG_ENDIAN

/* Our target is a ia32 machine, always little endian */

# define host2target_16(x)	swap16(x)
# define host2target_32(x)	swap32(x)
# define host2target_64(x)	swap64(x)
# define target2host_16(x)	swap16(x)
# define target2host_32(x)	swap32(x)
# define target2host_64(x)	swap64(x)

#else

# define host2target_16(x)	(x)
# define host2target_32(x)	(x)
# define host2target_64(x)	(x)
# define target2host_16(x)	(x)
# define target2host_32(x)	(x)
# define target2host_64(x)	(x)

#endif
