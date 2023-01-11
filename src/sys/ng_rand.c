/**
 * @file
 *
 * LibISAAC implementation.
 *
 * Basic on the 32-bit portable implementation by Bob Jenkins, originally
 * public domain:
 * https://www.burtleburtle.net/bob/c/randport.c
 *
 * @copyright Copyright ? 2020, Matja? Gu?tin <dev@matjaz.it>
 * <https://matjaz.it>. All rights reserved.
 * @license BSD 3-clause license.
 */

#include "ng_rand.h"

#if configEnable_Random

#if __NG_BITS_PER_LONG == 32
#define LONG_TYPE_SIZE 4
#elif __NG_BITS_PER_LONG == 64
#define LONG_TYPE_SIZE 8
#endif

#define ISAAC_BYTES (ISAAC_BITS >> 3)

/**
 * Amount of elements in ISAAC's context arrays.
 */
#define ISAAC_ELEMENTS 32U

/**
 * Max bytes supported in the seed.
 */
#define ISAAC_SEED_MAX_BYTES ISAAC_ELEMENTS

typedef struct isaac_ctx isaac_ctx_t;

/**
 * Context of the ISAAC CPRNG.
 *
 * No need to inspect it manually, use the functions instead.
 *
 * Maps to `randctx` from the original implementation.
 */
struct isaac_ctx
{
  isaac_uint_t result[ISAAC_ELEMENTS]; /** In this field the pseudo-random data is generated. */
  isaac_uint_t mem[ISAAC_ELEMENTS]; /** Internal field. */
  
  isaac_uint_t a; /** Internal field. */
  isaac_uint_t b; /** Internal field. */
  isaac_uint_t c; /** Internal field. */
  
  /**
   * Index of the next value to output in the stream.
   *
   * Note: this value could be a uint16_t instead of a isaac_uint_t, but by
   * using an isaac_uint_t we avoid any padding at the end of the struct.
   */
  isaac_reseed_f reseed;
  uint16_t seed_index;
  uint16_t stream_index;
  uint16_t seeding:1;
} ;

/**
 * Initialises the ISAAC CPRNG with a seed.
 *
 * The seed is copied value-wise into the ISAAC state, not byte-wise. That
 * means that a uint8_t array {1,2,3,4} is copied into the ctx->result[]
 * isaac_uint_t array as {1,2,3,4,0,...,0}, where each value is a isaac_uint_t
 * value.
 * Looking at the bytes and assuming little Endian byte order, the result is
 * {1,2,3,4} --> {1,0,0,0,2,0,0,0,3,0,0,0,4,0,0,0,0,...,0}.
 *
 * The reason behind this choice is to avoid issues with endianness; as ISAAC
 * works on isaac_uint_t values rather than their bytes, setting the
 * isaac_uint_t values and not their bytes, shall produce the same CPRNG stream
 * on architectures with different endianness.
 * An isaac_uint_t* could also be a valid choice as seed
 * input, but seeds are usually cryptographic keys and those are byte arrays,
 * so a developer could be confused on how to insert a uint8_t* seed into
 * a isaac_uint_t*.
 *
 * Maps to `void randinit(randctx *r, word flag)` from the original
 * implementation. Equivalent to a true `flag` with a seed provided. The
 * false `flag` is not available, as it should not be used for security
 * purposes.
 *
 * @warning
 * Failing to provide a seed (NULL or long 0 bytes), will make the whole CSPRNG
 * insecure as a zero-seed is used instead. This is only useful if a
 * **non-cryptographic** PRNG is required.
 *
 * @warning
 * Providing a seed with low entropy will result in the whole CSPRNG to be
 * weak.
 *
 * @param[in, out] ctx the ISAAC state to be initialised. Does nothing when
 * NULL.
 * @param[in] seed pointer to the seed to use, which is copied into the context.
 * - If NULL, then a zero seed is used instead (**insecure!**)
 * @param[in] seed_bytes amount of **bytes** in the seed, max
 * #ISAAC_SEED_MAX_BYTES.
 * - If 0, then a zero seed is used instead (**insecure!**)
 * - If > #ISAAC_SEED_MAX_BYTES, then only #ISAAC_SEED_MAX_BYTES will be used
 * - If < #ISAAC_SEED_MAX_BYTES, then the provided bytes will be used and the
 *   rest will be zero-padded.
 */
