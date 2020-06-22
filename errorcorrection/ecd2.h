/**
 * ecd2.h    
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below. Version as of 20071228, works also for Ekert-91
 *  type protocols.
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
 * --
 * 
 *    Error correction demon. (modifications to original errcd see below).
 * 
 *    Runs in the background and performs a cascade
 *    error correction algorithm on a block of raw keys. Communication with a
 *    higher level controller is done via a command pipeline, and communication
 *    with the other side is done via packets which are sent and received via
 *    pipes in the filesystem. Some parameters (connection locations,
 *    destinations) are communicated via command line parameters, others are sent
 *    via the command interface. The program is capable to handle several blocks
 *    simultaneously, and to connect corresponding messages to the relevant
 *    blocks.
 *    The final error-corrected key is stored in a file named after the first
 *    epoch. If a block processing is requested on one side, it will fix the role
 *    to "Alice", and the remote side will be the "bob" which changes the bits
 *    accordingly. Definitions according to the flowchart in DSTA deliverable D3
 * 
 * usage:
 * 
 *   errcd -c commandpipe -s sendpipe -r receivepipe
 *         -d rawkeydirectory -f finalkeydirectory
 *         -l notificationpipe
 *         -q responsepipe -Q querypipe
 *         [ -e errormargin ]
 *         [ -E expectederror ]
 *         [ -k ]
 *         [ -J basicerror ]
 *         [ -T errorbehaviour ]
 *         [ -V verbosity ]
 *         [ -I ]
 *         [ -i ]
 *         [ -p ]
 *         [ -B BER | -b rounds ]
 * 
 * options/parameters:
 * 
 *  DIRECTORY / CONNECTION PARAMETERS:
 * 
 *   -c commandpipe:       pipe for receiving commands. A command is typically an
 *                         epoch name and a number of blocks to follow, separated
 *                         by a whitespace. An optional error argument can be
 *                         passed as a third parameter. Commands are read via
 *                         fscanf, and should be terminated with a newline.
 *   -s sendpipe:          binary connection which reaches to the transfer
 *                         program. This is for packets to be sent out to the
 *                         other side. Could be replaced by sockets later.
 *   -r receivepipe:       same as sendpipe, but for incoming packets.
 *   -d rawkeydirectory:   directory which contains epoch files for raw keys in
 *                         stream-3 format
 *   -f finalkeydirectory: Directory which contains the final key files.
 *   -l notificationpipe:  whenever a final key block is processed, its epoch name
 *                         is written into this pipe or file. The content of the
 *                         message is determined by the verbosity flag.
 *   -Q querypipe:         to request the current status of a particular epoch
 *                         block, requests may be sent into this pipe. Syntax TBD.
 *   -q respondpipe:       Answers to requests will be written into this pipe or
 *                         file.
 * 
 *  CONTROL OPTIONS:
 * 
 *   -e errormargin:       A float parameter for how many standard deviations
 *                         of the detected errors should be added to the
 *                         information leakage estimation to eve, assuming a
 *                         poissonian statistics on the found errors (i.e.,
 *                         if 100 error bits are found, one standard deviation
 *                         in the error rate QBER is QBER /Sqrt(10). )
 *                         Default is set to 0.
 *   -E expectederror:     an initial error rate can be given for choosing the
 *                         length of the first test. Default is 0.05. This may
 *                         be overridden by a servoed quantity or by an explicit
 *                         statement in a command.
 *   -k                    killfile option. If set, the raw key files will be
 *                         deleted after writing the final key into a file.
 *   -J basicerror:        Error rate which is assumed to be generated outside the
 *                         influence of an eavesdropper.
 *   -T errorbehavior:     Determines the way how to react on errors which should
 *                         not stop the demon. Default is 0. detailed behavior:
 *                         0: terminate program on everything
 *                         1: ignore errors on wrong packets???
 *                         2: ignore errors inherited from other side
 *   -V verbosity:         Defines verbosity mode on the logging output after a
 *                         block has been processed. options:
 *                         0: just output the raw block name (epoch number in hex)
 *                         1: output the block name, number of final bits
 *                         2: output block name, num of initial bits, number of
 *                            final bits, error rate
 *                         3: same as 2, but in plain text
 *                         4: same as 2, but with explicit number of leaked bits
 *                            in the error correction procedure
 *                         5: same as 4, but with plain text comments
 *   -I                    ignoreerroroption. If this option is on, the initial
 *                         error measurement for block optimization is skipped,
 *                         and the default value or supplied value is chosen. This
 *                         option should increase the efficiency of the key
 *                         regeneration if a servo for the error rate is on.
 *   -i                    deviceindependent option. If this option is set,
 *                         the deamon expects to receive a value for the Bell
 *                         violation parameter to estimate the knowledge of an
 *                         eavesdropper.
 *   -p                    avoid privacy amplification. For debugging purposes, to
 *                         find the residual error rate
 *   -B BER:               choose the number of BICONF rounds to meet a final
 *                         bit error probability of BER. This assumes a residual
 *                         error rate of 10^-4 after the first two rounds.
 *   -b rounds:            choose the number of BICONF rounds. Defaults to 10,
 *                         corresponding to a BER of 10^-7.
 * 
 * 
 * History: first specs 17.9.05chk
 * 
 * status 01.4.06 21:37; runs through and leaves no errors in final key.
 *        1.5.06 23:20: removed biconf indexing bugs & leakage errors
 *        2.5.06 10:14  fixed readin problem with word-aligned lengths
 *        3.5.06 19:00 does not hang over 300 calls
 *        28.10.06     fixed sscanf to read in epochs >0x7fffffff
 *        14.07.07     logging leaked bits, verbosity options 4+5
 *        9.-18.10.07      fixed Bell value transmission for other side
 *        24.10.08         fixed error estimation for BB84
 * 
 * 
 *        introduce rawbuf variable to clean buffer in keyblock struct (status?)
 * 
 * modified version of errcd to take care of the followig problems:
 *    - initial key permutation
 *    - more efficient biconf check
 *    - allow recursive correction after biconf error discoveries
 *     status: seems to work. needs some cleanup, and needs to be tested for
 *       longer key lenghts to confirm the BER below 10^-7 with some confidence.
 *       (chk 21.7.07)
 *       - inserted error margin option to allow for a few std deviations of the
 *       detected error
 * 
 * open questions / issues:
 *    check assignment of short indices for bit length....
 *    check consistency of processing status
 *    get a good RNG source or recycle some bits from the sequence....currently
 *      it uses urandom as a seed source.
 *    The pseudorandom generator in this program is a Gold sequence and possibly
 *      dangerous due to short-length correlations - perhaps something better?
 *    should have more consistency tests on packets
 *    still very chatty on debug information
 *    query/response mechanism not implemented yet
 * 
 */

