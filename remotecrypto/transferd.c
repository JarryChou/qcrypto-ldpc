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

#define DEBUG 1
#undef DEBUG

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
  //9 E            //10 b
  arg_ec_out_pipe, arg_debuglogs 
};

enum ReceiveMode {
  rcvmode_waiting_to_read_next = 0,
  rcvmode_peer_terminated = 0,
  rcvmode_beginning_with_header = 0,
  rcvmode_finishing_header = 1,
  rcvmode_got_header_start_reading_data = 3,
  rcvmode_finished_reading_a_packet = 4,
};

enum receiveHeader {
  rhead_file, rhead_message, rhead_errc_packet
};

/* global variables for IO handling */
char fname[11][FNAMELENGTH] = {"", "", "", "", "", "",
                               "", "", "", "", ""}; /* stream files */
char ffnam[11][FNAMELENGTH + 11], ffn2[FNAMELENGTH + 11];
char tempFileName[FNAMELENGTH + 11]; /* stores temporary file name */
int killmode = DEFAULT_KILLMODE;  /* if !=1, delete infile after use */
int handle[11];                   /* global handles for packet streams */
FILE *debuglog;

/* error handling */
char *errormessage[76] = {
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
};

int emsg(int code) {
  fprintf(stderr, "%s\n", errormessage[code]);

  fprintf(debuglog, "err msg: %s\n", errormessage[code]);
  fflush(debuglog);

  //Use exit instead of return to reduce clutter
  exit(-code);
  //return code;
};

/* global variables for IO handling */

/* some helpers */
#define MIN(A, B) ((A) > (B) ? (B) : (A))

struct timeval HALFSECOND = {0, 50000};
/* helper for name. adds a slash, hex file name and a termial 0 */
char hexdigits[] = "0123456789abcdef";
void atohex(char *target, unsigned int v) {
  int i;
  target[0] = '/';
  for (i = 1; i < 9; i++) target[i] = hexdigits[(v >> (32 - i * 4)) & 15];
  target[9] = 0;
}

