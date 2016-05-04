#pragma once
/**
 * @file  SFMT-avx2.h
 * @brief SIMD oriented Fast Mersenne Twister(SFMT) for Intel AVX2
 *
 * @author Seizh
 * @note this file is based from SFMT-sse2.h
 *  (see original SFMT implementation)
 * Copyright (C) 2015 SeizhLab. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */

#ifndef SFMT_AVX2_H
#define SFMT_AVX2_H

#if SFMT_SL1 < 16
  #error sorry, assumed SFMT_SL1 >= 16.
#endif
#if SFMT_N & 1
  #error sorry, assumed SFMT_N is even number.
#endif
#if SFMT_POS1 & 1
  #error sorry, assumed SFMT_POS1 is even number.
#endif

/**
 * This function represents the recursion formula.
 * @param r an output
 * @param a double 128-bit part of the interal state array
 * @param b double 128-bit part of the interal state array
 * @param c double 128-bit part of the interal state array
 */
inline static void mm_recursion2(__m256i* r, __m256i a, __m256i b, __m256i c)
{
    __m256i x, y, z;

    x = _mm256_slli_si256(a, SFMT_SL2);
    x = _mm256_xor_si256(x, a);
    y = _mm256_srli_epi32(b, SFMT_SR1);
    y = _mm256_and_si256(y, avx2_param_mask.ymm);
    x = _mm256_xor_si256(x, y);
    z = _mm256_srli_si256(c, SFMT_SR2);
    x = _mm256_xor_si256(x, z);

    /* assume SFMT_SL1 >= 16 */
    z = _mm256_permute2f128_si256(c, x, 0x21);     /* [c.upper, x.lower] */
    z = _mm256_slli_epi32(z, SFMT_SL1);
    x = _mm256_xor_si256(x, z);

    _mm256_store_si256(r, x);
}

/**
 * This function fills the internal state array with pseudorandom
 * integers.
 * @param sfmt SFMT internal state
 */
void sfmt_gen_rand_all(sfmt_t * sfmt) {
    int i;
    __m256i r;
    w128_t * pstate = sfmt->state;

    r = _mm256_load_si256((__m256i*)&pstate[SFMT_N - 2]);
    for (i = 0; i < SFMT_N - SFMT_POS1; i+=2) {
        mm_recursion2(
            (__m256i*)&pstate[i],
            _mm256_load_si256((__m256i*)&pstate[i] ),
            _mm256_load_si256((__m256i*)&pstate[i + SFMT_POS1]),
            r
        );
        r = _mm256_load_si256((__m256i*)&pstate[i]);
    }
    for (; i < SFMT_N; i+= 2) {
        mm_recursion2(
            (__m256i*)&pstate[i],
            _mm256_load_si256((__m256i*)&pstate[i] ),
            _mm256_load_si256((__m256i*)&pstate[i + SFMT_POS1 - SFMT_N]),
            r
        );
        r = _mm256_load_si256((__m256i*)&pstate[i]);
    }
}

/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 * @param sfmt SFMT internal state.
 * @param array an 128-bit array to be filled by pseudorandom numbers.
 * @param size number of 128-bit pseudorandom numbers to be generated.
 */
static void gen_rand_array(sfmt_t * sfmt, w128_t * array, int size)
{
    int i, j;
    __m256i r;
    w128_t * pstate = sfmt->state;

    r = _mm256_loadu_si256((__m256i*)&pstate[SFMT_N - 2]);
    for (i = 0; i < SFMT_N - SFMT_POS1; i+=2) {
        mm_recursion2(
            (__m256i*)&array[i],
            _mm256_load_si256((__m256i*)&pstate[i] ),
            _mm256_load_si256((__m256i*)&pstate[i + SFMT_POS1]),
            r
        );
        r = _mm256_load_si256((__m256i*)&array[i]);
    }
    for (; i < SFMT_N; i+=2) {
        mm_recursion2(
            (__m256i*)&array[i],
            _mm256_load_si256((__m256i*)&pstate[i] ),
            _mm256_load_si256((__m256i*)&array[i + SFMT_POS1 - SFMT_N]),
            r
        );
        r = _mm256_loadu_si256((__m256i*)&array[i]);
    }
    for (; i < size - SFMT_N; i+=2) {
        mm_recursion2(
            (__m256i*)&array[i],
            _mm256_load_si256((__m256i*)&array[i - SFMT_N] ),
            _mm256_load_si256((__m256i*)&array[i + SFMT_POS1 - SFMT_N]),
            r
        );
        r = _mm256_load_si256((__m256i*)&array[i]);

    }
    for (j = 0; j < 2 * SFMT_N - size; j += 2) {
        _mm256_store_si256((__m256i*)&pstate[j], _mm256_load_si256((__m256i*)&array[j + size - SFMT_N]));
    }
    for (; i < size; i+=2, j+=2) {
        mm_recursion2(
            (__m256i*)&array[i],
            _mm256_load_si256((__m256i*)&array[i - SFMT_N] ),
            _mm256_load_si256((__m256i*)&array[i + SFMT_POS1 - SFMT_N]),
            r
        );
        r = _mm256_load_si256((__m256i*)&array[i]);
        _mm256_store_si256((__m256i*)&pstate[j], r);
    }
}
#endif
