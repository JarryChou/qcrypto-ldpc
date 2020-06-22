/**
 * globalvars.h     
 * Definition file for global variables in ecd2
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
 * This is a definition file for global variables used in ecd2.
 * These variables are initialized in ecd2.h.
 * 
 * Some of these variables can actually be passed in as a param,
 * but currently kept as globalvars to retain consistency with
 * previous coding style
 */

#ifndef ECD2_GLOBALS
#define ECD2_GLOBALS

#include <stdio.h>

#include "defaultdefinitions.h"
#include "packets.h"

// Used by thread_mgmt
extern int killmode;   /* decides on removal of raw key files */
extern struct blockpointer *blocklist;

// Used by comms
extern struct packet_to_send *next_packet_to_send;
extern struct packet_to_send *last_packet_to_send;

// Used by qber_estim
extern int ini_err_skipmode; /* 1 if error est to be skipped */

// Used by cascade_biconf
extern int biconf_rounds;

// File handle(s) & meta info used across the application for I/O
extern char fname[8][FNAMELENGTH]; /* filenames */
extern int handle[8];    /* handles for files accessed by raw I/O */
extern FILE *fhandle[8]; /* handles for files accessed by buffered I/O */

// Used by priv_amp
extern int bellmode;
extern int disable_privacyamplification;
extern float errormargin;
extern float intrinsicerr;
extern int verbosity_level;

#endif
