/**
 * ecd2_thread_mgmt.h     
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
 * used for thread management.
 * 
 */

#ifndef ECD2_THREAD_MGMT
#define ECD2_THREAD_MGMT

// Libraries
/// \cond for doxygen annotation
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
/// \endcond

#include "defaultdefinitions.h"
#include "globalvars.h"
#include "proc_state.h"
#include "keyblock.h"
#include "packets.h"
#include "../packetheaders/pkt_header_3.h"
#include "../packetheaders/pkt_header_7.h"

#include "debug.h"
#include "helpers.h"

// THREAD MANAGEMENT
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* code to check if a requested bunch of epochs already exists in the thread list. 
  Uses the start epoch and an epoch number as arguments; 
  returns 0 if the requested epochs are not used yet, otherwise 1. */
int check_epochoverlap(unsigned int epoch, int num);

// MAIN FUNCTIONS
/* code to prepare a new thread from a series of raw key files. Takes epoch,
   number of epochs and an initially estimated error as parameters. Returns
   0 on success, and 1 if an error occurred (maybe later: errorcode) 
   
   Note: uses globalvar killmode
   */
int create_thread(unsigned int epoch, int num, float inierr, float BellValue);

/* function to obtain the pointer to the thread for a given epoch index.
   Argument is epoch, return value is pointer to a struct keyblock or NULL
   if none found. */
struct keyblock *get_thread(unsigned int epoch);

/* function to remove a thread out of the list. parameter is the epoch index,
   return value is 0 for success and 1 on error. This function is called if
   there is no hope for error recovery or for a finished thread. */
int remove_thread(unsigned int epoch);

#endif /* ECD2_THREAD_MGMT */