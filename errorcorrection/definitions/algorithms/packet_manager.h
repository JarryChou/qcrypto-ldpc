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
 * that process the incoming packets (hence termed "packet handlers"). Think of it as the errormessage array,
 * but it contains functions that you call instead.
 * 
 * A packet handler is a function pointer that handles the corresponding subtype of an incoming packet.
 * A function pointer is basically a pointer to a function
 * Each packet handler returns an int (error code) and takes in a process block and a receivebuf.
 * 
 * The subtypes covered by a packet manager is consecutive.
 * 
 * While the term used is "manager", "routing" could also be used to describe this if it is easier to understand.
 * 
 * For more information on function pointers see:
 * https://stackoverflow.com/questions/252748/how-can-i-use-an-array-of-function-pointers
 * https://stackoverflow.com/questions/337449/how-does-one-declare-an-array-of-constant-function-pointers-in-c
 * 
 */
typedef int (* PacketHandlerArray[])(ProcessBlock* processBlock, char* receivebuf);

/**
 * @brief Algorithm packet manager struct. Contains pointers to the packet handlers and metadata.
 * 
 * Stored in process block
 * 
 */
typedef struct ALGORITHM_PKT_MNGR {
    const PacketHandlerArray *FUNC_HANDLERS;         ///< Pointer to function pointers/handlers for current algorithm
    const int FIRST_SUBTYPE;                         ///< The first subtype covered by the function handlers
    const int LAST_SUBTYPE;                          ///< The last subtype covered by this algorithm pkt manager
    const int ALLOW_NULL_PRCBLKS;                    ///< Boolean. Function handlers take in process block as a param. This flag allows them to be null, otherwise error is thrown.
} ALGORITHM_PKT_MNGR;

#endif