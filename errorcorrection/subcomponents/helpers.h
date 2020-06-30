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

#include "../definitions/processblock.h"

#include "rnd.h"

// HELPER ENUMS
/**
 * @brief File handle enums
 * 
 */
enum HandleId {
  handleId_commandPipe = 0,
  handleId_sendPipe = 1,
  handleId_receivePipe = 2,
  handleId_rawKeyDir = 3,
  handleId_finalKeyDir = 4,
  handleId_notifyPipe = 5,
  handleId_queryPipe = 6,
  handleId_queryRespPipe = 7,
  handleId_numberOfHandles = 8
};


/// @name FUNCTIONS RECOMMENDED TO THE COMPILER FOR INLINE
/// @{
// (What is inline? See https://stackoverflow.com/questions/2082551/what-does-inline-mean)
/**
 * @brief Helper for mask for a given index i on the longint array
 * 
 * If they were intended to be inlined might as well just set it as a macro
 * 
 * @param i 
 * @return bt_mask 
 */
#define bt_mask(I) (1 << (31 - ((I) & 31)))    // __inline__ unsigned int bt_mask(int i) { return 1 << (31 - (modulo32(i))); }

/**
 * @brief Helper function for parity isolation
 * 
 * If they were intended to be inlined might as well just set it as a macro
 * 
 * @param i 
 * @return firstmask 
 */
#define firstmask(I) (0xffffffff >> (I))       // __inline__ unsigned int firstmask(int i) { return 0xffffffff >> i; }
#define lastmask(I) (0xffffffff << (31 - (I))) // __inline__ unsigned int lastmask(int i) { return 0xffffffff << (31 - i); }
/// @}

// DEFINED FUNCTIONS
/* ------------------------------------------------------------------------- */
/// @name FUNCTIONS RECOMMENDED TO THE COMPILER FOR INLINE
/// @{
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) > (B) ? (B) : (A))
/// @}

/// @name GENERAL HELPER FUNCTIONS
/// @{
#define log2Ceil(X) ((X) ? (32 - __builtin_clz(X)) : 0) ///< See https://stackoverflow.com/questions/671815/what-is-the-fastest-most-efficient-way-to-find-the-highest-set-bit-msb-in-an-i
#define countSetBits __builtin_popcount                 ///< See https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
#define wordIndex(BIT_INDEX) ((BIT_INDEX) / 32)         ///< Macro to get index of 32-bit word given the index of the bit
#define wordCount(LAST_BIT_INDEX) (((LAST_BIT_INDEX) + 31) / 32) ///< Macro to get number of 32-bit words that used by buffer
#define modulo32(X) ((X) & 31)                          ///< Can also be defined as "get least significant 5 bits", but this definition is more meaningful
void atohex(char *target, unsigned int v);
/// @}


/// @name HELPER PERMUTATION FUNCTIONS USED FOR QBER ESTIMATION & CASCADE
/// @{
void cleanup_revealed_bits(ProcessBlock *pb);
void prepare_permut_core(ProcessBlock *pb);
void prepare_permutation(ProcessBlock *pb);
void prepare_paritylist_basic(unsigned int *d, unsigned int *t, int k, int w);
int singleLineParity(unsigned int *d, int start, int end);
/// @}


#endif /* ECD2_HELPER */