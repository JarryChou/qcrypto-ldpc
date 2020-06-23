/**
 * helpers.h     
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below.
 * 
 *  Copyright (C) 2020 Matthew Lee, National University
 *                          of Singapore <crazoter@gmail.com>
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

#include "rnd.h"
#include "keyblock.h"

// HELPER ENUMS

// File handle enums
enum HandleId {
  handleId_commandPipe = 0,
  handleId_sendPipe = 1,
  handleId_receivePipe = 2,
  handleId_rawKeyDir = 3,
  handleId_finalKeyDir = 4,
  handleId_notifyPipe = 5,
  handleId_queryPipe = 6,
  handleId_queryRespPipe = 7
};


// FUNCTIONS RECOMMENDED TO THE COMPILER FOR INLINE
// (What is inline? See https://stackoverflow.com/questions/2082551/what-does-inline-mean)
/* ------------------------------------------------------------------------- */
/* helper for mask for a given index i on the longint array */
__inline__ unsigned int bt_mask(int i) { return 1 << (31 - (i & 31)); }
/* helper function for parity isolation */
__inline__ unsigned int firstmask(int i) { return 0xffffffff >> i; }
__inline__ unsigned int lastmask(int i) { return 0xffffffff << (31 - i); }

// DEFINED FUNCTIONS
/* ------------------------------------------------------------------------- */
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) > (B) ? (B) : (A))

// HELPER FUNCTION DECLARATIONS
/* ------------------------------------------------------------------------- */
// GENERAL HELPER FUNCTIONS
/* helper to obtain the smallest power of two to carry a number a */
int get_order(int a);

/* get the number of bits necessary to carry a number x ; 
  result is e.g. 3 for parameter 8, 5 for parameter 17 etc. */
int get_order_2(int x); 

/* helper: count the number of set bits in a longint */
int count_set_bits(unsigned int a);

/* helper for name. adds a slash, hex file name and a terminal 0 */
void atohex(char *target, unsigned int v);

/* helper: eve's error knowledge */
float phi(float z);
float binentrop(float q);

// HELPER PERMUTATION FUNCTIONS BETWEEN CASCADE & QBER ESTIMATION
/* helper function to compress key down in a sinlge sequence to eliminate the
   revealeld bits. updates workbits accordingly, and reduces number of
   revealed bits in the leakage_bits_counter  */
void cleanup_revealed_bits(struct keyblock *kb);

/* helper function to do generate the permutation array in the kb structure.
   does also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
   Parameter: pointer to kb structure */
void prepare_permutation(struct keyblock *kb);

/* permutation core function; is used both for biconf and initial permutation */
void prepare_permut_core(struct keyblock *kb);

/* helper function to preare a parity list of a given pass in a block.
   Parameters are a pointer to the sourcebuffer, pointer to the target buffer,
   and an integer arg for the blocksize to use, and the number of workbits */
void prepare_paritylist_basic(unsigned int *d, unsigned int *t, int k, int w);

/* helper function to prepare parity lists from original and unpermutated key.
   arguments are a pointer to the thread structure, a pointer to the target
   parity buffer 0 and another pointer to paritybuffer 1. No return value,
   as no errors are tested here. */
void prepare_paritylist1(struct keyblock *kb, unsigned int *d0, unsigned int *d1);


#endif /* ECD2_HELPER */