#ifndef ECD2_H
#define ECD2_H

// Libraries
#include <getopt.h>

// Definition header files
#include "defaultdefinitions.h"
#include "globalvars.h"

// ecd2 Components
#include "cascade_biconf.h"
#include "comms.h"
#include "helpers.h"
#include "qber_estim.h"
#include "thread_mgmt.h"
#include "debug.h"
#include "errmsg.h"
#include "priv_amp.h"

// GLOBAL VARIABLES (defined in globalvar.h)
/* ------------------------------------------------------------------------- */

/* structs */
blocklist = NULL;
next_packet_to_send = NULL;
last_packet_to_send = NULL;

/* global parameters and variables */
int biconf_rounds = DEFAULT_BICONF_ROUNDS;   /* how many times */

int ini_err_skipmode = DEFAULT_ERR_SKIPMODE; /* 1 if error est to be skipped */

int killmode = DEFAULT_KILLMODE;   /* decides on removal of raw key files */

char fname[8][FNAMELENGTH] = {"", "", "", "", "", "", "", ""}; /* filenames */
int handle[8];    /* handles for files accessed by raw I/O */
FILE *fhandle[8]; /* handles for files accessed by buffered I/O */

int bellmode = 0; /* 0: use estimated error, 1: use supplied bell value */
int disable_privacyamplification = 0; /* off normally, != 0 for debugging */
float errormargin = DEFAULT_ERR_MARGIN;
float intrinsicerr = DEFAULT_INTRINSIC; /* error rate not under control of eve */
int verbosity_level = DEFAULT_VERBOSITY;


// Static structs & variables (global only within the scope of ecd2)
/* ------------------------------------------------------------------------- */

/* head node pointing to a simply joined list of entries */
struct packet_received *rec_packetlist = NULL;

float initialerr = DEFAULT_INIERR; /* What error to assume initially */
int runtimeerrormode = DEFAULT_RUNTIMEERRORMODE;
int biconf_length = DEFAULT_BICONF_LENGTH;   /* how long is a biconf */

// ENUMS
/* ------------------------------------------------------------------------- */

// MAIN FUNCTION DECLARATIONS (OTHERS)
/* ------------------------------------------------------------------------- */
/* process a command (e.g. epoch epochNum), terminated with \0 */
int process_command(char *in);

/* main code */
int main(int argc, char *argv[]);

#endif /* ECD2_H */