/**
 * @file helpers.h     
 * @brief helper functions or helper enums for ecd2
 * 
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below.
 * 
 *  Copyright (C) 2020 Matthew Lee, National University
 *                          of Singapore <crazoter@gmail.com>
 * 
 *  Copyright (C) 2005-2007 Christian Kurtsiefer, National University
 *                          of Singapore <christian.kurtsiefer@gmail.com>
 * 
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Public License as published
 *  by the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
 * 
 *  This source code is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  Please refer to the GNU Public License for more details.
 * 
 *  You should have received a copy of the GNU Public License along with
 *  this source code; if not, write to:
 *  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 * 
 * --
 * 
 * This is a refactored version of Kurtsiefer's ecd2.c to modularize parts of the
 * code. This section modularizes the parts of the code that contains helper functions,
 * or anything related to helper code (e.g. enums for readability)
 * 
 */

#ifndef ECD2_HELPER
#define ECD2_HELPER

/// \cond for doxygen annotation
#include <math.h>
#include <string.h>
#include <stdint.h>
/// \endcond

#include "../definitions/algorithms/algorithms.h"
#include "../definitions/processblock.h"

#include "rnd.h"

// DEFINED FUNCTIONS
/* ------------------------------------------------------------------------- */
/// @name FUNCTIONS RECOMMENDED TO THE COMPILER FOR INLINE
/// @{
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) > (B) ? (B) : (A))
/// @}

/// @name GENERAL HELPER FUNCTIONS
/// @{
#define log2Ceil(X) ((X) ? (32 - __builtin_clz(X)) : 0)                                 ///< See https://stackoverflow.com/questions/671815/what-is-the-fastest-most-efficient-way-to-find-the-highest-set-bit-msb-in-an-i
#define countSetBits __builtin_popcount                                                 ///< See https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
#define wordIndex(BIT_INDEX) ((BIT_INDEX) / 32)                                         ///< Macro to get index of 32-bit word given the index of the bit
#define wordCount(LAST_BIT_INDEX) (((LAST_BIT_INDEX) + 31) / 32)                        ///< Macro to get number of 32-bit words that used by buffer
#define modulo32(X) ((X) & 31)                                                          ///< Can also be defined as "get least significant 5 bits", but this definition is more meaningful
#define uint32AllZeroExceptAtN(N_FROM_RIGHT) (1 << (31 - (modulo32(N_FROM_RIGHT))))     ///< previously uint32AllZeroExceptAtN
#define uint32AllOnesExceptFirstN(N_BITS) (0xffffffff >> (N_BITS))                      ///< previously firstmask __inline__ unsigned int firstmask(int i) { return 0xffffffff >> i; }
#define uint32AllOnesExceptLastN(N_BITS) (0xffffffff << (31 - (N_BITS)))                ///< previously lastmask __inline__ unsigned int lastmask(int i) { return 0xffffffff << (31 - i); }
void atohex(char *target, unsigned int v);
/// @}


/// @name HELPER PERMUTATION FUNCTIONS USED FOR QBER ESTIMATION & CASCADE
/// @{
void helper_cleanupRevealedBits(ProcessBlock *pb);
void helper_prepPermutationCore(ProcessBlock *pb);
void helper_prepPermutationWrapper(ProcessBlock *pb);
void helper_prepParityList(unsigned int *d, unsigned int *t, int k, int w);
int helper_singleLineParity(unsigned int *bitBuffer, int startBitIndex, int endBitIndex);
// int helper_singleLineParityMasked(unsigned int *d, unsigned int *m, int start, int end); ///< Unused
// void flipBit(unsigned int *d, int bitindex);
#define flipBit(BITBUF_PTR, BIT_INDEX) ((BITBUF_PTR)[wordIndex(BIT_INDEX)] ^= uint32AllZeroExceptAtN(BIT_INDEX)) ///< Helper to flip bit
/// @}


#endif /* ECD2_HELPER */