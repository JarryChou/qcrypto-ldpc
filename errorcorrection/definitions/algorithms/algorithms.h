/**
 * @file algorithms.h  
 * @brief Definition file for error correction algorithms
 * 
 * All available algorithms have been put into this one file so that you can more easily see and 
 * understand what is going on
 * 
 */

#ifndef EC_ERRC_ALGORITHMS
#define EC_ERRC_ALGORITHMS 1

#include "../../subcomponents/cascade_biconf.h"
#include "../../subcomponents/priv_amp.h"
#include "../../subcomponents/qber_estim.h"

/// @name ALGORITHM DECISION BY QBER_FOLLOWER
/// @{

/**   
 * @brief What PROC_ROLE_QBER_EST_FOLLOWER sends in EcPktHdr_QberEstBitsAck
 * 
 *  If this is not "LET_PROC_ROLE_QBER_EST_INITIATOR_DECIDE", the PROC_ROLE_QBER_EST_FOLLOWER (bob) will be the one
 * initiating the EC procedure.
 * 
 */
enum EC_ALGORITHM {
    // EC_ALG_LET_PROC_ROLE_QBER_EST_INITIATOR_DECIDE = 0,   ///< PROC_ROLE_QBER_EST_FOLLOWER makes no preparation whatsoever. Unimplemented.
    EC_ALG_CASCADE_CONTINUE_ROLES = 1,          ///< Same as original code, PROC_ROLE_QBER_EST_FOLLOWER will continue to be the follower for EC
    EC_ALG_CASCADE_FLIP_ROLES = 2,              ///< PROC_ROLE_QBER_EST_FOLLOWER initiates the cascade biconf process instead
    EC_ALG_LDPC_CONTINUE_ROLES = 3,             ///< PROC_ROLE_QBER_EST_FOLLOWER ACKs without preparation and waits for checksum from PROC_ROLE_QBER_EST_INITIATOR.
    EC_ALG_LDPC_FLIP_ROLES = 4                  ///< PROC_ROLE_QBER_EST_FOLLOWER initiates the LDPC process instead
};
typedef enum EC_ALGORITHM EC_ALGORITHM;

/// @} 

/// @name PACKET HANDLERS
/// @{

/**
 * @brief Packet handlers for the QBER estimation algorithm (function pointers) on QBER follower side
 * 
 * All algorithms involve commmunications between the parties. This array contains pointers to functions
 * that process the incoming packets (hence termed "packet handlers").
 * 
 * For more information on function pointers see:
 * https://stackoverflow.com/questions/252748/how-can-i-use-an-array-of-function-pointers
 * https://stackoverflow.com/questions/337449/how-does-one-declare-an-array-of-constant-function-pointers-in-c
 * 
 */

extern int (* const pktHandlers_qber_follower[])(ProcessBlock* processBlock, char* receivebuf);
/// Since array indexes start from 0, this is to align the array index with the subtype number
// #define ALG_QBER_FOLLOWER_SUBTYPE_NO_OFFSET 0

/**
 * @brief Packet handlers for the QBER estimation algorithm (function pointers) on QBER initiator side
 * 
 * All algorithms involve commmunications between the parties. This array contains pointers to functions
 * that process the incoming packets (hence termed "packet handlers").
 * 
 */

extern int (* const pktHandlers_qber_initiator[])(ProcessBlock* processBlock, char* receivebuf);
/// Since array indexes start from 0, this is to align the array index with the subtype number
// #define ALG_QBER_INITIATOR_SUBTYPE_NO_OFFSET 2

/**
 * @brief Packet handlers for the CASCADE algorithm (function pointers) on EC initiator side
 * 
 * All algorithms involve commmunications between the parties. This array contains pointers to functions
 * that process the incoming packets (hence termed "packet handlers").
 * 
 */
extern int (* pktHandlers_cascade_initiator[])(ProcessBlock* processBlock, char* receivebuf);
/// Since array indexes start from 0, this is to align the array index with the subtype number
// #define ALG_CASCADE_INITIATOR_SUBTYPE_NO_OFFSET 4

/**
 * @brief Packet handlers for the CASCADE algorithm (function pointers) on EC initiator side
 * 
 * All algorithms involve commmunications between the parties. This array contains pointers to functions
 * that process the incoming packets (hence termed "packet handlers").
 * 
 */
extern int (* pktHandlers_cascade_follower[])(ProcessBlock* processBlock, char* receivebuf);
/// Since array indexes start from 0, this is to align the array index with the subtype number
// #define ALG_CASCADE_FOLLOWER_SUBTYPE_NO_OFFSET 4

/// @}

#endif