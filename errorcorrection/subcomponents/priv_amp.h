/**
 * @file priv_amp.h   
 * @brief privacy amplification portions of the code
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
 * used for privacy amplification.
 * 
 */

#ifndef ECD2_PRIV_AMP
#define ECD2_PRIV_AMP

// Libraries
/// \cond for doxygen annotation
#include <stdio.h>
#include <fcntl.h>
/// \endcond

// Definition header files
#include "../definitions/defaultdefinitions.h"
#include "../definitions/globalvars.h"
#include "../definitions/processblock.h"
#include "../definitions/packets.h"
#include "../../packetheaders/pkt_header_7.h"

// Other components
#include "comms.h"
#include "helpers.h"
#include "debug.h"
#include "processblock_mgmt.h"

/// @name PRIVACY AMPLIFICATION HELPER FUNCTIONS
/// @{
float phi(float z);
float binentrop(float q);
/// @}

/// @name PRIVACY AMPLIFICATION MAIN FUNCTIONS
/// @{
int initiate_privacyamplification(ProcessBlock *kb);
int receive_privamp_msg(char *receivebuf);
int do_privacy_amplification(ProcessBlock *kb, unsigned int seed, int lostbits);
/// @}

#endif /* ECD2_PRIV_AMP */