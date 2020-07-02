/** @file processblock.h: 
 * @brief Header file containing the definitions of the processBlock & block related data
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

  --
  Old layout of struct
  Rough check of where the variables are used
  Legend:
  C: Used in Cascade Biconf
  Q: Used in QBER est
  P: Used in Privacy amp
  !: Clearly identified as an important variable that should not be abstracted out
  In brackets: Coupled with, but not directly used by 

  comms, processBlock_mgmt, helpers & debug not logged

  unsigned int startEpoch;          QP   !     
  unsigned int numberOfEpochs;      QP   !     
  unsigned int *rawMemPtr;          M    !     
  unsigned int *mainBufPtr;         CQP  !        
  unsigned int *testedBitsMarker;   CQ      
  unsigned int *permuteBufPtr;      C   Cascade only but used for testedBitsMarker, kept in metadata
  unsigned short int *permuteIndex; C     
  unsigned short int *reverseIndex; C     
  PROCESSOR_ROLE processorRole;     CQ      
  int initialBits;                  QP      
  int leakageBits;                  CQP     
  int processingState;              CQ   !
  int initialErrRate;               Q    QBER only but part of the metadata for the block 
  Boolean skipQberEstim;            Q     
  int estimatedError;               Q     
  int estimatedSampleSize;          Q     
  float localError;                 Q     
  unsigned int rngState;            CPQ      
  int k0, k1;                       C     
  int workbits;                     CP     
  int partitions0, partitions1;     C     
  unsigned int *lp0, *lp1;          C     
  unsigned int *rp0, *rp1;          C     
  unsigned int *pd0, *pd1;          C     
  int diffBlockCount;               C     
  int diffBlockCountMax;            C     
  unsigned int *diffidx;            C     
  unsigned int *diffidxe;           C     
  int binarySearchDepth;            C     
  int biconfRound;                  C     
  int biconfLength;                 C     
  int correctedErrors;              CP      
  int finalKeyBits;                 P     
  float bellValue;                  QP   


*/
#ifndef EC_PROCESSBLOCK_H
#define EC_PROCESSBLOCK_H

#include "defaultdefinitions.h"
#include "algorithms/data_manager.h"
#include "algorithms/packet_manager.h"

// Forward define structs from algorithms/data_manager and packet_manager
//typedef struct ALGORITHM_PKT_MNGR ALGORITHM_PKT_MNGR;
//typedef struct ALGORITHM_DATA_MNGR ALGORITHM_DATA_MNGR;

/**
 * @brief Processor roles to define what the current ecd2 does w.r.t how it processes the processBlock.
 * 
 * These terms are used for clarity without the need of domain knowledge e.g. client/server.
 */
enum EC_PROCESSOR_ROLE {
  PROC_ROLE_QBER_EST_INITIATOR = 0,     ///< Previously identified as Alice, the party that initiates QBER est
  PROC_ROLE_QBER_EST_FOLLOWER = 1,      ///< Previously identified as Bob
  PROC_ROLE_EC_INITIATOR = 2,           ///< After QBER estimation, the party that initiates the error correction algorithm
  PROC_ROLE_EC_FOLLOWER = 3             ///< After QBER estimation, the party on the other end
};
typedef enum EC_PROCESSOR_ROLE PROCESSOR_ROLE;

/** @ProcessBlock
 * @brief Definition of a structure containing all informations about a block */
typedef struct ProcessBlk {
  // Block meta information
  unsigned int startEpoch;              /**< initial epoch of block */
  unsigned int numberOfEpochs;          /**< number of consecutive epochs being processed */
  unsigned int *rawMemPtr;              /**< buffer root for freeing block afterwards */
  unsigned int *mainBufPtr;             /**< points to main buffer for key */
  unsigned int *permuteBufPtr;          /**< keeps permuted bits */
  unsigned int *testedBitsMarker;       /**< marks tested bits */
  unsigned int rngState;                /**< keeps the state of the PRNG for this processBlock */
  PROCESSOR_ROLE processorRole;         /**< defines which role to take on a block: 0: Alice, 1: Bob */
  int initialBits;                      /**< bits to start with */
  int initialErrRate;                   /**< in multiples of 2^-16 */
  int leakageBits;                      /**< information which has gone public */
  int processingState;                  /**< determines processing status current block. See defines below for interpretation */
  // Post Error correction variables
  int workbits;                         /**< bits to work with for BICONF/parity check */
  int correctedErrors;                  /**< number of corrected bits */
  // Privacy amplification variables
  int finalKeyBits;                     /**< how much is left */
  float bellValue;                      /**< for Ekert-type protocols */
  struct ALGORITHM_PKT_MNGR *algorithmPktMngr;   ///< Metadata for the current error correction algorithm, defined in algorithms.h
  struct ALGORITHM_DATA_MNGR *algorithmDataMngr; ///< Enum to identify type of struct that algorithmDataPtr points to
  char* algorithmDataPtr;                  ///< Ptr to struct containing additional / working data for the current error correction algorithm, defined in algorithms.h
  // Temporarily allowed to reduce impact of porting
  unsigned short int *permuteIndex;     /**< keeps permutation */
  unsigned short int *reverseIndex;     /**< reverse permutation */
  float localError;                     // Used by qber and cascade, local error after qber estimation
} ProcessBlock;

/** @struct blockpointer
 * @brief Structure to hold list of blocks. 
 * 
 * This helps dispatching packets? 
 */
typedef struct blockpointer {
  unsigned int epoch;
  ProcessBlock *content;                /**< the gory details */
  struct blockpointer *next;            /**< next in chain; if NULL then end */
  struct blockpointer *previous;        /**< previous block */
} ProcessBlockDequeNode;

#endif