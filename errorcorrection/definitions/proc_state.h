/**
 * @file proc_state.h     
 * @brief Defines processing states
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
 * used for defining processing state.
 * 
 */

#ifndef ECD2_PROC_STATE
#define ECD2_PROC_STATE

/// @name Definitions of the processing state
/// @{
#define PSTATE_JUST_LOADED 0                    /**< no processing yet (passive role) */
// #define PSTATE_NEGOTIATING_ROLE 1               /**< in role negotiation with other side (UNUSED) */
#define PSTATE_AWAIT_ERR_EST_RESP 2             /**< waiting for error est response from bob */
#define PSTATE_AWAIT_ERR_EST_MORE_BITS 3        /**< waiting for more error est bits from Alice */
#define PSTATE_ERR_KNOWN 4                      /**< know my error estimation */
#define PSTATE_PERFORMED_PARITY 5               /**< know my error estimation */
#define PSTATE_DOING_BICONF 6                   /**< last biconf round */
/// @}

#endif