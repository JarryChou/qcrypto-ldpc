/** @file ec_packet_def.h: 
 * @brief header file containing the definitions of the message headers for the error correction procedure.

 * Part of the quantum key distribution software. This 
                 file contains message header definitions for the error
		 correction procedure. See main file (ecd2.c) for usag
		 details. Version as of 20071201
	    
 *  Copyright (C) 2020 Matthew Lee, National University
 *                          of Singapore <crazoter@gmail.com>
 * 
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

   A more detailled description of the headers can be
   found in the file errc_formats.

   first version chk 22.10.05
   status 22.3.06 12:00chk
   fixed message 8 for bell value transmission
*/

#ifndef EC_PACKET_DEF
#define EC_PACKET_DEF

enum EcSubtypes {
    SUBTYPE_QBER_EST_BITS = 0,
    SUBTYPE_QBER_EST_REQ_MORE_BITS = 2,
    SUBTYPE_QBER_EST_BITS_ACK = 3,
    SUBTYPE_CASCADE_PARITY_LIST = 4,
    SUBTYPE_CASCADE_BIN_SEARCH_MSG = 5,
    SUBTYPE_CASCADE_BICONF_INIT_REQ = 6,
    SUBTYPE_CASCADE_BICONF_PARITY_RESP = 7,
    SUBTYPE_START_PRIV_AMP = 8
};

#define EC_PACKET_TAG 6

/**
 * @brief this one is just a mockup for reading a message to determine its length.
 * 
   It represents only the common entry at the begnning of all message files
 * 
 */
typedef struct ERRC_PROTO {
    unsigned int tag; /**< always 6 */
    unsigned int totalLengthInBytes; /**< including header */
    unsigned int subtype;           /**< 0 for PRNG based subset */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< defines implicitly the block length */
} EcPktHdr_Base;

/**
 * @brief packet for PRNG based subset for bit subset transmission in err estim
 * 
 */
typedef struct  ERRC_ERRDET_0 {
    unsigned int tag;               /**< always 6 */
    unsigned int totalLengthInBytes;        
    unsigned int subtype;           /**< 0 for PRNG based subset */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< defines implicitly the block length */
    unsigned int seed;              /**< seed for PRNG */
    unsigned int numberofbits;      /**< bits to follow */
    unsigned int fixedErrorRate;         /**< initial error est skip? */
    float bellValue;                /**< may contain a value for Bell violat */
} EcPktHdr_QberEstBits;

/**
 * @brief packet for explicitely indexed bit fields with a good RNG for err est
 * 
 */
typedef  struct  ERRC_ERRDET_1 {
    unsigned int tag;               /**< always 6 */
    unsigned int totalLengthInBytes;
    unsigned int subtype;           /**< 1 for good random number based subset */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< defines implicitly the block length */
    unsigned int bitlength;         /**< compression bit width for index diff */
    unsigned int numberofbits;      /**< number of bits to follow */
    unsigned int errormode;         /**< initial error est skip? */
} errc_ed_1__;

/**
 * @brief packet for requesting more sample bits
 * 
 */
typedef struct  ERRC_ERRDET_2 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< length of the packet; fixed to 24 */
    unsigned int subtype;           /**< 2 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int requestedbits;     /**< number of additionally required bits */
} EcPktHdr_QberEstReqMoreBits;


/**
 * @brief Acknowledgment packet for communicating the error rate
 * 
 */
typedef struct  ERRC_ERRDET_3 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet incl header */
    unsigned int subtype;           /**< 3 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int tested_bits;       /**< number of bits tested */
    unsigned int number_of_errors;  /**< number of mismatches found */
} EcPktHdr_QberEstBitsAck;

/**
 * @brief first parity check bit info
 * 
 */
typedef struct  ERRC_ERRDET_4 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet incl header */
    unsigned int subtype;           /**< 4 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int k0;                /**< size of partition 0 */
    unsigned int k1;                /**< size of partition 1 */
    unsigned int totalbits;         /**< number of bits considered */
    unsigned int seed;              /**< seed for PRNG doing permutation */
} EcPktHdr_CascadeParityList;	

/**
 * @brief Binary search message packet
 * 
 */
typedef struct  ERRC_ERRDET_5 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet incl header */
    unsigned int subtype;           /**< 5 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int number_entries;    /**< number of blocks with parity mismatch */
    unsigned int index_present;     /**< format /presence of index data  */
    unsigned int runlevel;          /**<  pass and bisectioning depth */
} EcPktHdr_CascadeBinSearchMsg;	

#define RUNLEVEL_FIRSTPASS 0 /**< for message 5 */
#define RUNLEVEL_SECONDPASS 0x80000000 /**< for message 5 */
#define RUNLEVEL_LEVELMASK 0x80000000 /**< for message 5 */
#define RUNLEVEL_ROUNDMASK 0x3fffffff /**< for message 5 */
#define RUNLEVEL_BICONF 0x40000000 /**< for message 5: this indicates a biconf search */


/**
 * @brief BICONF initiating message
 * 
 */
typedef struct  ERRC_ERRDET_6 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet (28) */
    unsigned int subtype;           /**< 6 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int seed;
    unsigned int number_of_bits;    /**< the number bits requested for biconf */
} EcPktHdr_CascadeBiconfInitReq;	

/**
 * @brief BICONF response message
 * 
 */
typedef struct  ERRC_ERRDET_7 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet (24) */
    unsigned int subtype;           /**< 7 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int parity;            /**< result of the parity test (0 or 1) */
} EcPktHdr_CascadeBiconfParityResp;	

/**
 * @brief privacy amplification start message
 * 
 */
typedef struct ERRC_ERRDET_8 {
    unsigned int tag;               /**< 6 for an error correction packet */
    unsigned int totalLengthInBytes;        /**< the length of the packet (32) */
    unsigned int subtype;           /**< 8 for request of bit number packet */
    unsigned int epoch;             /**< defines epoch of first packet */
    unsigned int number_of_epochs;  /**< length of the block */
    unsigned int seed;              /**< new seed for PRNG */
    unsigned int lostbits ;         /**< number of lost bits in this run */
    unsigned int correctedbits;     /**< number of bits corrected in */
} EcPktHdr_StartPrivAmp;	

/**
 * @brief structure which holds packets to send
 * 
 */
typedef struct packet_to_send {
  int length;                  /**< in bytes */
  char *packet;                /**< pointer to content */
  struct packet_to_send *next; /**< next one to send */
} PacketToSendNode;

/**
 * @brief structure to hold received messages
 * 
 */
typedef struct packet_received {
  int length;                   /**< in bytes */
  char *packet;                 /**< pointer to content */
  struct packet_received *next; /**< next in chain */
} PacketReceivedNode;

#endif