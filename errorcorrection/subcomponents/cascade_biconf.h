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
void fix_permutedbits(ProcessBlock *kb); 
/// @}


/// @name CASCADE BICONF HELPER FUNCTIONS
/// @{
void generate_selectbitstring(ProcessBlock *kb, unsigned int seed);
void generate_BICONF_bitstring(ProcessBlock *kb);
int do_paritylist_and_diffs(ProcessBlock *kb, int pass);
void fix_parity_intervals(ProcessBlock *kb, unsigned int *inh_idx);
void correct_bit(unsigned int *d, int bitindex);
int singleLineParityMasked(unsigned int *d, unsigned int *m, int start, int end);
/// @}

/// @name CASCADE BICONF MAIN FUNCTIONS
/// @{
int process_binsearch_alice(ProcessBlock *kb, EcPktHdr_CascadeBinSearchMsg *in_head);
int initiate_biconf(ProcessBlock *kb);
int generateBiconfReply(ProcessBlock *kb, char *receivebuf);
int initiate_biconf_binarysearch(ProcessBlock *kb, int biconflength);
int prepare_first_binsearch_msg(ProcessBlock *kb, int pass);
int start_binarysearch(ProcessBlock *kb, char *receivebuf);
int process_binarysearch(ProcessBlock *kb, char *receivebuf);
int process_binsearch_bob(ProcessBlock *kb, EcPktHdr_CascadeBinSearchMsg *in_head);
int receive_biconfreply(ProcessBlock *kb, char *receivebuf);
/// @}

#endif /* ECD2_CASCADE_BICONF */