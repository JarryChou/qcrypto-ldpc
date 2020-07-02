// While these are mostly definitions, they are put into C files.
// See https://stackoverflow.com/questions/249701/why-arent-my-compile-guards-preventing-multiple-definition-inclusions
#include "algorithms.h"

// ALGORITHM STRUCT FOR PROCESS BLOCK
// Contains information & functions on the algorithm w.r.t. packet handling
// ********************************************************************************
// I realize this will not be used, but I left this in to make it more clear what's going on
const PacketHandlerArray ALG_PKTHNDLRS_QBER_FOLLOWER = {
    qber_processReceivedQberEstBits             ///< Subtype 0
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_FOLLOWER = {
    &ALG_PKTHNDLRS_QBER_FOLLOWER,               // funcHandlers
    0,                                          // FIRST_SUBTYPE
    sizeof(ALG_PKTHNDLRS_QBER_FOLLOWER)          // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_QBER_FOLLOWER[0]),
    True                                        // allowNullPrcBlks
};

const PacketHandlerArray ALG_PKTHNDLRS_QBER_INITIATOR = {
    qber_replyWithMoreBits,                     ///< Subtype 2
    qber_prepareErrorCorrection                 ///< Subtype 3
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_QBER_INITATOR = {
    &ALG_PKTHNDLRS_QBER_INITIATOR,              // funcHandlers
    2,                                          // FIRST_SUBTYPE
    2 + sizeof(ALG_PKTHNDLRS_QBER_INITIATOR)         // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_QBER_INITIATOR[0]) - 1,
    False                                       // allowNullPrcBlks
};

const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_FOLLOWER = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_initiatorAlice_processBinSearch,    ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_INITIATOR = {
    &ALG_PKTHNDLRS_CASCADE_INITIATOR,           // funcHandlers
    4,                                          // FIRST_SUBTYPE
    4 + sizeof(ALG_PKTHNDLRS_CASCADE_FOLLOWER)       // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_CASCADE_FOLLOWER[0]) - 1,
    False                                       // allowNullPrcBlks
};

const PacketHandlerArray ALG_PKTHNDLRS_CASCADE_INITIATOR = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_followerBob_processBinSearch,       ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
const ALGORITHM_PKT_MNGR ALG_PKT_MNGR_CASCADE_FOLLOWER = {
    &ALG_PKTHNDLRS_CASCADE_FOLLOWER,            // funcHandlers
    4,                                          // FIRST_SUBTYPE
    4 + sizeof(ALG_PKTHNDLRS_CASCADE_INITIATOR)      // LAST_SUBTYPE, automatically calculated
        / sizeof(ALG_PKTHNDLRS_CASCADE_INITIATOR[0]) - 1,
    False                                       // allowNullPrcBlks
};





// ALGORITHM DATA STRUCT FOR PROCESS BLOCK
// Contains information & functions on the algorithm w.r.t. the data stored
// ********************************************************************************
/** @brief Init struct containing data */
int initDataStruct(ProcessBlock* processBlock) {
    if (processBlock->algorithmDataPtr) {
        // Should be null
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
            fprintf(stderr, "Unsupported datatype initDataStruct");
            return 81;
    }
    /* if malloc failed */
    if (!(processBlock->algorithmDataPtr)) 
        return 34;
    return 0;
}
/** @brief Free struct containing data */
int freeDataStruct(ProcessBlock* processBlock) {
    // Free the struct itself
    free2(processBlock->algorithmDataPtr);
    // Delink references to the algorithm data block metadata
    processBlock->algorithmDataPtr = NULL;
    processBlock->algorithmDataMngr = NULL;
    return 0;
}

int initQberData(ProcessBlock* processBlock) {
    return initDataStruct(processBlock);
}
int freeQberData(ProcessBlock* processBlock) {
    return freeDataStruct(processBlock);
}
const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_QBER = {
    ALG_DATATYPE_QBER,      // Enum which identifies the data type of the data
    &initQberData,          // Function to call to initialize the data
    &freeQberData           // Function to call to free the data
};

int initCascadeData(ProcessBlock* processBlock) {
    return initDataStruct(processBlock);
}
int freeCascadeData(ProcessBlock* processBlock) {
    CascadeData *data = (CascadeData *)processBlock->algorithmDataPtr;
    // Free cascade data items
    /* parity storage */
    if (data->lp0) 
        free2(data->lp0);
    if (data->diffidx) 
        free2(data->diffidx);
    // Free the struct itself
    return freeDataStruct(processBlock);
}
const ALGORITHM_DATA_MNGR ALG_DATA_MNGR_CASCADE = {
    ALG_DATATYPE_CASCADE,   // Enum which identifies the data type of the data
    &initCascadeData,       // Function to call to initialize the data
    &freeCascadeData        // Function to call to free the data
};