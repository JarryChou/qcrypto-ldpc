/**
 * debug.h   
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
 * code. The ecd2_debug section modularizes the parts of the code that is predominantly
 * used for debugging purposes.
 * 
 */

#ifndef ECD2_DEBUG
#define ECD2_DEBUG

// Libraries
/// \cond for doxygen annotation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
/// \endcond

// Definitions
#include "keyblock.h"

// Definitions to enable certain debugging parts of the code
#define DEBUG 1
/* #define SYSTPERMUTATION */ /* for systematic rather than rand permut */
/* #define mallocdebug */

// DEBUGGER HELPER FUNCTIONS
/* ------------------------------------------------------------------------- */
/* malloc wrapper to include debug logs if needed */
char *malloc2(unsigned int s);

/* free wrapper to include debug logs if needed */
void free2(void *p);

/* helper to dump message into a file */
void dumpmsg(struct keyblock *kb, char *msg);

/* helper function to dump the state of the system to a disk file . 
 Dumps the keyblock structure, if present the buffer files, 
 the parity files and the diffidx buffers as plain binaries */
void dumpstate(struct keyblock *kb);

/* for debug: output permutation */
void output_permutation(struct keyblock *kb);

#endif /* ECD2_DEBUG */