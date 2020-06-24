/**
 * @file ecd2_qber_estim.h     
 * @brief code for QBER estimation.
 * 
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

/// \cond for doxygen annotation
#include <stdio.h>
/// \endcond

#include "defaultdefinitions.h"
#include "globalvars.h"
#include "keyblock.h"
#include "packets.h"
#include "proc_state.h"

#include "comms.h"
#include "debug.h"
#include "helpers.h"
#include "thread_mgmt.h"

/**
 * @brief 
 * 
 */
enum ReplyMode {
  replyMode_terminate = 0,
  replyMode_moreBits = 1,
  replyMode_continue = 2
};

/// @name ERROR ESTIMATION HELPER FUNCTIONS
/// @{
void prepare_paritylist1(struct keyblock *kb, unsigned int *d0, unsigned int *d1);
/// @}

/// @name ERROR ESTIMATION MAIN FUNCTIONS
/// @{
int testbits_needed(float e);
int errorest_1(unsigned int epoch);
int process_esti_message_0(char *receivebuf);
int send_more_esti_bits(char *receivebuf);
int prepare_dualpass(char *receivebuf);
/// @}

#endif /* ECD2_QBER_ESTIM */