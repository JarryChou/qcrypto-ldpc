/**
 * comms.h     
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
 * used for communications w.r.t. error correction.
 * 
 */

#ifndef ECD2_COMMS
#define ECD2_COMMS

// Libraries
/// \cond for doxygen annotation
#include <stdlib.h>
/// \endcond

// Definition files
#include "proc_state.h"
#include "helpers.h"
#include "keyblock.h"
#include "packets.h"
#include "globalvars.h"

// Other components
#include "debug.h"

// COMMUNICATIONS
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper to insert a send packet in the sendpacket queue. 
  Parameters are a pointer to the structure and its length. 
  Return value is 0 on success or !=0 on malloc failure */
int insert_sendpacket(char *message, int length);

/* helper function to prepare a message containing a given sample of bits.
   parameters are a pointer to the thread, the number of bits needed and an
   errorormode (0 for normal error est, err*2^16 forskip ). returns a pointer
   to the message or NULL on error.
   Modified to tell the other side about the Bell value for privacy amp in
   the device indep mode
*/
struct ERRC_ERRDET_0 *fillsamplemessage(struct keyblock *kb, int bitsneeded, int errormode, float BellValue);

/* helper function for binsearch replies; mallocs and fills msg header */
struct ERRC_ERRDET_5 *make_messagehead_5(struct keyblock *kb);

/* function to prepare the first message head for a binary search. This assumes
   that all the parity buffers have been malloced and the remote parities
   reside in the proper arrays. This function will be called several times for
   different passes; it expexts local parities to be evaluated already.
   Arguments are a keyblock pointer, and a pass number. returns 0 on success,
   or an error code accordingly. */
int prepare_first_binsearch_msg(struct keyblock *kb, int pass);

#endif /* ECD2_COMMS */