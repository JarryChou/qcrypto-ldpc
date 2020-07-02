/**
 * @file data_manager.h  
 * @brief Definition file for structure involving data management of algorithm specific data and its metadata in the processblock
 */

#ifndef EC_ERRC_ALGORITHMS_DATAMNGR
#define EC_ERRC_ALGORITHMS_DATAMNGR

#include "../processblock.h"
// Forward declaration because ALGORITHM_DATA_MNGR uses process block, but process block also uses it
typedef struct ProcessBlk ProcessBlock;


/// @name ALGORITHM ADDITIONAL DATA
/// @brief Contains information & functions on the algorithm w.r.t. the data stored
/// @{
typedef enum ALGORITHM_DATATYPE {
    ALG_DATATYPE_QBER,
    ALG_DATATYPE_CASCADE,
} ALGORITHM_DATATYPE;

/**
 * @brief Metadata containing information on the data, as well as function pointers to initializing and freeing the data
 * 
 * For the types of implementations see algorithms.c
 * 
 */
typedef struct ALGORITHM_DATA_MNGR {
    const ALGORITHM_DATATYPE DATA_TYPE;                         ///< Enum which identifies the data type of the data
    const int (* initData)(ProcessBlock* processBlock);         ///< Function to call to initialize the data
    const int (* freeData)(ProcessBlock* processBlock);         ///< Function to call to free the data
} ALGORITHM_DATA_MNGR;

/// @brief container for additional variables required only for QBER estimation
typedef struct ALGORITHM_QBER_DATA {
    int skipQberEstim;                    /**< Boolean. determines if error estimation has to be done */
    int estimatedError;                   /**< number of estimated error bits */
    int estimatedSampleSize;              /**< sample size for error estimation  */
} QberData;

/// @brief container for additional variables required only for cascade algorithm
typedef struct ALGORITHM_CASCADE_DATA {
    int k0, k1;                           /**< binary block search lengths */
    int partitions0, partitions1;         /**< number of partitions of k0,k1 length */
    unsigned int *lp0, *lp1;              /**< pointer to local parity info 0 / 1 */
    unsigned int *rp0, *rp1;              /**< pointer to remote parity info 0 / 1 */
    unsigned int *pd0, *pd1;              /**< pointer to parity difference fileld */
    int diffBlockCount;                   /**< number of different blocks in current round */
    int diffBlockCountMax;                /**< number of malloced entries for diff indices */
    unsigned int *diffidx;                /**< pointer to a list of parity mismatch blocks */
    unsigned int *diffidxe;               /**< end of interval */
    int binarySearchDepth;                /**< encodes state of the scan. Starts with 0, and contains the pass (0/1) in the MSB */
    int biconfRound;                      /**< contains the biconf round number, starting with 0 */
    int biconfLength;                     /**< current length of a biconf check range */
} CascadeData;

#endif