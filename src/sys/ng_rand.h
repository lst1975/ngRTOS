/**
 * @file
 *
 * This library offers the tiny and fast ISAAC cryptographically secure pseudo
 * random number generator (CSPRNG), in its 32-bit and 64-bit version wrapped
 * into a modern, ISO C11, documented API, ready for embedded usage.
 *
 * Use the #ISAAC_BITS macro to compile LibISAAC optimised for 32 or 64 bits.
 * The 32 bit is the classic ISAAC; the 64 bit is ISAAC-64. Note that the output
 * differs between the two and the context grows twice in size when using the 64
 * bit version, but you also get twice the bytes per reshuffling.
 *
 * Then the usage of ISAAC is easy:
 * - init the context with a secret seed with isaac_init()
 * - get any amount of pseudo-random numbers with isaac_stream()
 * - if you need bytes instead of numbers, convert them using the utility
 *   functions isaac_to_big_endian() or isaac_to_little_endian()
 * - When you are done using ISAAC, cleanup the context to avoid leaking secret
 *   data with isaac_cleanup()
 *
 * **About ISAAC**
 *
 * Quoting from its
 * [web page](https://www.burtleburtle.net/bob/rand/isaacafa.html):
 *
 * > ISAAC (Indirection, Shift, Accumulate, Add, and Count) generates 32-bit
 * > random numbers. Averaged out, it requires 18.75 machine cycles to
 * > generate each 32-bit value. Cycles are guaranteed to be at least 240
 * > values long, and they are 28295 values long on average. The results are
 * > uniformly distributed, unbiased, and unpredictable unless you know the
 * > seed.
 * > [...]
 * > ISAAC-64 generates a different sequence than ISAAC, but it uses the same
 * > principles. It uses 64-bit arithmetic. It generates a 64-bit result every
 * > 19 instructions. All cycles are at least 272 values, and the average
 * > cycle length is 216583.
 *
 * ISAAC and its original source code is created by Bob Jenkins and
 * released into the public domain.
 *
 * This implementation is based on the original `rand.c` and
 * `isaac64.c`, which uses 32-bit and 64-bit words respectively.
 *
 * @copyright Copyright ? 2020, Matja? Gu?tin <dev@matjaz.it>
 * <https://matjaz.it>. All rights reserved.
 * @license BSD 3-clause license.
 */

#ifndef __ngrtos_RAND_H__
#define __ngrtos_RAND_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ng_config.h"

#if configEnable_Random

/**
 * Version of the LibISAAC API using semantic versioning.
 */
#define LIBISAAC_VERSION "1.0.0"

#include <stdint.h>
#include <stddef.h>

#include "ng_defs.h"

/**
 * @property #ISAAC_BITS
 * Set it to 32 or 64 to optimise ISAAC for 32 or 64 bit words respectively.
 *
 * The 32 bit is the classic ISAAC; the 64 bit is ISAAC-64. Note that the output
 * differs between the two and the context grows twice in size when using the 64
 * bit version, but you also get twice the bytes per reshuffling.
 */
/**
 * @property isaac_uint_t
 * An integer or word used by ISAAC, either a uint32_t or uint64_t.
 */
#define ISAAC_BITS __NG_BITS_PER_LONG

#if (ISAAC_BITS == 32)
typedef uint32_t isaac_uint_t;
#elif (ISAAC_BITS == 64)
typedef uint64_t isaac_uint_t;
#else
#error "ISAAC: only 32 or 64 bit words are supported."
#endif

typedef void (*isaac_reseed_f)(void *);

void isaac_RNG_Rand_Fill(isaac_uint_t* ints, size_t amount);
uint32_t isaac_RNG_Rand(void);

ng_result_e ng_rand_init(void);
void ng_rand_deinit(void);

#else

static inline ng_result_e ng_rand_init(void)
{
  NGRTOS_UNUSED(seed);
  return NG_result_ok;
}

static inline void ng_rand_deinit(void)
{
  return;
}

#endif

#ifdef __cplusplus
}
#endif

#endif  /* __ngrtos_RAND_H__ */
