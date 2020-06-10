/* transferd.c : Part of the quantum key distribution software for serving
                 as a classical communication gateway between the two sides.
                 Description see below. Version as of 20070101

 Copyright (C) 2005-2006, 2020 Christian Kurtsiefer, National University
                         of Singapore <christian.kurtsiefer@gmail.com>

 This source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Public License as published
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 This source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 Please refer to the GNU Public License for more details.

 You should have received a copy of the GNU Public License along with
 this source code; if not, write to:
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

--


   program to arrange for file transfer from one machine to the other. This is
   taking care of the sockets between two machines. Each machine has a version
   of it running. This deamon listens to a command port for file names to be
   transferred, which are specified relative to a initially agreed directory,
   and transmitts the underlying file to the other side. On the receiving side,
   files are saved in a directory, and a notofication is placed in a file.

   status: worked on more than 55k epochs on feb 7 06
   tried to fix some errcd packet errors 30.4.06chk

 usage: transferd -d sourcedirectory -c commandsocket -t targetmachine
                  -D destinationdir -l notificationfile -s sourceIP
                  [-e ec_in_pipe -E ec_out_pipe ]
                  [-k]
                  [-m messagesource -M messagedestintion ]
                  [-p portNumber]
                  [-P targetPortNumber]
                  [-v verbosity]
                  [-b debuglogs]

 parameters:

  -d srcdir:        source directory for files to be transferred
  -c commandpipe:   where to create a fifo in the file system to
                    listen to files to be transferred. the path has to be
                    absolute.
  -t target:        IP address of target machine
  -D destdir:       destination directory
  -l notify:        if a packet arrives and has been saved, a notification
                    (the file name itself) is sent to the file or pipe named
                    in the parameter of this option
  -s srcIP:         listen to connections on the local ip identified by the
                    paremeter. By default, the system listens on all ip
                    addresses of the machine.
  -k killoption:    If this is activated, a file gets destroyed in
                    the source directory after it has been sent.
  -m src:           message source pipe. this opens a local fifo in the file
                    tree where commands can be tunneled over to the other side.
  -M dest:          if a command message is sent in from the other side, it
                    will be transferred into the file or pipe named in the
                    parameter of this file.
  -p portnum:       port number used for communication. By default, this is
                    port number 4852.
  -v verbosity:     choose verbosity. 0: no normal output to stdout
                    1: connect/disconnect to stdout
                    2: talk about receive/send events
                    3: include file error events
  -e ec_in_pipe:    pipe for receiving packets from errorcorrecting demon
  -E ec_out_pipe:   pipe to send packes to the error correcting deamon
  -b debuglogs:		File to write debuglogs to


  momentarily, the communication is implemented via tcp/ip packets. the program
  acts either as a server or a client, depending on the status of the other
  side.  if client mode fails, it goes into server mode. if no connection is
  available within a few seconds, it tries to connect to the client again.

  Transfer rationale: The same channel will be used for messages, files and
  error correction packets. Since there is no simple way to extract the length
  of the file for all possible future extensions, the transmission of whatever
  is preceeded by a header involving a
     typedef struct stream_header {int type;
                                   unsigned int length; };
  were type is 0 for simple files, 1 for messages, 2 for errc packets and
  length designates the
  length of the data in bytes. In a later stage, the messages might be sent via
  an out-of-band marker, but this is currently not implemented.

History: first test seems to work 6.9.05chk
  stable version 16.9. started modifying for errorcorrecting packets
  modified for closing many open files feb4 06 chk

To Do:
- use udp protocol instead of tcp, and/or allow for setting more robust
  comm options.
- check messaging system ok, but what happens with emty messages?
- allow for other file types - added another port....
- clean up debugging code

*/

#ifndef TRANSFERD_H   /* Include guard */
#define TRANSFERD_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern int h_errno;

/* default definitions */
#define DEFAULT_KILLMODE 0   /* don't remove files after sending */
#define FNAMELENGTH 200      /* length of file name buffers */
#define FNAMFORMAT "%199s"   /* for sscanf of filenames */
#define tmpfileext "/tmprec" /* temporary receive file */
#define DEFAULT_PORT 4852    /* standard communication */
#define MINPORT 1024         /* port boundaries */
#define MAXPORT 60000
#define RECEIVE_BACKLOG 2   /* waiting requests on remote queue */
#define MSG_BACKLOG 10      /* waiting requests */
#define LOC_BUFSIZE 1 << 22 /* 512k buffer should last for a file */
#define LOC_BUFSIZE2 10000  /* 10k buffer for errc messages */
#define TARGETFILEMODE (O_WRONLY | O_TRUNC | O_CREAT)
#define FILE_PERMISSIONS 0644 /* for all output files */
#define READFILEMODE O_RDONLY
#define MESSAGELENGTH 1024   /* message length */
#define FIFOPERMISSIONS 0600 /* only user can access */
#define FIFOMODE O_RDONLY | O_NONBLOCK
#define FIFOOUTMODE O_RDWR
#define DUMMYMODE O_WRONLY
#define DEFAULT_IGNOREFILEERROR 1
#define DEFAULT_VERBOSITY 1

