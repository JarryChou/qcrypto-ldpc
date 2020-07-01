/** @file cascade_biconf.h     
 * @brief Cascade biconf algorithm implementation
 * 
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below.
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
 * This is a refactored version of Kurtsiefer's ecd2.c to modularize parts of the
 * code. This section modularizes the parts of the code that mainly implements the
 * Cascade biconf algorithm.
 * 
 */

#ifndef ECD2_CASCADE_BICONF
#define ECD2_CASCADE_BICONF

// Libraries
/// \cond omit doxygen dependency diagram
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/// \endcond

// Definitions
#include "../definitions/defaultdefinitions.h"
#include "../definitions/globalvars.h"
#include "../definitions/processblock.h"
#include "../definitions/packets.h"
#include "../definitions/proc_state.h"

// Other components
#include "comms.h"
#include "debug.h"
#include "helpers.h"
#include "priv_amp.h"
#include "rnd.h"
#include "processblock_mgmt.h"

/// @name PERMUTATION HELPER FUNCTIONS
/// @{
void fix_permutedbits(ProcessBlock *pb); 
/// @}


/// @name CASCADE BICONF HELPER FUNCTIONS
/// @{
// void generate_selectbitstring(ProcessBlock *pb, unsigned int seed); // Unused
void cascade_generateBiconfString(ProcessBlock *pb);
int cascade_prepParityListAndDiffs(ProcessBlock *pb, int pass);
void cascade_fixParityIntervals(ProcessBlock *pb, unsigned int *inh_idx);
void cascade_setStateKnowMyErrorThenCalck0k1(ProcessBlock *processBlock);
void cascade_prepareParityList1(ProcessBlock *pb, unsigned int *d0, unsigned int *d1);
/// @}

/// @name CASCADE BICONF MAIN FUNCTIONS
/// @{
int cascade_initiateAfterQber(ProcessBlock *processBlock);
int cascade_initiateBiconf(ProcessBlock *pb);
int cascade_generateBiconfReply(ProcessBlock *pb, char *receivebuf);
int cascade_makeBiconfBinSearchReq(ProcessBlock *pb, int biconfLength);
int cascade_prepFirstBinSearchMsg(ProcessBlock *pb, int pass);
int cascade_startBinSearch(ProcessBlock *pb, char *receivebuf);
int cascade_initiatorAlice_processBinSearch(ProcessBlock *pb, EcPktHdr_CascadeBinSearchMsg *in_head);
int cascade_followerBob_processBinSearch(ProcessBlock *pb, EcPktHdr_CascadeBinSearchMsg *in_head);
int cascade_receiveBiconfReply(ProcessBlock *pb, char *receivebuf);
/// @}

#endif /* ECD2_CASCADE_BICONF */