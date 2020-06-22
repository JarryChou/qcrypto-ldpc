/**
 * defaultdefinitions.h     
 * Definition file for default variables in ecd2
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
 */

#ifndef ECD2_DEFAULTS
#define ECD2_DEFAULTS

// DEFINITIONS
/* ------------------------------------------------------------------------- */
/* default definitions */
#define FNAMELENGTH 200    /* length of file name buffers */
#define FNAMFORMAT "%200s" /* for sscanf of filenames */
#define DEFAULT_ERR_MARGIN 0. /* eavesdropper is assumed to have full knowledge on raw key */
#define MIN_ERR_MARGIN 0.          /* for checking error margin entries */
#define MAX_ERR_MARGIN 100.        /* for checking error margin entries */
#define DEFAULT_INIERR 0.075       /* initial error rate */
#define MIN_INI_ERR 0.005          /* for checking entries */
#define MAX_INI_ERR 0.14           /* for checking entries */
#define USELESS_ERRORBOUND 0.15    /* for estimating the number of test bits */
#define DESIRED_K0_ERROR 0.18      /* relative error on k */
#define INI_EST_SIGMA 2.           /* stddev on k0 for initial estimation */
#define DEFAULT_KILLMODE 0         /* no raw key files are removed by default */
#define DEFAULT_INTRINSIC 0        /* all errors are due to eve */
#define MAX_INTRINSIC 0.05         /* just a safe margin */
#define DEFAULT_RUNTIMEERRORMODE 0 /* all error s stop daemon */
#define MAXRUNTIMEERROR 2
#define FIFOINMODE O_RDWR | O_NONBLOCK
#define FIFOOUTMODE O_RDWR
#define FILEINMODE O_RDONLY
#define FILEOUTMODE O_WRONLY | O_CREAT | O_TRUNC
#define OUTPERMISSIONS 0600
#define TEMPARRAYSIZE (1 << 11) /* to capture 64 kbit of raw key */
#define MAXBITSPERTHREAD (1 << 16)
#define DEFAULT_VERBOSITY 0
#define DEFAULT_BICONF_LENGTH 256 /* length of a final check */
#define DEFAULT_BICONF_ROUNDS 10  /* number of BICONF rounds */
#define MAX_BICONF_ROUNDS 100     /* enough for BER < 10^-27 */
#define AVG_BINSEARCH_ERR 0.0032  /* what I have seen at some point for 10k. This goes with the inverse of the length for block lengths between 5k-40k */
#define DEFAULT_ERR_SKIPMODE 0 /* initial error estimation is done */
#define CMD_INBUFLEN 200

#endif