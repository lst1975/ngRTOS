/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __NGRTOS_ENDIAN_H__
#define __NGRTOS_ENDIAN_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include "ng_arch.h"

#define NG__UINT16_C(v) ((uint16_t)(v))
#define NG__UINT32_C(v) ((uint32_t)(v))
#define NG__UINT64_C(v) ((uint64_t)(v))

#define RTE_STATIC_BSWAP16(v) \
	((((uint16_t)(v) & NG__UINT16_C(0x00ffUL)) << 8) | \
	 (((uint16_t)(v) & NG__UINT16_C(0xff00UL)) >> 8))

#define RTE_STATIC_BSWAP32(v) \
	((((uint32_t)(v) & NG__UINT32_C(0x000000ffUL)) << 24) | \
	 (((uint32_t)(v) & NG__UINT32_C(0x0000ff00UL)) <<  8) | \
	 (((uint32_t)(v) & NG__UINT32_C(0x00ff0000UL)) >>  8) | \
	 (((uint32_t)(v) & NG__UINT32_C(0xff000000UL)) >> 24))

#define RTE_STATIC_BSWAP64(v) \
	((((uint64_t)(v) & NG__UINT64_C(0x00000000000000ffUL)) << 56) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x000000000000ff00UL)) << 40) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x0000000000ff0000UL)) << 24) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x00000000ff000000UL)) <<  8) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x000000ff00000000UL)) >>  8) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x0000ff0000000000UL)) >> 24) | \
	 (((uint64_t)(v) & NG__UINT64_C(0x00ff000000000000UL)) >> 40) | \
	 (((uint64_t)(v) & NG__UINT64_C(0xff00000000000000UL)) >> 56))

/* ARM architecture is bi-endian (both big and little). */
#if __NG_BYTE_ORDER == __NG_LITTLE_ENDIAN

#define rte_cpu_to_le_16(x) (x)
#define rte_cpu_to_le_32(x) (x)
#define rte_cpu_to_le_64(x) (x)

#define rte_cpu_to_be_16(x) rte_bswap16(x)
#define rte_cpu_to_be_32(x) rte_bswap32(x)
#define rte_cpu_to_be_64(x) rte_bswap64(x)

#define rte_le_to_cpu_16(x) (x)
#define rte_le_to_cpu_32(x) (x)
#define rte_le_to_cpu_64(x) (x)

#define rte_be_to_cpu_16(x) rte_bswap16(x)
#define rte_be_to_cpu_32(x) rte_bswap32(x)
#define rte_be_to_cpu_64(x) rte_bswap64(x)

#define RTE_CPU_TO_LE_16(X) (X)
#define RTE_CPU_TO_LE_32(X) (X)
#define RTE_CPU_TO_LE_64(X) (X)

#define RTE_CPU_TO_BE_16(X) RTE_STATIC_BSWAP16(X)
#define RTE_CPU_TO_BE_32(X) RTE_STATIC_BSWAP32(X)
#define RTE_CPU_TO_BE_64(X) RTE_STATIC_BSWAP64(X)

#define RTE_LE_TO_CPU_16(X) (X)
#define RTE_LE_TO_CPU_32(X) (X)
#define RTE_LE_TO_CPU_64(X) (X)

#define RTE_BE_TO_CPU_16(X) RTE_STATIC_BSWAP16(X)
#define RTE_BE_TO_CPU_32(X) RTE_STATIC_BSWAP32(X)
#define RTE_BE_TO_CPU_64(X) RTE_STATIC_BSWAP64(X)

#else /* __NG_BIG_ENDIAN */

#define rte_cpu_to_le_16(x) rte_bswap16(x)
#define rte_cpu_to_le_32(x) rte_bswap32(x)
#define rte_cpu_to_le_64(x) rte_bswap64(x)

#define rte_cpu_to_be_16(x) (x)
#define rte_cpu_to_be_32(x) (x)
#define rte_cpu_to_be_64(x) (x)

#define rte_le_to_cpu_16(x) rte_bswap16(x)
#define rte_le_to_cpu_32(x) rte_bswap32(x)
#define rte_le_to_cpu_64(x) rte_bswap64(x)

#define rte_be_to_cpu_16(x) (x)
#define rte_be_to_cpu_32(x) (x)
#define rte_be_to_cpu_64(x) (x)

#define RTE_CPU_TO_LE_16(X) RTE_STATIC_BSWAP16(X)
#define RTE_CPU_TO_LE_32(X) RTE_STATIC_BSWAP32(X)
#define RTE_CPU_TO_LE_64(X) RTE_STATIC_BSWAP64(X)

#define RTE_CPU_TO_BE_16(X) (X)
#define RTE_CPU_TO_BE_32(X) (X)
#define RTE_CPU_TO_BE_64(X) (X)

#define RTE_LE_TO_CPU_16(X) RTE_STATIC_BSWAP16(X)
#define RTE_LE_TO_CPU_32(X) RTE_STATIC_BSWAP32(X)
#define RTE_LE_TO_CPU_64(X) RTE_STATIC_BSWAP64(X)

#define RTE_BE_TO_CPU_16(X) (X)
#define RTE_BE_TO_CPU_32(X) (X)
#define RTE_BE_TO_CPU_64(X) (X)
#endif

#define rte_htons  rte_cpu_to_be_16
#define rte_htonl  rte_cpu_to_be_32
#define rte_htonll rte_cpu_to_be_64

#define rte_ntohs  rte_be_to_cpu_16
#define rte_ntohl  rte_be_to_cpu_32
#define rte_ntohll rte_be_to_cpu_64

#define RTE_HTONS  RTE_CPU_TO_BE_16
#define RTE_HTONL  RTE_CPU_TO_BE_32
#define RTE_HTONLL RTE_CPU_TO_BE_64

#define RTE_NTOHS  RTE_BE_TO_CPU_16
#define RTE_NTOHL  RTE_BE_TO_CPU_32
#define RTE_NTOHLL RTE_BE_TO_CPU_64

#ifdef __cplusplus
    }
#endif

#endif
