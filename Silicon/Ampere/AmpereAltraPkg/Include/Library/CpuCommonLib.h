/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_COMMON_H_
#define _CPU_COMMON_H_

#include <Library/UefiLib.h>

/* Ctypen, bits[3(n - 1) + 2 : 3(n - 1)], for n = 1 to 7 */
#define CLIDR_CTYPE_SHIFT(Level)    (3 * (Level - 1))
#define CLIDR_CTYPE_MASK(Level)     (7 << CLIDR_CTYPE_SHIFT(Level))
#define CLIDR_CTYPE(Clidr, Level) \
  (((Clidr) & CLIDR_CTYPE_MASK(Level)) >> CLIDR_CTYPE_SHIFT(Level))

#define CCSIDR_NUMSETS_SHIFT        13
#define CCSIDR_NUMSETS_MASK         0xFFFE000
#define CCSIDR_NUMSETS(Ccsidr) \
  (((Ccsidr) & CCSIDR_NUMSETS_MASK) >> CCSIDR_NUMSETS_SHIFT)
#define CCSIDR_ASSOCIATIVITY_SHIFT  3
#define CCSIDR_ASSOCIATIVITY_MASK   0x1FF8
#define CCSIDR_ASSOCIATIVITY(Ccsidr) \
  (((Ccsidr) & CCSIDR_ASSOCIATIVITY_MASK) >> CCSIDR_ASSOCIATIVITY_SHIFT)
#define CCSIDR_LINE_SIZE_SHIFT      0
#define CCSIDR_LINE_SIZE_MASK       0x7
#define CCSIDR_LINE_SIZE(Ccsidr) \
  (((Ccsidr) & CCSIDR_LINE_SIZE_MASK) >> CCSIDR_LINE_SIZE_SHIFT)

#define SOC_EFUSE_SHADOWn(s,x)        (SMPRO_EFUSE_SHADOW0 + (s) * SOCKET_BASE_OFFSET + (x) * 4)

#define SUBNUMA_MODE_MONOLITHIC        0
#define SUBNUMA_MODE_HEMISPHERE        1
#define SUBNUMA_MODE_QUADRANT          2

UINT8 CPUGetSubNumaMode (VOID);

/*
 * Return the number of SubNUMA region
 */
UINT8 CPUGetNumOfSubNuma(VOID);

/*
 * Return the SubNUMA node of a CPM
 */
UINT8 CPUGetSubNumNode(UINT8 Socket, UINT32 Cpm);

/*
 * Return the value of CLIDR register
 */
UINT64 Aarch64ReadCLIDRReg ();

/*
 * Return the value of CCSID register
 */
UINT64 Aarch64ReadCCSIDRReg (UINT64 Level);

/*
 * Return the associativity of cache
 */
UINT32 CpuGetAssociativity (UINTN Level);

/*
 * Return the cache size.
 */
UINT32 CpuGetCacheSize (UINTN Level);

#endif /* _CPU_COMMON_H_ */