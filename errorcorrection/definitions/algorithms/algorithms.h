/**
 * @file algorithms.h  
 * @brief Definition file for error correction algorithms
 */

#ifndef EC_ERRC_ALGORITHMS
#define EC_ERRC_ALGORITHMS

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

#endif