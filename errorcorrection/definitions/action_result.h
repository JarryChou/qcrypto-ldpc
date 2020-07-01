/**
 * @file action_result.h  
 * @brief Definition file for action result (Metadata for function abstraction) in ecd2
 */

#ifndef EC_ACTION_RESULT
#define EC_ACTION_RESULT

#include "packets.h"

/**   
 * @brief Metadata for function abstraction
 * 
 * This is passed in as a parameter and populated by functions that require a decision at a higher-level
 * after having processed an incoming packet, such as what algorithm to use, and subsequently communicating
 * that decision (if necessary) to the other party.
 * 
 * Only functions that require a decision are passed such a parameter; some functions may produce different
 * packets (e.g. Cascade iterations), but if they do not need to make decisions at a higher level, this param
 * is omitted from their function call.
 * 
 * An example of a higher level decision is usually regarding the type of algorithm to use e.g. Cascade, LDPC,
 * Polar codes or privacy amplification. This is currently not in place w.r.t. QBER estimation algorithms though.
 * 
 */
enum EC_DECISION_REQUIRED {
    /// There is no decision to be made. The function fully encapsulates what needs to be done.
    /**
     * In this case, even if the function can result in a variety of functions being sent (e.g. )
     */
    AR_NONE = 0, 

    /// There is a decision to be made at the ecd2.c level that doesn't need to be communicated.
    /** 
     * However, the decision does not affect what is sent to the other party. 
     * This is currently NOT in use.
     * 
     * (Potential) Uses in the program: 
     *  1.  Privacy amplification, where an internal decision 
     *      has to be made on what privacy amplification algorithm to use 
     *      (right now there's only 1 so this is a stub)
     */
    AR_INTERNAL_DECISION = 1,

    /// There is a decision to be made at the ecd2.c level that needs to be communicated.
    /** 
     * The decision affects what is sent to the other party.
     * However, the communication does not involve any pre-filled packets by the function;
     * the decision function is to create its own EC packet and send that instead.
     * 
     * Uses in the program: 
     *  1.  Error Correction, where a decision on an algorithm has to be made and communicated
     *      to the other party.
     */
    AR_DECISION_INVOLVING_NEW_PACKET = 2,

    /// There is a decision to be made at the ecd2.c level that needs to be communicated.
    /** 
     * The decision affects what is sent to the other party.
     * In addition, the communication involves pre-filled packets by the function;
     * the decision function is to use the pre-filled EC packet and send that, together with
     * any other packets it needs to send (if necessary)
     * 
     * Uses in the program: 
     *  1.  Error Correction, where a decision on an algorithm has to be made and communicated
     *      to the other party.
     */
    AR_DECISION_INVOLVING_PREFILLED_DATA = 3
};
typedef enum EC_DECISION_REQUIRED DECISION_REQUIRED;

/**
 * @brief Struct to contain the metadata.
 * 
 * Nice to have: Some kind of high level abstraction that identifies the type of data this contains.
 * Right now that is hardcoded.
 * 
 */
typedef struct EC_ACTION_RESULT_STRUCT {
    DECISION_REQUIRED nextActionEnum;
    // enum EC_SUBTYPES subtype;
    char *bufferToSend;
    unsigned int bufferLengthInBytes;
} ActionResult;

#endif