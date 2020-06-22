/**
 * ecd2_qber_estim.h     
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
 * code. This section modularizes the parts of the code that is predominantly
 * used for QBER estimation.
 * 
 */

#ifndef ECD2_QBER_ESTIM
#define ECD2_QBER_ESTIM

#include <stdio.h>

#include "defaultdefinitions.h"
#include "globalvars.h"
#include "keyblock.h"
#include "packets.h"
#include "proc_state.h"

// ENUMS

enum ReplyMode {
  replyMode_terminate = 0,
  replyMode_moreBits = 1,
  replyMode_continue = 2
};

// ERROR ESTIMATION
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
// MAIN FUNCTIONS
/* function to provide the number of bits needed in the initial error
   estimation; eats the local error (estimated or guessed) as a float. Uses
   the maximum for either estimating the range for k0 with the desired error,
   or a sufficient separation from the no-error-left area. IS that fair?
   Anyway, returns a number of bits. */
int testbits_needed(float e);

/* function to initiate the error estimation procedure. parameter is
   statrepoch, return value is 0 on success or !=0 (w error encoding) on error.
 */
int errorest_1(unsigned int epoch);

/* function to process the first error estimation packet. Argument is a pointer
   to the receivebuffer with both the header and the data section. Initiates
   the error estimation, and prepares the next  package for transmission.
   Currently, it assumes only PRNG-based bit selections.

   Return value is 0 on success, or an error message useful for emsg.

*/
int process_esti_message_0(char *receivebuf);

/* function to reply to a request for more estimation bits. Argument is a
   pointer to the receive buffer containing the message. Just sends over a
   bunch of more estimaton bits. Currently, it uses only the PRNG method.

   Return value is 0 on success, or an error message otherwise. */
int send_more_esti_bits(char *receivebuf);

/* function to proceed with the error estimation reply. Estimates if the
   block deserves to be considered further, and if so, prepares the permutation
   array of the key, and determines the parity functions of the first key.
   Return value is 0 on success, or an error message otherwise. */
int prepare_dualpass(char *receivebuf);

#endif /* ECD2_QBER_ESTIM */