// global variables because
int verbosity = DEFAULT_VERBOSITY;
int opt, i, retval; /* general parameters */
#ifdef DEBUG
int ii;
#endif
int hasParam[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* sockets and destination structures */
int sendskt, recskt, activeSocket;
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
int writemode, writeindex, cmdmode, messagemode;
char message[MESSAGELENGTH];
/* int keepawake_handle; */
int keepawake_handle_arg_msg_src; /* avoid the input pipe seeing EOF */
int keepawake_handle_arg_ec_in_pipe;
int ignorefileerror = DEFAULT_IGNOREFILEERROR;
// WARNING: "running" is always 1
int running;
FILE *cmdinhandle;

int parseArguments(int argc, char *argv[]) {
  /* parsing options */
  opterr = 0; /* be quiet when there are no options */
  while ((opt = getopt(argc, argv, "d:c:t:D:l:s:km:M:p:e:E:b:")) != EOF) {
    i = 0; /* for setinf names/modes commonly */
    switch (opt) {
      case 'b': i++;
      case 'E': i++;
      case 'e': i++;
      case 'M': i++; /* funky way of saving file names */
      case 'm': i++;
      case 's': i++;
      case 'l': i++;
      case 'D': i++;
      case 't': i++;
      case 'c': i++;
      case 'd':
        /* stream number is in i now */
        if (1 != sscanf(optarg, FNAMFORMAT, fname[i])) return -emsg(1 + i);
        fname[i][FNAMELENGTH - 1] = 0;        /* security termination */
        if (hasParam[i]) return -emsg(1 + i); /* already defined mode */
        hasParam[i] = 1;
        break;
      case 'k': /* killmode */
        killmode = 1;
        break;
      case 'p': /* set origin portNumber */
        if (sscanf(optarg, "%d", &portNumber) != 1) return -emsg(65);
        if ((portNumber < MINPORT) || (portNumber > MAXPORT)) return -emsg(66);
      case 'P':  // targetPortNumber
        if (sscanf(optarg, "%d", &targetPortNumber) != 1) return -emsg(65);
        if ((targetPortNumber < MINPORT) || (targetPortNumber > MAXPORT))
          return -emsg(66);
        break;
    }
  }

  /* check argument completeness */
  for (i = 0; i < 5; i++) {
    if (hasParam[i] == 0) {
      return -emsg(i + 67);
    }
  }
  if (hasParam[arg_msg_src] != hasParam[arg_msg_dest]) {
    return -emsg(15); /* not same message mode */
  }
  /* add directory slash for sourcefile if missing */
  if (fname[arg_srcdir][strlen(fname[arg_srcdir]) - 1] != '/') {
    strncat(fname[arg_srcdir], "/", FNAMELENGTH);
    fname[arg_srcdir][FNAMELENGTH - 1] = 0;
  }
  cmdinhandle = fopen("/tmp/cryptostuff/cmdins", "w+");
  
  /* get all sockets */
  if ((sendskt = socket(AF_INET, SOCK_STREAM, 0)) == 0 ||  /* outgoing packets */
    (recskt = socket(AF_INET, SOCK_STREAM, 0)) == 0) {  /* incoming packets */
    return -emsg(16);
  }

  /* command pipe */
  if (access(fname[arg_cmdpipe], F_OK) == -1) { /* fifo does not exist */
    if (mkfifo(fname[arg_cmdpipe], FIFOPERMISSIONS)) {
      return -emsg(17);
    }
  }

  if ((cmdhandle = fopen(fname[arg_cmdpipe], "r+")) == 0) { return -emsg(18); }
  /*cmdhandle=open(fname[arg_cmdpipe],FIFOMODE);
if (cmdhandle==-1) return -emsg(18);
keepawake_handle= open(fname[arg_cmdpipe],DUMMYMODE); */
  /* keep server alive */

  /* message pipe */
  if (hasParam[arg_msg_src]) {                    /* message pipes exist */
    if (access(fname[arg_msg_src], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_msg_src], FIFOPERMISSIONS)) {
        return -emsg(19);
      }
    }
    if ((msginhandle = open(fname[arg_msg_src], FIFOMODE)) == -1) {
      return -emsg(20);  /* cannot open FIFO */
    }
    keepawake_handle_arg_msg_src = open(fname[arg_msg_src], DUMMYMODE); /* avoid message congestion */
  };

  /* errc_in pipe */
  if (hasParam[arg_ec_in_pipe]) {                    /* listen to it */
    if (access(fname[arg_ec_in_pipe], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_ec_in_pipe], FIFOPERMISSIONS)) {
        return -emsg(11);
      }
    }
    if ((ercinhandle = open(fname[arg_ec_in_pipe], FIFOMODE)) == -1) {
      return -emsg(12);  /* cannot open FIFO */
    }
    keepawake_handle_arg_ec_in_pipe = open(fname[arg_ec_in_pipe], DUMMYMODE); /* avoid message congestion */
  };
  /* errc_out pipe */
  if (hasParam[arg_ec_out_pipe]) {                    /* open it */
    if (access(fname[arg_ec_out_pipe], F_OK) == -1) { /* fifo does not exist */
      if (mkfifo(fname[arg_ec_out_pipe], FIFOPERMISSIONS)) {
        return -emsg(13);
      }
    }
    ercouthandle = open(fname[arg_ec_out_pipe], FIFOOUTMODE);
    if (ercouthandle == -1) {
      return -emsg(14); /* cannot open FIFO */
    }
  };

  // Debug logs
  if (hasParam[10]) { /* open it */
    printf("Opening debuglog: %s\n", fname[arg_debuglogs]);
    debuglog = fopen(fname[arg_debuglogs], "w+");
  };
}

