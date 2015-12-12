#include "FNV.h"

#define HAVE_64BIT_LONG_LONG

/*
 * hash_64 - 64 bit Fowler/Noll/Vo-0 hash code
 *
 * @(#) $Revision: 5.1 $
 * @(#) $Id: hash_64.c,v 5.1 2009/06/30 09:01:38 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/fnv/RCS/hash_64.c,v $
 *
 ***
 *
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 *
 ***
 *
 * NOTE: The FNV-0 historic hash is not recommended.  One should use
 *	 the FNV-1 hash instead.
 *
 * To use the 64 bit FNV-0 historic hash, pass FNV0_64_INIT as the
 * Fnv64_t hashval argument to fnv_64_buf() or fnv_64_str().
 *
 * To use the recommended 64 bit FNV-1 hash, pass FNV1_64_INIT as the
 * Fnv64_t hashval argument to fnv_64_buf() or fnv_64_str().
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy!	:-)
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define FNV_VERSION "5.0.2"	/* @(#) FNV Version */

/*
 * 32 bit magic FNV-1a prime
 */
#define FNV_32_PRIME ((Fnv32_t)0x01000193)

/*
 * 32 bit FNV-0 hash type
 */
typedef uint32_t Fnv32_t;

/*
 * 32 bit FNV-0 zero initial basis
 *
 * This historic hash is not recommended.  One should use
 * the FNV-1 hash and initial basis instead.
 */
#define FNV0_32_INIT ((Fnv32_t)0)

/*
 * 32 bit FNV-1 and FNV-1a non-zero initial basis
 *
 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
 *
 *              chongo <Landon Curt Noll> /\../\
 *
 * NOTE: The \'s above are not back-slashing escape characters.
 * They are literal ASCII  backslash 0x5c characters.
 *
 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
 */
#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)
#define FNV1_32A_INIT FNV1_32_INIT


/*
 * determine how 64 bit unsigned values are represented
 */
// #include "longlong.h"


/*
 * 64 bit FNV-0 hash
 */
#if defined(HAVE_64BIT_LONG_LONG)
typedef uint64_t Fnv64_t;
#else /* HAVE_64BIT_LONG_LONG */
typedef struct {
    u_int32_t w32[2]; /* w32[0] is low order, w32[1] is high order word */
} Fnv64_t;
#endif /* HAVE_64BIT_LONG_LONG */


/*
 * 64 bit FNV-0 zero initial basis
 *
 * This historic hash is not recommended.  One should use
 * the FNV-1 hash and initial basis instead.
 */
#if defined(HAVE_64BIT_LONG_LONG)
#define FNV0_64_INIT ((Fnv64_t)0)
#else /* HAVE_64BIT_LONG_LONG */
extern const Fnv64_t fnv0_64_init;
#define FNV0_64_INIT (fnv0_64_init)
#endif /* HAVE_64BIT_LONG_LONG */


/*
 * 64 bit FNV-1 non-zero initial basis
 *
 * The FNV-1 initial basis is the FNV-0 hash of the following 32 octets:
 *
 *              chongo <Landon Curt Noll> /\../\
 *
 * NOTE: The \'s above are not back-slashing escape characters.
 * They are literal ASCII  backslash 0x5c characters.
 *
 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
 */
#if defined(HAVE_64BIT_LONG_LONG)
#define FNV1_64_INIT ((Fnv64_t)0xcbf29ce484222325ULL)
#define FNV1A_64_INIT FNV1_64_INIT
#else /* HAVE_64BIT_LONG_LONG */
extern const fnv1_64_init;
extern const Fnv64_t fnv1a_64_init;
#define FNV1_64_INIT (fnv1_64_init)
#define FNV1A_64_INIT (fnv1a_64_init)
#endif /* HAVE_64BIT_LONG_LONG */


/*
 * hash types
 */
enum fnv_type {
    FNV_NONE = 0,	/* invalid FNV hash type */
    FNV0_32 = 1,	/* FNV-0 32 bit hash */
    FNV1_32 = 2,	/* FNV-1 32 bit hash */
    FNV1a_32 = 3,	/* FNV-1a 32 bit hash */
    FNV0_64 = 4,	/* FNV-0 64 bit hash */
    FNV1_64 = 5,	/* FNV-1 64 bit hash */
    FNV1a_64 = 6,	/* FNV-1a 64 bit hash */
};


/*
 * FNV-0 defines the initial basis to be zero
 */
#if !defined(HAVE_64BIT_LONG_LONG)
const Fnv64_t fnv0_64_init = { 0UL, 0UL };
#endif /* ! HAVE_64BIT_LONG_LONG */


/*
 * FNV-1 defines the initial basis to be non-zero
 */
#if !defined(HAVE_64BIT_LONG_LONG)
const Fnv64_t fnv1_64_init = { 0x84222325UL, 0xcbf29ce4UL };
#endif /* ! HAVE_64BIT_LONG_LONG */


/*
 * 64 bit magic FNV-0 and FNV-1 prime
 */
