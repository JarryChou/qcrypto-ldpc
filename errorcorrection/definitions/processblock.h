/** @file processblock.h: 
 * @brief Header file containing the definitions of the processblock & block related data
   for the error correction procedure.
   
 Part of the quantum key distribution software. This 
                 file contains message header definitions for the error
		 correction procedure. See main file (ecd2.c) for usag
		 details. Version as of 20071201
	    
 Copyright (C) 2005-2007 Christian Kurtsiefer, National University
                         of Singapore <christian.kurtsiefer@gmail.com>

 This source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Public License as published
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 This source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 Please refer to the GNU Public License for more details.

 You should have received a copy of the GNU Public License along with
 this source code; if not, write to:
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#ifndef EC_PROCESSBLOCK_H
#define EC_PROCESSBLOCK_H

/** @struct processblock
 * @brief Definition of a structure containing all informations about a block */
typedef struct processblock {
  unsigned int startepoch;          /**< initial epoch of block */
  unsigned int numberofepochs;      /**< number of consecutive epochs being processed */
  unsigned int *rawmem;             /**< buffer root for freeing block afterwards */
  unsigned int *mainbuf;            /**< points to main buffer for key */
  unsigned int *permutebuf;         /**< keeps permuted bits */
  unsigned int *testmarker;         /**< marks tested bits */
  unsigned short int *permuteindex; /**< keeps permutation */
  unsigned short int *reverseindex; /**< reverse permutation */
  int role;                         /**< defines which role to take on a block: 0: Alice, 1: Bob */
  int initialbits;                  /**< bits to start with */
  int leakagebits;                  /**< information which has gone public */
  int processingstate;              /**< determines processing status  current block. See defines below for interpretation */
  int initialerror;                 /**< in multiples of 2^-16 */
  int errormode;                    /**< determines if error estimation has to be done */
  int estimatederror;               /**< number of estimated error bits */
  int estimatedsamplesize;          /**< sample size for error  estimation  */
  int finalerrors;                  /**< number of discovered errors */
  int RNG_usage;                    /**< defines mode of randomness. 0: PRNG, 1: good stuff (which has not been implemented) */
  unsigned int RNG_state;           /**< keeps the state of the PRNG for this thread */
  int k0, k1;                       /**< binary block search lengths */
  int workbits;                     /**< bits to work with for BICONF/parity check */
  int partitions0, partitions1;     /**< number of partitions of k0,k1 length */
  unsigned int *lp0, *lp1;          /**< pointer to local parity info 0 / 1 */
  unsigned int *rp0, *rp1;          /**< pointer to remote parity info 0 / 1 */
  unsigned int *pd0, *pd1;          /**< pointer to parity difference fileld */
  int diffnumber;                   /**< number of different blocks in current round */
  int diffnumber_max;               /**< number of malloced entries for diff indices */
  unsigned int *diffidx;            /**< pointer to a list of parity mismatch blocks */
  unsigned int *diffidxe;           /**< end of interval */
  int binsearch_depth;              /**< encodes state of the scan. Starts with 0, and contains the pass (0/1) in the MSB */
  int biconf_round;                 /**< contains the biconf round number, starting with 0 */
  int biconflength;                 /**< current length of a biconf check range */
  int correctederrors;              /**< number of corrected bits */
  int finalkeybits;                 /**< how much is left */
  float BellValue;                  /**< for Ekert-type protocols */
} ProcessBlock;

/** @struct blockpointer
 * @brief Structure to hold list of blocks. 
 * 
 * This helps dispatching packets? 
 */
typedef struct blockpointer {
  unsigned int epoch;
  ProcessBlock *content;      /**< the gory details */
  struct blockpointer *next;     /**< next in chain; if NULL then end */
  struct blockpointer *previous; /**< previous block */
} ProcessBlockDequeNode;

#endif