int main(int argc, char *argv[]) {
  parseArguments(argc, argv);

  /* client socket for sending data */
  sendadr.sin_family = AF_INET;
  remoteinfo = gethostbyname(fname[arg_target]);
  if (!remoteinfo) {
    switch (h_errno) {
      case HOST_NOT_FOUND:
        return -emsg(21);
      case NO_ADDRESS:
        return -emsg(22);
      case TRY_AGAIN:
        return -emsg(23);
      default:
        return -emsg(24);
    }
  }
  /* extract host-IP */
  sendadr.sin_addr = *(struct in_addr *)*remoteinfo->h_addr_list;
  sendadr.sin_port = htons(targetPortNumber);

  /* create socket for server / receiving files */
  recadr.sin_family = AF_INET;
  if (fname[arg_srcIP][0]) { /* port defined */
    if (inet_aton(fname[arg_srcIP], &recadr.sin_addr)) return -emsg(25);
  } else {
    recadr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  recadr.sin_port = htons(portNumber);
  /* try to reuse address */
  i = 1;
  retval = setsockopt(recskt, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
  if (retval == -1) return -emsg(64);
  if (bind(recskt, (struct sockaddr *)&recadr, sizeof(recadr))) {
    switch (errno) { /* gove perhaps some specific errors */
      default:
        fprintf(stderr, "error in bind: %d\n", errno);
        return -emsg(26);
    }
  }
  if (listen(recskt, RECEIVE_BACKLOG)) return -emsg(32);

  /* try to test directory existence */
  if (stat(fname[arg_srcdir], &dirstat)) return -emsg(27); /* src directory */
  if ((dirstat.st_mode & S_IFMT) != S_IFDIR) return -emsg(28); /* no dir */

  if (stat(fname[arg_destdir], &dirstat)) return -emsg(29); /* src directory */
  if ((dirstat.st_mode & S_IFMT) != S_IFDIR) return -emsg(30); /* no dir */

  /* try to get send/receive buffers */
  fileBuffer = (char *)malloc(LOC_BUFSIZE);
  receivedDataBuffer = (char *)malloc(LOC_BUFSIZE);
  errorCorrectionInBuffer = (char *)malloc(LOC_BUFSIZE2);
  if (!fileBuffer || !receivedDataBuffer || !errorCorrectionInBuffer) return -emsg(41);
  ehead = (struct errc_header *)errorCorrectionInBuffer; /* for header */

  /* prepare file name for temporary file storage */
  strncpy(tempFileName, fname[arg_destdir], FNAMELENGTH);
  strcpy(&tempFileName[strlen(tempFileName)], tmpfileext);

  /* prepare shutdown...never? */
  running = 1;

  do {           /* while link should be maintained */
    activeSocket = 0; /* mark unsuccessful connection attempt */
    /* wait half a second for a server connection */
    FD_ZERO(&readqueue);
    timeout = HALFSECOND;
    FD_SET(recskt, &readqueue);
    retval = select(FD_SETSIZE, &readqueue, (fd_set *)0, (fd_set *)0, &timeout);
    if (retval == -1) return -emsg(33);
    if (retval) { /* there is a request */
      if (!FD_ISSET(recskt, &readqueue)) {
        return -emsg(34); /* cannot be?*/
      }
      /* accept connection */
      remotelen = sizeof(remoteadr);
      retval = accept(recskt, (struct sockaddr *)&remoteadr, &remotelen);
      if (retval < 0) {
        fprintf(stderr, "Errno: %d ", errno);
        return -emsg(35);
      }
      /* use new socket */
      activeSocket = retval;
    } else { /* timeout has occured. attempt to make client connection */
      fcntl(sendskt, F_SETFL, O_NONBLOCK); /* prepare nonblock mode */
      retval = connect(sendskt, (struct sockaddr *)&sendadr, sizeof(sendadr));
      /* check for anythinng else than EINPROGRESS */
      if (retval) { /* an error has occured */
        if ((errno == EALREADY) || (errno == ECONNABORTED)) {
          continue; /* trying already...*/
        }
        if (errno != EINPROGRESS) {
          if (errno == ETIMEDOUT) continue;
          fprintf(stderr, "errno: %d", errno);
          return -emsg(36);
        } else {
          /* wait half a second for response of connecting */
          FD_ZERO(&writequeue);
          FD_SET(sendskt, &writequeue);
          timeout = HALFSECOND;
          retval = select(FD_SETSIZE, (fd_set *)0, &writequeue, (fd_set *)0,
                          &timeout);
          if (retval) {
            socklen = sizeof(retval);
            if (getsockopt(sendskt, SOL_SOCKET, SO_ERROR, &retval, &socklen))
              return -emsg(38);
            if (retval) {
              /* printf("point2e\n"); */
              if (errno == EINPROGRESS) continue;
              fprintf(stderr, "errno: %d", errno);
              return -emsg(38);
            }
            /* Weee! we succeeded geting a connection */
            activeSocket = sendskt;
          } else { /* a timeout has occured */
            activeSocket = 0;
          }
        }
      } else { /* it worked in the first place */
        activeSocket = sendskt;
      }
    }

    // If there is a connection, just printf
    if (activeSocket) {
      if (verbosity > 0) {
        printf("connected.\n");
        fflush(stdout);
#ifdef DEBUG
        fprintf(debuglog, "connected.\n");
        fflush(debuglog);
#endif
      }
    }

    // Begin communications
    receivemode = rcvmode_waiting_to_read_next; //0; /* wait for a header */
    receiveindex = 0;
    writemode = 0;
    writeindex = 0;
    dataToSendBuffer = NULL;
    packinmode = 0; /* waiting for header */
    cmdmode = 0;
    messagemode = 0;  /* finish eah thing */
    while (activeSocket) { /* link is active,  wait on input sockets */
      FD_ZERO(&readqueue);
      FD_ZERO(&writequeue);
      FD_SET(activeSocket, &readqueue);
      if (!cmdmode) FD_SET(fileno(cmdhandle), &readqueue);
      if (dataToSendBuffer) FD_SET(activeSocket, &writequeue); /* if we need to write */
      if (hasParam[arg_msg_src] && !messagemode) FD_SET(msginhandle, &readqueue);
      if (hasParam[arg_ec_in_pipe] && (packinmode != 4)) FD_SET(ercinhandle, &readqueue);
      timeout = HALFSECOND;
      retval = select(FD_SETSIZE, &readqueue, &writequeue, NULL, NULL);
      if (retval < 0) return -emsg(39);
        /* eat through set */
#ifdef DEBUG
      fprintf(debuglog, "select returned %d\n", retval);
      fflush(debuglog);
#endif
      // If there is something to read from the active socket
      if (FD_ISSET(activeSocket, &readqueue)) {
#ifdef DEBUG
        fprintf(debuglog, "tcp read received event\n");
#endif
        switch (receivemode) {
          case rcvmode_waiting_to_read_next: /* beginning with header */
            receivemode = rcvmode_finishing_header;
          case rcvmode_finishing_header: /* finishing header */
            retval = read(activeSocket, &((char *)&rhead)[receiveindex],
                          sizeof(rhead) - receiveindex);
#ifdef DEBUG
            fprintf(debuglog, "tcp receive stage 1:%d bytes\n", retval);
            fflush(debuglog);
#endif
            if (retval == 0) { /* end of file, peer terminated */
              receivemode = rcvmode_peer_terminated;
              goto reconnect;
              break;
            } else if (retval == -1) {
              if (errno == EAGAIN) {
                break;
              }
              fprintf(stderr, "errno: %d ", errno);
#ifdef DEBUG
              fprintf(debuglog, "error st1: %d\n", errno);
              fflush(debuglog);
#endif
              return -emsg(40);
            }
#ifdef DEBUG
            fprintf(debuglog, "p3\n");
            fflush(debuglog);
#endif
            receiveindex += retval;
            // Drop packet if the data received is less than a header
            if (receiveindex < sizeof(rhead)) {
              break;
            }
#ifdef DEBUG
            fprintf(debuglog, "p4, len:%d\n", rhead.length);
            for (i = 0; i < 12; i++)
              fprintf(debuglog, "%02x ", ((unsigned char *)&rhead)[i]);
            fprintf(debuglog, "\n");
            fflush(debuglog);
#endif
            /* got header, start reading data */
            // If data exceeds buffer size, throw error
            if (rhead.length > LOC_BUFSIZE) {
              return -emsg(59);
            }
            receiveindex = 0;
            receivemode = rcvmode_got_header_start_reading_data;
#ifdef DEBUG
            fprintf(debuglog, "tcp receive before stage3, expect %d bytes\n",
                    rhead.length);
            fflush(debuglog);
#endif
            break;
          case rcvmode_got_header_start_reading_data: 
            // Based on the header info, read in the body data
            retval = read(activeSocket, &receivedDataBuffer[receiveindex],
                          MIN(LOC_BUFSIZE, rhead.length) - receiveindex);
#ifdef DEBUG
            fprintf(debuglog, "tcp rec stage 3:%d bytes, wanted:%d\n", retval,
                    MIN(LOC_BUFSIZE, rhead.length) - receiveindex);
            fflush(debuglog);
#endif
            // If fail to read body
            if (retval == -1) {
              if (errno == EAGAIN) break;
              fprintf(stderr, "errno: %d ", errno);
#ifdef DEBUG
              fprintf(debuglog, "errno (read, stag3): %d", errno);
              fflush(debuglog);
#endif
              if (errno == ECONNRESET) goto reconnect;
              return -emsg(42);
            }
            receiveindex += retval;
            // If read the entire body (based on header)
            if (receiveindex >= rhead.length) {
              receivemode = rcvmode_finished_reading_a_packet; /* done */
#ifdef DEBUG
              fprintf(debuglog, "tcp receive stage 4 reached\n");
              fflush(debuglog);
#endif
            }
            break;
        }
        if (receivemode == rcvmode_finished_reading_a_packet) { /* stream read complete */
          switch (rhead.type) {
            case rhead_file: 
            /* we got a file */
            /*if (verbosity >1) */
#ifdef DEBUG
              fprintf(debuglog, "got file via tcp, len:%d\n", rhead.length);
              fflush(debuglog);
#endif
              /* open target file */
              strncpy(ffnam[arg_destdir], fname[arg_destdir], FNAMELENGTH);
              atohex(&ffnam[arg_destdir][strlen(ffnam[arg_destdir])], rhead.epoch);
              destfile = open(tempFileName, TARGETFILEMODE, FILE_PERMISSIONS);
              if (destfile < 0) {
                fprintf(debuglog, "destfile  val: %x\n", destfile);
                fprintf(debuglog, "file name: %s, len: %d\n", ffnam[arg_destdir],
                        rhead.length);
                fprintf(debuglog, "errno on opening: %d\n", errno);
                fflush(debuglog);
                destfile = open("transferdump", O_WRONLY);
                if (destfile != -1) {
                  write(destfile, receivedDataBuffer, rhead.length);
                  close(destfile);
                }
                return -emsg(43);
              }
              if ((int)rhead.length != write(destfile, receivedDataBuffer, rhead.length))
                return -emsg(44);
              close(destfile);
              /* rename file */
              if (rename(tempFileName, ffnam[arg_destdir])) {
                fprintf(stderr, "rename errno: %d ", errno);
                return -emsg(75);
              }
              /* send notification */
              loghandle = fopen(fname[arg_notify], "a");
              if (!loghandle) return -emsg(49);
              fprintf(loghandle, "%08x\n", rhead.epoch);
              fflush(loghandle);
              fclose(loghandle);
#ifdef DEBUG
              fprintf(debuglog, "sent notif on file %08x\n", rhead.epoch);
              fflush(debuglog);
#endif
              break;
            case rhead_message: /* incoming message */
                    /* if (verbosity>1) */
#ifdef DEBUG
              fprintf(debuglog, "got message via TCP...");
              fflush(debuglog);
#endif
              if (hasParam[arg_msg_dest]) {
                msgouthandle = fopen(fname[arg_msg_dest], "a");
                if (!msgouthandle) return -emsg(45);
                /* should we add a newline? */
                retval = fwrite(receivedDataBuffer, sizeof(char), rhead.length, msgouthandle);
#ifdef DEBUG
                fprintf(debuglog, "retval from fwrite is :%d...", retval);
                fflush(debuglog);
#endif
                if (retval != (int)rhead.length) return -emsg(46);
                fflush(msgouthandle);
                fclose(msgouthandle);
#ifdef DEBUG
                fprintf(debuglog, "message>>%40s<< sent to msgouthandle.\n",
                        receivedDataBuffer);
                for (ii = 0; ii < (int)rhead.length; ii++) {
                  fprintf(debuglog, " %02x", receivedDataBuffer[ii]);
                  if ((ii & 0xf) == 0xf) fprintf(debuglog, "\n");
                }
                fprintf(debuglog, "\n");
                fflush(debuglog);
#endif
                break;
              } else {
                return -emsg(47); /* do not expect msg */
              }
            case rhead_errc_packet: /* got errc packet */
              if (hasParam[arg_ec_out_pipe]) {
                write(ercouthandle, receivedDataBuffer, rhead.length);
              }
              break;
            default:
              return -emsg(48); /* unexpected data type */
          }
          receivemode = rcvmode_waiting_to_read_next; /* ready to read next */
          receiveindex = 0;
        }
      }
      if (FD_ISSET(ercinhandle, &readqueue)) {
        switch (packinmode) {
          case 0: /* wait for header */
            packinmode = 1;
            erci_idx = 0;
          case 1: /* finish reading header */
            retval = read(ercinhandle, &errorCorrectionInBuffer[erci_idx],
                          sizeof(struct errc_header) - erci_idx);
            if (retval == -1) {
              if (errno == EAGAIN) break;
              fprintf(stderr, "errno: %d ", errno);
              return -emsg(72);
            }
            erci_idx += retval;
            if (erci_idx < sizeof(struct errc_header)) break;
            /* got header, read data */
            if (ehead->length > LOC_BUFSIZE2) return -emsg(73);
            packinmode = 3; /* erci_idx continues on same buffer */
          case 3:           /* read more data */
            retval = read(ercinhandle, &errorCorrectionInBuffer[erci_idx],
                          MIN(LOC_BUFSIZE2, ehead->length) - erci_idx);
            if (retval == -1) {
              if (errno == EAGAIN) break;
              fprintf(stderr, "errno: %d ", errno);
              return -emsg(74);
            }
            erci_idx += retval;
            if (erci_idx >= ehead->length) packinmode = 4; /* done */
            break;
        }
      }
      if (FD_ISSET(fileno(cmdhandle), &readqueue)) {
        cmdmode = 0; /* in case something goes wrong */
                     /* a command is coming */
#ifdef DEBUG
        fprintf(debuglog, "got incoming command note\n");
        fflush(debuglog);
#endif
        if (1 != fscanf(cmdhandle, FNAMFORMAT, transfername)) return -emsg(62);

        if (sscanf(transfername, "%x", &srcepoch) != 1) {
          if (verbosity > 2) printf("file read error.\n");
          if (ignorefileerror) {
            goto parseescape;
          } else {
            return -emsg(52);
          }
        }
#ifdef DEBUG
        fprintf(debuglog, "command read in:>>%s,,\n", transfername);
        fflush(debuglog);
#endif
        /* consistency check for messages? */
        if (srcepoch < oldsrcepoch) {
          fprintf(cmdinhandle, "*cmdin: %s\n", transfername);
          fflush(cmdinhandle);
          goto parseescape;
        }
        oldsrcepoch = srcepoch;
        fprintf(cmdinhandle, "cmdin: %s\n", transfername);
        fflush(cmdinhandle);
        strncpy(ftnam, fname[arg_srcdir], FNAMELENGTH - 1);
        ftnam[FNAMELENGTH - 1] = 0;
        strncat(ftnam, transfername, FNAMELENGTH - 1);
        ftnam[FNAMELENGTH - 1] = 0;
#ifdef DEBUG
        fprintf(debuglog, "transfername: >>%s<<\n", ftnam);
        fflush(debuglog);
#endif
        if (stat(ftnam, &srcfilestat)) { /* stat failed */
                                         /* if (verbosity>2) */
#ifdef DEBUG
          fprintf(debuglog, "(1)file read error.\n");
          fflush(debuglog);
#endif
          if (ignorefileerror) {
            goto parseescape;
          } else {
            return -emsg(50);
          }
        }
        if (!S_ISREG(srcfilestat.st_mode)) {
          /* if (verbosity>2) */
#ifdef DEBUG
          fprintf(debuglog, "(2)file read error.\n");
          fflush(debuglog);
#endif
          if (ignorefileerror) {
            goto parseescape;
          } else {
            return -emsg(51);
          }
        }
        if (srcfilestat.st_size > LOC_BUFSIZE) return -emsg(60);
        cmdmode = 1;
      }
    parseescape:
      if (hasParam[arg_msg_src]) /* there could be a message */
        if (FD_ISSET(msginhandle, &readqueue)) {
          /* read message */
          retval = read(msginhandle, message, MESSAGELENGTH);
#ifdef DEBUG
          fprintf(debuglog, "got local message in event; retval frm read:%d\n",
                  retval);
          fflush(debuglog);
#endif
          if (retval == -1) return -emsg(57);
          if (retval >= MESSAGELENGTH) return -emsg(58);
          message[MESSAGELENGTH - 1] = 0; /* security termination */
          message[retval] = 0;
          /* debug logging */
#ifdef DEBUG
          fprintf(debuglog, "message sent:>>%s<<", message);
          fflush(debuglog);
          fflush(debuglog);
#endif
          messagemode = 1;
        }
      if (FD_ISSET(activeSocket, &writequeue)) { /* check writing */
#ifdef DEBUG
        fprintf(debuglog, "writeevent received, writemode:%d\n", writemode);
        fflush(debuglog);
#endif
        switch (writemode) {
          case 0: /* nothing interesting */
                  /* THIS SHOULD NOT HAPPEN */
#ifdef DEBUG
            fprintf(debuglog, "nothing to write...\n");
            fflush(debuglog);
#endif
            break;
          case 1: /*  write header */
            retval = write(activeSocket, &((char *)&shead)[writeindex],
                           sizeof(shead) - writeindex);
#ifdef DEBUG
            fprintf(debuglog, "sent header, want:%d, sent:%d\n",
                    sizeof(shead) - writeindex, retval);
            fflush(debuglog);
#endif
            if (retval == -1) return -emsg(55);
            writeindex += retval;
            if (writeindex < (int)sizeof(shead)) break;
            writeindex = 0;
            writemode = 2; /* next level... */
                           /* printf("written header\n"); */
          case 2:          /* write data */
            retval =
                write(activeSocket, &dataToSendBuffer[writeindex], shead.length - writeindex);
#ifdef DEBUG
            fprintf(debuglog, "send data;len: %d, retval: %d, idx %d\n",
                    shead.length, retval, writeindex);
            fflush(debuglog);
#endif
            if (retval == -1) return -emsg(56);
            writeindex += retval;
            if (writeindex < (int)shead.length) break;
            writemode = 3;
            /* if (verbosity>1) */
#ifdef DEBUG
            fprintf(debuglog, "sent file\n");
            fflush(debuglog);
#endif
          case 3: /* done... */
            switch (shead.type) {
              case 0:
                cmdmode = 0; /* file has been sent */
                /* remove source file */
                if (killmode) {
                  if (unlink(ftnam)) return -emsg(63);
                }
                dataToSendBuffer = NULL; /* nothing to be sent from this */
                break;
              case 1:
                messagemode = 0;
                dataToSendBuffer = NULL;
                break;
              case 2:
                packinmode = 0;
                dataToSendBuffer = NULL;
                break;
            }
            writemode = 0;
            break;
        }
      }

      /* test for next transmission in the queue */
      if (messagemode && !writemode) { /* prepare for writing */
                                       /* prepare header */
#ifdef DEBUG
        fprintf(debuglog, "prepare for sending message\n");
        fflush(debuglog);
#endif
        shead.type = 1;
        shead.length = strlen(message) + 1;
        shead.epoch = 0;
        /* prepare for sending message buffer */
        writemode = 1;
        writeindex = 0;
        dataToSendBuffer = message;
        continue; /* skip other tests for writing */
      }
      if (cmdmode && !writemode) {
        /* read source file */
        srcfile = open(ftnam, READFILEMODE);
        if (srcfile == -1) {
          fprintf(debuglog, "return val open: %x, errno: %d\n", srcfile, errno);
          fprintf(debuglog, "file name: >%s<\n", ftnam);
          return -emsg(53);
        }
        retval = read(srcfile, fileBuffer, LOC_BUFSIZE);
        close(srcfile);
#ifdef DEBUG
        fprintf(debuglog,
                "prepare for sending file; read file with return value %d\n",
                retval);
        fflush(debuglog);
#endif
        if (retval != srcfilestat.st_size) return -emsg(54);
        /* prepare send header */
        shead.type = 0;
        shead.length = retval;
        shead.epoch = srcepoch;
        writemode = 1;
        writeindex = 0; /* indicate header writing */
        dataToSendBuffer = fileBuffer;
        continue; /* skip other test for writing */
      }
      if ((packinmode == 4) && !writemode) { /* copy errc packet */
        /* prepare header & writing */
        shead.type = 2;
        shead.length = ehead->length;
        shead.epoch = 0;
        writemode = 1;
        writeindex = 0;
        dataToSendBuffer = errorCorrectionInBuffer;
      }
    }
#ifdef DEBUG
    fprintf(debuglog, "loop\n");
    fflush(debuglog);
#endif
    continue; /* loop is fine */
  reconnect:
    /* close open sockets and wait for next connection */
    close(activeSocket);
#ifdef DEBUG
    fprintf(debuglog, "comm socket was closed.\n");
    fflush(debuglog);
#endif
    if (activeSocket == sendskt) {                    /* renew send socket */
      sendskt = socket(AF_INET, SOCK_STREAM, 0); /* outgoing packets */
      if (!sendskt) return -emsg(16);
      /* client socket for sending data */
      sendadr.sin_family = AF_INET;
      remoteinfo = gethostbyname(fname[arg_target]);
      if (!remoteinfo) {
        switch (h_errno) {
          case HOST_NOT_FOUND:
            return -emsg(21);
          case NO_ADDRESS:
            return -emsg(22);
          case TRY_AGAIN:
            return -emsg(23);
          default:
            return -emsg(24);
        }
      }
      /* extract host-IP */
      sendadr.sin_addr = *(struct in_addr *)*remoteinfo->h_addr_list;
      sendadr.sin_port = htons(portNumber);
    }
    if (verbosity > 0) {
      printf("disconnected.\n");
      fflush(stdout);
    }
    activeSocket = 0;
  } while (running); /* while link should be maintained */

  /* end benignly */
  printf("ending benignly\n");

  /* clean up sockets */
  fclose(cmdhandle);
  /* close(cmdhandle); */
  /* close(keepawake_handle); */
  if (hasParam[arg_msg_src]) {
    close(msginhandle);
    close(keepawake_handle_arg_msg_src);
  }
  close(recskt);
  close(sendskt);
  if (hasParam[arg_ec_in_pipe]) {
    close(ercinhandle);
    close(keepawake_handle_arg_ec_in_pipe);
  }
  if (hasParam[arg_ec_out_pipe]) close(ercouthandle);
  /* free buffer */
  free(receivedDataBuffer);
  free(fileBuffer);
  free(errorCorrectionInBuffer);
  fclose(cmdinhandle);

  fclose(debuglog);
  return 0;
}
