/**
 * ecd2.c     
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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
/* definitions of packet headers */
#include "errcorrect.h"
#include "rnd.h"

// DEBUGGING STUFF
/* ------------------------------------------------------------------------- */
/* #define SYSTPERMUTATION */ /* for systematic rather than rand permut */
/* #define mallocdebug */

#define NOT_DEBUG 1
#undef NOT_DEBUG
#define DEBUG 1

// DEFINITIONS
/* ------------------------------------------------------------------------- */
/* typical file names */
#define RANDOMGENERATOR "/dev/urandom" /* this does not block but is BAD....*/

/* definition of stream-3 and stream-7 */
#define TYPE_3_TAG 3
#define TYPE_3_TAG_U 0x103
#define TYPE_7_TAG 7
#define TYPE_7_TAG_U 0x107

/* definition of the processing state */
#define PRS_JUSTLOADED 0       /* no processing yet (passive role) */
#define PRS_NEGOTIATEROLE 1    /* in role negotiation with other side */
#define PRS_WAITRESPONSE1 2    /* waiting for error est response from bob */
#define PRS_GETMOREEST 3       /* waiting for more error est bits from Alice */
#define PRS_KNOWMYERROR 4      /* know my error estimation */
#define PRS_PERFORMEDPARITY1 5 /* know my error estimation */
#define PRS_DOING_BICONF 6     /* last biconf round */

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

// STRUCTS
/* ------------------------------------------------------------------------- */
/* type declaration for stream-3 raw key files; should come from extra h file */
typedef struct header_3 { /* header for type-3 stream packet */
  int tag;
  unsigned int epoc;
  unsigned int length;
  int bitsperentry;
} h3;

/* type declaration for stream-7 final key file */
typedef struct header_7 { /* header for type-7 stream packet */
  int tag;
  unsigned int epoc;
  unsigned int numberofepochs;
  int numberofbits;
} h7;

/* definition of a structure containing all informations about a block */
typedef struct keyblock {
  unsigned int startepoch; /* initial epoch of block */
  unsigned int numberofepochs;
  unsigned int *rawmem;     /* buffer root for freeing block afterwards */
  unsigned int *mainbuf;    /* points to main buffer for key */
  unsigned int *permutebuf; /* keeps permuted bits */
  unsigned int *testmarker; /* marks tested bits */
  unsigned short int *permuteindex; /* keeps permutation */
  unsigned short int *reverseindex; /* reverse permutation */
  int role;        /* defines which role to take on a block: 0: Alice, 1: Bob */
  int initialbits; /* bits to start with */
  int leakagebits; /* information which has gone public */
  int processingstate;     /* determines processing status  current block. See defines below for interpretation */
  int initialerror;        /* in multiples of 2^-16 */
  int errormode;           /* determines if error estimation has to be done */
  int estimatederror;      /* number of estimated error bits */
  int estimatedsamplesize; /* sample size for error  estimation  */
  int finalerrors;         /* number of discovered errors */
  int RNG_usage; /* defines mode of randomness. 0: PRNG, 1: good stuff */
  unsigned int RNG_state; /* keeps the state of the PRNG for this thread */
  int k0, k1;             /* binary block search lengths */
  int workbits;           /* bits to work with for BICONF/parity check */
  int partitions0, partitions1; /* number of partitions of k0,k1 length */
  unsigned int *lp0, *lp1;      /* pointer to local parity info 0 / 1 */
  unsigned int *rp0, *rp1;      /* pointer to remote parity info 0 / 1 */
  unsigned int *pd0, *pd1;      /* pointer to parity difference fileld */
  int diffnumber;         /* number of different blocks in current round */
  int diffnumber_max;     /* number of malloced entries for diff indices */
  unsigned int *diffidx;  /* pointer to a list of parity mismatch blocks */
  unsigned int *diffidxe; /* end of interval */
  int binsearch_depth;    /* encodes state of the scan. Starts with 0, and contains the pass (0/1) in the MSB */
  int biconf_round;    /* contains the biconf round number, starting with 0 */
  int biconflength;    /* current length of a biconf check range */
  int correctederrors; /* number of corrected bits */
  int finalkeybits;    /* how much is left */
  float BellValue;     /* for Ekert-type protocols */
} kblock;

/* structure to hold list of blocks. This helps dispatching packets? */
typedef struct blockpointer {
  unsigned int epoch;
  struct keyblock *content;      /* the gory details */
  struct blockpointer *next;     /* next in chain; if NULL then end */
  struct blockpointer *previous; /* previous block */
} erc_bp__;
struct blockpointer *blocklist = NULL;

