/**
 * @file packet_manager.h  
 * @brief Definition file for structure involving packet management of algorithm and its metadata in the processblock
 */

#ifndef EC_ERRC_ALGORITHMS_PKTMNGR
#define EC_ERRC_ALGORITHMS_PKTMNGR

#include "../processblock.h"
// Forward declaration because ALGORITHM_DATA_MNGR uses process block, but process block also uses it
typedef struct ProcessBlk ProcessBlock;

/**
 * @brief Type definition for an array of packet handlers
 * 
 * All algorithms involve commmunications between the parties. This array contains pointers to functions
 * that process the incoming packets (hence termed "packet handlers").
 * 
 * A packet handler is a function pointer that handles the corresponding subtype of an incoming packet.
 * A function pointer is basically a pointer to a function
 * Each packet handler returns an int (error code) and takes in a process block and a receivebuf.
 * 
 *  For more information on function pointers see:
 * https://stackoverflow.com/questions/252748/how-can-i-use-an-array-of-function-pointers
 * https://stackoverflow.com/questions/337449/how-does-one-declare-an-array-of-constant-function-pointers-in-c
 * 
 */
typedef int (* PacketHandlerArray[])(ProcessBlock* processBlock, char* receivebuf);

/**
 * @brief 
 * 
 */
typedef struct ALGORITHM_PKT_MNGR {
    const PacketHandlerArray* FUNC_HANDLERS;              ///< Pointer to function pointers/handlers for current algorithm
    /// Since array indexes start from 0, this is to align the array index with the subtype number
    const int SUBTYPE_NUM_OFFSET;                         // Store offset from first function handler to its packet subtype number
    const int FUNC_HANDLER_COUNT;                         ///< Number of function handlers
    const int ALLOW_NULL_PRCBLKS;                         ///< Boolean. Function handlers take in process block as a param. This flag allows them to be null, otherwise error is thrown.
} ALGORITHM_PKT_MNGR;

#endif