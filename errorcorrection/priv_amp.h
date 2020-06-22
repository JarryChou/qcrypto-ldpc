/**
 * priv_amp.h     
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
 * used for privacy amplification.
 * 
 */

#ifndef ECD2_PRIV_AMP
#define ECD2_PRIV_AMP

// Libraries
#include <stdio.h>
#include <fcntl.h>

// Definition header files
#include "defaultdefinitions.h"
#include "globalvars.h"
#include "keyblock.h"
#include "packets.h"
#include "../packetheaders/pkt_header_7.h"

// Other components
#include "comms.h"
#include "helpers.h"
#include "debug.h"
#include "thread_mgmt.h"

// PRIVACY AMPLIFICATION
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS

// MAIN FUNCTIONS
/* function to initiate the privacy amplification. Sends out a message with
   a PRNG seed (message 8), and hand over to the core routine for the PA.
   Parameter is keyblock, return is error or 0 on success. */
int initiate_privacyamplification(struct keyblock *kb);

/* function to process a privacy amplification message. parameter is incoming
   message, return value is 0 or an error code. Parses the message and passes
   the real work over to the do_privacyamplification part */
int receive_privamp_msg(char *receivebuf);

/* do core part of the privacy amplification. Calculates the compression ratio
   based on the lost bits, saves the final key and removes the thread from the
   list.    */
int do_privacy_amplification(struct keyblock *kb, unsigned int seed, int lostbits);

#endif /* ECD2_PRIV_AMP */