/* structure which holds packets to send */
typedef struct packet_to_send {
  int length;                  /* in bytes */
  char *packet;                /* pointer to content */
  struct packet_to_send *next; /* next one to send */
} pkt_s;
struct packet_to_send *next_packet_to_send = NULL;
struct packet_to_send *last_packet_to_send = NULL;

/* structure to hold received messages */
typedef struct packet_received {
  int length;                   /* in bytes */
  char *packet;                 /* pointer to content */
  struct packet_received *next; /* next in chain */
} pack_r;

/* head node pointing to a simply joined list of entries */
struct packet_received *rec_packetlist = NULL;

// GLOBAL VARIABLES
/* ------------------------------------------------------------------------- */
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

/* global parameters and variables */
char fname[8][FNAMELENGTH] = {"", "", "", "", "", "", "", ""}; /* filenames */
int handle[8];    /* handles for files accessed by raw I/O */
FILE *fhandle[8]; /* handles for files accessed by buffered I/O */
float errormargin = DEFAULT_ERR_MARGIN;
float initialerr = DEFAULT_INIERR; /* What error to assume initially */
int killmode = DEFAULT_KILLMODE;   /* decides on removal of raw key files */
float intrinsicerr = DEFAULT_INTRINSIC; /* error rate not under control of eve */
int runtimeerrormode = DEFAULT_RUNTIMEERRORMODE;
int verbosity_level = DEFAULT_VERBOSITY;
int biconf_length = DEFAULT_BICONF_LENGTH;   /* how long is a biconf */
int biconf_rounds = DEFAULT_BICONF_ROUNDS;   /* how many times */
int ini_err_skipmode = DEFAULT_ERR_SKIPMODE; /* 1 if error est to be skipped */
int disable_privacyamplification = 0; /* off normally, != 0 for debugging */
int bellmode = 0; /* 0: use estimated error, 1: use supplied bell value */

// ENUMS
/* ------------------------------------------------------------------------- */
enum HandleId {
  handleId_commandPipe = 0,
  handleId_sendPipe = 1,
  handleId_receivePipe = 2,
  handleId_rawKeyDir = 3,
  handleId_finalKeyDir = 4,
  handleId_notifyPipe = 5,
  handleId_queryPipe = 6,
  handleId_queryRespPipe = 7
};

enum ReplyMode {
  replyMode_terminate = 0,
  replyMode_moreBits = 1,
  replyMode_continue = 2
};

// FUNCTIONS RECOMMENDED TO THE COMPILER FOR INLINE
// (What is inline? See https://stackoverflow.com/questions/2082551/what-does-inline-mean)
/* ------------------------------------------------------------------------- */
/* helper for mask for a given index i on the longint array */
__inline__ unsigned int bt_mask(int i) { return 1 << (31 - (i & 31)); }
/* helper function for parity isolation */
__inline__ unsigned int firstmask(int i) { return 0xffffffff >> i; }
__inline__ unsigned int lastmask(int i) { return 0xffffffff << (31 - i); }

// DEFINED FUNCTIONS
/* ------------------------------------------------------------------------- */
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) > (B) ? (B) : (A))

// HELPER FUNCTION DECLARATIONS
/* ------------------------------------------------------------------------- */
// GENERAL HELPER FUNCTIONS
int emsg(int code);

/* helper to obtain the smallest power of two to carry a number a */
int get_order(int a);

/* get the number of bits necessary to carry a number x ; 
  result is e.g. 3 for parameter 8, 5 for parameter 17 etc. */
int get_order_2(int x); 

/* helper: count the number of set bits in a longint */
int count_set_bits(unsigned int a);

/* helper for name. adds a slash, hex file name and a terminal 0 */
void atohex(char *target, unsigned int v);

/* helper: eve's error knowledge */
float phi(float z);
float binentrop(float q);

/* helper function to get a seed from the random device; returns seed or 0 on error */
unsigned int get_r_seed(void);

/* helper function to compress key down in a sinlge sequence to eliminate the
   revealeld bits. updates workbits accordingly, and reduces number of
   revealed bits in the leakage_bits_counter  */
void cleanup_revealed_bits(struct keyblock *kb);

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

// COMMUNICATIONS
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper to insert a send packet in the sendpacket queue. 
  Parameters are a pointer to the structure and its length. 
  Return value is 0 on success or !=0 on malloc failure */
int insert_sendpacket(char *message, int length);

/* helper function to prepare a message containing a given sample of bits.
   parameters are a pointer to the thread, the number of bits needed and an
   errorormode (0 for normal error est, err*2^16 forskip ). returns a pointer
   to the message or NULL on error.
   Modified to tell the other side about the Bell value for privacy amp in
   the device indep mode
*/
struct ERRC_ERRDET_0 *fillsamplemessage(struct keyblock *kb, int bitsneeded, int errormode, float BellValue);

