/**
 * @file comms.h  
 * @brief code on communications w.r.t. error correction.
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
#include "../definitions/proc_state.h"
#include "../definitions/keyblock.h"
#include "../definitions/packets.h"
#include "../definitions/globalvars.h"

// Other components
#include "debug.h"
#include "helpers.h"

/// @name COMMUNICATIONNS HELPER FUNCTIONS
/// @{
int insert_sendpacket(char *message, int length);
struct ERRC_ERRDET_0 *fillsamplemessage(struct keyblock *kb, int bitsneeded, int errormode, float BellValue);
struct ERRC_ERRDET_5 *make_messagehead_5(struct keyblock *kb);
int prepare_first_binsearch_msg(struct keyblock *kb, int pass);
/// @}

#endif /* ECD2_COMMS */