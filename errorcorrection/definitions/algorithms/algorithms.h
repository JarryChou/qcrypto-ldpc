/**
 * @file algorithms.h  
 * @brief Definition file for error correction algorithms
 */

#ifndef EC_ERRC_ALGORITHMS
#define EC_ERRC_ALGORITHMS

/**   
 * @brief What QBER_EST_FOLLOWER sends in EcPktHdr_QberEstBitsAck
 * 
 * If this is not "LET_QBER_INITIATOR_DECIDE", the QBER_EST_FOLLOWER (bob) will be the one
 * initiating the EC procedure.
 * 
 */
enum QBER_FOLLOWER_ALGORITHM_DECISION {
    LET_QBER_INITIATOR_DECIDE = 0,   ///< QBER_EST_FOLLOWER does not initiate the EC process.
    CASCADE = 1,                ///< QBER_EST_FOLLOWER initiates the cascade biconf process
    LDPC = 2                    ///< QBER_EST_FOLLOWER initiates the LDPC process
};
typedef enum QBER_FOLLOWER_ALGORITHM_DECISION QBER_EST_FOLLOWER_ALGORITHM_DECISION;

#endif