/* helper function for binsearch replies; mallocs and fills msg header */
struct ERRC_ERRDET_5 *make_messagehead_5(struct keyblock *kb);

/* function to prepare the first message head for a binary search. This assumes
   that all the parity buffers have been malloced and the remote parities
   reside in the proper arrays. This function will be called several times for
   different passes; it expexts local parities to be evaluated already.
   Arguments are a keyblock pointer, and a pass number. returns 0 on success,
   or an error code accordingly. */
int prepare_first_binsearch_msg(struct keyblock *kb, int pass);

// THREAD MANAGEMENT
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* code to check if a requested bunch of epochs already exists in the thread list. 
  Uses the start epoch and an epoch number as arguments; 
  returns 0 if the requested epochs are not used yet, otherwise 1. */
int check_epochoverlap(unsigned int epoch, int num);

// MAIN FUNCTIONS
/* code to prepare a new thread from a series of raw key files. Takes epoch,
   number of epochs and an initially estimated error as parameters. Returns
   0 on success, and 1 if an error occurred (maybe later: errorcode) */
int create_thread(unsigned int epoch, int num, float inierr, float BellValue);

/* function to obtain the pointer to the thread for a given epoch index.
   Argument is epoch, return value is pointer to a struct keyblock or NULL
   if none found. */
struct keyblock *get_thread(unsigned int epoch);

/* function to remove a thread out of the list. parameter is the epoch index,
   return value is 0 for success and 1 on error. This function is called if
   there is no hope for error recovery or for a finished thread. */
int remove_thread(unsigned int epoch);


// ERROR ESTIMATION
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
// MAIN FUNCTIONS
/* function to provide the number of bits needed in the initial error
   estimation; eats the local error (estimated or guessed) as a float. Uses
   the maximum for either estimating the range for k0 with the desired error,
   or a sufficient separation from the no-error-left area. IS that fair?
   Anyway, returns a number of bits. */
int testbits_needed(float e);

/* function to initiate the error estimation procedure. parameter is
   statrepoch, return value is 0 on success or !=0 (w error encoding) on error.
 */
int errorest_1(unsigned int epoch);

/* function to process the first error estimation packet. Argument is a pointer
   to the receivebuffer with both the header and the data section. Initiates
   the error estimation, and prepares the next  package for transmission.
   Currently, it assumes only PRNG-based bit selections.

   Return value is 0 on success, or an error message useful for emsg.

*/
int process_esti_message_0(char *receivebuf);

/* function to reply to a request for more estimation bits. Argument is a
   pointer to the receive buffer containing the message. Just sends over a
   bunch of more estimaton bits. Currently, it uses only the PRNG method.

   Return value is 0 on success, or an error message otherwise. */
int send_more_esti_bits(char *receivebuf);

/* function to proceed with the error estimation reply. Estimates if the
   block deserves to be considered further, and if so, prepares the permutation
   array of the key, and determines the parity functions of the first key.
   Return value is 0 on success, or an error message otherwise. */
int prepare_dualpass(char *receivebuf);

// PERMUTATIONS
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper to fix the permuted/unpermuted bit changes; decides via a parameter
   in kb->binsearch_depth MSB what polarity to take */
void fix_permutedbits(struct keyblock *kb);

/* helper function to do generate the permutation array in the kb structure.
   does also re-ordering (in future), and truncates the discussed key to a
   length of multiples of k1so there are noleftover bits in the two passes.
   Parameter: pointer to kb structure */
void prepare_permutation(struct keyblock *kb);

// MAIN FUNCTIONS
/* permutation core function; is used both for biconf and initial permutation */
void prepare_permut_core(struct keyblock *kb);

// CASCADE BICONF
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS
/* helper function to preare a parity list of a given pass in a block.
   Parameters are a pointer to the sourcebuffer, pointer to the target buffer,
   and an integer arg for the blocksize to use, and the number of workbits */
void prepare_paritylist_basic(unsigned int *d, unsigned int *t, int k, int w);

/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer. parameters are a keyblock pointer, and a seed for the RNG.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_selectbitstring(struct keyblock *kb, unsigned int seed);

/* helper function to generate a pseudorandom bit pattern into the test bit
   buffer AND transfer the permuted key buffer into it for a more compact
   parity generation in the last round.
   Parameters are a keyblock pointer.
   the rest is extracted out of the kb structure (for final parity test) */
void generate_BICONF_bitstring(struct keyblock *kb);

