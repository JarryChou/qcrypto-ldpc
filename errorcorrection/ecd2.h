/**
 * @file ecd2.h    
 * @brief Error correction demon. (modifications to original errcd see below).
 * 
 * Part of the quantum key distribution software for error
 *  correction and privacy amplification. Description
 *  see below. Version as of 20071228, works also for Ekert-91
 *  type protocols.
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
 *    Update 25/6/20: Unfortunately I don't have access to the flowchart, so I'll
 *    have to base my definitions off the code itself. I have thus decided to use
 *    intuitive names define the roles based on what they do in the system. Universal
 *    names can be adopted in the future in referring to these roles. 
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
 *   -E initialerr:         an initial error rate can be given for choosing the
 *                         length of the first test. Default is 0.05. This may
 *                         be overridden by a servoed quantity or by an explicit
 *                         statement in a command.
 *   -k                    remove_raw_keys_after_use option. If set, the raw key files will be
 *                         deleted after writing the final key into a file.
 *   -J intrinsicerr:      Error rate which is assumed to be generated outside the
 *                         influence of an eavesdropper.
 *   -T runtimeerrormode:  Determines the way how to react on errors which should
 *                         not stop the demon. Default is 0. detailed behavior:
 *                         0: terminate program on everything
 *                         1: ignore errors on wrong packets???
 *                         2: ignore errors inherited from other side
 *   -V verbosity_level:   Defines verbosity mode on the logging output after a
 *                         block has been processed. options:
 *                         0: just output the raw block name (epoch number in hex)
 *                         1: output the block name, number of final bits
 *                         2: output block name, num of initial bits, number of
 *                            final bits, error rate
 *                         3: same as 2, but in plain text
 *                         4: same as 2, but with explicit number of leaked bits
 *                            in the error correction procedure
 *                         5: same as 4, but with plain text comments
 *   -I                    skip_qber_estimation. If this option is on, the initial
 *                         error measurement for block optimization is skipped,
 *                         and the default value or supplied value is chosen. This
 *                         option should increase the efficiency of the key
 *                         regeneration if a servo for the error rate is on.
 *   -i                    bellmode (deviceindependent) option. If this option is set,
 *                         the deamon expects to receive a value for the Bell
 *                         violation parameter to estimate the knowledge of an
 *                         eavesdropper.
 *   -p                    disable_privacyamplification option. avoid privacy amplification. For debugging purposes, to
 *                         find the residual error rate
 *   -B biconf_BER:        choose the number of BICONF rounds to meet a final
 *                         bit error probability of BER. This assumes a residual
 *                         error rate of 10^-4 after the first two rounds.
 *   -b biconfRounds:     choose the number of BICONF rounds. Defaults to 10,
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
 *        introduce rawbuf variable to clean buffer in processblock struct (status?)
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
/// \cond for doxygen annotation
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
/// \endcond

// Definition header files
#include "definitions/defaultdefinitions.h"
#include "definitions/globalvars.h"

// ecd2 Components
#include "subcomponents/cascade_biconf.h"
#include "subcomponents/comms.h"
#include "subcomponents/helpers.h"
#include "subcomponents/qber_estim.h"
#include "subcomponents/processblock_mgmt.h"
#include "subcomponents/debug.h"
#include "subcomponents/priv_amp.h"


/// @name GLOBAL VARIABLES (defined in globalvar.h)
/// @{
/* structs */
ProcessBlockDequeNode *processBlockDeque = NULL;
struct packet_to_send *nextPacketToSend = NULL;
struct packet_to_send *lastPacketToSend = NULL;

/* global parameters and variables */
Arguments arguments = {
    DEFAULT_VERBOSITY,          // verbosity_level
    DEFAULT_RUNTIMEERRORMODE,   // runtimeerrormode
    DEFAULT_BICONF_ROUNDS,      // biconfRounds
    DEFAULT_ERR_MARGIN,         // errormargin
    DEFAULT_INITIAL_ERR,        // initialerr
    DEFAULT_INTRINSIC,          // intrinsicerr
    DEFAULT_KILLMODE,           // remove_raw_keys_after_use
    DEFAULT_ERR_SKIPMODE,       // skip_qber_estimation
    False,                      // bellmode
    False                       // disable_privacyamplification
                                // fname
                                // handle
                                // fhandle
};
/// @}

/// @name Static structs & variables (global only within the scope of ecd2)
/// @{
// Static variables in this case is used to reduce the need to pass variables between functions,
// Allowing for compartmentalized code while reducing pointer manipulation
static ReceivedPacketNode *receivedPacketLinkedList = NULL; /**< head node pointing to a simply joined list of entries */

static int input_last_index;          /**< For parsing cmd input */
static char *dpnt = NULL;             /**< For parsing cmd input */
static char *readbuf = NULL;          /**< pointer to temporary readbuffer storage */
static int sendIndex;                /**< for sending out packets */
static int receiveIndex;             /**< for receiving packets */
static EcPktHdr_Base msgprotobuf; /**< for reading header of receive packet */

static char *errormessage[] = {
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
    "error creating new processblock",
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
    "error reading in epochs in a processblock on bob side",
    "cannot get processblock for message 0",
    "cannot find processblock in list",
    "cannot find processblock for message 2", /* 50 */
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
    "in errorest1: cannot get processblock",
    "wrong pass index",
    "cmd input buffer overflow", /* 75 */
    "cannot parse biconf round argument",
    "biconf round number exceeds bounds of 1...100",
    "cannot parse final BER argument",
    "BER argument out of range",
    "Reply mode out of bounds", // 80
    "Unsupported functionality (See logs for more input)",
};
/// @}

// Helper enums

// HELPER FUNCTION DECLARATIONS
/* ------------------------------------------------------------------------- */
int emsg(int code);
int parse_options(int argc, char *argv[]);

// for fd_set and select syscalls
int has_pipe_event(fd_set* readqueue_ptr, fd_set* writequeue_ptr, int selectmax, Boolean hasContentToSend, struct timeval timeout);
// for writing data into sendpipe
int write_into_sendpipe();
// for reading command from cmdpipe into cmd_input
int read_from_cmdpipe(char* cmd_input);
// for processing command in cmd_input to create processblock & start qber estimation
int create_processblock_and_start_qber_using_cmd(char* dpnt, char* cmd_input);
/* process a command (e.g. epoch epochNum), terminated with \0 */
int process_command(char *in);      
// Read header from receive pipe
int read_header_from_receivepipe();
// Read body from  receive  pipe
int read_body_from_receivepipe();

// MAIN FUNCTION DECLARATION
/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[]);   

#endif /* ECD2_H */