void isaac_init(isaac_ctx_t* ctx, isaac_reseed_f reseed, 
           const uint8_t *seed, uint16_t seed_bytes);

/**
 * Provides the next pseudo-random integer.
 *
 * Because ISAAC works on 32 or 64 bit values, the stream is in integers
 * instead of bytes.
 * To convert them to bytes:
 * - get some values into a isaac_uint_t buffer with isaac_stream()
 * - allocate a uint8_t buffer
 * - convert the isaac_uint_t buffer to the uint8_t one using the utility
 *   functions isaac_to_little_endian() or isaac_to_big_endian() for little and
 *   big endian respectively.
 *
 * Every #ISAAC_ELEMENTS values generated it will automatically reshuffle
 * the ISAAC state to cache #ISAAC_ELEMENTS new elements. This means that
 * the first #ISAAC_ELEMENTS values after seeding are very cheap (just
 * copying values from the state) and the #ISAAC_ELEMENTS+1st value is
 * more expensive and so on.
 *
 * @param[in, out] ctx the ISAAC state, already initialised.
 * Does nothing when NULL.
 * @param[out] ints pseudo-random integers. Does nothing when NULL.
 * @param[in] amount quantity of 32-bit/64-bit integers to generate.
 */
void isaac_stream(isaac_ctx_t* ctx, isaac_uint_t* ints, size_t amount);

/**
 * Provides the next pseudo-random integer.
 *
 * Because ISAAC works on 32 or 64 bit values, the stream is in integers
 * instead of bytes.
 * To convert them to bytes:
 * - get some values into a isaac_uint_t buffer with isaac_stream()
 * - allocate a uint8_t buffer
 * - convert the isaac_uint_t buffer to the uint8_t one using the utility
 *   functions isaac_to_little_endian() or isaac_to_big_endian() for little and
 *   big endian respectively.
 *
 * Every #ISAAC_ELEMENTS values generated it will automatically reshuffle
 * the ISAAC state to cache #ISAAC_ELEMENTS new elements. This means that
 * the first #ISAAC_ELEMENTS values after seeding are very cheap (just
 * copying values from the state) and the #ISAAC_ELEMENTS+1st value is
 * more expensive and so on.
 *
 * @param[in, out] ctx the ISAAC state, already initialised.
 * Does nothing when NULL.
 * @param[out] ints pseudo-random integers. Does nothing when NULL.
 * @param[in] amount quantity of 32-bit/64-bit integers to generate.
 */
isaac_uint_t isaac_rand(isaac_ctx_t* const ctx);

/**
 * @internal
 * Copies the seed into ctx->result[], padding it with zeros.
 *
 * @param ctx the ISAAC state
 * @param seed bytes of the seed. If NULL, a zero-seed is used.
 * @param seed_bytes amount of bytes in the seed.
 */
uint16_t isaac_set_seed(isaac_ctx_t* ctx, const uint8_t* seed, uint16_t seed_count);

#define ISAAC_MIN(a, b) ((a) < (b)) ? (a) : (b)

#if ISAAC_BITS > 32
#define ISAAC_IND(mm, x)  (*(uint64_t*)((uint8_t*)(mm) \
              + ((x) & ((ISAAC_ELEMENTS - 1) << 3))))

#define ISAAC_STEP(mix, a, b, mm, m, m2, r, x) \
{ \
  x = *m;  \
  a = (mix) + *(m2++); \
  *(m++) = y = ISAAC_IND(mm, x) + a + b; \
  *(r++) = b = ISAAC_IND(mm, y >> 8U) + x; \
}

#define ISAAC_MIX(a, b, c, d, e, f, g, h) \
{ \
   a -= e; f ^= h >> 9U;  h += a; \
   b -= f; g ^= a << 9U;  a += b; \
   c -= g; h ^= b >> 23U; b += c; \
   d -= h; a ^= c << 15U; c += d; \
   e -= a; b ^= d >> 14U; d += e; \
   f -= b; c ^= e << 20U; e += f; \
   g -= c; d ^= f >> 17U; f += g; \
   h -= d; e ^= g << 14U; g += h; \
}

/* Explanations why it does not look like 1.618033988749894848...:
 * https://stackoverflow.com/a/4948967
 * https://softwareengineering.stackexchange.com/a/63605
 */
#define GOLDEN_RATIO 0x9e3779b97f4a7c13LL
#else
#define ISAAC_IND(mm, x) ((mm)[(x >> 2U) & (ISAAC_ELEMENTS - 1)])

