/**
 * cascade_biconf.h     
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below.
 * 
 *  Copyright (C) 2020 Matthew Lee, National University
 *                          of Singapore <crazoter@gmail.com>
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
 * code. This section modularizes the parts of the code that mainly implements  the
 * Cascade biconf algorithm.
 * 
 */

#ifndef ECD2_CASCADE_BICONF
#define ECD2_CASCADE_BICONF

// Libraries
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Definitions
#include "defaultdefinitions.h"
#include "globalvars.h"
#include "keyblock.h"
#include "packets.h"
#include "proc_state.h"

// Other components
#include "comms.h"
#include "debug.h"
#include "helpers.h"
#include "priv_amp.h"
#include "rnd.h"
#include "thread_mgmt.h"

// PERMUTATIONS
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper to fix the permuted/unpermuted bit changes; decides via a parameter
   in kb->binsearch_depth MSB what polarity to take */
void fix_permutedbits(struct keyblock *kb);

// MAIN FUNCTIONS

// CASCADE BICONF
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer. parameters are a keyblock pointer, and a seed for the RNG.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_selectbitstring(struct keyblock *kb, unsigned int seed);

/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer AND transfer the permuted key buffer into it for a more compact
   parity generation in the last round.
   Parameters are a keyblock pointer.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_BICONF_bitstring(struct keyblock *kb);

/* helper function to preare a parity list of a given pass in a block, compare
   it with the received list and return the number of differing bits  */
int do_paritylist_and_diffs(struct keyblock *kb, int pass);

/* helper program to half parity difference intervals ; takes kb and inh_index
   as parameters; no weired stuff should happen. return value is the number
   of initially dead intervals */
void fix_parity_intervals(struct keyblock *kb, unsigned int *inh_idx);

/* helper for correcting one bit in pass 0 or 1 in their field */
void correct_bit(unsigned int *d, int bitindex);

/* helper funtion to get a simple one-line parity from a large string.
   parameters are the string start buffer, a start and an enx index. returns
   0 or 1 */
int single_line_parity(unsigned int *d, int start, int end);

/* helper funtion to get a simple one-line parity from a large string, but
   this time with a mask buffer to be AND-ed on the string.
   parameters are the string buffer, mask buffer, a start and and end index.
   returns  0 or 1 */
int single_line_parity_masked(unsigned int *d, unsigned int *m, int start, int end);

// MAIN FUNCTIONS
/* function to process a binarysearch request on alice identity. Installs the
   difference index list in the first run, and performs the parity checks in
   subsequent runs. should work with both passes now
   - work in progress, need do fix bitloss in last round
 */
int process_binsearch_alice(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head);

/* function to initiate a BICONF procedure on Bob side. Basically sends out a
   package calling for a BICONF reply. Parameter is a thread pointer, and
   the return value is 0 or an error code in case something goes wrong.   */
int initiate_biconf(struct keyblock *kb);

/* start the parity generation process on Alice side. parameter contains the
   input message. Reply is 0 on success, or an error message. Should create
   a BICONF response message */
int generate_biconfreply(char *receivebuf);

/* function to generate a single binary search request for a biconf cycle.
   takes a keyblock pointer and a length of the biconf block as a parameter,
   and returns an error or 0 on success.
   Takes currently the subset of the biconf subset and its complement, which
   is not very efficient: The second error could have been found using the
   unpermuted short sample with nuch less bits.
   On success, a binarysearch packet gets emitted with 2 list entries. */
int initiate_biconf_binarysearch(struct keyblock *kb, int biconflength);

/* function to proceed with the parity evaluation message. This function
   should start the Binary search machinery.
   Argument is receivebuffer as usual, returnvalue 0 on success or err code.
   Should spit out the first binary search message */
int start_binarysearch(char *receivebuf);

/* function to process a binarysearch request. distinguishes between the two
   symmetries in the evaluation. This is onyl a wrapper.
   on alice side, it does a passive check; on bob side, it possibly corrects
   for errors. */
int process_binarysearch(char *receivebuf);

/* function to process a binarysearch request on bob identity. Checks parity
   lists and does corrections if necessary.
   initiates the next step (BICONF on pass 1) for the next round if ready.
*/
int process_binsearch_bob(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head);

/* start the parity generation process on bob's side. Parameter contains the
   parity reply form Alice. Reply is 0 on success, or an error message.
   Should either initiate a binary search, re-issue a BICONF request or
   continue to the parity evaluation. */
int receive_biconfreply(char *receivebuf);

#endif /* ECD2_CASCADE_BICONF */