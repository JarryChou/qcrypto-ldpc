/**
 * @file algorithms.h  
 * @brief Definition file for error correction algorithms
 *
 * File that defines implemented algorithms
 * 
 */

#ifndef EC_ERRC_ALGORITHMS
#define EC_ERRC_ALGORITHMS 1

#include "../globalvars.h"

#include "packet_manager.h"
#include "data_manager.h"

#include "../../subcomponents/cascade_biconf.h"
#include "../../subcomponents/priv_amp.h"
#include "../../subcomponents/qber_estim.h"

/// @name ALGORITHM DECISION BY QBER_FOLLOWER
/// @{
/**   
 * @brief What PROC_ROLE_QBER_EST_FOLLOWER sends in EcPktHdr_QberEstBitsAck
 * 
 * In the current application, you can configure (in code) the one who initiates QBER estimation to be different from the one who initiates
 * the error correction procedure. In this case, the QBER_FOLLOWER (the one who didn't initiate QBER est.) 
 * is the one who tells the QBER_INITIATOR via the acknowledgement packet:
 *  1. What algorithm to use for error correction
 *  2. Who should initiate the error correction (i.e. who should be Alice)
 * 
 * Unimplemented: If this is not "LET_PROC_ROLE_QBER_EST_INITIATOR_DECIDE", the PROC_ROLE_QBER_EST_FOLLOWER (bob) will be the one
 * initiating the EC procedure.
 * 
 */
enum ALGORITHM_DECISION {
    // ALG_LET_PROC_ROLE_QBER_EST_INITIATOR_DECIDE = 0,     ///< PROC_ROLE_QBER_EST_FOLLOWER makes no preparation whatsoever. Unimplemented.
    ALG_CASCADE_CONTINUE_ROLES = 1,                         ///< Same as original code, PROC_ROLE_QBER_EST_FOLLOWER will continue to be the follower for EC
    ALG_CASCADE_FLIP_ROLES = 2,                             ///< PROC_ROLE_QBER_EST_FOLLOWER initiates the cascade biconf process instead
    ALG_LDPC_CONTINUE_ROLES = 3,                            ///< PROC_ROLE_QBER_EST_FOLLOWER ACKs without preparation and waits for checksum from PROC_ROLE_QBER_EST_INITIATOR.
    ALG_LDPC_FLIP_ROLES = 4                                 ///< PROC_ROLE_QBER_EST_FOLLOWER initiates the LDPC process instead
};
typedef enum ALGORITHM_DECISION ALGORITHM_DECISION;
/// @} 

/// @name PACKET HANDLER ARRAYS
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
// Type defined in packet_manager.h, variable declared in algorithms.c  
extern const PacketHandlerArray ALG_PKTHNDLRS_QBER_FOLLOWER;
extern const PacketHandlerArray ALG_PKTHNDLRS_QBER_INITIATOR;
extern const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_FOLLOWER;
extern const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_INITIATOR;
/// @}

/// @name ALGORITHM PACKET MANAGER STRUCTS FOR PROCESS BLOCK
/// @{
// Type defined in packet_manager.h, variable declared in algorithms.c  
extern const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_FOLLOWER;
extern const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_INITATOR;
extern const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_FOLLOWER;
extern const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_INITIATOR;
/// @}

/// @name ALGORITHM DATA MANAGER STRUCTS FOR PROCESS BLOCK
/// @{
// Type defined in data_manager.h, variable declared in algorithms.c 
extern const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_QBER; 
extern const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_CASCADE;
/// @}

#endif