/**
 * @file globalvars.h     
 * @brief Definition file for global variables and arguments in ecd2
 * 
 *  Copyright (C) 2020 Matthew Lee, National University
 *                          of Singapore <crazoter@gmail.com>
 * 
 *  Copyright (C) 2005-2007 Christian Kurtsiefer, National University
 *                          of Singapore <christian.kurtsiefer@gmail.com>
 * 
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Public License as published
 *  by the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
 * 
 *  This source code is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  Please refer to the GNU Public License for more details.
 * 
 *  You should have received a copy of the GNU Public License along with
 *  this source code; if not, write to:
 *  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 * 
 * --
 * 
 * This is a definition file for global variables used in ecd2.
 * These variables are initialized in ecd2.h.
 * 
 * Some of these variables can actually be passed in as a param,
 * but currently kept as globalvars to retain consistency with
 * previous coding style
 */

#ifndef ECD2_GLOBALS
#define ECD2_GLOBALS

/// \cond for doxygen annotation
#include <stdio.h>
/// \endcond

#include "defaultdefinitions.h"
#include "../subcomponents/helpers.h"
#include "packets.h"

//  STRUCTS
/**
 * @brief Arguments struct to be parsed in
 * 
 * Default values defined in ecd2.h
 */
struct arguments {
    // Enums
    enum { VERBOSITY_EPOCH = 0, 
        VERBOSITY_EPOCH_FIN = 1, 
        VERBOSITY_EPOCH_INI_FIN_ERR = 2, 
        VERBOSITY_EPOCH_INI_FIN_ERR_PLAIN = 3, 
        VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT = 4, 
        VERBOSITY_EPOCH_INI_FIN_ERR_EXPLICIT_WITH_COMMENTS = 5 
    } verbosity_level;  /**< verbosity level of output, used by priv_amp */
   
    enum { END_ON_ERR = 0, 
        IGNORE_INVALID_PACKETS = 1, 
        IGNORE_ERRS_ON_OTHER_END = 2 
    } runtimeerrormode; /**< Determines the way how to react on errors which should not stop the demon. */

    int biconfRounds;  /**< number of biconf rounds, used by cascade_biconf */
    float errorMargin;  /**< number of standard deviations of the detected errors should be added to eve's information leakage estimation, used by priv_amp*/ 
    float initialErrRate;   /**< What error to assume initially */
    float intrinsicErrRate; /**< error rate generated outside eavesdropper, used by priv_amp */
    
    // Helper parameter to help calculate # of biconfRounds
    // float biconf_BER;   /**< BER argument to determine biconf rounds */

    /// @name booleans
    /// @{
    Boolean removeRawKeysAfterUse;
    Boolean skipQberEstimation; /**< used by qber_estim */
    Boolean bellmode;   /**< Expect to receive a value for the Bell violation  param, used by priv_amp */
    Boolean disablePrivAmp; /**< used by priv_amp  */
    //// @}

    char fname[handleId_numberOfHandles][FNAME_LENGTH];
    int handle[handleId_numberOfHandles];
    FILE *fhandle[8];
};
typedef struct arguments Arguments;

// GLOBAL VARIABLES
// Defined in ecd2.h
extern Arguments arguments; /**< contains arguments read in when initializing program */

/// @name Global variables used by processblock_mgmt 
/// @{
extern ProcessBlockDequeNode *processBlockDeque;
/// @}

/// @name Global variables used by comms
/// @{
extern PacketToSendNode*nextPacketToSend;
extern PacketToSendNode*lastPacketToSend;
/// @}


#endif