/* stream header definition */
typedef struct stream_header {
  int type;            /* 0: ordinary file, 1: message */
  unsigned int length; /* len in bytes */
  unsigned int epoch;
} sh;

/* errorcorrecting packet header def */
typedef struct errc_header {
  int tag;
  unsigned int length;
} eh;

// Enumerators 
// Introducing enumerators has very little overhead, and it also makes it much more readable
// Refactor TypeMode to HasParam
enum HasParamParameter { 
  //0 d       //1 c        //2 t
  arg_srcdir, arg_cmdpipe, arg_target, 
  //3 D        //4 l        //5 s
  arg_destdir, arg_notify, arg_srcIP, 
  //6 m        //7 M        //8 e
  arg_msg_src, arg_msg_dest, arg_ec_in_pipe, 
  //9 E            //10 b         //11 i     //Helper enum to keep count of # of params
  arg_ec_out_pipe, arg_debuglogs, arg_cmdin, arg_param_count
};

enum ConnectionTimeoutResult {
  conn_continueTrying = 0, conn_successfulConnection = 1
};

enum ReceiveMode {
  rcvmode_waiting_to_read_next = 0,
  rcvmode_peer_terminated = 0,
  rcvmode_beginning_with_header = 0,
  rcvmode_finishing_header = 1,
  rcvmode_got_header_start_reading_data = 3,
  rcvmode_finished_reading_a_packet = 4,
};

enum HeaderType {
  head_file = 0, 
  head_message = 1, 
  head_errc_packet = 2
};

enum ReadResult {
  readResult_reconnect = 0, readResult_successful = 1
};

// Note: very similar (if not equal) to ReceiveMode. May want to merge
enum PackInMode {
  packinmode_wait_for_header = 0,
  packinmode_finish_reading_header = 1,
  packinmode_got_header_start_reading_data = 3,
  packinmode_finished_reading_a_packet = 4
};

enum WriteMode {
  writemode_not_writing = 0,
  writemode_write_header = 1,
  writemode_write_data = 2,
  writemode_write_complete = 3
};

/* global variables for IO handling */
char fname[arg_param_count][FNAMELENGTH] = {"", "", "", "", "", "",
                               "", "", "", "", ""}; /* stream files */
char ffnam[arg_param_count][FNAMELENGTH + arg_param_count], ffn2[FNAMELENGTH + arg_param_count];
char tempFileName[FNAMELENGTH + arg_param_count]; /* stores temporary file name */
int killmode = DEFAULT_KILLMODE;  /* if !=1, delete infile after use */
int handle[arg_param_count];                   /* global handles for packet streams */
FILE *debuglog;

/* error handling */
char *errormessage[78] = {
    "No error.",
    "error parsing source directory name", /* 1 */
    "error parsing command socket name",
    "error parsing target machine name",
    "error parsing destination directory name",
    "error parsing notification destination name", /* 5 */
    "error parsing remote server socket name",
    "error parsing message source pipe",
    "error parsing message destination file/pipe",
    "error parsing errorcorrection instream pipe",
    "error parsing errorcorrection outstream pipe", /* 10 */
    "cannot create errc_in pipe",
    "cannot open errc_in pipe",
    "cannot create errc_out pipe",
    "cannot open errc_out pipe",
    "no consistent message pipline definition (must have both)", /* 15 */
    "cannot create socket",
    "cannot create command FIFO",
    "cannot open command FIFO ",
    "cannot create message FIFO",
    "cannot open message FIFO", /* 20 */
    "target host not found",
    "valid target name has no IP",
    "temporary IP resolve error. Try later",
    "unspecified target host resolve error.",
    "invalid local IP", /* 25 */
    "error in binding socket",
    "cannot stat source directory",
    "specified source is not a directory",
    "cannot stat target directory",
    "specified target dir is not a directory", /* 30 */
    "error reading command",
    "cannot listen on incoming request socket",
    "Error from waiting for server connections",
    "unlogical return fromselect",
    " ; error accepting connection", /* 35 */
    " ; error in connecting to peer",
    "getsockopt failed.",
    " ; socket error occured.",
    "select on input lines failed w error.",
    "Error reading stream header form external source.", /* 40 */
    "cannot malloc send/receive buffers.",
    "error reading stream data",
    "cannot open target file",
    "cannot write stream to file",
    "cannot open message target", /* 45 */
    "cannot write message into local target",
    "received message but no local message target specified",
    "unexpected data type received",
    "cannot open notofication target",
    "cannot stat source file", /* 50 */
    "source is not a regular file",
    "cannot extract epoch from filename",
    "cannot open source file",
    "length read mismatch from source file",
    "Cannot send header", /* 55 */
    "cannot sent data stream",
    "cannot read message",
    "message too long",
    "received message longer than buffer.",
    "transferred larger than buffer", /* 60 */
    "socket probably closed.",
    "reached end of command pipe??????",
    "cannot remove source file."
    "cannot set reuseaddr socket option",
    "error parsing port number", /* 65 */
    "port number out of range",
    "no source directory specified",
    "no commandsocket name specified",
    "no target url specified",
    "no destination directory specified", /* 70 */
    "no arrival notify destination specified",
    "Error reading stream header form errc source.",
    "received packet longer than erc buffer.",
    "error reading erc packet",
    "error renaming target file", /* 75 */
    "cannot open debuglog",
    "cannot open cmdinhandle"
};

