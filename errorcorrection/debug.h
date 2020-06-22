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

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

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

// ERROR MANAGEMENT

/* error handling */
char *errormessage[] = {
    "No error.",
    "Error reading in verbosity argument.", /* 1 */
    "Error reading name for command pipe.",
    "Error reading name for sendpipe.",
    "Error reading name for receive pipe.",
    "Error reading directory name for raw key.", /* 5 */
    "Error reading directory name for final key.",
    "Error reading name for notify pipe.",
    "Error reading name for query pipe.",
    "Error reading name for response-to-query pipe.",
    "Error parsing error threshold.", /* 10 */
    "Error threshold out of range (0.01...0.3)",
    "Error parsing initial error level",
    "Initial error level out of range (0.01...0.3)",
    "Error parsing intrinsic error level",
    "Intrinsic error level out of range (0...0.05)", /* 15 */
    "Error parsing runtime behavior (range must be 0..?)",
    "One of the pipelines of directories is not specified.",
    "Cannot stat or open command handle",
    "command handle is not a pipe",
    "Cannot stat/open send pipe", /* 20 */
    "send pipe is not a pipe",
    "Cannot stat/open receive pipe",
    "receive pipe is not a pipe",
    "Cannot open notify target",
    "Cannot stat/open query input pipe", /* 25 */
    "query intput channel is not a pipe",
    "Cannot open query response pipe",
    "select call failed in main loop",
    "error writing to target pipe",
    "command set to short", /* 30 */
    "estimated error out of range",
    "wrong number of epochs specified.",
    "overlap with existing epochs",
    "error creating new thread",
    "error initiating error estimation", /* 35 */
    "error reading message",
    "cannot malloc message buffer",
    "cannot malloc message buffer header",
    "cannot open random number generator",
    "cannot get enough random numbers", /* 40 */
    "initial error out of useful bound",
    "not enough bits for initial testing",
    "cannot malloc send buffer pointer",
    "received wrong packet type",
    "received unrecognized message subtype", /* 45 */
    "epoch overlap error on bob side",
    "error reading in epochs in a thread on bob side",
    "cannot get thread for message 0",
    "cannot find thread in list",
    "cannot find thread for message 2", /* 50 */
    "received invalid seed.",
    "inconsistent test-bit number received",
    "can't malloc parity buffer",
    "cannot malloc difference index buffer",
    "cannot malloc binarysearch message buf", /* 55 */
    "illegal role in binsearch",
    "don't know index encoding",
    "cannot malloc binarysearch message II buf",
    "illegal pass argument",
    "cannot malloc BCONF request message", /* 60 */
    "cannot malloc BICONF response message",
    "cannot malloc privamp message",
    "cannot malloc final key structure",
    "cannot open final key target file",
    "write error in fnal key", /* 65 */
    "cannot remove raw key file",
    "cannot open raw key file",
    "cannot read rawkey header",
    "incorrect epoch in rawkey",
    "wrong bitnumber in rawkey (must be 1)", /* 70 */
    "bitcount too large in rawkey",
    "could not read enough bytes from rawkey",
    "in errorest1: cannot get thread",
    "wrong pass index",
    "cmd input buffer overflow", /* 75 */
    "cannot parse biconf round argument",
    "biconf round number exceeds bounds of 1...100",
    "cannot parse final BER argument",
    "BER argument out of range",
};

#endif /* ECD2_DEBUG */