#if defined(HAVE_64BIT_LONG_LONG)
#define FNV_64_PRIME ((Fnv64_t)0x100000001b3ULL)
#else /* HAVE_64BIT_LONG_LONG */
#define FNV_64_PRIME_LOW ((unsigned long)0x1b3)	/* lower bits of FNV prime */
#define FNV_64_PRIME_SHIFT (8)		/* top FNV prime shift above 2^32 */
#endif /* HAVE_64BIT_LONG_LONG */


/*
 * fnv_64a_buf - perform a 64 bit Fowler/Noll/Vo FNV-1a hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	64 bit hash as a static hash type
 *
 * NOTE: To use the recommended 64 bit FNV-1a hash, use FNV1A_64_INIT as the
 * 	 hval arg on the first call to either fnv_64a_buf() or fnv_64a_str().
 */
static Fnv64_t fnv_64a_buf(const void *buf, size_t len, Fnv64_t hval) {
  const unsigned char *bp = (const unsigned char *)buf;	/* start of buffer */
  const unsigned char *be = bp + len;		/* beyond end of buffer */
  
#if defined(HAVE_64BIT_LONG_LONG)
  /* FNV-1a hash each octet of the buffer */
  while (bp < be) {
    
    /* xor the bottom with the current octet */
    hval ^= (Fnv64_t)*bp++;
    
    /* multiply by the 64 bit FNV magic prime mod 2^64 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
    hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */
  }
  
#else /* HAVE_64BIT_LONG_LONG */
  
  unsigned long val[4];			/* hash value in base 2^16 */
  unsigned long tmp[4];			/* tmp 64 bit value */
  
  /* Convert Fnv64_t hval into a base 2^16 array */
  val[0] = hval.w32[0];
  val[1] = (val[0] >> 16);
  val[0] &= 0xffff;
  val[2] = hval.w32[1];
  val[3] = (val[2] >> 16);
  val[2] &= 0xffff;
  
  /* FNV-1a hash each octet of the buffer */
  while (bp < be) {
    
    /* xor the bottom with the current octet */
    val[0] ^= (unsigned long)*bp++;
    
    /*
     * multiply by the 64 bit FNV magic prime mod 2^64
     *
     * Using 0x100000001b3 we have the following digits base 2^16:
     *
     *	0x0	0x100	0x0	0x1b3
     *
     * which is the same as:
     *
     *	0x0	1<<FNV_64_PRIME_SHIFT	0x0	FNV_64_PRIME_LOW
     */
    /* multiply by the lowest order digit base 2^16 */
    tmp[0] = val[0] * FNV_64_PRIME_LOW;
    tmp[1] = val[1] * FNV_64_PRIME_LOW;
    tmp[2] = val[2] * FNV_64_PRIME_LOW;
    tmp[3] = val[3] * FNV_64_PRIME_LOW;
    /* multiply by the other non-zero digit */
    tmp[2] += val[0] << FNV_64_PRIME_SHIFT;	/* tmp[2] += val[0] * 0x100 */
    tmp[3] += val[1] << FNV_64_PRIME_SHIFT;	/* tmp[3] += val[1] * 0x100 */
    /* propagate carries */
    tmp[1] += (tmp[0] >> 16);
    val[0] = tmp[0] & 0xffff;
    tmp[2] += (tmp[1] >> 16);
    val[1] = tmp[1] & 0xffff;
    val[3] = tmp[3] + (tmp[2] >> 16);
    val[2] = tmp[2] & 0xffff;
    /*
     * Doing a val[3] &= 0xffff; is not really needed since it simply
     * removes multiples of 2^64.  We can discard these excess bits
     * outside of the loop when we convert to Fnv64_t.
     */
  }
  
  /*
   * Convert base 2^16 array back into an Fnv64_t
   */
  hval.w32[1] = ((val[3]<<16) | val[2]);
  hval.w32[0] = ((val[1]<<16) | val[0]);
  
#endif /* HAVE_64BIT_LONG_LONG */
  
  /* return our new hash value */
  return hval;
}

/*
 * fnv_32a_buf - perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the recommended 32 bit FNV-1a hash, use FNV1_32A_INIT as the
 * 	 hval arg on the first call to either fnv_32a_buf() or fnv_32a_str().
 */
static Fnv32_t fnv_32a_buf(const void * buf, size_t len, Fnv32_t hval) {
  const unsigned char * bp = (const unsigned char *)buf;	/* start of buffer */
  const unsigned char * be = bp + len;		/* beyond end of buffer */
  
  /*
   * FNV-1a hash each octet in the buffer
   */
  while (bp < be) {
    
    /* xor the bottom with the current octet */
    hval ^= (Fnv32_t)*bp++;
    
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
    hval *= FNV_32_PRIME;
#else
    hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
  }
  
  /* return our new hash value */
  return hval;
}

long long
FNV::calcFNV1a_64(const void * ptr, size_t len) {
  return fnv_64a_buf(ptr, len, FNV1A_64_INIT);
}

int
FNV::calcFNV1a_32(const void * ptr, size_t len) {
  return fnv_32a_buf(ptr, len, FNV1_32A_INIT);
}
