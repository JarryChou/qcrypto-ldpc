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
/// \endcond

#include "../definitions/keyblock.h"

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
 * @param i 
 * @return __inline__ bt_mask 
 */
__inline__ unsigned int bt_mask(int i) { return 1 << (31 - (i & 31)); }

/**
 * @brief Helper function for parity isolation
 * 
 * @param i 
 * @return __inline__ firstmask 
 */
__inline__ unsigned int firstmask(int i) { return 0xffffffff >> i; }
__inline__ unsigned int lastmask(int i) { return 0xffffffff << (31 - i); }
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
int get_order(int a);
int get_order_2(int x); 
int count_set_bits(unsigned int a);
void atohex(char *target, unsigned int v);
/// @}


/// @name HELPER PERMUTATION FUNCTIONS USED FOR QBER ESTIMATION & CASCADE
/// @{
void cleanup_revealed_bits(struct keyblock *kb);
void prepare_permut_core(struct keyblock *kb);
void prepare_permutation(struct keyblock *kb);
void prepare_paritylist_basic(unsigned int *d, unsigned int *t, int k, int w);
/// @}


#endif /* ECD2_HELPER */