#define ISAAC_STEP(mix, a, b, mm, m, m2, r, x) \
{ \
  x = *m;  \
  a = (a^(mix)) + *(m2++); \
  *(m++) = y = ISAAC_IND(mm, x) + a + b; \
  *(r++) = b = ISAAC_IND(mm, y >> 8U) + x; \
}

#define ISAAC_MIX(a, b, c, d, e, f, g, h) \
{ \
   a ^= b << 11U; d += a; b += c; \
   b ^= c >> 2U;  e += b; c += d; \
   c ^= d << 8U;  f += c; d += e; \
   d ^= e >> 16U; g += d; e += f; \
   e ^= f << 10U; h += e; f += g; \
   f ^= g >> 4U;  a += f; g += h; \
   g ^= h << 8U;  b += g; h += a; \
   h ^= a >> 9U;  c += h; a += b; \
}

/* Explanations why it does not look like 1.618033988749894848...:
 * https://stackoverflow.com/a/4948967
 * https://softwareengineering.stackexchange.com/a/63605
 */
#define GOLDEN_RATIO 0x9e3779b9L
#endif

static void isaac_shuffle(isaac_ctx_t* ctx);

void isaac_init(isaac_ctx_t* const ctx,
        isaac_reseed_f reseed,
        const uint8_t* const seed,
        const uint16_t seed_bytes)
{
  isaac_uint_t a, b, c, d, e, f, g, h;
  uint16_t i; /* Fastest index over elements in result[] and mem[]. */
  
  ctx->seed_index   = 0;
  ctx->stream_index = ISAAC_ELEMENTS;
  ctx->seeding      = 0;
  ctx->a            = 0;
  ctx->b            = 0;
  ctx->c            = 0;
  ctx->reseed       = reseed;
  
  a = b = c = d = e = f = g = h = GOLDEN_RATIO;
  /* Scramble it */
  for (i = 0; i < 4; i++)
  {
    ISAAC_MIX(a, b, c, d, e, f, g, h);
  }
  isaac_set_seed(ctx, seed, seed_bytes);
  
  /* Initialise using the contents of result[] as the seed. */
  for (i = 0; i < ISAAC_ELEMENTS; i += 8)
  {
    a += ctx->result[i + 0];
    b += ctx->result[i + 1];
    c += ctx->result[i + 2];
    d += ctx->result[i + 3];
    e += ctx->result[i + 4];
    f += ctx->result[i + 5];
    g += ctx->result[i + 6];
    h += ctx->result[i + 7];
    ISAAC_MIX(a, b, c, d, e, f, g, h);
    ctx->mem[i + 0] = a;
    ctx->mem[i + 1] = b;
    ctx->mem[i + 2] = c;
    ctx->mem[i + 3] = d;
    ctx->mem[i + 4] = e;
    ctx->mem[i + 5] = f;
    ctx->mem[i + 6] = g;
    ctx->mem[i + 7] = h;
  }
  
  /* Do a second pass to make all of the seed affect all of ctx->mem. */
  for (i = 0; i < ISAAC_ELEMENTS; i += 8)
  {
    a += ctx->mem[i + 0];
    b += ctx->mem[i + 1];
    c += ctx->mem[i + 2];
    d += ctx->mem[i + 3];
    e += ctx->mem[i + 4];
    f += ctx->mem[i + 5];
    g += ctx->mem[i + 6];
    h += ctx->mem[i + 7];
    ISAAC_MIX(a, b, c, d, e, f, g, h);
    ctx->mem[i + 0] = a;
    ctx->mem[i + 1] = b;
    ctx->mem[i + 2] = c;
    ctx->mem[i + 3] = d;
    ctx->mem[i + 4] = e;
    ctx->mem[i + 5] = f;
    ctx->mem[i + 6] = g;
    ctx->mem[i + 7] = h;
  }

  if (reseed)
  {
    reseed(ctx);
  }
  else
  {
    isaac_shuffle(ctx);
  }
}

/**
 * @internal
 * Copies the seed into ctx->result[], padding it with zeros.
 *
 * @param ctx the ISAAC state
 * @param seed bytes of the seed. If NULL, a zero-seed is used.
 * @param seed_bytes amount of bytes in the seed.
 */
