// While these are purely definitions, they are put into C files.
// See https://stackoverflow.com/questions/249701/why-arent-my-compile-guards-preventing-multiple-definition-inclusions
#include "algorithms.h"

int (* const pktHandlers_qber_follower[])(ProcessBlock* processBlock, char* receiveBuf) = {
    qber_processReceivedQberEstBits             ///< Subtype 0
};
#define ALG_QBER_FOLLOWER_SUBTYPE_NO_OFFSET 0

int (* const pktHandlers_qber_initiator[])(ProcessBlock* processBlock, char* receiveBuf) = {
    qber_replyWithMoreBits,                     ///< Subtype 2
    qber_prepareErrorCorrection                 ///< Subtype 3
};
#define ALG_QBER_INITIATOR_SUBTYPE_NO_OFFSET 2

int (* pktHandlers_cascade_initiator[])(ProcessBlock* processBlock, char* receiveBuf) = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_initiatorAlice_processBinSearch,    ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
#define ALG_CASCADE_INITIATOR_SUBTYPE_NO_OFFSET 4

int (* pktHandlers_cascade_follower[])(ProcessBlock* processBlock, char* receiveBuf) = {
    cascade_startBinSearch,                     ///< Subtype 4
    cascade_followerBob_processBinSearch,       ///< Subtype 5
    cascade_generateBiconfReply,                ///< Subtype 6   
    cascade_receiveBiconfReply,                 ///< Subtype 7
    privAmp_receivePrivAmpMsg                   ///< Subtype 8
};
#define ALG_CASCADE_FOLLOWER_SUBTYPE_NO_OFFSET 4