/* global variables for IO handling */

/* some helpers */
#define MIN(A, B) ((A) > (B) ? (B) : (A))

struct timeval HALFSECOND = {0, 50000};
/* helper for name. adds a slash, hex file name and a termial 0 */
char hexdigits[] = "0123456789abcdef";

// global variables because the code is unreadable 
// and this is the fastest way to make it readable
// maybe some time in the future you who are reading this will refactor it to make it even better
int verbosity = DEFAULT_VERBOSITY;
int opt, i, retval; /* general parameters */
#ifdef DEBUG
int ii;
#endif
int hasParam[arg_param_count] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* sockets and destination structures */
int sendskt, recskt, activeSocketForIncomingData = -1, activeSocketForOutgoingData = -1;
socklen_t socklen;
FILE *cmdhandle;
int portNumber = DEFAULT_PORT; /* defines communication port */
int targetPortNumber = DEFAULT_PORT;
int msginhandle = 0;
int ercinhandle = 0, ercouthandle = 0; /* error correction pipes */
unsigned int remotelen;
struct sockaddr_in sendadr, recadr, remoteadr;
struct hostent *remoteinfo;
/* file handles */
int srcfile, destfile;
FILE *loghandle, *msgouthandle;
struct stat dirstat; /* for checking directories */
struct stat srcfilestat;
fd_set readqueue, writequeue;
struct timeval timeout;         /* for select command */
struct stream_header shead;     /* for sending */
struct stream_header rhead;     /* for receiving */
char *receivedDataBuffer, *fileBuffer, *errorCorrectionInBuffer;   /* send- and receive buffers, ercin buffer */
char *dataToSendBuffer;                   /* for write procedure */
char transfername[FNAMELENGTH]; /* read transfer file name */
char ftnam[FNAMELENGTH];        /* full transfer file name */
unsigned int srcepoch;
unsigned oldsrcepoch = 0;
int receivemode; /* for filling input buffer */
unsigned int receiveindex;
int packinmode;            /* for reading errc packets from pipe */
unsigned int erci_idx = 0; /* initialize to keep compiler happy */
struct errc_header *ehead; /* for reading packets */
/* flags for select mechanism */
int writemode, writeindex, hasFileToSend, hasMessageToSend;
char message[MESSAGELENGTH];
/* int keepawake_handle; */
int keepawake_handle_arg_msg_src; /* avoid the input pipe seeing EOF */
int keepawake_handle_arg_ec_in_pipe;
int ignorefileerror = DEFAULT_IGNOREFILEERROR;
// WARNING: "running" is always 1
int running;
FILE *cmdinhandle;

// Helper functions
int emsg(int code);
void atohex(char *target, unsigned int v);

// Main functions
int main(int argc, char *argv[]);
int parseArguments(int argc, char *argv[]);
int setupPipes();
int createSockets();
int testSrcAndDestDirs();
int setupTempFileName();
int setupBuffers();
int waitForConnectionWithTimeout();
void testAndRectifyForDoubleConnection(); // New code, see comments at waitForConnectionWithTimeout()
int printConnected();
int resetCommunicationVariables();
int blockUntilEvent();
int read_FromActiveSocket_ToReceivedDataBuffer();
int closeAndRecreateSendSocket(); // Return enum ReadResult based on whether reconnection is required.
int read_FromEcInHandle_ToErrorCorrectionInBuffer();
int read_FromCmdHandle_ToTransferName();
int read_FromMsgInHandle_ToMessageArray();
int tryWrite_FromDataToSendBuffer_ToActiveSocket();
int prepareMessageSendHeader();
int read_FromFtnam_ToFileBuffer();
int prepareFileSendHeader();
int prepareEcPacketSendHeader();
int cleanup();

#endif // TRANSFERD_H