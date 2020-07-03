// While these are mostly definitions, they are put into C files.
// See https://stackoverflow.com/questions/249701/why-arent-my-compile-guards-preventing-multiple-definition-inclusions
#include "algorithms.h"

// ALGORITHM STRUCT FOR PROCESS BLOCK
// Contains information & functions on the algorithm w.r.t. packet handling
// ********************************************************************************

/// @name QBER_FOLLOWER PACKET MANAGER
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
/**
 * @brief Pointers to packet handler functions for QBER_FOLLOWER
 * 
 * (this specific array and manager won't get used in the current implementation)
 * I realize this will not be used, but I left this in to make it more clear what's going on
 * 
 */
const PacketHandlerArray ALG_PKTHNDLRS_QBER_FOLLOWER = {
    qber_processReceivedQberEstBits             ///< Subtype 0
};
/**
 * @brief Metadata to be stored in process block w.r.t. packet handling
 * 
 */
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_FOLLOWER = {
    &ALG_PKTHNDLRS_QBER_FOLLOWER,               // funcHandlers
    0,                                          // FIRST_SUBTYPE
    sizeof(ALG_PKTHNDLRS_QBER_FOLLOWER)          // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_QBER_FOLLOWER[0]),
    True                                        // allowNullPrcBlks
};
/// @} 

/// @name QBER_INITIATOR PACKET MANAGER
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
/**
 * @brief Pointers to packet handler functions for QBER_INITIATOR
 * 
 */
const PacketHandlerArray ALG_PKTHNDLRS_QBER_INITIATOR = {
    qber_replyWithMoreBits,                     ///< Subtype 2
    qber_prepareErrorCorrection                 ///< Subtype 3
};
/**
 * @brief Metadata to be stored in process block w.r.t. packet handling
 * 
 */
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_INITATOR = {
    &ALG_PKTHNDLRS_QBER_INITIATOR,              // funcHandlers
    2,                                          // FIRST_SUBTYPE
    2 + sizeof(ALG_PKTHNDLRS_QBER_INITIATOR)         // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_QBER_INITIATOR[0]) - 1,
    False                                       // allowNullPrcBlks
};
/// @} 

/// @name CASCADE_INITIATOR PACKET MANAGER
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_INITIATOR = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_initiatorAlice_processBinSearch,    ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_INITIATOR = {
    &ALG_PKTHNDLRS_CASCADE_INITIATOR,           // funcHandlers
    4,                                          // FIRST_SUBTYPE
    4 + sizeof(ALG_PKTHNDLRS_CASCADE_INITIATOR) // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_CASCADE_INITIATOR[0]) - 1,
    False                                       // allowNullPrcBlks
};
/// @} 

/// @name CASCADE_FOLLOWER PACKET MANAGER
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_FOLLOWER = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_followerBob_processBinSearch,       ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_FOLLOWER = {
    &ALG_PKTHNDLRS_CASCADE_FOLLOWER,            // funcHandlers
    4,                                          // FIRST_SUBTYPE
    4 + sizeof(ALG_PKTHNDLRS_CASCADE_FOLLOWER)  // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_CASCADE_FOLLOWER[0]) - 1,
    False                                       // allowNullPrcBlks
};
/// @} 





// ALGORITHM DATA STRUCT FOR PROCESS BLOCK
// Contains information & functions on the algorithm w.r.t. the data stored
// ********************************************************************************

/// @name CASCADE_FOLLOWER PACKET MANAGER
/// @brief Contains information & functions on the algorithm w.r.t. packet handling
/// @{
/**
 * @brief General function for mallocing the struct containing data without initializing it's internal variables
 * 
 * Update this if you want to add a new algorithm
 * 
 * @param processBlock 
 * @return int error code
 */
int initDataStruct(ProcessBlock* processBlock) {
    if (processBlock->algorithmDataPtr) {
        // the data ptr should be null or else a logic err occurred
        return 84;
    }
    switch (processBlock->algorithmDataMngr->DATA_TYPE) {
        case ALG_DATATYPE_QBER:
            processBlock->algorithmDataPtr = malloc2(sizeof(QberData));
            break;
        case ALG_DATATYPE_CASCADE:
            processBlock->algorithmDataPtr = malloc2(sizeof(CascadeData));
            break;
        default:
            fprintf(stderr, "Unsupported data type %d @ initDataStruct\n", 
                    processBlock->algorithmDataMngr->DATA_TYPE);
            return 81;
    }
    /* if malloc failed */
    if (!(processBlock->algorithmDataPtr)) 
        return 34;
    return 0;
}
/**
 * @brief General function for freeing the struct containing data without freeing it's internal variables
 * 
 * Make sure to free the variables inside before calling this
 * 
 * @param processBlock 
 * @return int error code
 */
int freeDataStruct(ProcessBlock* processBlock) {
    // Free the struct itself
    free2(processBlock->algorithmDataPtr);
    // Delink references to the algorithm data block metadata
    processBlock->algorithmDataPtr = NULL;
    processBlock->algorithmDataMngr = NULL;
    return 0;
}
/// @} 

/// @name QBER ESTIMATION DATA MANAGER
/// @{
/// @brief function to initialize QBER specific data
int initQberData(ProcessBlock* processBlock) {
    return initDataStruct(processBlock);
}
/// @brief function to free QBER specific data
int freeQberData(ProcessBlock* processBlock) {
    return freeDataStruct(processBlock);
}
/// @brief Contains information & functions on the qber est. algorithm w.r.t. data handling
const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_QBER = {
    ALG_DATATYPE_QBER,      // Enum which identifies the data type of the data
    &initQberData,          // Function to call to initialize the data
    &freeQberData           // Function to call to free the data
};
/// @} 

/// @name CASCADE DATA MANAGER
/// @{
/// @brief function to initialize CASCADE BICONF specific data
int initCascadeData(ProcessBlock* processBlock) {
    int errorCode = initDataStruct(processBlock);
    if (errorCode) return errorCode;
    // Make sure every pointer variable is properly initialized before use
    CascadeData *cscData = (CascadeData *)(processBlock->algorithmDataPtr);
    cscData->diffidx = NULL;
    cscData->diffidxe = NULL;
    cscData->lp0 = NULL;
    cscData->lp1 = NULL;
    cscData->pd0 = NULL;
    cscData->pd1 = NULL;
    cscData->rp0 = NULL;
    cscData->rp1 = NULL;
    return 0;
}
/// @brief function to free CASCADE BICONF specific data
int freeCascadeData(ProcessBlock* processBlock) {
    CascadeData *data = (CascadeData *)(processBlock->algorithmDataPtr);
    // Free cascade data items
    /* parity storage */
    if (data->lp0) 
        free2(data->lp0);
    if (data->diffidx) 
        free2(data->diffidx);
    // Free the struct itself
    return freeDataStruct(processBlock);
}
/// @brief Contains information & functions on the cascade algorithm w.r.t. data handling
const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_CASCADE = {
    ALG_DATATYPE_CASCADE,   // Enum which identifies the data type of the data
    &initCascadeData,       // Function to call to initialize the data
    &freeCascadeData        // Function to call to free the data
};
/// @} 