uint16_t isaac_set_seed(isaac_ctx_t* const ctx,
           const uint8_t* const seed,
           uint16_t seed_bytes)
{
  uint16_t idx = __LDREXH(&ctx->seed_index);
  if (seed != NULL && idx < ISAAC_ELEMENTS)
  {
    seed_bytes = ISAAC_MIN((ISAAC_ELEMENTS-idx)*ISAAC_BYTES, seed_bytes);
    memcpy(&ctx->result[idx], seed, seed_bytes);
    __STREXH(idx+seed_bytes/ISAAC_BYTES, &ctx->seed_index);
    return seed_bytes;
  }
  else
  {
    ctx->seeding = 1;    
  }
  return 0;
}

/**
 * @internal
 * Permutes the ISAAC state.
 *
 * Maps to `void isaac(randctx*)` from the original implementation.
 *
 * @param ctx the ISAAC state
 */
static void isaac_shuffle(isaac_ctx_t* const ctx)
{
  isaac_uint_t* m;
  isaac_uint_t* mm = ctx->mem;
  isaac_uint_t* m2;
  isaac_uint_t* r = ctx->result;
  isaac_uint_t* mend;
  isaac_uint_t a = ctx->a;
  isaac_uint_t b = ctx->b + (++ctx->c);
  isaac_uint_t x;
  isaac_uint_t y;
#if ISAAC_BITS > 32
  for (m = mm, mend = m2 = m + (ISAAC_ELEMENTS >> 1); m < mend;)
  {
    ISAAC_STEP(~(a ^ (a << 21U)), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a >> 5U), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a << 12U), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a >> 33U), a, b, mm, m, m2, r, x);
  }
  for (m2 = mm; m2 < mend;)
  {
    ISAAC_STEP(~(a ^ (a << 21U)), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a >> 5U), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a << 12U), a, b, mm, m, m2, r, x);
    ISAAC_STEP(a ^ (a >> 33U), a, b, mm, m, m2, r, x);
  }
#else
  for (m = mm, mend = m2 = m + (ISAAC_ELEMENTS >> 1); m < mend;)
  {
    ISAAC_STEP(a << 13U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a >> 6U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a << 2U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a >> 16U, a, b, mm, m, m2, r, x);
  }
  for (m2 = mm; m2 < mend;)
  {
    ISAAC_STEP(a << 13U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a >> 6U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a << 2U, a, b, mm, m, m2, r, x);
    ISAAC_STEP(a >> 16U, a, b, mm, m, m2, r, x);
  }
#endif
  ctx->b = b;
  ctx->a = a;
}

static void isaac_check_shuffle(isaac_ctx_t* const ctx)
{
  uint16_t idx = __LDREXH(&ctx->stream_index);
  if (idx >= ISAAC_ELEMENTS >> 1)
  {
    if (ctx->reseed)
    {
      __STREXH(0, &ctx->seed_index);
      __DMB();
      ctx->reseed(ctx);
    }
    else
    {
      if (idx >= ISAAC_ELEMENTS)
      {
        /* Out of elements. Reshuffling and preparing new batch. */
        isaac_shuffle(ctx);
        __STREXH(0, &ctx->stream_index);
      }
    }
  }
}

void 
isaac_stream(isaac_ctx_t* const ctx, isaac_uint_t *ints, size_t amount)
{
  uint16_t idx = __LDREXH(&ctx->stream_index);
  
  while (amount)
  {
    uint16_t available;
    available = ISAAC_MIN(ISAAC_ELEMENTS - idx, amount);
    amount -= available;
    while (available--)
    {
      *ints++ = ctx->result[idx++];
    };

    isaac_check_shuffle(ctx);
  }
}

isaac_uint_t 
isaac_rand(isaac_ctx_t* const ctx)
{
  uint16_t idx;
  isaac_check_shuffle(ctx);
  idx = __LDREXH(&ctx->stream_index);
  __STREXH(idx+1, &ctx->stream_index);
  return ctx->result[idx];
}

static isaac_ctx_t __rand_ctx;

ng_result_e ng_rand_init(void)
{
  isaac_uint_t seed = 0;
  isaac_init(&__rand_ctx, NULL, (uint8_t *)&seed, sizeof(seed));
  return NG_result_ok;
}

void ng_rand_deinit(void)
{
}

void isaac_RNG_Rand_Fill(isaac_uint_t* ints, size_t amount)
{
  isaac_stream(&__rand_ctx, ints, amount);
}
uint32_t isaac_RNG_Rand(void)
{
  return isaac_rand(&__rand_ctx);
}

#endif