/* helper function to preare a parity list of a given pass in a block, compare
   it with the received list and return the number of differing bits  */
int do_paritylist_and_diffs(struct keyblock *kb, int pass);

/* helper function to prepare parity lists from original and unpermutated key.
   arguments are a pointer to the thread structure, a pointer to the target
   parity buffer 0 and another pointer to paritybuffer 1. No return value,
   as no errors are tested here. */
void prepare_paritylist1(struct keyblock *kb, unsigned int *d0, unsigned int *d1);

/* helper program to half parity difference intervals ; takes kb and inh_index
   as parameters; no weired stuff should happen. return value is the number
   of initially dead intervals */
void fix_parity_intervals(struct keyblock *kb, unsigned int *inh_idx)

/* helper for correcting one bit in pass 0 or 1 in their field */
void correct_bit(unsigned int *d, int bitindex);

/* helper funtion to get a simple one-line parity from a large string.
   parameters are the string start buffer, a start and an enx index. returns
   0 or 1 */
int single_line_parity(unsigned int *d, int start, int end);

/* helper funtion to get a simple one-line parity from a large string, but
   this time with a mask buffer to be AND-ed on the string.
   parameters are the string buffer, mask buffer, a start and and end index.
   returns  0 or 1 */
int single_line_parity_masked(unsigned int *d, unsigned int *m, int start, int end);

// MAIN FUNCTIONS
/* function to process a binarysearch request on alice identity. Installs the
   difference index list in the first run, and performs the parity checks in
   subsequent runs. should work with both passes now
   - work in progress, need do fix bitloss in last round
 */
int process_binsearch_alice(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head);

/* function to initiate a BICONF procedure on Bob side. Basically sends out a
   package calling for a BICONF reply. Parameter is a thread pointer, and
   the return value is 0 or an error code in case something goes wrong.   */
int initiate_biconf(struct keyblock *kb);

/* start the parity generation process on Alice side. parameter contains the
   input message. Reply is 0 on success, or an error message. Should create
   a BICONF response message */
int generate_biconfreply(char *receivebuf);

/* function to generate a single binary search request for a biconf cycle.
   takes a keyblock pointer and a length of the biconf block as a parameter,
   and returns an error or 0 on success.
   Takes currently the subset of the biconf subset and its complement, which
   is not very efficient: The second error could have been found using the
   unpermuted short sample with nuch less bits.
   On success, a binarysearch packet gets emitted with 2 list entries. */
int initiate_biconf_binarysearch(struct keyblock *kb, int biconflength);

/* function to proceed with the parity evaluation message. This function
   should start the Binary search machinery.
   Argument is receivebuffer as usual, returnvalue 0 on success or err code.
   Should spit out the first binary search message */

int start_binarysearch(char *receivebuf);

/* function to process a binarysearch request. distinguishes between the two
   symmetries in the evaluation. This is onyl a wrapper.
   on alice side, it does a passive check; on bob side, it possibly corrects
   for errors. */
int process_binarysearch(char *receivebuf);

/* function to process a binarysearch request on bob identity. Checks parity
   lists and does corrections if necessary.
   initiates the next step (BICONF on pass 1) for the next round if ready.
*/
int process_binsearch_bob(struct keyblock *kb, struct ERRC_ERRDET_5 *in_head);

/* start the parity generation process on bob's side. Parameter contains the
   parity reply form Alice. Reply is 0 on success, or an error message.
   Should either initiate a binary search, re-issue a BICONF request or
   continue to the parity evaluation. */
int receive_biconfreply(char *receivebuf);

// PRIVACY AMPLIFICATION
/* ------------------------------------------------------------------------- */
// HELPER FUNCTIONS

// MAIN FUNCTIONS
/* function to initiate the privacy amplification. Sends out a message with
   a PRNG seed (message 8), and hand over to the core routine for the PA.
   Parameter is keyblock, return is error or 0 on success. */
int initiate_privacyamplification(struct keyblock *kb);

/* function to process a privacy amplification message. parameter is incoming
   message, return value is 0 or an error code. Parses the message and passes
   the real work over to the do_privacyamplification part */
int receive_privamp_msg(char *receivebuf);

/* do core part of the privacy amplification. Calculates the compression ratio
   based on the lost bits, saves the final key and removes the thread from the
   list.    */
int do_privacy_amplification(struct keyblock *kb, unsigned int seed,
                             int lostbits);

// MAIN FUNCTION DECLARATIONS (OTHERS)
/* ------------------------------------------------------------------------- */
/* process a command (e.g. epoch epochNum), terminated with \0 */
int process_command(char *in);

/* main code */
int main(int argc, char *argv[]);