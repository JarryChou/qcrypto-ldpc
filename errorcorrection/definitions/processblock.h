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

#include "defaultdefinitions.h"

/**
 * @brief Processor roles to define what the current ecd2 does w.r.t how it processes the processblock.
 * 
 * These terms are used for clarity without the need of domain knowledge e.g. client/server.
 */
enum EC_PROCESSOR_ROLE {
  INITIATOR = 0,  ///< Also known as Alice
  FOLLOWER = 1    ///< Also known as Bob
};
typedef enum EC_PROCESSOR_ROLE PROCESSOR_ROLE;

/** @struct processblock
 * @brief Definition of a structure containing all informations about a block */
typedef struct processblock {
  unsigned int startEpoch;          /**< initial epoch of block */
  unsigned int numberOfEpochs;      /**< number of consecutive epochs being processed */
  unsigned int *rawMemPtr;             /**< buffer root for freeing block afterwards */
  unsigned int *mainBufPtr;            /**< points to main buffer for key */
  unsigned int *permuteBufPtr;         /**< keeps permuted bits */
  unsigned int *testedBitsMarker;         /**< marks tested bits */
  unsigned short int *permuteIndex; /**< keeps permutation */
  unsigned short int *reverseIndex; /**< reverse permutation */
  PROCESSOR_ROLE processorRole;      /**< defines which role to take on a block: 0: Alice, 1: Bob */
  int initialBits;                  /**< bits to start with */
  int leakageBits;                  /**< information which has gone public */
  int processingState;              /**< determines processing status  current block. See defines below for interpretation */
  int initialErrRate;                 /**< in multiples of 2^-16 */
  Boolean skipQberEstim;                    /**< determines if error estimation has to be done */
  int estimatedError;               /**< number of estimated error bits */
  int estimatedSampleSize;          /**< sample size for error  estimation  */
  // int finalerrors;                  /**< number of discovered errors */
  // int RNG_usage;                    /**< defines mode of randomness. 0: PRNG, 1: good stuff (which has not been implemented) */
  unsigned int rngState;           /**< keeps the state of the PRNG for this processblock */
  int k0, k1;                       /**< binary block search lengths */
  int workbits;                     /**< bits to work with for BICONF/parity check */
  int partitions0, partitions1;     /**< number of partitions of k0,k1 length */
  unsigned int *lp0, *lp1;          /**< pointer to local parity info 0 / 1 */
  unsigned int *rp0, *rp1;          /**< pointer to remote parity info 0 / 1 */
  unsigned int *pd0, *pd1;          /**< pointer to parity difference fileld */
  int diffBlockCount;                   /**< number of different blocks in current round */
  int diffBlockCountMax;               /**< number of malloced entries for diff indices */
  unsigned int *diffidx;            /**< pointer to a list of parity mismatch blocks */
  unsigned int *diffidxe;           /**< end of interval */
  int binarySearchDepth;              /**< encodes state of the scan. Starts with 0, and contains the pass (0/1) in the MSB */
  int biconfRound;                 /**< contains the biconf round number, starting with 0 */
  int biconflength;                 /**< current length of a biconf check range */
  int correctedErrors;              /**< number of corrected bits */
  int finalKeyBits;                 /**< how much is left */
  float bellValue;                  /**< for Ekert-type protocols */
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